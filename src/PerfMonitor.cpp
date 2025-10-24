#include "PerfMonitor.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstring>

PerfMonitor::PerfMonitor() : first_reading_(true), initialized_(false) {
    // Initialize perf event file descriptors
    perf_fds_.resize(8, -1);
}

PerfMonitor::~PerfMonitor() {
    // Close all perf event file descriptors
    for (int fd : perf_fds_) {
        if (fd >= 0) {
            close(fd);
        }
    }
}

bool PerfMonitor::initialize() {
    if (initialized_) {
        return true;
    }
    
#ifdef __linux__
    // Setup performance counters
    perf_events_["cpu_cycles"] = 0;
    perf_events_["instructions"] = 1;
    perf_events_["cache_references"] = 2;
    perf_events_["cache_misses"] = 3;
    perf_events_["branch_instructions"] = 4;
    perf_events_["branch_misses"] = 5;
    perf_events_["context_switches"] = 6;
    perf_events_["page_faults"] = 7;
    
    // Setup perf events
    if (!setupPerfEvent(perf_fds_[0], PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES)) {
        std::cerr << "Failed to setup CPU cycles counter" << std::endl;
        return false;
    }
    
    if (!setupPerfEvent(perf_fds_[1], PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS)) {
        std::cerr << "Failed to setup instructions counter" << std::endl;
        return false;
    }
    
    if (!setupPerfEvent(perf_fds_[2], PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES)) {
        std::cerr << "Failed to setup cache references counter" << std::endl;
        return false;
    }
    
    if (!setupPerfEvent(perf_fds_[3], PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES)) {
        std::cerr << "Failed to setup cache misses counter" << std::endl;
        return false;
    }
    
    if (!setupPerfEvent(perf_fds_[4], PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS)) {
        std::cerr << "Failed to setup branch instructions counter" << std::endl;
        return false;
    }
    
    if (!setupPerfEvent(perf_fds_[5], PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES)) {
        std::cerr << "Failed to setup branch misses counter" << std::endl;
        return false;
    }
    
    if (!setupPerfEvent(perf_fds_[6], PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CONTEXT_SWITCHES)) {
        std::cerr << "Failed to setup context switches counter" << std::endl;
        return false;
    }
    
    if (!setupPerfEvent(perf_fds_[7], PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS)) {
        std::cerr << "Failed to setup page faults counter" << std::endl;
        return false;
    }
    
    initialized_ = true;
    std::cout << "✅ PerfMonitor initialized with hardware performance counters" << std::endl;
    return true;
#else
    // On non-Linux platforms, initialize with dummy data
    perf_events_["cpu_cycles"] = 0;
    perf_events_["instructions"] = 1;
    perf_events_["cache_references"] = 2;
    perf_events_["cache_misses"] = 3;
    perf_events_["branch_instructions"] = 4;
    perf_events_["branch_misses"] = 5;
    perf_events_["context_switches"] = 6;
    perf_events_["page_faults"] = 7;
    
    initialized_ = true;
    std::cout << "⚠️  PerfMonitor initialized in compatibility mode (hardware counters not available on this platform)" << std::endl;
    return true;
#endif
}

bool PerfMonitor::setupPerfEvent(int& fd, uint64_t type, uint64_t config, int cpu) {
#ifdef __linux__
    struct perf_event_attr attr;
    memset(&attr, 0, sizeof(attr));
    
    attr.type = type;
    attr.size = sizeof(attr);
    attr.config = config;
    attr.disabled = 1;
    attr.exclude_kernel = 0;
    attr.exclude_hv = 1;
    attr.exclude_idle = 1;
    attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    
    fd = syscall(__NR_perf_event_open, &attr, 0, cpu, -1, 0);
    if (fd < 0) {
        return false;
    }
    
    // Enable the counter
    if (ioctl(fd, PERF_EVENT_IOC_ENABLE, 0) < 0) {
        close(fd);
        fd = -1;
        return false;
    }
    
    return true;
#else
    // On non-Linux platforms, return false to indicate perf events are not available
    fd = -1;
    return false;
#endif
}

