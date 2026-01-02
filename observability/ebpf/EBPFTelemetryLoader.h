// Copyright 2026, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant (EPIC 10.3 Agent 6: OOB Telemetry)
//
// Purpose: User-space loader for eBPF telemetry probes
//
// EPIC 10.3 Agent 6: Out-of-band observability loader
// Loads eBPF uprobes, attaches to Join/Filter/IndexScan, reads events
//
// INVARIANT CONTRACT:
// - Fail-closed on eBPF load errors (observability is not optional)
// - Read-only access to Operation metadata (enforced by eBPF verifier)
// - Performance budget: < 2.0% aggregate overhead
// - All events structured (JSON-LD for observability plane integration)

#ifndef QLEVER_OBSERVABILITY_EBPF_TELEMETRYLOADER_H
#define QLEVER_OBSERVABILITY_EBPF_TELEMETRYLOADER_H

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "util/Synchronized.h"

namespace observability::ebpf {

// Forward declarations for libbpf types (avoid header pollution)
struct bpf_object;
struct bpf_link;
struct ring_buffer;

// ============================================================================
// Operation Event (Mirror of eBPF struct)
// ============================================================================

struct OperationEvent {
    uint64_t timestamp_ns;     // Monotonic timestamp
    uint64_t operation_ptr;    // Pointer to Operation object
    uint32_t pid;              // Process ID
    uint32_t tid;              // Thread ID
    uint8_t  operation_type;   // 0=Join, 1=Filter, 2=IndexScan
    uint8_t  event_type;       // 0=enter, 1=exit
    uint16_t reserved;         // Padding
    uint64_t duration_ns;      // Execution duration (exit only)
    uint64_t result_rows;      // Number of result rows (exit only)
    uint64_t result_cols;      // Number of result columns (exit only)

    // Convert to JSON-LD for observability plane
    [[nodiscard]] std::string toJsonLD() const;

    // Human-readable operation type name
    [[nodiscard]] std::string operationTypeName() const;

    // Human-readable event type name
    [[nodiscard]] std::string eventTypeName() const;
};

// ============================================================================
// Overhead Statistics
// ============================================================================

struct OverheadStatistics {
    uint64_t total_probes_fired = 0;    // Cumulative probe invocations
    uint64_t total_overhead_ns = 0;     // Cumulative probe overhead
    uint64_t baseline_execution_ns = 0; // Total execution time without probes
    float current_overhead_pct = 0.0f;  // Current overhead percentage

    // Check if overhead exceeds budget
    [[nodiscard]] bool exceedsBudget(float budget_pct = 2.0f) const {
        return current_overhead_pct > budget_pct;
    }

    // Convert to JSON-LD
    [[nodiscard]] std::string toJsonLD() const;
};

// ============================================================================
// Read-Only Operation Metadata Guard (Agent 5 Dependency)
// ============================================================================

// Forward-compatible stub for Agent 1's OpaqueHandle
// Currently aliases to const Operation*, will be replaced when FFI available
class Operation;  // Forward declaration
using OpaqueHandle = const Operation*;

// Read-only access guard for Operation metadata
// Enforces memory isolation invariant: eBPF can read, but not write
class OperationMetadataGuard {
public:
    explicit OperationMetadataGuard(OpaqueHandle handle) : handle_(handle) {
        // Validation: handle must be non-null and read-only
        if (!handle_) {
            throw std::invalid_argument("OperationMetadataGuard: null handle");
        }
    }

    // Read operation descriptor (read-only access)
    [[nodiscard]] std::string getDescriptor() const;

    // Read result width (read-only access)
    [[nodiscard]] size_t getResultWidth() const;

    // Read result size estimate (read-only access)
    [[nodiscard]] uint64_t getSizeEstimate() const;

    // Get operation pointer (for correlation with eBPF events)
    [[nodiscard]] uint64_t getOperationPtr() const {
        return reinterpret_cast<uint64_t>(handle_);
    }

