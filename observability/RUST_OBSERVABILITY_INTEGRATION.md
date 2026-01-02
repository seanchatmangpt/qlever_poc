# Rust Observability Plane Integration

**EPIC 10.3 Agent 6 Deliverable: Integration Plan for Rust Observability**

**Author:** Claude Assistant (Agent 6: OOB Telemetry)

**Date:** 2026-01-02

**Status:** Specification Complete - Ready for Agent 1 (FFI) Integration

---

## Overview

This document specifies the integration points between the eBPF telemetry infrastructure (Agent 6) and the Rust observability plane. The integration enables out-of-band (OOB) telemetry collection from C++ query operations and consumption by Rust-based observability services.

### Key Principles

1. **Zero-Copy Event Transfer**: eBPF events are shared via memory-mapped ring buffers
2. **Structured Telemetry**: All events are JSON-LD formatted for machine readability
3. **Read-Only Access**: Rust observability plane consumes events but cannot modify C++ state
4. **Performance Budget**: < 2.0% aggregate overhead (including eBPF + Rust processing)
5. **Fail-Closed**: Telemetry failures are observable (not silent)

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    C++ QLever Core                          │
│                                                              │
│  ┌──────────┐   ┌──────────┐   ┌──────────────┐           │
│  │  Join    │   │  Filter  │   │  IndexScan   │           │
│  │::execute │   │::execute │   │::execute     │           │
│  └────┬─────┘   └────┬─────┘   └────┬─────────┘           │
│       │              │              │                       │
│       │ (uprobes)    │ (uprobes)    │ (uprobes)            │
│       ▼              ▼              ▼                       │
│  ┌────────────────────────────────────────────┐            │
│  │        eBPF Kernel Instrumentation         │            │
│  │  (qlever_uprobes.bpf.o)                    │            │
│  │                                             │            │
│  │  - Capture enter/exit events                │            │
│  │  - Measure duration, rows, cols             │            │
│  │  - Compute overhead statistics              │            │
│  └─────────────────┬──────────────────────────┘            │
│                    │                                         │
│                    │ (ring buffer)                           │
│                    ▼                                         │
│  ┌────────────────────────────────────────────┐            │
│  │   EBPFTelemetryLoader (C++ User-Space)     │            │
│  │                                             │            │
│  │  - Poll ring buffer for events              │            │
│  │  - Convert to JSON-LD                       │            │
│  │  - Enforce overhead budget                  │            │
│  │  - Expose events to FFI boundary            │            │
│  └─────────────────┬──────────────────────────┘            │
│                    │                                         │
└────────────────────┼─────────────────────────────────────────┘
                     │ (FFI boundary)
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              Rust Observability Plane                        │
│                                                              │
│  ┌────────────────────────────────────────────┐            │
│  │   ObservabilityEventConsumer (Rust)        │            │
│  │                                             │            │
│  │  - Consume JSON-LD events from C++          │            │
│  │  - Deserialize to Rust structs              │            │
│  │  - Forward to telemetry sinks               │            │
│  │    (Prometheus, OpenTelemetry, Logs)        │            │
│  └────────────────────────────────────────────┘            │
│                                                              │
│  Telemetry Sinks:                                            │
│  - Prometheus metrics export (HTTP /metrics)                 │
│  - OpenTelemetry traces (OTLP)                               │
│  - Structured logs (JSON-LD)                                 │
│  - Real-time dashboard (WebSocket)                           │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## Integration Points

### 1. FFI Event Stream (Agent 1 Dependency)

**Status:** Blocked on Agent 1 (FFI Architect) - OpaqueHandle definitions required

**Forward-Compatible Stub:**

```cpp
// observability/ebpf/EBPFTelemetryFFI.h
// (To be implemented when Agent 1 completes FFI interface)

extern "C" {
    // Initialize eBPF telemetry loader
    // Returns opaque handle to telemetry stream
    void* qlever_ebpf_telemetry_init(const char* ebpf_object_path);

    // Poll for new events (non-blocking)
    // Returns number of events read
    // Events are written to out_buffer as JSON-LD strings
    int32_t qlever_ebpf_telemetry_poll(
        void* telemetry_handle,
        char* out_buffer,
        size_t buffer_size,
        size_t max_events
    );

    // Get current overhead statistics
    // Returns overhead percentage (x100 for precision)
    uint32_t qlever_ebpf_telemetry_overhead_pct(void* telemetry_handle);

    // Cleanup telemetry resources
    void qlever_ebpf_telemetry_destroy(void* telemetry_handle);
}
```

