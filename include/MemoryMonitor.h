#pragma once

#include <string>
#include <fstream>

struct MemoryStats {
    // Core memory metrics (in kB)
    unsigned long mem_total;
    unsigned long mem_free;
    unsigned long mem_available;
    unsigned long buffers;
    unsigned long cached;
    unsigned long swap_cached;
    
    // Active/inactive memory
    unsigned long active;
    unsigned long inactive;
    
    // I/O related metrics
    unsigned long dirty;
    unsigned long writeback;
    
    // Calculated percentages and indicators
    double memory_usage_percent;
    double available_percent;
    double buffer_efficiency;
    double cache_efficiency;
    bool memory_pressure;
    bool storage_bottleneck;
    bool write_bottleneck;
};

class MemoryMonitor {
public:
    MemoryMonitor();
    ~MemoryMonitor() = default;
    
    bool update();       // Call this every second
    void printStats();   // Print current stats
    
private:
    bool parseMemInfo();
    void calculatePercentages();
    void detectBottlenecks();
    
    std::ifstream meminfo_file_;
    MemoryStats current_;
};