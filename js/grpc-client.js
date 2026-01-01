const grpc = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');
const path = require('path');

const PROTO_PATH = path.join(__dirname, 'qlever.proto');

const packageDefinition = protoLoader.loadSync(PROTO_PATH, {
  keepCase: true,
  longs: String,
  enums: String,
  defaults: true,
  oneofs: true
});

const qleverProto = grpc.loadPackageDefinition(packageDefinition).qlever;

class GrpcQleverClient {
  constructor(serverUrl = 'localhost:50051') {
    this.client = new qleverProto.QleverService(
      serverUrl,
      grpc.credentials.createInsecure()
    );
    this.handle = null;
  }

  async openIndex(indexPath, config = null) {
    return new Promise((resolve, reject) => {
      this.client.openIndex(
        { indexPath, config: config ? JSON.stringify(config) : '' },
        (err, response) => {
          if (err) reject(err);
          else {
            this.handle = response.handle;
            resolve(response);
          }
        }
      );
    });
  }

  async query(sparql, timings = false) {
    if (!this.handle) throw new Error('Index not opened');

    return new Promise((resolve, reject) => {
      this.client.query(
        { handle: this.handle, sparql, timings },
        (err, response) => {
          if (err) reject(err);
          else resolve(response);
        }
      );
    });
  }

  async parseAndPlan(sparql) {
    if (!this.handle) throw new Error('Index not opened');

    return new Promise((resolve, reject) => {
      this.client.parseAndPlan(
        { handle: this.handle, sparql },
        (err, response) => {
          if (err) reject(err);
          else resolve(response);
        }
      );
    });
  }

  async executePlan(planId, timings = false) {
    if (!this.handle) throw new Error('Index not opened');

    return new Promise((resolve, reject) => {
      this.client.executePlan(
        { handle: this.handle, planId, timings },
        (err, response) => {
          if (err) reject(err);
          else resolve(response);
        }
      );
    });
  }

  async textSearch(query, limit = 50) {
    if (!this.handle) throw new Error('Index not opened');

    return new Promise((resolve, reject) => {
      this.client.textSearch(
        { handle: this.handle, query, limit },
        (err, response) => {
          if (err) reject(err);
          else resolve(response);
        }
      );
    });
  }

  async cacheResult(name, sparql) {
    if (!this.handle) throw new Error('Index not opened');

    return new Promise((resolve, reject) => {
      this.client.cacheResult(
        { handle: this.handle, name, sparql },
        (err, response) => {
          if (err) reject(err);
          else resolve(response);
        }
      );
    });
  }

  async eraseResult(name) {
    if (!this.handle) throw new Error('Index not opened');

    return new Promise((resolve, reject) => {
      this.client.eraseResult(
        { handle: this.handle, name },
        (err, response) => {
          if (err) reject(err);
          else resolve(response);
        }
      );
    });
  }

  async clearCache() {
    if (!this.handle) throw new Error('Index not opened');

    return new Promise((resolve, reject) => {
      this.client.clearCache(
        { handle: this.handle },
        (err, response) => {
          if (err) reject(err);
          else resolve(response);
        }
      );
    });
  }

  async writeMaterializedView(name, sparql) {
    if (!this.handle) throw new Error('Index not opened');

    return new Promise((resolve, reject) => {
      this.client.writeMaterializedView(
        { handle: this.handle, name, sparql },
        (err, response) => {
          if (err) reject(err);
          else resolve(response);
        }
      );
    });
  }

  async loadMaterializedView(name) {
    if (!this.handle) throw new Error('Index not opened');

    return new Promise((resolve, reject) => {
      this.client.loadMaterializedView(
        { handle: this.handle, name },
        (err, response) => {
          if (err) reject(err);
          else resolve(response);
        }
      );
    });
  }

  async streamQuery(sparql) {
    if (!this.handle) throw new Error('Index not opened');

    return new Promise((resolve, reject) => {
      const chunks = [];
      const call = this.client.streamQuery({
        handle: this.handle,
        sparql
      });

      call.on('data', (chunk) => {
        chunks.push(chunk);
      });

      call.on('error', (err) => {
        reject(err);
      });

      call.on('end', () => {
        resolve(chunks);
      });
    });
  }

  async close() {
    if (!this.handle) return;

    return new Promise((resolve, reject) => {
      this.client.closeIndex(
        { handle: this.handle },
        (err, response) => {
          if (err) reject(err);
          else {
            this.handle = null;
            resolve(response);
          }
        }
      );
    });
  }
}

module.exports = GrpcQleverClient;
