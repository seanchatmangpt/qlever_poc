class WasmProcessor {
  constructor(wasmModule) {
    this.wasm = wasmModule;
    this.store = null;
  }

  async initialize(indexPath) {
    if (this.wasm && this.wasm.WasmStore) {
      this.store = new this.wasm.WasmStore(indexPath);
      return true;
    }
    return false;
  }

  filterResults(resultsJson, filterVar, filterValue) {
    if (!this.store) {
      throw new Error('WASM store not initialized');
    }
    try {
      const filtered = this.store.filter_results(resultsJson, filterVar, filterValue);
      return JSON.parse(filtered);
    } catch (error) {
      console.error('Filter error:', error);
      throw error;
    }
  }

  limitResults(resultsJson, limit) {
    if (!this.store) {
      throw new Error('WASM store not initialized');
    }
    try {
      const limited = this.store.limit_results(resultsJson, limit);
      return JSON.parse(limited);
    } catch (error) {
      console.error('Limit error:', error);
      throw error;
    }
  }

  aggregateResults(resultsJson, groupByVar, aggVar, aggType) {
    if (!this.store) {
      throw new Error('WASM store not initialized');
    }
    try {
      const aggregated = this.store.aggregate_results(resultsJson, groupByVar, aggVar, aggType);
      return JSON.parse(aggregated);
    } catch (error) {
      console.error('Aggregation error:', error);
      throw error;
    }
  }

  static mergeResults(resultsList) {
    try {
      const merged = WasmProcessor.WasmStore.merge_results(JSON.stringify(resultsList));
      return JSON.parse(merged);
    } catch (error) {
      console.error('Merge error:', error);
      throw error;
    }
  }

  static deduplicateResults(resultsJson) {
    try {
      const deduplicated = WasmProcessor.WasmStore.deduplicate_results(resultsJson);
      return JSON.parse(deduplicated);
    } catch (error) {
      console.error('Deduplication error:', error);
      throw error;
    }
  }

  processQuery(resultsJson, operations) {
    let result = JSON.parse(resultsJson);

    for (const op of operations) {
      switch (op.type) {
        case 'filter':
          result = this.filterResults(JSON.stringify(result), op.variable, op.value);
          break;
        case 'limit':
          result = this.limitResults(JSON.stringify(result), op.limit);
          break;
        case 'aggregate':
          result = this.aggregateResults(
            JSON.stringify(result),
            op.groupBy,
            op.variable,
            op.function
          );
          break;
        case 'deduplicate':
          result = WasmProcessor.deduplicateResults(JSON.stringify(result));
          break;
      }
    }

    return result;
  }
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = WasmProcessor;
}
