// Copyright 2026, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant (EPIC 10.3 Agent 6: OOB Telemetry)
//
// Purpose: eBPF uprobe instrumentation for QLever operation telemetry
//
// EPIC 10.3 Agent 6 Deliverable: Out-of-band observability probes
// Performance Budget: < 2.0% aggregate overhead
//
// INVARIANT CONTRACT:
// - Read-only access to Operation metadata (no writes)
// - No instrumentation inside SIMD hot-loops (register spilling penalty)
// - Fail-closed on eBPF load errors (observability is not optional)
// - All telemetry events are structured (machine-readable)
//
// Gate Dependencies:
// - Agent 2 (FPV witness) - ensures correctness of probed functions
// - Agent 5 (memory isolation) - validates read-only access boundaries

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

// ============================================================================
// Data Structures for Telemetry Events
// ============================================================================

// Operation execution event (emitted on enter/exit)
struct operation_event {
    __u64 timestamp_ns;         // Monotonic timestamp
    __u64 operation_ptr;        // Pointer to Operation object (for correlation)
    __u32 pid;                  // Process ID
    __u32 tid;                  // Thread ID
    __u8  operation_type;       // 0=Join, 1=Filter, 2=IndexScan
    __u8  event_type;           // 0=enter, 1=exit
    __u16 reserved;             // Padding
    __u64 duration_ns;          // Execution duration (only on exit)
    __u64 result_rows;          // Number of result rows (only on exit)
    __u64 result_cols;          // Number of result columns (only on exit)
};

// Overhead measurement event (emitted periodically)
struct overhead_event {
    __u64 timestamp_ns;         // Monotonic timestamp
    __u64 total_probes_fired;   // Cumulative probe invocations
    __u64 total_overhead_ns;    // Cumulative probe overhead
    __u32 current_overhead_pct; // Current overhead percentage (x100 for precision)
    __u32 reserved;             // Padding
};

// ============================================================================
// BPF Maps (Communication with User-Space)
// ============================================================================

// Ring buffer for operation events
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024); // 256KB ring buffer
} operation_events SEC(".maps");

// Hash map for tracking operation entry timestamps (for duration calculation)
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240); // Support 10K concurrent operations
    __type(key, __u64);         // operation_ptr
    __type(value, __u64);       // entry_timestamp_ns
} operation_entry_times SEC(".maps");

// Global counters for overhead measurement
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 4);
    __type(key, __u32);
    __type(value, __u64);
} global_counters SEC(".maps");

// Counter indices
#define COUNTER_PROBES_FIRED 0
#define COUNTER_OVERHEAD_NS  1
#define COUNTER_BASELINE_NS  2
#define COUNTER_RESERVED     3

// ============================================================================
// Helper Functions
// ============================================================================

// Read Operation descriptor (read-only access via BPF_CORE_READ)
// Note: This is a STUB - will be replaced when Agent 1's FFI is available
// Currently assumes direct Operation* access
static __always_inline void read_operation_descriptor(void* operation_ptr,
                                                      char* descriptor_out,
                                                      __u32 max_len) {
    // STUB: In production, this would read OpaqueHandle metadata
    // For now, just zero-fill (minimal overhead)
    #pragma unroll
    for (__u32 i = 0; i < max_len && i < 64; i++) {
        descriptor_out[i] = 0;
    }
}

// Increment global counter atomically
static __always_inline void increment_counter(__u32 counter_id, __u64 delta) {
    __u64* counter = bpf_map_lookup_elem(&global_counters, &counter_id);
    if (counter) {
        __sync_fetch_and_add(counter, delta);
    }
}

// Get current monotonic timestamp
static __always_inline __u64 get_timestamp_ns(void) {
    return bpf_ktime_get_ns();
}

// ============================================================================
// Uprobe Handlers - Join::execute()
// ============================================================================

