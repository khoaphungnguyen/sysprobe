#pragma once

#include <string>
#include <fstream>

struct MemoryStats {
    unsigned long mem_total;
    unsigned long mem_free;
    unsigned long mem_available;
    unsigned long buffers;
    unsigned long cached;
    unsigned long swap_cached;
    unsigned long active;
    unsigned long inactive;
    unsigned long dirty;
    unsigned long writeback;
    
    // Calculated percentages
    double memory_usage_percent;
    double available_percent;
    double buffer_percent;
    double cache_percent;
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
    
    bool update();
    void printStats();
    
    // Add these getter methods
    double getMemoryUsage() const { return current_.memory_usage_percent; }
    double getAvailableMemory() const { return current_.mem_available; }
    double getBufferUsage() const { return current_.buffer_percent; }
    double getCacheUsage() const { return current_.cache_percent; }
    
private:
    bool parseProcMeminfo();
    void calculatePercentages();
    void detectBottlenecks();
    
    std::ifstream meminfo_file_;
    MemoryStats current_;
    MemoryStats previous_;
    bool first_reading_;
    
};