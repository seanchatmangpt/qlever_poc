const { createClient } = require('redis');

class RedisCacheLayer {
  constructor(options = {}) {
    this.url = options.url || 'redis://localhost:6379';
    this.ttl = options.ttl || 3600;
    this.client = null;
    this.connected = false;
  }

  async connect() {
    try {
      this.client = createClient({ url: this.url });
      this.client.on('error', err => console.log('Redis Client Error', err));
      await this.client.connect();
      this.connected = true;
      console.log('Connected to Redis');
    } catch (error) {
      console.warn('Redis connection failed, continuing without caching:', error.message);
      this.connected = false;
    }
  }

  async get(key) {
    if (!this.connected) return null;
    try {
      const value = await this.client.get(key);
      if (value) {
        return JSON.parse(value);
      }
      return null;
    } catch (error) {
      console.error('Cache get error:', error);
      return null;
    }
  }

  async set(key, value, ttl = null) {
    if (!this.connected) return false;
    try {
      const expirySeconds = ttl || this.ttl;
      await this.client.setEx(key, expirySeconds, JSON.stringify(value));
      return true;
    } catch (error) {
      console.error('Cache set error:', error);
      return false;
    }
  }

  async del(key) {
    if (!this.connected) return false;
    try {
      await this.client.del(key);
      return true;
    } catch (error) {
      console.error('Cache delete error:', error);
      return false;
    }
  }

  async invalidatePattern(pattern) {
    if (!this.connected) return 0;
    try {
      const keys = await this.client.keys(pattern);
      if (keys.length > 0) {
        await this.client.del(keys);
      }
      return keys.length;
    } catch (error) {
      console.error('Cache invalidation error:', error);
      return 0;
    }
  }

  async clear() {
    if (!this.connected) return false;
    try {
      await this.client.flushDb();
      return true;
    } catch (error) {
      console.error('Cache clear error:', error);
      return false;
    }
  }

  async disconnect() {
    if (this.client) {
      await this.client.quit();
      this.connected = false;
    }
  }

  getCacheKey(handle, sparql, type = 'query') {
    const hash = require('crypto')
      .createHash('md5')
      .update(sparql)
      .digest('hex');
    return `${type}:${handle}:${hash}`;
  }

  middleware() {
    return async (req, res, next) => {
      const originalJson = res.json;
      const self = this;

      req.cache = {
        get: (key) => self.get(key),
        set: (key, value, ttl) => self.set(key, value, ttl),
        del: (key) => self.del(key),
        getQueryKey: (handle, sparql) => self.getCacheKey(handle, sparql, 'query')
      };

      res.json = function(data) {
        const isCacheable = req.method === 'POST' && (
          req.path === '/api/query' ||
          req.path === '/api/text-search' ||
          req.path === '/api/query-streaming'
        );

        if (isCacheable && req.body) {
          const handle = req.body.handle;
          const queryKey = req.body.sparql || req.body.query;
          if (handle && queryKey) {
            const cacheKey = self.getCacheKey(handle, queryKey, 'result');
            self.set(cacheKey, data);
          }
        }

        return originalJson.call(this, data);
      };

      next();
    };
  }
}

module.exports = RedisCacheLayer;
