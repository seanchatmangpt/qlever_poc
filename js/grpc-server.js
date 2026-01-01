const grpc = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');
const path = require('path');
const axios = require('axios');

const PROTO_PATH = path.join(__dirname, 'qlever.proto');

const packageDefinition = protoLoader.loadSync(PROTO_PATH, {
  keepCase: true,
  longs: String,
  enums: String,
  defaults: true,
  oneofs: true
});

const qleverProto = grpc.loadPackageDefinition(packageDefinition).qlever;

const sessions = new Map();
let handleCounter = 0;

const queuedStreams = new Map();

const implementation = {
  openIndex: async (call, callback) => {
    const { indexPath, config } = call.request;

    try {
      const response = await axios.post('http://localhost:3000/api/open', {
        indexPath,
        config: config ? JSON.parse(config) : null
      });

      const handle = ++handleCounter;
      sessions.set(handle, {
        indexPath,
        createdAt: new Date()
      });

      callback(null, {
        handle,
        indexPath: response.data.indexPath,
        success: true,
        error: ''
      });
    } catch (error) {
      callback(null, {
        handle: 0,
        indexPath: '',
        success: false,
        error: error.message
      });
    }
  },

  query: async (call, callback) => {
    const { handle, sparql, timings } = call.request;

    if (!sessions.has(handle)) {
      callback(null, {
        variables: [],
        bindings: [],
        success: false,
        error: 'Invalid handle'
      });
      return;
    }

    try {
      const response = await axios.post('http://localhost:3000/api/query', {
        handle,
        sparql,
        timings: timings ? 1 : 0
      });

      const bindings = response.data.results.bindings.map(binding => ({
        values: binding
      }));

      callback(null, {
        variables: response.data.head.vars,
        bindings,
        timings: response.data.timings ? {
          query_ms: response.data.timings.query_ms,
          planning_ms: response.data.timings.planning_ms,
          execution_ms: response.data.timings.execution_ms
        } : null,
        success: true,
        error: ''
      });
    } catch (error) {
      callback(null, {
        variables: [],
        bindings: [],
        success: false,
        error: error.message
      });
    }
  },

  parseAndPlan: async (call, callback) => {
    const { handle, sparql } = call.request;

    if (!sessions.has(handle)) {
      callback(null, {
        planId: '',
        success: false,
        error: 'Invalid handle'
      });
      return;
    }

    try {
      const response = await axios.post('http://localhost:3000/api/parse-and-plan', {
        handle,
        sparql
      });

      callback(null, {
        planId: response.data.planId,
        success: true,
        error: ''
      });
    } catch (error) {
      callback(null, {
        planId: '',
        success: false,
        error: error.message
      });
    }
  },

  executePlan: async (call, callback) => {
    const { handle, planId, timings } = call.request;

    if (!sessions.has(handle)) {
      callback(null, {
        variables: [],
        bindings: [],
        success: false,
        error: 'Invalid handle'
      });
      return;
    }

    try {
      const response = await axios.post('http://localhost:3000/api/execute-plan', {
        handle,
        planId,
        timings: timings ? 1 : 0
      });

      const bindings = response.data.results.bindings.map(binding => ({
        values: binding
      }));

      callback(null, {
        variables: response.data.head.vars,
        bindings,
        timings: response.data.timings ? {
          query_ms: response.data.timings.query_ms,
          planning_ms: response.data.timings.planning_ms,
          execution_ms: response.data.timings.execution_ms
        } : null,
        success: true,
        error: ''
      });
    } catch (error) {
      callback(null, {
        variables: [],
        bindings: [],
        success: false,
        error: error.message
      });
    }
  },

  textSearch: async (call, callback) => {
    const { handle, query, limit } = call.request;

    if (!sessions.has(handle)) {
      callback(null, {
        variables: [],
        results: [],
        count: 0,
        error: 'Invalid handle'
      });
      return;
    }

    try {
      const response = await axios.post('http://localhost:3000/api/text-search', {
        handle,
        query,
        limit: limit || 50
      });

      const results = response.data.results.bindings.map(binding =>
        Object.values(binding).map(v => v.value).join(' ')
      );

      callback(null, {
        variables: response.data.head.vars,
        results,
        count: results.length,
        error: ''
      });
    } catch (error) {
      callback(null, {
        variables: [],
        results: [],
        count: 0,
        error: error.message
      });
    }
  },

  cacheResult: async (call, callback) => {
    const { handle, name, sparql } = call.request;

    if (!sessions.has(handle)) {
      callback(null, {
        handle,
        name,
        cached: false,
        error: 'Invalid handle'
      });
      return;
    }

    try {
      await axios.post('http://localhost:3000/api/pin-result', {
        handle,
        name,
        sparql
      });

      callback(null, {
        handle,
        name,
        cached: true,
        error: ''
      });
    } catch (error) {
      callback(null, {
        handle,
        name,
        cached: false,
        error: error.message
      });
    }
  },

  eraseResult: async (call, callback) => {
    const { handle, name } = call.request;

    if (!sessions.has(handle)) {
      callback(null, {
        handle,
        name,
        erased: false,
        error: 'Invalid handle'
      });
      return;
    }

    try {
      await axios.post('http://localhost:3000/api/erase-result', {
        handle,
        name
      });

      callback(null, {
        handle,
        name,
        erased: true,
        error: ''
      });
    } catch (error) {
      callback(null, {
        handle,
        name,
        erased: false,
        error: error.message
      });
    }
  },

  clearCache: async (call, callback) => {
    const { handle } = call.request;

    if (!sessions.has(handle)) {
      callback(null, {
        handle,
        cleared: false,
        error: 'Invalid handle'
      });
      return;
    }

    try {
      await axios.post('http://localhost:3000/api/clear-cache', { handle });

      callback(null, {
        handle,
        cleared: true,
        error: ''
      });
    } catch (error) {
      callback(null, {
        handle,
        cleared: false,
        error: error.message
      });
    }
  },

  writeMaterializedView: async (call, callback) => {
    const { handle, name, sparql } = call.request;

    if (!sessions.has(handle)) {
      callback(null, {
        handle,
        name,
        success: false,
        error: 'Invalid handle'
      });
      return;
    }

    try {
      await axios.post('http://localhost:3000/api/write-materialized-view', {
        handle,
        name,
        sparql
      });

      callback(null, {
        handle,
        name,
        success: true,
        error: ''
      });
    } catch (error) {
      callback(null, {
        handle,
        name,
        success: false,
        error: error.message
      });
    }
  },

  loadMaterializedView: async (call, callback) => {
    const { handle, name } = call.request;

    if (!sessions.has(handle)) {
      callback(null, {
        handle,
        name,
        success: false,
        error: 'Invalid handle'
      });
      return;
    }

    try {
      await axios.post('http://localhost:3000/api/load-materialized-view', {
        handle,
        name
      });

      callback(null, {
        handle,
        name,
        success: true,
        error: ''
      });
    } catch (error) {
      callback(null, {
        handle,
        name,
        success: false,
        error: error.message
      });
    }
  },

  closeIndex: async (call, callback) => {
    const { handle } = call.request;

    if (sessions.has(handle)) {
      sessions.delete(handle);
    }

    try {
      await axios.post('http://localhost:3000/api/close', { handle });

      callback(null, {
        handle,
        closed: true,
        error: ''
      });
    } catch (error) {
      callback(null, {
        handle,
        closed: false,
        error: error.message
      });
    }
  },

  streamQuery: async (call) => {
    const { handle, sparql } = call.request;

    if (!sessions.has(handle)) {
      call.destroy();
      return;
    }

    try {
      const response = await axios.post('http://localhost:3000/api/query', {
        handle,
        sparql,
        timings: 1
      });

      const bindings = response.data.results.bindings;
      const chunkSize = 50;

      for (let i = 0; i < bindings.length; i += chunkSize) {
        const chunk = bindings.slice(i, Math.min(i + chunkSize, bindings.length));
        const protoBindings = chunk.map(binding => ({
          values: binding
        }));

        call.write({
          bindings: protoBindings,
          isLast: i + chunkSize >= bindings.length,
          chunkIndex: Math.floor(i / chunkSize)
        });
      }

      call.end();
    } catch (error) {
      call.destroy();
    }
  }
};

function startServer() {
  const server = new grpc.Server();
  server.addService(qleverProto.QleverService.service, implementation);

  const PORT = process.env.GRPC_PORT || 50051;
  server.bindAsync(`0.0.0.0:${PORT}`, grpc.ServerCredentials.createInsecure(), () => {
    console.log(`gRPC server listening on 0.0.0.0:${PORT}`);
  });
}

startServer();