SEC("uprobe/join_execute_enter")
int BPF_KPROBE(join_execute_enter, void* this_ptr) {
    __u64 ts = get_timestamp_ns();
    __u64 operation_ptr = (__u64)this_ptr;

    // Store entry timestamp for duration calculation
    bpf_map_update_elem(&operation_entry_times, &operation_ptr, &ts, BPF_ANY);

    // Emit entry event
    struct operation_event* event = bpf_ringbuf_reserve(&operation_events,
                                                         sizeof(*event), 0);
    if (!event) {
        return 0; // Ring buffer full, drop event
    }

    event->timestamp_ns = ts;
    event->operation_ptr = operation_ptr;
    event->pid = bpf_get_current_pid_tgid() >> 32;
    event->tid = bpf_get_current_pid_tgid() & 0xFFFFFFFF;
    event->operation_type = 0; // Join
    event->event_type = 0;     // enter
    event->reserved = 0;
    event->duration_ns = 0;
    event->result_rows = 0;
    event->result_cols = 0;

    bpf_ringbuf_submit(event, 0);

    // Track probe invocations
    increment_counter(COUNTER_PROBES_FIRED, 1);

    return 0;
}

SEC("uprobe/join_execute_exit")
int BPF_KRETPROBE(join_execute_exit, void* this_ptr) {
    __u64 ts_exit = get_timestamp_ns();
    __u64 operation_ptr = (__u64)this_ptr;

    // Lookup entry timestamp
    __u64* ts_entry = bpf_map_lookup_elem(&operation_entry_times, &operation_ptr);
    __u64 duration_ns = 0;
    if (ts_entry) {
        duration_ns = ts_exit - *ts_entry;
        bpf_map_delete_elem(&operation_entry_times, &operation_ptr);
    }

    // Emit exit event
    struct operation_event* event = bpf_ringbuf_reserve(&operation_events,
                                                         sizeof(*event), 0);
    if (!event) {
        return 0;
    }

    event->timestamp_ns = ts_exit;
    event->operation_ptr = operation_ptr;
    event->pid = bpf_get_current_pid_tgid() >> 32;
    event->tid = bpf_get_current_pid_tgid() & 0xFFFFFFFF;
    event->operation_type = 0; // Join
    event->event_type = 1;     // exit
    event->reserved = 0;
    event->duration_ns = duration_ns;
    event->result_rows = 0; // TODO: Read from Result object (Agent 5 dependency)
    event->result_cols = 0; // TODO: Read from Result object (Agent 5 dependency)

    bpf_ringbuf_submit(event, 0);

    // Track overhead (approximate: probe duration)
    __u64 probe_overhead_ns = get_timestamp_ns() - ts_exit;
    increment_counter(COUNTER_OVERHEAD_NS, probe_overhead_ns);

    return 0;
}

// ============================================================================
// Uprobe Handlers - Filter::execute()
// ============================================================================

SEC("uprobe/filter_execute_enter")
int BPF_KPROBE(filter_execute_enter, void* this_ptr) {
    __u64 ts = get_timestamp_ns();
    __u64 operation_ptr = (__u64)this_ptr;

    bpf_map_update_elem(&operation_entry_times, &operation_ptr, &ts, BPF_ANY);

    struct operation_event* event = bpf_ringbuf_reserve(&operation_events,
                                                         sizeof(*event), 0);
    if (!event) {
        return 0;
    }

    event->timestamp_ns = ts;
    event->operation_ptr = operation_ptr;
    event->pid = bpf_get_current_pid_tgid() >> 32;
    event->tid = bpf_get_current_pid_tgid() & 0xFFFFFFFF;
    event->operation_type = 1; // Filter
    event->event_type = 0;     // enter
    event->reserved = 0;
    event->duration_ns = 0;
    event->result_rows = 0;
    event->result_cols = 0;

    bpf_ringbuf_submit(event, 0);
    increment_counter(COUNTER_PROBES_FIRED, 1);

    return 0;
}