bool PerfMonitor::update() {
    if (!initialized_) {
        if (!initialize()) {
            return false;
        }
    }
    
    // Store previous reading
    previous_ = current_;
    
#ifdef __linux__
    // Read performance counters
    for (const auto& [event_name, index] : perf_events_) {
        if (perf_fds_[index] < 0) continue;
        
        uint64_t count;
        if (read(perf_fds_[index], &count, sizeof(count)) != sizeof(count)) {
            continue;
        }
        
        // Update current counters
        if (event_name == "cpu_cycles") current_.cpu_cycles = count;
        else if (event_name == "instructions") current_.instructions = count;
        else if (event_name == "cache_references") current_.cache_references = count;
        else if (event_name == "cache_misses") current_.cache_misses = count;
        else if (event_name == "branch_instructions") current_.branch_instructions = count;
        else if (event_name == "branch_misses") current_.branch_misses = count;
        else if (event_name == "context_switches") current_.context_switches = count;
        else if (event_name == "page_faults") current_.page_faults = count;
    }
#else
    // On non-Linux platforms, simulate some basic metrics
    // This is a simplified simulation for demonstration purposes
    static unsigned long long counter = 0;
    counter += 1000; // Simulate some activity
    
    current_.cpu_cycles = counter;
    current_.instructions = counter * 2; // Simulate 2 IPC
    current_.cache_references = counter;
    current_.cache_misses = counter / 10; // Simulate 90% hit rate
    current_.branch_instructions = counter / 4;
    current_.branch_misses = counter / 40; // Simulate 2.5% miss rate
    current_.context_switches = counter / 100;
    current_.page_faults = counter / 1000;
#endif
    
    // Calculate metrics (skip first reading)
    if (!first_reading_) {
        calculateMetrics();
    } else {
        first_reading_ = false;
    }
    
    return true;
}

void PerfMonitor::calculateMetrics() {
    // Calculate IPC (Instructions Per Cycle)
    unsigned long long cycles_delta = current_.cpu_cycles - previous_.cpu_cycles;
    unsigned long long instructions_delta = current_.instructions - previous_.instructions;
    
    if (cycles_delta > 0) {
        current_.ipc = (double)instructions_delta / cycles_delta;
    } else {
        current_.ipc = 0.0;
    }
    
    // Calculate cache hit rate
    unsigned long long cache_refs_delta = current_.cache_references - previous_.cache_references;
    unsigned long long cache_misses_delta = current_.cache_misses - previous_.cache_misses;
    
    if (cache_refs_delta > 0) {
        current_.cache_hit_rate = 100.0 * (cache_refs_delta - cache_misses_delta) / cache_refs_delta;
    } else {
        current_.cache_hit_rate = 0.0;
    }
    
    // Calculate branch miss rate
    unsigned long long branch_inst_delta = current_.branch_instructions - previous_.branch_instructions;
    unsigned long long branch_miss_delta = current_.branch_misses - previous_.branch_misses;
    
    if (branch_inst_delta > 0) {
        current_.branch_miss_rate = 100.0 * branch_miss_delta / branch_inst_delta;
    } else {
        current_.branch_miss_rate = 0.0;
    }
    
    // Calculate rates (per second)
    current_.context_switch_rate = current_.context_switches - previous_.context_switches;
    current_.page_fault_rate = current_.page_faults - previous_.page_faults;
}

void PerfMonitor::printStats() {
    if (first_reading_) {
        std::cout << "Performance Counters (first reading - metrics not available yet)" << std::endl;
        return;
    }
    
    std::cout << "\n=== Hardware Performance Counters ===" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "IPC (Instructions/Cycle): " << std::setw(8) << current_.ipc << std::endl;
    std::cout << "Cache Hit Rate:           " << std::setw(8) << current_.cache_hit_rate << "%" << std::endl;
    std::cout << "Branch Miss Rate:         " << std::setw(8) << current_.branch_miss_rate << "%" << std::endl;
    std::cout << "Context Switches/sec:     " << std::setw(8) << current_.context_switch_rate << std::endl;
    std::cout << "Page Faults/sec:          " << std::setw(8) << current_.page_fault_rate << std::endl;
}

