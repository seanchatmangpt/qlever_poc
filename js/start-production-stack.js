const { spawn } = require('child_process');
const path = require('path');
const ServerCoordinator = require('./server-coordinator');

const servers = {
  http: null,
  websocket: null,
  graphql: null,
  grpc: null
};

const coordinator = new ServerCoordinator({
  httpPort: 3000,
  wsPort: 3002,
  graphqlPort: 3003,
  grpcPort: 50051,
  coordinatorPort: 3001,
  healthCheckInterval: 5000
});

async function startServer(name, command, cwd = process.cwd()) {
  return new Promise((resolve, reject) => {
    console.log(`\n[${name.toUpperCase()}] Starting...`);

    const proc = spawn('node', [command], {
      cwd,
      stdio: ['ignore', 'pipe', 'pipe']
    });

    servers[name] = proc;

    let outputBuffer = '';
    let errorBuffer = '';

    proc.stdout.on('data', (data) => {
      const output = data.toString();
      outputBuffer += output;
      process.stdout.write(`[${name.toUpperCase()}] ${output}`);

      if (output.includes('listening') || output.includes('Health Check')) {
        setTimeout(() => resolve(proc), 1000);
      }
    });

    proc.stderr.on('data', (data) => {
      errorBuffer += data.toString();
      process.stderr.write(`[${name.toUpperCase()}] ERROR: ${data.toString()}`);
    });

    proc.on('error', (error) => {
      console.error(`[${name.toUpperCase()}] Failed to start:`, error.message);
      reject(error);
    });

    proc.on('exit', (code) => {
      if (code !== 0) {
        console.error(`[${name.toUpperCase()}] Exited with code ${code}`);
        reject(new Error(`${name} exited with code ${code}`));
      }
    });

    setTimeout(() => {
      if (!servers[name].killed) {
        resolve(proc);
      }
    }, 5000);
  });
}

async function startProductionStack() {
  console.log(`
╔════════════════════════════════════════════════════════╗
║    QLever Production Stack (Self-Coordinating)        ║
╚════════════════════════════════════════════════════════╝
`);

  try {
    const jsDir = __dirname;

    console.log('\n[STACK] Starting all protocol servers in parallel...\n');

    await Promise.all([
      startServer('http', path.join(jsDir, 'server.js')),
      startServer('websocket', path.join(jsDir, 'websocket-server.js')),
      startServer('graphql', path.join(jsDir, 'graphql-server.js')),
      startServer('grpc', path.join(jsDir, 'grpc-server.js'))
    ]).catch(error => {
      console.error('[STACK] Error starting servers:', error.message);
    });

    console.log('\n[STACK] All protocol servers started');
    console.log('[STACK] Waiting for servers to stabilize...\n');

    await new Promise(resolve => setTimeout(resolve, 3000));

    await coordinator.startHealthChecking();

    coordinator.startManagementServer(3001);

    console.log('\n[STACK] Production stack fully initialized and ready');
    console.log('[STACK] Coordinator is monitoring all protocol servers\n');
  } catch (error) {
    console.error('[STACK] Fatal error:', error.message);
    process.exit(1);
  }
}

async function gracefulShutdown() {
  console.log('\n\n[STACK] Graceful shutdown initiated...\n');

  const shutdownPromises = [];
  for (const [name, proc] of Object.entries(servers)) {
    if (proc && !proc.killed) {
      console.log(`[STACK] Stopping ${name} server...`);
      shutdownPromises.push(
        new Promise(resolve => {
          const timeout = setTimeout(() => {
            console.warn(`[${name.toUpperCase()}] Force kill timeout`);
            proc.kill('SIGKILL');
            resolve();
          }, 5000);

          proc.on('exit', () => {
            clearTimeout(timeout);
            resolve();
          });

          proc.kill('SIGTERM');
        })
      );
    }
  }

  await Promise.all(shutdownPromises);
  console.log('[STACK] All servers stopped');
  process.exit(0);
}

process.on('SIGTERM', gracefulShutdown);
process.on('SIGINT', gracefulShutdown);

process.on('uncaughtException', (error) => {
  console.error('[STACK] Uncaught exception:', error);
  gracefulShutdown();
});

startProductionStack();
