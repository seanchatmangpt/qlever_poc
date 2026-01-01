import { describe, it, expect } from 'vitest';

describe('Infrastructure Modules - 80/20 Critical Functionality', () => {
  describe('ResilienceHandler', () => {
    it('should import ResilienceHandler', () => {
      const { ResilienceHandler } = require('../resilience-layer');
      expect(ResilienceHandler).toBeDefined();
    });

    it('should instantiate with config', () => {
      const { ResilienceHandler } = require('../resilience-layer');
      const handler = new ResilienceHandler({
        maxRetries: 3,
        retryDelay: 50,
        exponentialBackoff: true
      });
      expect(handler).toBeDefined();
    });

    it('should have executeWithResilience method', () => {
      const { ResilienceHandler } = require('../resilience-layer');
      const handler = new ResilienceHandler({ maxRetries: 1, retryDelay: 10 });
      expect(typeof handler.executeWithResilience).toBe('function');
    });

    it('should have createCircuitBreaker method', () => {
      const { ResilienceHandler } = require('../resilience-layer');
      const handler = new ResilienceHandler({ maxRetries: 1 });
      expect(typeof handler.createCircuitBreaker).toBe('function');
    });
  });

  describe('ContractEnforcer', () => {
    it('should import ContractEnforcer', () => {
      const { ContractEnforcer } = require('../contract-enforcer');
      expect(ContractEnforcer).toBeDefined();
    });

    it('should instantiate ContractEnforcer', () => {
      const { ContractEnforcer } = require('../contract-enforcer');
      const enforcer = new ContractEnforcer();
      expect(enforcer).toBeDefined();
    });

    it('should have assertStrict method', () => {
      const { ContractEnforcer } = require('../contract-enforcer');
      const enforcer = new ContractEnforcer();
      expect(typeof enforcer.assertStrict).toBe('function');
    });
  });

  describe('ProtocolInstrumentation', () => {
    it('should import ProtocolInstrumentation', () => {
      const ProtocolInstrumentation = require('../protocol-instrumentation');
      expect(ProtocolInstrumentation).toBeDefined();
    });

    it('should instantiate with protocol and config', () => {
      const ProtocolInstrumentation = require('../protocol-instrumentation');
      const instrumentation = new ProtocolInstrumentation('http', {
        maxRetries: 3,
        retryDelay: 50
      });
      expect(instrumentation).toBeDefined();
    });

    it('should have executeWithInstrumentation method', () => {
      const ProtocolInstrumentation = require('../protocol-instrumentation');
      const instrumentation = new ProtocolInstrumentation('test', {});
      expect(typeof instrumentation.executeWithInstrumentation).toBe('function');
    });

    it('should have getHealth method', () => {
      const ProtocolInstrumentation = require('../protocol-instrumentation');
      const instrumentation = new ProtocolInstrumentation('test', {});
      expect(typeof instrumentation.getHealth).toBe('function');
    });
  });

  describe('SystemHealthMonitor', () => {
    it('should import SystemHealthMonitor', () => {
      const { SystemHealthMonitor } = require('../system-health-monitor');
      expect(SystemHealthMonitor).toBeDefined();
    });

    it('should instantiate SystemHealthMonitor', () => {
      const { SystemHealthMonitor } = require('../system-health-monitor');
      const monitor = new SystemHealthMonitor();
      expect(monitor).toBeDefined();
    });

    it('should have runMonitoring async method', () => {
      const { SystemHealthMonitor } = require('../system-health-monitor');
      const monitor = new SystemHealthMonitor();
      expect(typeof monitor.runMonitoring).toBe('function');
    });

    it('should have health property', () => {
      const { SystemHealthMonitor } = require('../system-health-monitor');
      const monitor = new SystemHealthMonitor();
      expect(monitor.health).toBeDefined();
      expect(monitor.health.status).toBeDefined();
    });
  });
});
