/**
 * Test Setup
 * Runs before all tests to configure the testing environment
 */

import { beforeAll, afterAll, afterEach, vi } from 'vitest';

// ============================================================================
// Global Test Configuration
// ============================================================================

// Set test environment variables
process.env.NODE_ENV = 'test';
process.env.QLEVER_DEBUG = process.env.QLEVER_DEBUG || '0';
process.env.QLEVER_TEST_TIMEOUT = '30000';

// ============================================================================
// Global Test Hooks
// ============================================================================

beforeAll(() => {
  console.log('🧪 Starting QLever WASM Test Suite...');
  console.log(`📦 Node.js: ${process.version}`);
  console.log(`🌍 Platform: ${process.platform}`);
});

afterEach(() => {
  // Clear any mocks between tests
  vi.clearAllMocks();
});

afterAll(() => {
  console.log('✅ Test Suite Complete');
});

// ============================================================================
// Global Test Utilities
// ============================================================================

/**
 * Helper to wait for async operations
 */
export const waitFor = (ms: number) =>
  new Promise((resolve) => setTimeout(resolve, ms));

/**
 * Helper to timeout a promise
 */
export const withTimeout = <T>(
  promise: Promise<T>,
  ms: number,
  message = 'Operation timed out'
): Promise<T> => {
  return Promise.race([
    promise,
    new Promise<T>((_, reject) =>
      setTimeout(() => reject(new Error(message)), ms)
    ),
  ]);
};

/**
 * Helper for performance measurement
 */
export class PerformanceTimer {
  private start: number;

  constructor() {
    this.start = performance.now();
  }

  elapsed(): number {
    return performance.now() - this.start;
  }

  reset(): void {
    this.start = performance.now();
  }

  toString(): string {
    return `${this.elapsed().toFixed(2)}ms`;
  }
}

/**
 * Helper for memory tracking
 */
export class MemoryTracker {
  private startMemory: number;

  constructor() {
    if (typeof process !== 'undefined' && process.memoryUsage) {
      this.startMemory = process.memoryUsage().heapUsed;
    } else {
      this.startMemory = 0;
    }
  }

  used(): number {
    if (typeof process !== 'undefined' && process.memoryUsage) {
      return (process.memoryUsage().heapUsed - this.startMemory) / 1024 / 1024;
    }
    return 0;
  }

  toString(): string {
    return `${this.used().toFixed(2)}MB`;
  }
}

/**
 * Helper to retry failed operations
 */
export const retry = async <T>(
  fn: () => Promise<T>,
  maxAttempts = 3,
  delayMs = 100
): Promise<T> => {
  let lastError: Error | null = null;

  for (let i = 0; i < maxAttempts; i++) {
    try {
      return await fn();
    } catch (error) {
      lastError = error as Error;
      if (i < maxAttempts - 1) {
        await waitFor(delayMs * Math.pow(2, i)); // exponential backoff
      }
    }
  }

  throw lastError || new Error('Max retry attempts exceeded');
};

/**
 * Global test context
 */
declare global {
  var testContext: {
    startTime: number;
    memoryStart: number;
  };
}

globalThis.testContext = {
  startTime: Date.now(),
  memoryStart: typeof process !== 'undefined' ? process.memoryUsage().heapUsed : 0,
};

// ============================================================================
// Console Output Helpers
// ============================================================================

export const log = {
  section: (name: string) => {
    console.log(`\n${'='.repeat(70)}`);
    console.log(`📋 ${name}`);
    console.log(`${'='.repeat(70)}`);
  },

  success: (msg: string) => {
    console.log(`✅ ${msg}`);
  },

  error: (msg: string) => {
    console.log(`❌ ${msg}`);
  },

  warning: (msg: string) => {
    console.log(`⚠️  ${msg}`);
  },

  info: (msg: string) => {
    console.log(`ℹ️  ${msg}`);
  },

  debug: (msg: string, obj?: any) => {
    if (process.env.QLEVER_DEBUG === '1') {
      console.log(`🔍 ${msg}`, obj || '');
    }
  },
};

// Make utilities available globally
declare global {
  var PerformanceTimer: typeof PerformanceTimer;
  var MemoryTracker: typeof MemoryTracker;
  var waitFor: typeof waitFor;
  var withTimeout: typeof withTimeout;
  var retry: typeof retry;
  var log: typeof log;
}

globalThis.PerformanceTimer = PerformanceTimer;
globalThis.MemoryTracker = MemoryTracker;
globalThis.waitFor = waitFor;
globalThis.withTimeout = withTimeout;
globalThis.retry = retry;
globalThis.log = log;