**Rust FFI Bindings (Auto-Generated via bindgen):**

```rust
// rust/src/observability/ebpf_bindings.rs
// (Generated from EBPFTelemetryFFI.h via bindgen)

use std::os::raw::{c_char, c_int, c_void};

extern "C" {
    pub fn qlever_ebpf_telemetry_init(
        ebpf_object_path: *const c_char
    ) -> *mut c_void;

    pub fn qlever_ebpf_telemetry_poll(
        telemetry_handle: *mut c_void,
        out_buffer: *mut c_char,
        buffer_size: usize,
        max_events: usize,
    ) -> c_int;

    pub fn qlever_ebpf_telemetry_overhead_pct(
        telemetry_handle: *mut c_void
    ) -> u32;

    pub fn qlever_ebpf_telemetry_destroy(
        telemetry_handle: *mut c_void
    );
}
```

### 2. Event Schema (JSON-LD)

**OperationEvent Schema:**

```json
{
  "@context": "https://qlever.cs.uni-freiburg.de/ns/observability",
  "@type": "OperationEvent",
  "timestamp_ns": 1735826400000000000,
  "operation_ptr": "0xDEADBEEF",
  "pid": 1234,
  "tid": 5678,
  "operation_type": "Join",
  "event_type": "exit",
  "duration_ns": 1000000,
  "result_rows": 100,
  "result_cols": 5
}
```

**OverheadStatistics Schema:**

```json
{
  "@type": "OverheadStatistics",
  "total_probes_fired": 10000,
  "total_overhead_ns": 15000000,
  "baseline_execution_ns": 1000000000,
  "current_overhead_pct": 1.5
}
```

### 3. Rust Event Consumer

**Rust Struct Definitions:**

```rust
// rust/src/observability/operation_event.rs

use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum OperationType {
    Join,
    Filter,
    IndexScan,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum EventType {
    Enter,
    Exit,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct OperationEvent {
    #[serde(rename = "@type")]
    pub type_field: String,  // "OperationEvent"

    pub timestamp_ns: u64,
    pub operation_ptr: String,  // Hex string
    pub pid: u32,
    pub tid: u32,
    pub operation_type: OperationType,
    pub event_type: EventType,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub duration_ns: Option<u64>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub result_rows: Option<u64>,

    #[serde(skip_serializing_if = "Option::is_none")]
    pub result_cols: Option<u64>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct OverheadStatistics {
    #[serde(rename = "@type")]
    pub type_field: String,  // "OverheadStatistics"

    pub total_probes_fired: u64,
    pub total_overhead_ns: u64,
    pub baseline_execution_ns: u64,
    pub current_overhead_pct: f32,
}

impl OverheadStatistics {
    pub fn exceeds_budget(&self, budget_pct: f32) -> bool {
        self.current_overhead_pct > budget_pct
    }
}
```

**Event Consumer Service:**

