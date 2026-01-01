const ARCHITECTURE = {
  version: '1.0',
  timestamp: new Date().toISOString(),

  LAYER_0_FOUNDATION: {
    description: 'Core QLever C++ Engine with FFI bridges',
    components: [
      'src/qlever_c.cpp - C wrapper exposing QLever C++ API',
      'rust/src/lib.rs - Rust bindings to QLever C++ with streaming support',
      'rust/src/wasm.rs - WebAssembly module for client-side result processing'
    ],
    capabilities: [
      'SPARQL query execution',
      'Result streaming',
      'Query planning',
      'Materialized view management',
      'Text search'
    ]
  },

  LAYER_1_PROTOCOL_SERVERS: {
    description: '4 multi-protocol access points to QLever engine',
    servers: {
      HTTP: {
        file: 'js/server.js',
        port: 3000,
        features: [
          'REST endpoints for all operations',
          'Express.js framework',
          'Integrated resilience wrapping',
          'Per-endpoint metrics collection',
          'Contract enforcement',
          '/api/* endpoints',
          '/health, /system/health, /system/metrics, /system/status'
        ]
      },
      WebSocket: {
        file: 'js/websocket-server.js',
        port: 3002,
        features: [
          'Real-time event-driven streaming',
          'Socket.io connection management',
          'Batch result streaming',
          'Live health updates via socket events',
          'Connection-specific session handling'
        ]
      },
      GraphQL: {
        file: 'js/graphql-server-instrumented.js',
        port: 3003,
        features: [
          'Flexible query interface with introspection',
          'Type-safe schema',
          'Apollo Server framework',
          'Built on ProtocolInstrumentation framework',
          'Native self-observation'
        ]
      },
      gRPC: {
        file: 'js/grpc-server.js',
        port: 50051,
        features: [
          'Binary protocol with Protocol Buffers',
          'High-performance streaming',
          'Service definition in qlever.proto',
          'Unary and server-side streaming'
        ]
      }
    }
  },

  LAYER_2_INSTRUMENTATION_FRAMEWORK: {
    description: 'Unified framework providing self-observation to all protocols',
    file: 'js/protocol-instrumentation.js',
    class: 'ProtocolInstrumentation',
    provides: {
      metrics: 'Operation count, error tracking, latency analysis (min/max/avg/P50/P95/P99)',
      resilience: 'Automatic retry, exponential backoff, circuit breaker per operation',
      contracts: 'Data integrity validation against defined contracts',
      health: 'Per-operation health scoring and aggregation',
      endpoints: 'Automatic /health, /metrics, /operation/:name, /recommendations'
    },
    usage: 'new ProtocolInstrumentation(protocolName, config).executeWithInstrumentation(op, handler, contracts)'
  },

  LAYER_3_RESILIENCE: {
    description: 'Automatic failure detection, classification, and recovery',
    file: 'js/resilience-layer.js',
    class: 'ResilienceHandler',
    capabilities: {
      retry: 'Exponential backoff: 50ms, 100ms, 200ms (3 retries max)',
      failureClassification: [
        'connection-refused → retry then fallback',
        'timeout → increase timeout window, retry',
        'invalid-handle → request new session',
        'serialization-error → attempt data recovery',
        'memory-pressure → reduce batch size',
        'index-corruption → trigger recovery'
      ],
      circuitBreaker: {
        states: 'closed (normal) → open (failing) → half-open (recovering)',
        threshold: '5 failures triggers open state',
        reset: '60 second timeout'
      }
    }
  },

  LAYER_4_HEALTH_MONITORING: {
    description: 'Unified health score aggregation and recommendations',
    file: 'js/system-health-monitor.js',
    class: 'SystemHealthMonitor',
    metrics: {
      protocolAvailability: '40% weight',
      contractSatisfaction: '30% weight',
      resilienceCapability: '30% weight',
      score: '0-100%',
      status: 'Excellent (≥95%), Good (80-94%), Degraded (60-79%), Critical (<60%)'
    },
    recommendations: 'Automatically generated based on health signals'
  },

  LAYER_5_CONTRACT_ENFORCEMENT: {
    description: 'Explicit data integrity contracts between all layers',
    file: 'js/contract-enforcer.js',
    class: 'ContractEnforcer',
    contracts: {
      'HTTP.Response.Structure': 'Requires head.vars[] and results.bindings[]',
      'HTTP.Timings.Structure': 'Requires query_ms, planning_ms, execution_ms as numbers',
      'HTTP.Handle.Validity': 'Requires handle > 0',
      'gRPC.Message.Serialization': 'Requires JSON-serializable messages',
      'Cache.KeyFormat': 'Requires "type:handle:hash" format',
      'SPARQL.Query.Syntax': 'Requires SELECT|ASK|CONSTRUCT keywords',
      'Result.Binding.Consistency': 'Requires all bindings have declared variables',
      'Index.Handle.Lifecycle': 'Requires open (number) → closed (null)',
      'Protocol.Latency.Bound': 'Latency < bound_ms',
      'Message.Roundtrip.Integrity': 'Sent === received (no corruption)'
    }
  },

  LAYER_6_COORDINATION: {
    description: 'Cross-protocol orchestration and unified management',
    file: 'js/server-coordinator.js',
    class: 'ServerCoordinator',
    responsibilities: {
      monitoring: 'Continuous health checking of all 4 protocol servers',
      aggregation: 'Unified metrics aggregation across protocols',
      routing: 'Automatic failover to healthy alternatives',
      recommendations: 'System-wide recommendations based on cross-protocol health',
      endpoints: [
        '/status - Current system state',
        '/health - Aggregated health score',
        '/metrics - Unified operation metrics',
        '/recommendations - Operational guidance',
        '/protocol/:name/details - Protocol-specific insights'
      ]
    }
  },

  LAYER_7_PRODUCTION_ORCHESTRATION: {
    description: 'Production stack launcher with graceful shutdown',
    file: 'js/start-production-stack.js',
    class: 'Production Stack Launcher',
    responsibilities: [
      'Start all 4 protocol servers in parallel',
      'Initialize coordinator and health monitoring',
      'Manage graceful shutdown on SIGTERM/SIGINT',
      'Force kill timeout handling'
    ],
    startCommand: 'node js/start-production-stack.js'
  },

  VALIDATION_FRAMEWORK: {
    description: 'End-to-end testing and validation of self-observation',
    components: {
      'Protocol Validator': 'js/protocol-validator.js - End-to-end protocol testing',
      'Stress Tester': 'js/stress-tester.js - Edge case and boundary testing',
      'Load Tester': 'js/load-tester.js - Sustained load testing with resilience observation',
      'Coherence Test': 'js/coherence-test.js - 5-phase system integration validation',
      'Integration Test': 'js/integration-test-self-observing.js - Comprehensive system test'
    }
  },

  CACHING_OPTIMIZATION: {
    description: 'Distributed caching layer for performance',
    file: 'js/redis-cache.js',
    class: 'RedisCacheLayer',
    features: [
      'Query result caching with TTL',
      'Distributed cache across all protocols',
      'Pattern-based invalidation',
      'Fallback to non-caching mode if Redis unavailable'
    ]
  },

  WASM_PROCESSING: {
    description: 'Client-side result processing without network round-trips',
    files: [
      'rust/src/wasm.rs - WebAssembly module implementation',
      'js/wasm-utils.js - JavaScript wrapper',
      'js/wasm-example.html - Interactive browser example'
    ],
    capabilities: [
      'Filter results by variable value',
      'Limit result set size',
      'Aggregate results with count/sum/avg/min/max',
      'Deduplicate results',
      'Merge multiple result sets'
    ]
  },

  DATA_FLOW: {
    'Request Path': [
      'Client (HTTP/WS/GraphQL/gRPC)',
      '→ Protocol Server (Layer 1)',
      '→ ProtocolInstrumentation (Layer 2)',
      '→ Resilience Handler (Layer 3)',
      '→ HTTP Bridge to C++ (Layer 0)',
      '→ QLever C++ Engine (Core)',
      '→ RDF Index & Query Execution'
    ],
    'Response Path': [
      'Results from QLever C++ Engine',
      '→ Contract Enforcement (Layer 5)',
      '→ Health Monitor Update (Layer 4)',
      '→ Metrics Recording (Layer 2)',
      '→ Protocol Server Response (Layer 1)',
      '→ Client receives result'
    ],
    'Observation Path': [
      'Every operation records metrics',
      '→ Resilience layer tracks failures',
      '→ Health monitor aggregates scores',
      '→ Coordinator collects across protocols',
      '→ Management API exposes unified health'
    ]
  },

  SELF_OBSERVATION_PROPERTIES: {
    'Complete Observability': 'Every layer observes itself and adjacent layers - no black boxes',
    'Explicit Contracts': 'Every interface has defined expectations - violations detected immediately',
    'Adaptive Behavior': 'System classifies failures automatically with recovery strategies per type',
    'Unified Monitoring': 'All signals aggregated into single health score with recommendations',
    'Proof of Integration': 'Coherence test validates all systems work together; Load test proves stability'
  },

  DEPLOYMENT: {
    development: 'Individual servers can be started separately for testing',
    production: 'Start all servers together with: node js/start-production-stack.js',
    monitoring: 'Coordinator provides real-time health at http://localhost:3001/status',
    testing: 'Run integration test with: node js/integration-test-self-observing.js'
  },

  METRICS_TRACKED: {
    perOperation: [
      'Total execution count',
      'Error count',
      'Latency (min/max/avg/P50/P95/P99)',
      'Success rate percentage',
      'Recent error history',
      'Custom metadata'
    ],
    perProtocol: [
      'Overall availability',
      'Operation count by type',
      'Error rates by classification',
      'Circuit breaker states',
      'Health score calculation'
    ],
    systemWide: [
      'Uptime duration',
      'Total operations across all protocols',
      'Aggregate success rate',
      'Cross-protocol failover status',
      'Resource utilization'
    ]
  }
};

if (require.main === module) {
  console.log(JSON.stringify(ARCHITECTURE, null, 2));
}

module.exports = ARCHITECTURE;
