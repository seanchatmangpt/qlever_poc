const axios = require('axios');

class ResilienceHandler {
  constructor(options = {}) {
    this.maxRetries = options.maxRetries || 3;
    this.retryDelay = options.retryDelay || 100;
    this.fallbackMode = false;
    this.failureLog = [];
    this.recoveryLog = [];
    this.circuitBreakers = new Map();
    this.recoveryStrategies = {
      'connection-refused': this.handleConnectionRefused.bind(this),
      'timeout': this.handleTimeout.bind(this),
      'invalid-handle': this.handleInvalidHandle.bind(this),
      'serialization-error': this.handleSerializationError.bind(this),
      'memory-pressure': this.handleMemoryPressure.bind(this),
      'index-corruption': this.handleIndexCorruption.bind(this)
    };
  }

  async executeWithResilience(operation, operationName, options = {}) {
    const startTime = Date.now();
    let lastError = null;
    let retryCount = 0;

    for (let attempt = 0; attempt <= this.maxRetries; attempt++) {
      try {
        const result = await operation();
        if (retryCount > 0) {
          this.recoveryLog.push({
            operation: operationName,
            recovered: true,
            attempts: attempt,
            duration: Date.now() - startTime,
            timestamp: new Date().toISOString()
          });
        }
        return result;
      } catch (error) {
        lastError = error;
        retryCount = attempt;

        const failureType = this.classifyFailure(error);
        const strategy = this.recoveryStrategies[failureType];

        this.failureLog.push({
          operation: operationName,
          failureType,
          error: error.message,
          attempt: attempt + 1,
          timestamp: new Date().toISOString()
        });

        if (strategy) {
          const shouldRetry = await strategy(error, attempt, options);
          if (!shouldRetry) {
            break;
          }
        }

        if (attempt < this.maxRetries) {
          const delayMs = this.retryDelay * Math.pow(2, attempt);
          await this.delay(delayMs);
        }
      }
    }

    throw new ResilienceError(
      `Operation failed after ${retryCount + 1} attempts: ${lastError.message}`,
      lastError,
      this.failureLog.slice(-retryCount - 1)
    );
  }

  classifyFailure(error) {
    const message = error.message.toLowerCase();

    if (message.includes('econnrefused') || message.includes('refused')) {
      return 'connection-refused';
    }
    if (message.includes('timeout') || message.includes('timed out')) {
      return 'timeout';
    }
    if (message.includes('invalid handle') || message.includes('unknown handle')) {
      return 'invalid-handle';
    }
    if (message.includes('serializ')) {
      return 'serialization-error';
    }
    if (message.includes('memory') || message.includes('oom') || message.includes('out of')) {
      return 'memory-pressure';
    }
    if (message.includes('corrupt') || message.includes('invalid data')) {
      return 'index-corruption';
    }

    return 'unknown';
  }

  async handleConnectionRefused(error, attempt, options) {
    console.log(`[Resilience] Connection refused (attempt ${attempt + 1}), retrying...`);
    if (attempt >= 2) {
      this.fallbackMode = true;
      console.log('[Resilience] Entering fallback mode');
      return false;
    }
    return attempt < this.maxRetries;
  }

  async handleTimeout(error, attempt, options) {
    console.log(`[Resilience] Timeout (attempt ${attempt + 1}), increasing timeout...`);
    if (options.timeout) {
      options.timeout *= 1.5;
    }
    return attempt < this.maxRetries;
  }

  async handleInvalidHandle(error, attempt, options) {
    console.log('[Resilience] Invalid handle detected, requesting new session...');
    if (options.onHandleInvalid) {
      await options.onHandleInvalid();
    }
    return false;
  }

  async handleSerializationError(error, attempt, options) {
    console.log('[Resilience] Serialization error, attempting data recovery...');
    if (options.onSerializationError) {
      const recovered = await options.onSerializationError();
      return recovered && attempt < this.maxRetries;
    }
    return false;
  }

  async handleMemoryPressure(error, attempt, options) {
    console.log('[Resilience] Memory pressure detected, reducing batch size...');
    if (options.batchSize) {
      options.batchSize = Math.max(1, Math.floor(options.batchSize / 2));
    }
    return attempt < this.maxRetries;
  }

