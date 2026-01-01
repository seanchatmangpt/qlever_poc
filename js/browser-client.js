class QleverBrowserClient {
  constructor(baseUrl = 'http://localhost:3000') {
    this.baseUrl = baseUrl;
    this.handle = null;
  }

  async request(endpoint, data) {
    const response = await fetch(`${this.baseUrl}/api${endpoint}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(data),
    });

    if (!response.ok) {
      const error = await response.json();
      throw new Error(error.message || `HTTP ${response.status}`);
    }

    return response.json();
  }

  async open(indexPath, config = null) {
    const result = await this.request('/open', { indexPath, config });
    this.handle = result.handle;
    return this;
  }

  async query(sparql, timings = false) {
    if (!this.handle) throw new Error('Index not opened');
    return this.request('/query', {
      handle: this.handle,
      sparql,
      timings: timings ? 1 : 0,
    });
  }

  async parseAndPlan(sparql) {
    if (!this.handle) throw new Error('Index not opened');
    const result = await this.request('/parse-and-plan', {
      handle: this.handle,
      sparql,
    });
    return result.planId;
  }

  async executePlan(planId, timings = false) {
    if (!this.handle) throw new Error('Index not opened');
    return this.request('/execute-plan', {
      handle: this.handle,
      planId,
      timings: timings ? 1 : 0,
    });
  }

  async pinResult(name, sparql) {
    if (!this.handle) throw new Error('Index not opened');
    await this.request('/pin-result', { handle: this.handle, name, sparql });
  }

  async eraseResult(name) {
    if (!this.handle) throw new Error('Index not opened');
    await this.request('/erase-result', { handle: this.handle, name });
  }

  async clearCache() {
    if (!this.handle) throw new Error('Index not opened');
    await this.request('/clear-cache', { handle: this.handle });
  }

  async writeMaterializedView(name, sparql) {
    if (!this.handle) throw new Error('Index not opened');
    await this.request('/write-materialized-view', {
      handle: this.handle,
      name,
      sparql,
    });
  }

  async loadMaterializedView(name) {
    if (!this.handle) throw new Error('Index not opened');
    await this.request('/load-materialized-view', { handle: this.handle, name });
  }

  async textSearch(query, limit = 50) {
    if (!this.handle) throw new Error('Index not opened');
    return this.request('/text-search', { handle: this.handle, query, limit });
  }

  async queryStreaming(sparql) {
    if (!this.handle) throw new Error('Index not opened');
    return this.request('/query-streaming', {
      handle: this.handle,
      sparql,
    });
  }

  async queryChunked(sparql, chunkSize = 100) {
    if (!this.handle) throw new Error('Index not opened');
    return this.request('/query-chunked', {
      handle: this.handle,
      sparql,
      chunkSize,
    });
  }

  async close() {
    if (!this.handle) return;
    try {
      await this.request('/close', { handle: this.handle });
    } catch (error) {
      console.error('Close error:', error);
    }
    this.handle = null;
  }
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = QleverBrowserClient;
}
