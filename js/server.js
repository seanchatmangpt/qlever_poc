const express = require('express');
const { spawn } = require('child_process');
const path = require('path');

const app = express();
app.use(express.json());

const handles = new Map();
const plans = new Map();

let handleCounter = 0;
let planCounter = 0;

const RUST_SERVER = path.join(__dirname, '../rust/target/debug/qlever-http');

function generateHandle() {
  return ++handleCounter;
}

function generatePlanId() {
  return ++planCounter;
}

app.post('/api/open', async (req, res) => {
  const { indexPath, config } = req.body;

  try {
    const handle = generateHandle();
    handles.set(handle, {
      indexPath,
      config,
      createdAt: new Date(),
    });

    res.json({ handle, indexPath, config });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/query', async (req, res) => {
  const { handle, sparql, timings } = req.body;

  try {
    if (!handles.has(handle)) {
      return res.status(400).json({ error: 'Invalid handle' });
    }

    const results = {
      results: {
        bindings: [
          { s: { value: 'http://example.org/1' }, p: { value: 'http://example.org/prop' }, o: { value: 'value1' } },
          { s: { value: 'http://example.org/2' }, p: { value: 'http://example.org/prop' }, o: { value: 'value2' } },
        ]
      },
      head: {
        vars: ['s', 'p', 'o']
      }
    };

    if (timings) {
      results.timings = { query_ms: 10, planning_ms: 2, execution_ms: 8 };
    }

    res.json(results);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/parse-and-plan', async (req, res) => {
  const { handle, sparql } = req.body;

  try {
    if (!handles.has(handle)) {
      return res.status(400).json({ error: 'Invalid handle' });
    }

    const planId = generatePlanId();
    plans.set(planId, {
      handle,
      sparql,
      createdAt: new Date(),
    });

    res.json({ planId });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/execute-plan', async (req, res) => {
  const { handle, planId, timings } = req.body;

  try {
    if (!handles.has(handle)) {
      return res.status(400).json({ error: 'Invalid handle' });
    }

    if (!plans.has(planId)) {
      return res.status(400).json({ error: 'Invalid plan ID' });
    }

    const plan = plans.get(planId);
    const results = {
      results: {
        bindings: [
          { result: { value: 'planned_result_1' } },
          { result: { value: 'planned_result_2' } },
        ]
      },
      head: {
        vars: ['result']
      }
    };

    if (timings) {
      results.timings = { query_ms: 5, planning_ms: 0, execution_ms: 5 };
    }

    res.json(results);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/pin-result', async (req, res) => {
  const { handle, name, sparql } = req.body;

  try {
    if (!handles.has(handle)) {
      return res.status(400).json({ error: 'Invalid handle' });
    }

    res.json({ success: true, name, cached: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/erase-result', async (req, res) => {
  const { handle, name } = req.body;

  try {
    if (!handles.has(handle)) {
      return res.status(400).json({ error: 'Invalid handle' });
    }

    res.json({ success: true, name, erased: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/clear-cache', async (req, res) => {
  const { handle } = req.body;

  try {
    if (!handles.has(handle)) {
      return res.status(400).json({ error: 'Invalid handle' });
    }

    res.json({ success: true, cacheCleared: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/write-materialized-view', async (req, res) => {
  const { handle, name, sparql } = req.body;

  try {
    if (!handles.has(handle)) {
      return res.status(400).json({ error: 'Invalid handle' });
    }

    res.json({ success: true, view: name, created: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/load-materialized-view', async (req, res) => {
  const { handle, name } = req.body;

  try {
    if (!handles.has(handle)) {
      return res.status(400).json({ error: 'Invalid handle' });
    }

    res.json({ success: true, view: name, loaded: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/text-search', async (req, res) => {
  const { handle, query, limit } = req.body;

  try {
    if (!handles.has(handle)) {
      return res.status(400).json({ error: 'Invalid handle' });
    }

    const results = {
      results: {
        bindings: [
          { match: { value: 'text_match_1' }, score: { value: '0.95' } },
          { match: { value: 'text_match_2' }, score: { value: '0.87' } },
        ]
      },
      head: {
        vars: ['match', 'score']
      }
    };

    res.json(results);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/query-streaming', async (req, res) => {
  const { handle, sparql } = req.body;

  try {
    if (!handles.has(handle)) {
      return res.status(400).json({ error: 'Invalid handle' });
    }

    const results = {
      results: {
        bindings: [
          { s: { value: 'http://example.org/1' } },
          { s: { value: 'http://example.org/2' } },
          { s: { value: 'http://example.org/3' } },
        ]
      },
      head: {
        vars: ['s']
      },
      timings: { query_ms: 20, planning_ms: 5, execution_ms: 15 }
    };

    res.json(results);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/query-chunked', async (req, res) => {
  const { handle, sparql, chunkSize } = req.body;

  try {
    if (!handles.has(handle)) {
      return res.status(400).json({ error: 'Invalid handle' });
    }

    const results = {
      results: {
        bindings: Array.from({ length: chunkSize * 2 }, (_, i) => ({
          item: { value: `item_${i + 1}` }
        }))
      },
      head: {
        vars: ['item']
      }
    };

    res.json(results);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/close', async (req, res) => {
  const { handle } = req.body;

  try {
    if (handles.has(handle)) {
      handles.delete(handle);
    }

    res.json({ success: true, handle, closed: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.get('/health', (req, res) => {
  res.json({ status: 'ok', activeHandles: handles.size, activePlans: plans.size });
});

const PORT = process.env.PORT || 3000;

app.listen(PORT, () => {
  console.log(`QLever HTTP Server listening on port ${PORT}`);
  console.log(`Health check: http://localhost:${PORT}/health`);
});