    // Deleted write operations (enforce read-only contract)
    void setDescriptor(const std::string&) = delete;
    void setResultWidth(size_t) = delete;
    void setSizeEstimate(uint64_t) = delete;

private:
    OpaqueHandle handle_;  // Read-only handle (const pointer)
};

// ============================================================================
// Event Handler Callback
// ============================================================================

using OperationEventHandler = std::function<void(const OperationEvent&)>;

// ============================================================================
// eBPF Telemetry Loader
// ============================================================================

class EBPFTelemetryLoader {
public:
    // ========================================================================
    // Construction and Lifecycle
    // ========================================================================

    // Constructor: Specify path to eBPF object file
    explicit EBPFTelemetryLoader(std::string ebpf_object_path);

    // Destructor: Cleanup eBPF resources
    ~EBPFTelemetryLoader();

    // Non-copyable, non-movable (resource ownership)
    EBPFTelemetryLoader(const EBPFTelemetryLoader&) = delete;
    EBPFTelemetryLoader& operator=(const EBPFTelemetryLoader&) = delete;
    EBPFTelemetryLoader(EBPFTelemetryLoader&&) = delete;
    EBPFTelemetryLoader& operator=(EBPFTelemetryLoader&&) = delete;

    // ========================================================================
    // Lifecycle Management
    // ========================================================================

    // Load eBPF program into kernel and verify
    // Throws if load fails (fail-closed invariant)
    void load();

    // Attach uprobes to Join/Filter/IndexScan execute() methods
    // Requires binary path to QLever executable
    void attach(const std::string& binary_path);

    // Start event polling thread
    void start();

    // Stop event polling and detach probes
    void stop();

    // Check if loader is currently running
    [[nodiscard]] bool isRunning() const {
        return running_.load(std::memory_order_acquire);
    }

    // ========================================================================
    // Event Handling
    // ========================================================================

    // Register callback for operation events
    void setEventHandler(OperationEventHandler handler) {
        event_handler_ = std::move(handler);
    }

    // Get current overhead statistics
    [[nodiscard]] OverheadStatistics getOverheadStatistics() const;

    // Reset overhead counters
    void resetOverheadStatistics();

    // ========================================================================
    // Configuration
    // ========================================================================

    // Set overhead budget threshold (default 2.0%)
    // If exceeded, warning is emitted to observability plane
    void setOverheadBudget(float budget_pct) {
        overhead_budget_pct_ = budget_pct;
    }

    // Set polling interval for event ring buffer (default 100ms)
    void setPollingInterval(std::chrono::milliseconds interval) {
        polling_interval_ = interval;
    }

private:
    // ========================================================================
    // Internal Implementation
    // ========================================================================

    // Event polling thread function
    void pollEventsThread();

    // Callback for ring buffer events (called by libbpf)
    static int handleRingBufferEvent(void* ctx, void* data, size_t size);

    // Attach single uprobe to function
    void attachUprobe(const std::string& binary_path,
                      const std::string& function_name,
                      const std::string& probe_name,
                      bool is_retprobe);

    // Read global counters from eBPF maps
    void updateOverheadStatistics();

    // Emit warning if overhead budget exceeded
    void checkOverheadBudget();

    // ========================================================================
    // Data Members
    // ========================================================================

    std::string ebpf_object_path_;            // Path to .bpf.o file
    struct bpf_object* bpf_obj_ = nullptr;    // eBPF object handle
    struct ring_buffer* ring_buf_ = nullptr;  // Event ring buffer

    // Uprobe links (6 total: 3 functions × 2 probes each)
    std::vector<struct bpf_link*> uprobe_links_;

    // Event handling
    OperationEventHandler event_handler_;

    // Polling thread
    std::unique_ptr<std::thread> polling_thread_;
    std::atomic<bool> running_{false};
    std::chrono::milliseconds polling_interval_{100}; // 100ms default

    // Overhead tracking
    mutable ad_utility::Synchronized<OverheadStatistics> overhead_stats_;
    float overhead_budget_pct_ = 2.0f; // 2% default budget

    // Thread-safety
    mutable std::mutex state_mutex_;
};

// ============================================================================
// Global Singleton Instance (Optional)
// ============================================================================

// Get global telemetry loader (lazy initialization)
EBPFTelemetryLoader& globalTelemetryLoader();

}  // namespace observability::ebpf

#endif  // QLEVER_OBSERVABILITY_EBPF_TELEMETRYLOADER_H