  async handleIndexCorruption(error, attempt, options) {
    console.log('[Resilience] Index corruption detected, attempting recovery...');
    if (options.onIndexCorruption) {
      await options.onIndexCorruption();
    }
    return false;
  }

  async delay(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
  }

  createCircuitBreaker(serviceName, options = {}) {
    const defaults = {
      failureThreshold: 5,
      resetTimeout: 60000,
      monitoringInterval: 10000
    };
    const config = { ...defaults, ...options };

    const breaker = {
      state: 'closed',
      failureCount: 0,
      successCount: 0,
      lastFailureTime: null,
      createdAt: Date.now()
    };

    this.circuitBreakers.set(serviceName, breaker);

    setInterval(() => {
      if (breaker.state === 'open' && Date.now() - breaker.lastFailureTime > config.resetTimeout) {
        console.log(`[Circuit Breaker] ${serviceName} attempting recovery (half-open)`);
        breaker.state = 'half-open';
        breaker.successCount = 0;
      }
    }, config.monitoringInterval);

    return {
      execute: async (fn) => {
        if (breaker.state === 'open') {
          throw new Error(`Circuit breaker OPEN for ${serviceName}`);
        }

        try {
          const result = await fn();
          if (breaker.state === 'half-open') {
            breaker.successCount++;
            if (breaker.successCount >= 3) {
              console.log(`[Circuit Breaker] ${serviceName} recovered`);
              breaker.state = 'closed';
              breaker.failureCount = 0;
            }
          }
          return result;
        } catch (error) {
          breaker.failureCount++;
          breaker.lastFailureTime = Date.now();

          if (breaker.failureCount >= config.failureThreshold) {
            console.log(`[Circuit Breaker] ${serviceName} is OPEN (${breaker.failureCount} failures)`);
            breaker.state = 'open';
          }

          throw error;
        }
      },
      getState: () => breaker.state,
      reset: () => {
        breaker.state = 'closed';
        breaker.failureCount = 0;
        console.log(`[Circuit Breaker] ${serviceName} manually reset`);
      }
    };
  }

  getFailureReport() {
    const byType = {};
    for (const failure of this.failureLog) {
      if (!byType[failure.failureType]) {
        byType[failure.failureType] = [];
      }
      byType[failure.failureType].push(failure);
    }

    return {
      totalFailures: this.failureLog.length,
      totalRecoveries: this.recoveryLog.length,
      recoveryRate: this.failureLog.length > 0
        ? (this.recoveryLog.length / this.failureLog.length * 100).toFixed(2) + '%'
        : '0%',
      byType,
      fallbackModeActive: this.fallbackMode,
      circuitBreakerStates: Array.from(this.circuitBreakers.entries()).map(([name, breaker]) => ({
        service: name,
        state: breaker.state,
        failures: breaker.failureCount
      }))
    };
  }

  printReport() {
    const report = this.getFailureReport();

    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║            RESILIENCE PERFORMANCE REPORT                ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    console.log(`   Total Failures: ${report.totalFailures}`);
    console.log(`   Total Recoveries: ${report.totalRecoveries}`);
    console.log(`   Recovery Rate: ${report.recoveryRate}`);
    console.log(`   Fallback Mode: ${report.fallbackModeActive ? 'ACTIVE' : 'inactive'}\n`);

    console.log('   Failures by Type:');
    for (const [type, failures] of Object.entries(report.byType)) {
      console.log(`     ${type}: ${failures.length} occurrences`);
    }

    if (report.circuitBreakerStates.length > 0) {
      console.log('\n   Circuit Breakers:');
      for (const breaker of report.circuitBreakerStates) {
        const status = breaker.state === 'closed' ? '✓' : breaker.state === 'open' ? '✗' : '⊘';
        console.log(`     ${status} ${breaker.service}: ${breaker.state} (${breaker.failures} failures)`);
      }
    }

    console.log();
  }
}

class ResilienceError extends Error {
  constructor(message, originalError, failureLog) {
    super(message);
    this.originalError = originalError;
    this.failureLog = failureLog;
  }
}

module.exports = { ResilienceHandler, ResilienceError };
