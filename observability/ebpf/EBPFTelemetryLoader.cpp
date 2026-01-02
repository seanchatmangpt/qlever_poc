// Copyright 2026, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant (EPIC 10.3 Agent 6: OOB Telemetry)
//
// Purpose: Implementation of eBPF telemetry loader
//
// EPIC 10.3 Agent 6: Out-of-band observability implementation

#include "observability/ebpf/EBPFTelemetryLoader.h"

#include <cstring>
#include <sstream>
#include <stdexcept>

#include "engine/Operation.h"
#include "util/Exception.h"
#include "util/Log.h"

// libbpf headers (conditional compilation if eBPF support available)
#ifdef QLEVER_ENABLE_EBPF
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#else
// Stub definitions if eBPF not available
struct bpf_object {};
struct bpf_link {};
struct ring_buffer {};
#endif

namespace observability::ebpf {

// ============================================================================
// OperationEvent Implementation
// ============================================================================

std::string OperationEvent::toJsonLD() const {
    std::ostringstream oss;
    oss << "{"
        << "\"@type\":\"OperationEvent\""
        << ",\"timestamp_ns\":" << timestamp_ns
        << ",\"operation_ptr\":\"0x" << std::hex << operation_ptr << std::dec << "\""
        << ",\"pid\":" << pid
        << ",\"tid\":" << tid
        << ",\"operation_type\":\"" << operationTypeName() << "\""
        << ",\"event_type\":\"" << eventTypeName() << "\"";

    if (event_type == 1) {  // exit event
        oss << ",\"duration_ns\":" << duration_ns
            << ",\"result_rows\":" << result_rows
            << ",\"result_cols\":" << result_cols;
    }

    oss << "}";
    return oss.str();
}

std::string OperationEvent::operationTypeName() const {
    switch (operation_type) {
        case 0: return "Join";
        case 1: return "Filter";
        case 2: return "IndexScan";
        default: return "Unknown";
    }
}

std::string OperationEvent::eventTypeName() const {
    switch (event_type) {
        case 0: return "enter";
        case 1: return "exit";
        default: return "unknown";
    }
}

// ============================================================================
// OverheadStatistics Implementation
// ============================================================================

std::string OverheadStatistics::toJsonLD() const {
    std::ostringstream oss;
    oss << "{"
        << "\"@type\":\"OverheadStatistics\""
        << ",\"total_probes_fired\":" << total_probes_fired
        << ",\"total_overhead_ns\":" << total_overhead_ns
        << ",\"baseline_execution_ns\":" << baseline_execution_ns
        << ",\"current_overhead_pct\":" << current_overhead_pct
        << "}";
    return oss.str();
}

// ============================================================================
// OperationMetadataGuard Implementation (Read-Only Access)
// ============================================================================

std::string OperationMetadataGuard::getDescriptor() const {
    // Read-only access to Operation::getDescriptor()
    // Forward-compatible with Agent 1's OpaqueHandle
    return handle_->getDescriptor();
}

size_t OperationMetadataGuard::getResultWidth() const {
    return handle_->getResultWidth();
}

uint64_t OperationMetadataGuard::getSizeEstimate() const {
    return handle_->getSizeEstimate();
}

// ============================================================================
// EBPFTelemetryLoader Implementation
// ============================================================================

EBPFTelemetryLoader::EBPFTelemetryLoader(std::string ebpf_object_path)
    : ebpf_object_path_(std::move(ebpf_object_path)) {
#ifndef QLEVER_ENABLE_EBPF
    LOG(WARN) << "eBPF support not enabled. Telemetry loader is a no-op.";
#endif
}

EBPFTelemetryLoader::~EBPFTelemetryLoader() {
    if (running_.load()) {
        stop();
    }

#ifdef QLEVER_ENABLE_EBPF
    // Cleanup uprobe links
    for (auto* link : uprobe_links_) {
        if (link) {
            bpf_link__destroy(link);
        }
    }

    // Cleanup ring buffer
    if (ring_buf_) {
        ring_buffer__free(ring_buf_);
    }

    // Cleanup eBPF object
    if (bpf_obj_) {
        bpf_object__close(bpf_obj_);
    }
#endif
}

void EBPFTelemetryLoader::load() {
#ifdef QLEVER_ENABLE_EBPF
    LOG(INFO) << "Loading eBPF telemetry program from: " << ebpf_object_path_;

    // Load eBPF object file
    bpf_obj_ = bpf_object__open(ebpf_object_path_.c_str());
    if (!bpf_obj_) {
        throw std::runtime_error(
            "Failed to open eBPF object: " + ebpf_object_path_ +
            " (error: " + std::string(strerror(errno)) + ")");
    }

    // Load program into kernel
    int err = bpf_object__load(bpf_obj_);
    if (err) {
        bpf_object__close(bpf_obj_);
        bpf_obj_ = nullptr;
        throw std::runtime_error(
            "Failed to load eBPF program into kernel (error: " +
            std::to_string(err) + ")");
    }

    LOG(INFO) << "eBPF telemetry program loaded successfully.";
#else
    LOG(WARN) << "eBPF not enabled. Skipping load().";
#endif
}

void EBPFTelemetryLoader::attach(const std::string& binary_path) {
#ifdef QLEVER_ENABLE_EBPF
    if (!bpf_obj_) {
        throw std::runtime_error("eBPF object not loaded. Call load() first.");
    }

    LOG(INFO) << "Attaching eBPF uprobes to binary: " << binary_path;

    // Attach uprobes for Join::execute()
    attachUprobe(binary_path, "Join::computeResult", "uprobe/join_execute_enter", false);
    attachUprobe(binary_path, "Join::computeResult", "uprobe/join_execute_exit", true);

    // Attach uprobes for Filter::execute()
    attachUprobe(binary_path, "Filter::computeResult", "uprobe/filter_execute_enter", false);
    attachUprobe(binary_path, "Filter::computeResult", "uprobe/filter_execute_exit", true);

    // Attach uprobes for IndexScan::execute()
    attachUprobe(binary_path, "IndexScan::computeResult", "uprobe/indexscan_execute_enter", false);
    attachUprobe(binary_path, "IndexScan::computeResult", "uprobe/indexscan_execute_exit", true);

    LOG(INFO) << "Attached " << uprobe_links_.size() << " uprobes successfully.";
#else
    LOG(WARN) << "eBPF not enabled. Skipping attach().";
#endif
}

void EBPFTelemetryLoader::start() {
#ifdef QLEVER_ENABLE_EBPF
    if (!bpf_obj_) {
        throw std::runtime_error("eBPF object not loaded. Call load() first.");
    }

    if (running_.load()) {
        LOG(WARN) << "Telemetry loader already running.";
        return;
    }

    LOG(INFO) << "Starting eBPF event polling...";

    // Create ring buffer for events
    int map_fd = bpf_object__find_map_fd_by_name(bpf_obj_, "operation_events");
    if (map_fd < 0) {
        throw std::runtime_error("Failed to find operation_events map");
    }

    ring_buf_ = ring_buffer__new(map_fd, handleRingBufferEvent, this, nullptr);
    if (!ring_buf_) {
        throw std::runtime_error("Failed to create ring buffer");
    }

    // Start polling thread
    running_.store(true, std::memory_order_release);
    polling_thread_ = std::make_unique<std::thread>(&EBPFTelemetryLoader::pollEventsThread, this);

    LOG(INFO) << "eBPF telemetry polling started.";
#else
    LOG(WARN) << "eBPF not enabled. Skipping start().";
#endif
}

void EBPFTelemetryLoader::stop() {
#ifdef QLEVER_ENABLE_EBPF
    if (!running_.load()) {
        return;
    }

    LOG(INFO) << "Stopping eBPF telemetry polling...";

    // Stop polling thread
    running_.store(false, std::memory_order_release);
    if (polling_thread_ && polling_thread_->joinable()) {
        polling_thread_->join();
    }

    LOG(INFO) << "eBPF telemetry polling stopped.";
#else
    LOG(WARN) << "eBPF not enabled. Skipping stop().";
#endif
}

OverheadStatistics EBPFTelemetryLoader::getOverheadStatistics() const {
#ifdef QLEVER_ENABLE_EBPF
    const_cast<EBPFTelemetryLoader*>(this)->updateOverheadStatistics();
#endif
    return *overhead_stats_.rlock();
}

void EBPFTelemetryLoader::resetOverheadStatistics() {
#ifdef QLEVER_ENABLE_EBPF
    auto stats = overhead_stats_.wlock();
    stats->total_probes_fired = 0;
    stats->total_overhead_ns = 0;
    stats->baseline_execution_ns = 0;
    stats->current_overhead_pct = 0.0f;
#endif
}

// ============================================================================
// Internal Implementation
// ============================================================================

void EBPFTelemetryLoader::pollEventsThread() {
#ifdef QLEVER_ENABLE_EBPF
    while (running_.load(std::memory_order_acquire)) {
        // Poll ring buffer for events
        int err = ring_buffer__poll(ring_buf_, polling_interval_.count());
        if (err < 0 && err != -EINTR) {
            LOG(ERROR) << "Ring buffer poll error: " << err;
        }

        // Update and check overhead statistics
        updateOverheadStatistics();
        checkOverheadBudget();
    }
#endif
}

int EBPFTelemetryLoader::handleRingBufferEvent(void* ctx, void* data, size_t size) {
#ifdef QLEVER_ENABLE_EBPF
    auto* loader = static_cast<EBPFTelemetryLoader*>(ctx);

    if (size != sizeof(OperationEvent)) {
        LOG(ERROR) << "Invalid event size: " << size << " (expected " << sizeof(OperationEvent) << ")";
        return 0;
    }

    // Copy event data
    OperationEvent event;
    std::memcpy(&event, data, sizeof(event));

    // Invoke user callback if registered
    if (loader->event_handler_) {
        loader->event_handler_(event);
    }
#endif

    return 0;
}

void EBPFTelemetryLoader::attachUprobe(const std::string& binary_path,
                                       const std::string& function_name,
                                       const std::string& probe_name,
                                       bool is_retprobe) {
#ifdef QLEVER_ENABLE_EBPF
    struct bpf_program* prog = bpf_object__find_program_by_name(bpf_obj_, probe_name.c_str());
    if (!prog) {
        throw std::runtime_error("Failed to find eBPF program: " + probe_name);
    }

    // Attach uprobe
    struct bpf_link* link = bpf_program__attach_uprobe(
        prog, is_retprobe, -1, binary_path.c_str(), 0);

    if (!link) {
        throw std::runtime_error(
            "Failed to attach uprobe: " + probe_name +
            " to function: " + function_name);
    }

    uprobe_links_.push_back(link);

    LOG(DEBUG) << "Attached " << (is_retprobe ? "uretprobe" : "uprobe")
               << " to " << function_name;
#endif
}

void EBPFTelemetryLoader::updateOverheadStatistics() {
#ifdef QLEVER_ENABLE_EBPF
    if (!bpf_obj_) {
        return;
    }

    // Read global counters from eBPF map
    int map_fd = bpf_object__find_map_fd_by_name(bpf_obj_, "global_counters");
    if (map_fd < 0) {
        return;
    }

    auto stats = overhead_stats_.wlock();

    uint32_t key = 0;  // COUNTER_PROBES_FIRED
    uint64_t value = 0;
    if (bpf_map_lookup_elem(map_fd, &key, &value) == 0) {
        stats->total_probes_fired = value;
    }

    key = 1;  // COUNTER_OVERHEAD_NS
    if (bpf_map_lookup_elem(map_fd, &key, &value) == 0) {
        stats->total_overhead_ns = value;
    }

    key = 2;  // COUNTER_BASELINE_NS
    if (bpf_map_lookup_elem(map_fd, &key, &value) == 0) {
        stats->baseline_execution_ns = value;
    }

    // Calculate overhead percentage
    if (stats->baseline_execution_ns > 0) {
        stats->current_overhead_pct =
            (static_cast<float>(stats->total_overhead_ns) /
             static_cast<float>(stats->baseline_execution_ns)) * 100.0f;
    }
#endif
}

void EBPFTelemetryLoader::checkOverheadBudget() {
#ifdef QLEVER_ENABLE_EBPF
    auto stats = overhead_stats_.rlock();
    if (stats->exceedsBudget(overhead_budget_pct_)) {
        LOG(WARN) << "eBPF telemetry overhead exceeds budget: "
                  << stats->current_overhead_pct << "% (budget: "
                  << overhead_budget_pct_ << "%)";
    }
#endif
}

// ============================================================================
// Global Singleton
// ============================================================================

EBPFTelemetryLoader& globalTelemetryLoader() {
    static EBPFTelemetryLoader loader("observability/ebpf/qlever_uprobes.bpf.o");
    return loader;
}

}  // namespace observability::ebpf
