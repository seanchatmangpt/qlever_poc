const axios = require('axios');

class QleverClient {
  constructor(baseUrl = 'http://localhost:3000') {
    this.baseUrl = baseUrl;
    this.client = axios.create({
      baseURL: baseUrl,
      timeout: 30000,
      headers: {
        'Content-Type': 'application/json',
      },
    });
  }

  async open(indexPath, config = null) {
    try {
      const response = await this.client.post('/api/open', {
        indexPath,
        config,
      });
      this.handle = response.data.handle;
      return this;
    } catch (error) {
      throw new Error(`Failed to open index: ${error.message}`);
    }
  }

  async query(sparql, timings = false) {
    if (!this.handle) throw new Error('Index not opened');

    try {
      const response = await this.client.post('/api/query', {
        handle: this.handle,
        sparql,
        timings: timings ? 1 : 0,
      });
      return response.data;
    } catch (error) {
      throw new Error(`Query failed: ${error.message}`);
    }
  }

  async parseAndPlan(sparql) {
    if (!this.handle) throw new Error('Index not opened');

    try {
      const response = await this.client.post('/api/parse-and-plan', {
        handle: this.handle,
        sparql,
      });
      return response.data.planId;
    } catch (error) {
      throw new Error(`Parse and plan failed: ${error.message}`);
    }
  }

  async executePlan(planId, timings = false) {
    if (!this.handle) throw new Error('Index not opened');

    try {
      const response = await this.client.post('/api/execute-plan', {
        handle: this.handle,
        planId,
        timings: timings ? 1 : 0,
      });
      return response.data;
    } catch (error) {
      throw new Error(`Execute plan failed: ${error.message}`);
    }
  }

  async pinResult(name, sparql) {
    if (!this.handle) throw new Error('Index not opened');

    try {
      await this.client.post('/api/pin-result', {
        handle: this.handle,
        name,
        sparql,
      });
    } catch (error) {
      throw new Error(`Pin result failed: ${error.message}`);
    }
  }

  async eraseResult(name) {
    if (!this.handle) throw new Error('Index not opened');

    try {
      await this.client.post('/api/erase-result', {
        handle: this.handle,
        name,
      });
    } catch (error) {
      throw new Error(`Erase result failed: ${error.message}`);
    }
  }

  async clearCache() {
    if (!this.handle) throw new Error('Index not opened');

    try {
      await this.client.post('/api/clear-cache', {
        handle: this.handle,
      });
    } catch (error) {
      throw new Error(`Clear cache failed: ${error.message}`);
    }
  }

  async writeMaterializedView(name, sparql) {
    if (!this.handle) throw new Error('Index not opened');

    try {
      await this.client.post('/api/write-materialized-view', {
        handle: this.handle,
        name,
        sparql,
      });
    } catch (error) {
      throw new Error(`Write materialized view failed: ${error.message}`);
    }
  }

  async loadMaterializedView(name) {
    if (!this.handle) throw new Error('Index not opened');

    try {
      await this.client.post('/api/load-materialized-view', {
        handle: this.handle,
        name,
      });
    } catch (error) {
      throw new Error(`Load materialized view failed: ${error.message}`);
    }
  }

  async textSearch(query, limit = 50) {
    if (!this.handle) throw new Error('Index not opened');

    try {
      const response = await this.client.post('/api/text-search', {
        handle: this.handle,
        query,
        limit,
      });
      return response.data;
    } catch (error) {
      throw new Error(`Text search failed: ${error.message}`);
    }
  }

  async close() {
    if (!this.handle) return;

    try {
      await this.client.post('/api/close', {
        handle: this.handle,
      });
      this.handle = null;
    } catch (error) {
      console.error(`Close failed: ${error.message}`);
    }
  }
}

module.exports = QleverClient;
