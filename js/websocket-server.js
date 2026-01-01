const express = require('express');
const http = require('http');
const { Server: SocketIOServer } = require('socket.io');
const axios = require('axios');

const app = express();
app.use(express.json());
const server = http.createServer(app);
const io = new SocketIOServer(server, {
  cors: { origin: '*' }
});

const sessions = new Map();
let handleCounter = 0;

app.get('/health', (req, res) => {
  res.json({
    status: 'ok',
    activeSessions: sessions.size,
    wsConnections: io.engine.clientsCount
  });
});

io.on('connection', (socket) => {
  console.log(`Client connected: ${socket.id}`);

  socket.on('open', async (data) => {
    const handle = ++handleCounter;
    const indexPath = data.indexPath || './index';

    try {
      const response = await axios.post('http://localhost:3000/api/open', {
        indexPath,
        config: data.config || null
      });

      sessions.set(handle, {
        socketId: socket.id,
        indexPath,
        createdAt: new Date()
      });

      socket.emit('opened', { handle, success: true });
      socket.data.handle = handle;
    } catch (error) {
      socket.emit('error', { message: error.message });
    }
  });

  socket.on('query', async (data) => {
    const { handle, sparql, timings } = data;

    if (!sessions.has(handle)) {
      socket.emit('error', { message: 'Invalid handle' });
      return;
    }

    try {
      const response = await axios.post('http://localhost:3000/api/query', {
        handle,
        sparql,
        timings: timings ? 1 : 0
      });

      socket.emit('query-result', {
        handle,
        results: response.data.results.bindings,
        variables: response.data.head.vars,
        timings: response.data.timings,
        complete: true
      });
    } catch (error) {
      socket.emit('error', { message: error.message });
    }
  });

  socket.on('stream-query', async (data) => {
    const { handle, sparql, batchSize } = data;

    if (!sessions.has(handle)) {
      socket.emit('error', { message: 'Invalid handle' });
      return;
    }

    try {
      const response = await axios.post('http://localhost:3000/api/query', {
        handle,
        sparql,
        timings: 1
      });

      const bindings = response.data.results.bindings;
      const batch = batchSize || 100;

      for (let i = 0; i < bindings.length; i += batch) {
        socket.emit('stream-batch', {
          handle,
          batch: i / batch,
          results: bindings.slice(i, i + batch),
          variables: response.data.head.vars,
          isLast: i + batch >= bindings.length
        });
      }

      socket.emit('stream-complete', {
        handle,
        totalResults: bindings.length,
        timings: response.data.timings
      });
    } catch (error) {
      socket.emit('error', { message: error.message });
    }
  });

  socket.on('plan-and-execute', async (data) => {
    const { handle, sparql, reusable } = data;

    if (!sessions.has(handle)) {
      socket.emit('error', { message: 'Invalid handle' });
      return;
    }

    try {
      const planResponse = await axios.post('http://localhost:3000/api/parse-and-plan', {
        handle,
        sparql
      });

      const planId = planResponse.data.planId;

      const execResponse = await axios.post('http://localhost:3000/api/execute-plan', {
        handle,
        planId,
        timings: 1
      });

      if (reusable) {
        sessions.set(handle, Object.assign(sessions.get(handle), {
          lastPlanId: planId,
          lastQuery: sparql
        }));
      }

      socket.emit('plan-executed', {
        handle,
        planId,
        results: execResponse.data.results.bindings,
        variables: execResponse.data.head.vars,
        timings: execResponse.data.timings
      });
    } catch (error) {
      socket.emit('error', { message: error.message });
    }
  });

  socket.on('cache-result', async (data) => {
    const { handle, name, sparql } = data;

    if (!sessions.has(handle)) {
      socket.emit('error', { message: 'Invalid handle' });
      return;
    }

    try {
      await axios.post('http://localhost:3000/api/pin-result', {
        handle,
        name,
        sparql
      });

      socket.emit('result-cached', { handle, name, success: true });
    } catch (error) {
      socket.emit('error', { message: error.message });
    }
  });

  socket.on('text-search', async (data) => {
    const { handle, query, limit } = data;

    if (!sessions.has(handle)) {
      socket.emit('error', { message: 'Invalid handle' });
      return;
    }

    try {
      const response = await axios.post('http://localhost:3000/api/text-search', {
        handle,
        query,
        limit: limit || 50
      });

      socket.emit('search-results', {
        handle,
        results: response.data.results.bindings,
        variables: response.data.head.vars
      });
    } catch (error) {
      socket.emit('error', { message: error.message });
    }
  });

  socket.on('close', async (data) => {
    const { handle } = data;

    if (!sessions.has(handle)) {
      socket.emit('error', { message: 'Invalid handle' });
      return;
    }

    try {
      await axios.post('http://localhost:3000/api/close', { handle });
      sessions.delete(handle);
      socket.emit('closed', { handle, success: true });
    } catch (error) {
      socket.emit('error', { message: error.message });
    }
  });

  socket.on('disconnect', () => {
    console.log(`Client disconnected: ${socket.id}`);
    const handle = socket.data.handle;
    if (handle && sessions.has(handle)) {
      sessions.delete(handle);
    }
  });
});

const PORT = process.env.WS_PORT || 3002;
server.listen(PORT, () => {
  console.log(`WebSocket server listening on http://localhost:${PORT}`);
  console.log(`Health check: http://localhost:${PORT}/health`);
});
