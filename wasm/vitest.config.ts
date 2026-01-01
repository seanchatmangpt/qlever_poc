import { defineConfig } from 'vitest/config';
import path from 'path';

export default defineConfig({
  test: {
    // Environment setup
    environment: 'node',
    globals: true,
    isolate: true,

    // Test file patterns
    include: ['tests/**/*.test.ts', 'tests/**/*.test.js'],
    exclude: ['node_modules', 'dist', 'pkg', 'build', 'wasm_build'],

    // Coverage configuration
    coverage: {
      provider: 'v8',
      reporter: ['text', 'json', 'html', 'lcov'],
      exclude: [
        'node_modules/',
        'tests/',
        'examples/',
        'dist/',
        'pkg/',
        'build/',
        'wasm_build/',
      ],
      lines: 80,
      functions: 85,
      branches: 70,
      statements: 80,
    },

    // Timeout and performance
    testTimeout: 30000,
    hookTimeout: 30000,
    teardownTimeout: 10000,

    // Output and reporting
    reporters: ['verbose'],
    outputFile: {
      html: 'coverage/index.html',
      json: 'coverage/coverage.json',
    },

    // Parallel execution
    threads: true,
    maxThreads: 4,
    minThreads: 1,

    // Setup files
    setupFiles: ['./tests/setup.ts'],

    // Vitest-specific options
    mockReset: true,
    restoreMocks: true,
    clearMocks: true,

    // Benchmark options
    benchmark: {
      include: ['tests/performance.test.ts'],
      outputFile: 'coverage/benchmark.json',
      exclude: ['node_modules'],
    },
  },

  resolve: {
    alias: {
      '@': path.resolve(__dirname, './src'),
      '@tests': path.resolve(__dirname, './tests'),
    },
  },
});