SEC("uprobe/filter_execute_exit")
int BPF_KRETPROBE(filter_execute_exit, void* this_ptr) {
    __u64 ts_exit = get_timestamp_ns();
    __u64 operation_ptr = (__u64)this_ptr;

    __u64* ts_entry = bpf_map_lookup_elem(&operation_entry_times, &operation_ptr);
    __u64 duration_ns = 0;
    if (ts_entry) {
        duration_ns = ts_exit - *ts_entry;
        bpf_map_delete_elem(&operation_entry_times, &operation_ptr);
    }

    struct operation_event* event = bpf_ringbuf_reserve(&operation_events,
                                                         sizeof(*event), 0);
    if (!event) {
        return 0;
    }

    event->timestamp_ns = ts_exit;
    event->operation_ptr = operation_ptr;
    event->pid = bpf_get_current_pid_tgid() >> 32;
    event->tid = bpf_get_current_pid_tgid() & 0xFFFFFFFF;
    event->operation_type = 1; // Filter
    event->event_type = 1;     // exit
    event->reserved = 0;
    event->duration_ns = duration_ns;
    event->result_rows = 0;
    event->result_cols = 0;

    bpf_ringbuf_submit(event, 0);

    __u64 probe_overhead_ns = get_timestamp_ns() - ts_exit;
    increment_counter(COUNTER_OVERHEAD_NS, probe_overhead_ns);

    return 0;
}

// ============================================================================
// Uprobe Handlers - IndexScan::execute()
// ============================================================================

SEC("uprobe/indexscan_execute_enter")
int BPF_KPROBE(indexscan_execute_enter, void* this_ptr) {
    __u64 ts = get_timestamp_ns();
    __u64 operation_ptr = (__u64)this_ptr;

    bpf_map_update_elem(&operation_entry_times, &operation_ptr, &ts, BPF_ANY);

    struct operation_event* event = bpf_ringbuf_reserve(&operation_events,
                                                         sizeof(*event), 0);
    if (!event) {
        return 0;
    }

    event->timestamp_ns = ts;
    event->operation_ptr = operation_ptr;
    event->pid = bpf_get_current_pid_tgid() >> 32;
    event->tid = bpf_get_current_pid_tgid() & 0xFFFFFFFF;
    event->operation_type = 2; // IndexScan
    event->event_type = 0;     // enter
    event->reserved = 0;
    event->duration_ns = 0;
    event->result_rows = 0;
    event->result_cols = 0;

    bpf_ringbuf_submit(event, 0);
    increment_counter(COUNTER_PROBES_FIRED, 1);

    return 0;
}

SEC("uprobe/indexscan_execute_exit")
int BPF_KRETPROBE(indexscan_execute_exit, void* this_ptr) {
    __u64 ts_exit = get_timestamp_ns();
    __u64 operation_ptr = (__u64)this_ptr;

    __u64* ts_entry = bpf_map_lookup_elem(&operation_entry_times, &operation_ptr);
    __u64 duration_ns = 0;
    if (ts_entry) {
        duration_ns = ts_exit - *ts_entry;
        bpf_map_delete_elem(&operation_entry_times, &operation_ptr);
    }

    struct operation_event* event = bpf_ringbuf_reserve(&operation_events,
                                                         sizeof(*event), 0);
    if (!event) {
        return 0;
    }

    event->timestamp_ns = ts_exit;
    event->operation_ptr = operation_ptr;
    event->pid = bpf_get_current_pid_tgid() >> 32;
    event->tid = bpf_get_current_pid_tgid() & 0xFFFFFFFF;
    event->operation_type = 2; // IndexScan
    event->event_type = 1;     // exit
    event->reserved = 0;
    event->duration_ns = duration_ns;
    event->result_rows = 0;
    event->result_cols = 0;

    bpf_ringbuf_submit(event, 0);

    __u64 probe_overhead_ns = get_timestamp_ns() - ts_exit;
    increment_counter(COUNTER_OVERHEAD_NS, probe_overhead_ns);

    return 0;
}

// ============================================================================
// License Declaration (Required by eBPF Verifier)
// ============================================================================

char LICENSE[] SEC("license") = "GPL";
