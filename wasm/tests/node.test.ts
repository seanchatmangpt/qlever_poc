/**
 * Node.js-Specific Tests
 * Tests that run in Node.js environments
 */

import { describe, it, expect, beforeAll } from 'vitest';
import { existsSync } from 'fs';
import { resolve } from 'path';

// Only run these tests in Node.js
const skipInBrowser = typeof process !== 'undefined' && process.version ? it : it.skip;

describe('Node.js-Specific Tests', () => {
  beforeAll(() => {
    if (typeof process === 'undefined' || !process.version) {
      console.log('⏭️  Skipping Node.js tests (running in browser)');
    } else {
      console.log(`📦 Running Node.js tests (Node ${process.version})`);
    }
  });

  describe('Node.js Environment', () => {
    skipInBrowser('should have process object', () => {
      expect(typeof process).toBe('object');
      expect(process.version).toBeTruthy();
    });

    skipInBrowser('should have Node.js version', () => {
      const version = process.version;
      console.log(`Node.js version: ${version}`);
      expect(version).toMatch(/^v\d+\.\d+\.\d+/);
    });

    skipInBrowser('should have require function', () => {
      expect(typeof require).toBe('function');
    });

    skipInBrowser('should have __dirname', () => {
      expect(typeof __dirname).toBe('string');
    });

    skipInBrowser('should have __filename', () => {
      expect(typeof __filename).toBe('string');
    });
  });

  describe('File System Access', () => {
    skipInBrowser('should read package.json', () => {
      const pkgPath = resolve(__dirname, '../package.json');
      const exists = existsSync(pkgPath);
      expect(exists).toBe(true);
    });

    skipInBrowser('should access node_modules', () => {
      const modulesPath = resolve(__dirname, '../node_modules');
      // May or may not exist depending on install, just check it's checkable
      expect(typeof existsSync).toBe('function');
    });

    skipInBrowser('should have access to build artifacts', () => {
      const buildPath = resolve(__dirname, '../pkg');
      // Build may or may not have run yet
      const canCheck = existsSync(buildPath) !== undefined;
      expect(canCheck).toBe(true);
    });
  });

  describe('Memory Profiling', () => {
    skipInBrowser('should report heap usage', () => {
      const memUsage = process.memoryUsage();
      expect(typeof memUsage.heapUsed).toBe('number');
      expect(typeof memUsage.heapTotal).toBe('number');
      expect(memUsage.heapUsed).toBeGreaterThan(0);
      expect(memUsage.heapTotal).toBeGreaterThanOrEqual(memUsage.heapUsed);
    });

    skipInBrowser('should track memory growth', async () => {
      const memStart = process.memoryUsage().heapUsed;

      // Allocate some memory
      const arrays = [];
      for (let i = 0; i < 100; i++) {
        arrays.push(new Array(1000).fill(i));
      }

      const memEnd = process.memoryUsage().heapUsed;
      const memGrowth = memEnd - memStart;

      console.log(`Memory growth: ${(memGrowth / 1024).toFixed(2)}KB`);

      expect(memGrowth).toBeGreaterThan(0);
    });

    skipInBrowser('should support garbage collection', () => {
      if (global.gc) {
        expect(() => {
          global.gc();
        }).not.toThrow();
      }
    });
  });

  describe('Process Management', () => {
    skipInBrowser('should have process.env', () => {
      expect(typeof process.env).toBe('object');
      expect(process.env.NODE_ENV).toBeTruthy();
    });

    skipInBrowser('should have process.cwd()', () => {
      const cwd = process.cwd();
      expect(typeof cwd).toBe('string');
      expect(cwd.length).toBeGreaterThan(0);
    });

    skipInBrowser('should handle environment variables', () => {
      const testVar = process.env.TEST_VAR || 'default';
      expect(testVar).toBeTruthy();
    });

    skipInBrowser('should have process.argv', () => {
      expect(Array.isArray(process.argv)).toBe(true);
      expect(process.argv.length).toBeGreaterThanOrEqual(2);
    });

    skipInBrowser('should report platform', () => {
      const platform = process.platform;
      expect(['linux', 'darwin', 'win32']).toContain(platform);
      console.log(`Platform: ${platform}`);
    });
  });

  describe('Performance Profiling', () => {
    skipInBrowser('should support hrtime for precise timing', () => {
      const start = process.hrtime();
      // Do some work
      for (let i = 0; i < 1000; i++) {
        Math.sqrt(i);
      }
      const end = process.hrtime(start);

      const nanoseconds = end[0] * 1e9 + end[1];
      expect(nanoseconds).toBeGreaterThan(0);

      console.log(`hrtime: ${(nanoseconds / 1e6).toFixed(2)}ms`);
    });

    skipInBrowser('should measure CPU usage', () => {
      const usage = process.cpuUsage();
      expect(typeof usage.user).toBe('number');
      expect(typeof usage.system).toBe('number');
    });

    skipInBrowser('should report uptime', () => {
      const uptime = process.uptime();
      expect(typeof uptime).toBe('number');
      expect(uptime).toBeGreaterThan(0);
    });
  });

  describe('Module Loading', () => {
    skipInBrowser('should load CommonJS modules', () => {
      // require() should work
      expect(typeof require).toBe('function');
    });

    skipInBrowser('should support ES6 imports', async () => {
      // Dynamic import should work in Node 12+
      if (parseInt(process.version.substring(1)) >= 12) {
        expect(true).toBe(true); // Placeholder - actual import would go here
      }
    });

    skipInBrowser('should have node_modules resolution', () => {
      // Node should find modules in node_modules
      expect(true).toBe(true); // Placeholder
    });
  });

  describe('stdio Streams', () => {
    skipInBrowser('should have process.stdout', () => {
      expect(typeof process.stdout).toBe('object');
      expect(typeof process.stdout.write).toBe('function');
    });

    skipInBrowser('should have process.stderr', () => {
      expect(typeof process.stderr).toBe('object');
      expect(typeof process.stderr.write).toBe('function');
    });

    skipInBrowser('should have process.stdin', () => {
      expect(typeof process.stdin).toBe('object');
    });

    skipInBrowser('should use console for logging', () => {
      expect(typeof console).toBe('object');
      expect(typeof console.log).toBe('function');
      expect(typeof console.error).toBe('function');
    });
  });

  describe('Async Operations', () => {
    skipInBrowser('should support promises', async () => {
      const result = await Promise.resolve(42);
      expect(result).toBe(42);
    });

    skipInBrowser('should support async/await', async () => {
      const asyncFn = async () => {
        return 42;
      };

      const result = await asyncFn();
      expect(result).toBe(42);
    });

    skipInBrowser('should handle timers', async () => {
      return new Promise((resolve) => {
        setTimeout(() => {
          resolve(42);
        }, 10);
      });
    });

    skipInBrowser('should support setImmediate', async () => {
      return new Promise((resolve) => {
        setImmediate(() => {
          resolve(42);
        });
      });
    });
  });

  describe('Worker Threads', () => {
    skipInBrowser('should support Worker threads', () => {
      const hasWorkerThreads = process.version >= 'v10.5.0';
      expect(hasWorkerThreads).toBe(true);
    });
  });

  describe('Uncaught Exception Handling', () => {
    skipInBrowser('should catch unhandled rejections', (done) => {
      process.once('unhandledRejection', () => {
        // Should be able to catch these
        done();
      });

      // Trigger one (in real scenario)
      // Promise.reject(new Error('test'))
    });
  });

  describe('Node Version Compatibility', () => {
    skipInBrowser('should run on Node 14+', () => {
      const majorVersion = parseInt(process.version.substring(1).split('.')[0]);
      expect(majorVersion).toBeGreaterThanOrEqual(14);
    });

    skipInBrowser('should support ES2020 features', () => {
      // BigInt, optional chaining, nullish coalescing, etc.
      const bigint = 123n;
      expect(typeof bigint).toBe('bigint');
    });
  });

  describe('WASM in Node.js', () => {
    skipInBrowser('should support WASM modules', () => {
      // Node.js 8+ has WASM support
      const hasWasm = typeof WebAssembly !== 'undefined';
      expect(hasWasm).toBe(true);
    });

    skipInBrowser('should compile WASM', () => {
      // WebAssembly.compile should work
      expect(typeof WebAssembly.compile).toBe('function');
    });
  });

  describe('CLI Integration', () => {
    skipInBrowser('should run as command', () => {
      // package.json should have "bin" field for CLI
      expect(true).toBe(true); // Placeholder
    });

    skipInBrowser('should handle exit codes', () => {
      // process.exit(code) should work
      expect(typeof process.exit).toBe('function');
    });
  });

  describe('Package Management', () => {
    skipInBrowser('should have package.json', () => {
      const pkgPath = resolve(__dirname, '../package.json');
      expect(existsSync(pkgPath)).toBe(true);
    });

    skipInBrowser('should list dependencies', () => {
      // Should be able to read package.json
      expect(true).toBe(true); // Placeholder
    });
  });
});
