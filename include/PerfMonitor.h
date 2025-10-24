#pragma once

#include <string>
#include <vector>
#include <map>
#include <unistd.h>
#include <fcntl.h>

// Linux-specific includes
#ifdef __linux__
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#endif

struct PerfCounters {
    // Hardware performance counters
    unsigned long long cpu_cycles;
    unsigned long long instructions;
    unsigned long long cache_references;
    unsigned long long cache_misses;
    unsigned long long branch_instructions;
    unsigned long long branch_misses;
    unsigned long long context_switches;
    unsigned long long page_faults;
    
    // Calculated metrics
    double ipc;                    // Instructions per cycle
    double cache_hit_rate;         // Cache hit percentage
    double branch_miss_rate;       // Branch misprediction rate
    double context_switch_rate;   // Context switches per second
    double page_fault_rate;       // Page faults per second
};

// Linux-specific perf_event constants
#ifdef __linux__
// Use the system-provided constants from Linux headers
// No need to redefine them as they're already available
#else
// macOS/other platforms - define dummy constants
#define PERF_TYPE_HARDWARE 0
#define PERF_TYPE_SOFTWARE 1
#define PERF_COUNT_HW_CPU_CYCLES 0
#define PERF_COUNT_HW_INSTRUCTIONS 1
#define PERF_COUNT_HW_CACHE_REFERENCES 2
#define PERF_COUNT_HW_CACHE_MISSES 3
#define PERF_COUNT_HW_BRANCH_INSTRUCTIONS 4
#define PERF_COUNT_HW_BRANCH_MISSES 5
#define PERF_COUNT_SW_CONTEXT_SWITCHES 0
#define PERF_COUNT_SW_PAGE_FAULTS 1
#define PERF_FORMAT_TOTAL_TIME_ENABLED 1
#define PERF_FORMAT_TOTAL_TIME_RUNNING 2
#define PERF_EVENT_IOC_ENABLE 0x2400
#define __NR_perf_event_open 298
#endif

class PerfMonitor {
public:
    PerfMonitor();
    ~PerfMonitor();
    
    bool initialize();
    bool update();
    void printStats();
    void printAdvancedAnalysis();
    
    // Getters for integration
    double getIPC() const { return current_.ipc; }
    double getCacheHitRate() const { return current_.cache_hit_rate; }
    double getBranchMissRate() const { return current_.branch_miss_rate; }
    bool isCacheThrashing() const { return current_.cache_hit_rate < 80.0; }
    bool isBranchMispredicting() const { return current_.branch_miss_rate > 5.0; }
    
private:
    bool setupPerfEvent(int& fd, uint64_t type, uint64_t config, int cpu = -1);
    void calculateMetrics();
    void detectBottlenecks();
    
    // Perf event file descriptors
    std::vector<int> perf_fds_;
    std::map<std::string, int> perf_events_;
    
    PerfCounters current_;
    PerfCounters previous_;
    bool first_reading_;
    bool initialized_;
};
