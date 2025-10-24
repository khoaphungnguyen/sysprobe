#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <chrono>

struct ProcessStats {
    pid_t pid;
    std::string comm;              // Command name
    char state;                    // Process state
    unsigned long utime;           // User CPU time
    unsigned long stime;           // System CPU time
    unsigned long cutime;          // User time of children
    unsigned long cstime;          // System time of children
    unsigned long num_threads;     // Number of threads
    unsigned long vsize;           // Virtual memory size
    unsigned long rss;             // Resident set size
    unsigned long minflt;          // Minor page faults
    unsigned long majflt;          // Major page faults
    unsigned long cminflt;         // Minor page faults of children
    unsigned long cmajflt;         // Major page faults of children
    unsigned long voluntary_ctxt_switches;
    unsigned long nonvoluntary_ctxt_switches;
    
    // I/O accounting
    unsigned long rchar;           // Characters read
    unsigned long wchar;           // Characters written
    unsigned long syscr;           // Read system calls
    unsigned long syscw;           // Write system calls
    unsigned long read_bytes;      // Actual bytes read from storage
    unsigned long write_bytes;     // Actual bytes written to storage
    
    // Calculated metrics
    double cpu_usage_percent;
    double memory_usage_mb;
    double cache_hit_rate;         // (rchar - read_bytes) / rchar
    double io_efficiency;          // read_bytes / syscr
    double cpu_efficiency;         // utime / (utime + stime)
    double context_switch_rate;
    double page_fault_rate;
    
    // Status indicators
    bool is_cpu_intensive;
    bool is_memory_intensive;
    bool is_io_intensive;
    bool is_context_switching_heavy;
    bool is_page_faulting_heavy;
};

class ProcessMonitor {
public:
    ProcessMonitor();
    ~ProcessMonitor() = default;
    
    bool update();
    void printStats();
    void printProcessAnalysis();
    void printTopProcesses(int count = 10);
    void printProcessDetails(pid_t pid);
    
    // Process discovery
    std::vector<pid_t> discoverProcesses();
    bool isProcessAlive(pid_t pid);
    
    // Getters for integration
    const std::map<pid_t, ProcessStats>& getProcessStats() const { return process_stats_; }
    ProcessStats getProcessStats(pid_t pid) const;
    std::vector<pid_t> getTopCPUProcesses(int count = 5) const;
    std::vector<pid_t> getTopMemoryProcesses(int count = 5) const;
    std::vector<pid_t> getTopIOProcesses(int count = 5) const;
    
private:
    bool parseProcessStat(pid_t pid);
    bool parseProcessStatus(pid_t pid);
    bool parseProcessIO(pid_t pid);
    void calculateProcessMetrics(pid_t pid);
    void detectProcessBottlenecks(pid_t pid);
    
    std::map<pid_t, ProcessStats> process_stats_;
    std::map<pid_t, ProcessStats> previous_stats_;
    std::vector<pid_t> tracked_processes_;
    bool first_reading_;
    std::chrono::steady_clock::time_point last_update_;
};