```rust
// rust/src/observability/ebpf_consumer.rs

use std::ffi::CString;
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;
use tokio::sync::mpsc;

use crate::observability::ebpf_bindings::*;
use crate::observability::operation_event::*;

pub struct EBPFEventConsumer {
    telemetry_handle: *mut std::os::raw::c_void,
    event_tx: mpsc::UnboundedSender<OperationEvent>,
    running: Arc<Mutex<bool>>,
}

impl EBPFEventConsumer {
    pub fn new(ebpf_object_path: &str) -> Result<Self, Box<dyn std::error::Error>> {
        let path_cstr = CString::new(ebpf_object_path)?;

        // Initialize eBPF telemetry via FFI
        let handle = unsafe {
            qlever_ebpf_telemetry_init(path_cstr.as_ptr())
        };

        if handle.is_null() {
            return Err("Failed to initialize eBPF telemetry".into());
        }

        let (tx, rx) = mpsc::unbounded_channel();

        Ok(EBPFEventConsumer {
            telemetry_handle: handle,
            event_tx: tx,
            running: Arc::new(Mutex::new(false)),
        })
    }

    pub fn start(&mut self) {
        let mut running = self.running.lock().unwrap();
        if *running {
            return;
        }
        *running = true;

        // Spawn polling thread
        let handle = self.telemetry_handle;
        let tx = self.event_tx.clone();
        let running_flag = self.running.clone();

        thread::spawn(move || {
            let mut buffer = vec![0u8; 1024 * 1024];  // 1MB buffer

            while *running_flag.lock().unwrap() {
                let events_read = unsafe {
                    qlever_ebpf_telemetry_poll(
                        handle,
                        buffer.as_mut_ptr() as *mut i8,
                        buffer.len(),
                        100,  // max 100 events per poll
                    )
                };

                if events_read > 0 {
                    // Parse JSON-LD events from buffer
                    // (Implementation details omitted for brevity)
                    // Send events to mpsc channel
                }

                thread::sleep(Duration::from_millis(100));
            }
        });
    }

    pub fn stop(&mut self) {
        let mut running = self.running.lock().unwrap();
        *running = false;
    }

    pub fn get_overhead_pct(&self) -> f32 {
        unsafe {
            let pct_x100 = qlever_ebpf_telemetry_overhead_pct(self.telemetry_handle);
            (pct_x100 as f32) / 100.0
        }
    }
}

impl Drop for EBPFEventConsumer {
    fn drop(&mut self) {
        self.stop();
        unsafe {
            qlever_ebpf_telemetry_destroy(self.telemetry_handle);
        }
    }
}
```

---

## Performance Budget Enforcement

**C++ Side:**
- eBPF probes measure their own overhead and update global counters
- `EBPFTelemetryLoader::checkOverheadBudget()` logs warning if > 2%
- Overhead test gate (`ebpf_overhead_gate`) aborts build if budget exceeded

**Rust Side:**
- `EBPFEventConsumer::get_overhead_pct()` polls C++ telemetry for overhead
- Rust observability plane can disable eBPF collection if budget exceeded
- Overhead metrics exposed to Prometheus for alerting

---

## Integration Roadmap

### Phase 1: Stub FFI (Current - Agent 6 Complete)
- ✅ eBPF uprobes implemented
- ✅ Read-only access guards implemented
- ✅ Performance overhead measurement complete
- ⏸️ **BLOCKED:** FFI interface (Agent 1 dependency)

### Phase 2: FFI Bridge (Agent 1 Required)
- Implement `EBPFTelemetryFFI.h` C API
- Generate Rust bindings via `bindgen`
- Wire C++ `EBPFTelemetryLoader` to FFI exports

### Phase 3: Rust Consumer (Agent 1 + Agent 6)
- Implement `EBPFEventConsumer` service
- Deserialize JSON-LD events to Rust structs
- Forward events to telemetry sinks

### Phase 4: Telemetry Sinks (Post-Agent 6)
- Prometheus metrics exporter
- OpenTelemetry trace integration
- Real-time dashboard (WebSocket)

---

## Success Criteria

- ✅ eBPF uprobes compile and load into kernel
- ✅ Uprobes attached to Join/Filter/IndexScan::execute()
- ✅ Read-only access guard prevents writes to Operation metadata
- ✅ Performance overhead < 2.0% (measured via `ebpf_overhead_test`)
- ✅ Events structured as JSON-LD (Rust-consumable)
- ⏸️ FFI integration (blocked on Agent 1)
- ⏸️ Rust consumer service (blocked on Agent 1)

---

## References

- EPIC 10.3 Convergence Roadmap: Agent 6 specification
- EPIC 10.3 Collision Detection Report: Agent 1 (FFI) + Agent 6 (Telemetry) integration
- eBPF Programming Guide: https://www.kernel.org/doc/html/latest/bpf/
- libbpf Documentation: https://libbpf.readthedocs.io/
- JSON-LD Specification: https://www.w3.org/TR/json-ld/

---

**Agent 6 Status: DELIVERABLES COMPLETE (Pending Agent 1 FFI Integration)**
