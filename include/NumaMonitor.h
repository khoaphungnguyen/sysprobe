#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>

struct NumaNode {
    int node_id;
    unsigned long mem_total;
    unsigned long mem_free;
    unsigned long mem_used;
    double usage_percent;
    std::vector<int> cpu_cores;
    bool is_local;
};

struct VmstatCounters {
    // Page fault counters
    unsigned long pgfault;           // Minor page faults
    unsigned long pgmajfault;        // Major page faults
    unsigned long pgpgin;            // Pages paged in
    unsigned long pgpgout;           // Pages paged out
    unsigned long pswpin;            // Swap pages in
    unsigned long pswpout;           // Swap pages out
    unsigned long pgsteal;           // Pages stolen by kswapd
    unsigned long pgscan_kswapd;     // Pages scanned by kswapd
    unsigned long pgscan_direct;     // Pages scanned directly
    
    // Memory pressure indicators
    unsigned long nr_dirty;          // Dirty pages
    unsigned long nr_writeback;      // Pages being written back
    unsigned long nr_unstable;       // Unstable pages
    unsigned long nr_slab_reclaimable; // Reclaimable slab pages
    unsigned long nr_slab_unreclaimable; // Unreclaimable slab pages
    
    // Calculated rates
    double page_fault_rate;
    double major_fault_rate;
    double swap_rate;
    double memory_pressure;
    bool is_swapping;
    bool is_memory_pressured;
};

class NumaMonitor {
public:
    NumaMonitor();
    ~NumaMonitor() = default;
    
    bool update();
    void printStats();
    void printNumaTopology();
    void printMemoryPressureAnalysis();
    
    // Getters for integration
    int getNumaNodeCount() const { return numa_nodes_.size(); }
    double getTotalMemoryUsage() const;
    bool isMemoryPressured() const { return current_vmstat_.is_memory_pressured; }
    bool isSwapping() const { return current_vmstat_.is_swapping; }
    double getMemoryPressure() const { return current_vmstat_.memory_pressure; }
    
private:
    bool parseVmstat();
    bool discoverNumaTopology();
    bool parseNumaNode(int node_id);
    void calculateMemoryPressure();
    void detectBottlenecks();
    
    std::ifstream vmstat_file_;
    std::map<int, NumaNode> numa_nodes_;
    VmstatCounters current_vmstat_;
    VmstatCounters previous_vmstat_;
    bool first_reading_;
};
