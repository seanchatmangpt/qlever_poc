if (typeof module !== 'undefined' && module.exports) {
  module.exports = class WebSocketQleverClient {
    constructor(serverUrl = 'http://localhost:3002') {
      this.serverUrl = serverUrl;
      this.socket = null;
      this.handle = null;
      this.pendingQueries = new Map();
      this.queryId = 0;
    }

    connect() {
      return new Promise((resolve, reject) => {
        const io = require('socket.io-client');
        this.socket = io(this.serverUrl);

        this.socket.on('connect', () => {
          resolve();
        });

        this.socket.on('error', (error) => {
          reject(error);
        });

        this.setupEventHandlers();
      });
    }

    setupEventHandlers() {
      this.socket.on('opened', (data) => {
        const pending = this.pendingQueries.get('open');
        if (pending) {
          pending.resolve(data);
          this.pendingQueries.delete('open');
          this.handle = data.handle;
        }
      });

      this.socket.on('query-result', (data) => {
        const pending = this.pendingQueries.get(`query-${data.handle}`);
        if (pending) {
          pending.resolve(data);
          this.pendingQueries.delete(`query-${data.handle}`);
        }
      });

      this.socket.on('stream-batch', (data) => {
        const pending = this.pendingQueries.get(`stream-${data.handle}`);
        if (pending) {
          pending.onBatch(data);
        }
      });

      this.socket.on('stream-complete', (data) => {
        const pending = this.pendingQueries.get(`stream-${data.handle}`);
        if (pending) {
          pending.resolve(data);
          this.pendingQueries.delete(`stream-${data.handle}`);
        }
      });

      this.socket.on('plan-executed', (data) => {
        const pending = this.pendingQueries.get(`plan-${data.handle}`);
        if (pending) {
          pending.resolve(data);
          this.pendingQueries.delete(`plan-${data.handle}`);
        }
      });

      this.socket.on('result-cached', (data) => {
        const pending = this.pendingQueries.get(`cache-${data.handle}`);
        if (pending) {
          pending.resolve(data);
          this.pendingQueries.delete(`cache-${data.handle}`);
        }
      });

      this.socket.on('search-results', (data) => {
        const pending = this.pendingQueries.get(`search-${data.handle}`);
        if (pending) {
          pending.resolve(data);
          this.pendingQueries.delete(`search-${data.handle}`);
        }
      });

      this.socket.on('closed', (data) => {
        const pending = this.pendingQueries.get(`close-${data.handle}`);
        if (pending) {
          pending.resolve(data);
          this.pendingQueries.delete(`close-${data.handle}`);
        }
      });

      this.socket.on('error', (data) => {
        console.error('WebSocket Error:', data);
        this.rejectAllPending(new Error(data.message || 'WebSocket error'));
      });
    }

    async open(indexPath, config = null) {
      return new Promise((resolve, reject) => {
        this.pendingQueries.set('open', { resolve, reject });
        this.socket.emit('open', { indexPath, config });
      });
    }

    async query(sparql, timings = false) {
      return new Promise((resolve, reject) => {
        const key = `query-${this.handle}`;
        this.pendingQueries.set(key, { resolve, reject });
        this.socket.emit('query', { handle: this.handle, sparql, timings });
      });
    }

    async streamQuery(sparql, onBatch, batchSize = 100) {
      return new Promise((resolve, reject) => {
        const key = `stream-${this.handle}`;
        this.pendingQueries.set(key, {
          resolve,
          reject,
          onBatch: onBatch || (() => {})
        });
        this.socket.emit('stream-query', {
          handle: this.handle,
          sparql,
          batchSize
        });
      });
    }

    async planAndExecute(sparql, reusable = false) {
      return new Promise((resolve, reject) => {
        const key = `plan-${this.handle}`;
        this.pendingQueries.set(key, { resolve, reject });
        this.socket.emit('plan-and-execute', {
          handle: this.handle,
          sparql,
          reusable
        });
      });
    }

    async cacheResult(name, sparql) {
      return new Promise((resolve, reject) => {
        const key = `cache-${this.handle}`;
        this.pendingQueries.set(key, { resolve, reject });
        this.socket.emit('cache-result', { handle: this.handle, name, sparql });
      });
    }

    async textSearch(query, limit = 50) {
      return new Promise((resolve, reject) => {
        const key = `search-${this.handle}`;
        this.pendingQueries.set(key, { resolve, reject });
        this.socket.emit('text-search', { handle: this.handle, query, limit });
      });
    }

    async close() {
      return new Promise((resolve, reject) => {
        const key = `close-${this.handle}`;
        this.pendingQueries.set(key, { resolve, reject });
        this.socket.emit('close', { handle: this.handle });
      });
    }

    rejectAllPending(error) {
      for (const [key, pending] of this.pendingQueries) {
        if (pending.reject) {
          pending.reject(error);
        }
      }
      this.pendingQueries.clear();
    }
  };
} else {
  window.WebSocketQleverClient = class WebSocketQleverClient {
    constructor(serverUrl = 'http://localhost:3002') {
      this.serverUrl = serverUrl;
      this.socket = null;
      this.handle = null;
      this.pendingQueries = new Map();
    }

    connect() {
      return new Promise((resolve, reject) => {
        this.socket = io(this.serverUrl);

        this.socket.on('connect', () => {
          resolve();
        });

        this.socket.on('error', (error) => {
          reject(error);
        });

        this.setupEventHandlers();
      });
    }

    setupEventHandlers() {
      this.socket.on('opened', (data) => {
        const pending = this.pendingQueries.get('open');
        if (pending) {
          pending.resolve(data);
          this.pendingQueries.delete('open');
          this.handle = data.handle;
        }
      });

      this.socket.on('query-result', (data) => {
        const pending = this.pendingQueries.get(`query-${data.handle}`);
        if (pending) {
          pending.resolve(data);
          this.pendingQueries.delete(`query-${data.handle}`);
        }
      });

      this.socket.on('stream-batch', (data) => {
        const pending = this.pendingQueries.get(`stream-${data.handle}`);
        if (pending && pending.onBatch) {
          pending.onBatch(data);
        }
      });

      this.socket.on('stream-complete', (data) => {
        const pending = this.pendingQueries.get(`stream-${data.handle}`);
        if (pending) {
          pending.resolve(data);
          this.pendingQueries.delete(`stream-${data.handle}`);
        }
      });

      this.socket.on('plan-executed', (data) => {
        const pending = this.pendingQueries.get(`plan-${data.handle}`);
        if (pending) {
          pending.resolve(data);
          this.pendingQueries.delete(`plan-${data.handle}`);
        }
      });

      this.socket.on('result-cached', (data) => {
        const pending = this.pendingQueries.get(`cache-${data.handle}`);
        if (pending) {
          pending.resolve(data);
          this.pendingQueries.delete(`cache-${data.handle}`);
        }
      });

      this.socket.on('search-results', (data) => {
        const pending = this.pendingQueries.get(`search-${data.handle}`);
        if (pending) {
          pending.resolve(data);
          this.pendingQueries.delete(`search-${data.handle}`);
        }
      });

      this.socket.on('closed', (data) => {
        const pending = this.pendingQueries.get(`close-${data.handle}`);
        if (pending) {
          pending.resolve(data);
          this.pendingQueries.delete(`close-${data.handle}`);
        }
      });

      this.socket.on('error', (data) => {
        console.error('WebSocket Error:', data);
        this.rejectAllPending(new Error(data.message || 'WebSocket error'));
      });
    }

    async open(indexPath, config = null) {
      return new Promise((resolve, reject) => {
        this.pendingQueries.set('open', { resolve, reject });
        this.socket.emit('open', { indexPath, config });
      });
    }

    async query(sparql, timings = false) {
      return new Promise((resolve, reject) => {
        const key = `query-${this.handle}`;
        this.pendingQueries.set(key, { resolve, reject });
        this.socket.emit('query', { handle: this.handle, sparql, timings });
      });
    }

    async streamQuery(sparql, onBatch, batchSize = 100) {
      return new Promise((resolve, reject) => {
        const key = `stream-${this.handle}`;
        this.pendingQueries.set(key, {
          resolve,
          reject,
          onBatch: onBatch || (() => {})
        });
        this.socket.emit('stream-query', {
          handle: this.handle,
          sparql,
          batchSize
        });
      });
    }

    async planAndExecute(sparql, reusable = false) {
      return new Promise((resolve, reject) => {
        const key = `plan-${this.handle}`;
        this.pendingQueries.set(key, { resolve, reject });
        this.socket.emit('plan-and-execute', {
          handle: this.handle,
          sparql,
          reusable
        });
      });
    }

    async cacheResult(name, sparql) {
      return new Promise((resolve, reject) => {
        const key = `cache-${this.handle}`;
        this.pendingQueries.set(key, { resolve, reject });
        this.socket.emit('cache-result', { handle: this.handle, name, sparql });
      });
    }

    async textSearch(query, limit = 50) {
      return new Promise((resolve, reject) => {
        const key = `search-${this.handle}`;
        this.pendingQueries.set(key, { resolve, reject });
        this.socket.emit('text-search', { handle: this.handle, query, limit });
      });
    }

    async close() {
      return new Promise((resolve, reject) => {
        const key = `close-${this.handle}`;
        this.pendingQueries.set(key, { resolve, reject });
        this.socket.emit('close', { handle: this.handle });
      });
    }

    rejectAllPending(error) {
      for (const [key, pending] of this.pendingQueries) {
        if (pending.reject) {
          pending.reject(error);
        }
      }
      this.pendingQueries.clear();
    }
  };
}
