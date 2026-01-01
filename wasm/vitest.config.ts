import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    environment: 'node',
    globals: true,
    include: ['tests/**/*.perf.ts'],
    testTimeout: 300000,
    hookTimeout: 300000,
    benchmark: {
      include: ['tests/**/*.perf.ts'],
      outputFile: 'benchmarks.json',
    },
  },
});