void PerfMonitor::printAdvancedAnalysis() {
    if (first_reading_) {
        return;
    }
    
    std::cout << "\n🔍 ADVANCED CPU ANALYSIS" << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
    
    // CPU Efficiency Analysis
    std::cout << "📊 CPU EFFICIENCY METRICS" << std::endl;
    std::cout << "IPC: " << std::fixed << std::setprecision(2) << current_.ipc;
    if (current_.ipc > 2.0) {
        std::cout << " ✅ EXCELLENT - High instruction throughput" << std::endl;
    } else if (current_.ipc > 1.5) {
        std::cout << " 🟡 GOOD - Reasonable instruction throughput" << std::endl;
    } else {
        std::cout << " 🔴 POOR - Low instruction throughput, possible CPU bottleneck" << std::endl;
    }
    
    // Cache Performance Analysis
    std::cout << "Cache Hit Rate: " << std::fixed << std::setprecision(1) << current_.cache_hit_rate << "%";
    if (current_.cache_hit_rate > 95.0) {
        std::cout << " ✅ EXCELLENT - Cache working efficiently" << std::endl;
    } else if (current_.cache_hit_rate > 90.0) {
        std::cout << " 🟡 GOOD - Cache performing well" << std::endl;
    } else if (current_.cache_hit_rate > 80.0) {
        std::cout << " 🟠 WARNING - Cache misses increasing, memory pressure" << std::endl;
    } else {
        std::cout << " 🔴 CRITICAL - Cache thrashing detected!" << std::endl;
        std::cout << "   → Impact: Severe memory bandwidth bottleneck" << std::endl;
        std::cout << "   → Solution: Increase memory, optimize data access patterns" << std::endl;
    }
    
    // Branch Prediction Analysis
    std::cout << "Branch Miss Rate: " << std::fixed << std::setprecision(1) << current_.branch_miss_rate << "%";
    if (current_.branch_miss_rate < 2.0) {
        std::cout << " ✅ EXCELLENT - Branch predictor working well" << std::endl;
    } else if (current_.branch_miss_rate < 5.0) {
        std::cout << " 🟡 GOOD - Reasonable branch prediction" << std::endl;
    } else {
        std::cout << " 🔴 POOR - High branch misprediction rate" << std::endl;
        std::cout << "   → Impact: CPU pipeline stalls, reduced performance" << std::endl;
        std::cout << "   → Solution: Optimize code for better branch predictability" << std::endl;
    }
    
    // Context Switch Analysis
    if (current_.context_switch_rate > 10000) {
        std::cout << "🔴 HIGH CONTEXT SWITCHING: " << current_.context_switch_rate << " switches/sec" << std::endl;
        std::cout << "   → Impact: CPU overhead from frequent process switching" << std::endl;
        std::cout << "   → Solution: Reduce thread count, optimize scheduling" << std::endl;
    }
    
    // Page Fault Analysis
    if (current_.page_fault_rate > 1000) {
        std::cout << "🔴 HIGH PAGE FAULT RATE: " << current_.page_fault_rate << " faults/sec" << std::endl;
        std::cout << "   → Impact: Memory pressure, possible swapping" << std::endl;
        std::cout << "   → Solution: Increase memory, optimize memory usage" << std::endl;
    }
    
    // Overall Performance Assessment
    std::cout << std::endl;
    std::cout << "🎯 PERFORMANCE ASSESSMENT" << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
    
    bool has_issues = false;
    
    if (current_.ipc < 1.0) {
        std::cout << "🔴 CPU BOTTLENECK: Low IPC indicates CPU is not efficiently executing instructions" << std::endl;
        has_issues = true;
    }
    
    if (current_.cache_hit_rate < 85.0) {
        std::cout << "🔴 MEMORY BOTTLENECK: Cache thrashing indicates memory bandwidth issues" << std::endl;
        has_issues = true;
    }
    
    if (current_.branch_miss_rate > 5.0) {
        std::cout << "🔴 BRANCH PREDICTION ISSUE: High misprediction rate causing pipeline stalls" << std::endl;
        has_issues = true;
    }
    
    if (!has_issues) {
        std::cout << "✅ CPU PERFORMANCE HEALTHY - All metrics within optimal ranges" << std::endl;
    }
}
