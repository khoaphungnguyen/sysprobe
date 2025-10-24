#include "ProcessMonitor.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <unistd.h>

ProcessMonitor::ProcessMonitor() : first_reading_(true) {
    last_update_ = std::chrono::steady_clock::now();
}

bool ProcessMonitor::update() {
    // Store previous reading
    previous_stats_ = process_stats_;
    
    // Discover new processes
    std::vector<pid_t> current_processes = discoverProcesses();
    
    // Update tracked processes
    for (pid_t pid : current_processes) {
        if (isProcessAlive(pid)) {
            if (parseProcessStat(pid) && parseProcessStatus(pid) && parseProcessIO(pid)) {
                calculateProcessMetrics(pid);
                detectProcessBottlenecks(pid);
            }
        }
    }
    
    // Remove dead processes
    for (auto it = process_stats_.begin(); it != process_stats_.end();) {
        if (!isProcessAlive(it->first)) {
            it = process_stats_.erase(it);
        } else {
            ++it;
        }
    }
    
    first_reading_ = false;
    last_update_ = std::chrono::steady_clock::now();
    
    return true;
}

std::vector<pid_t> ProcessMonitor::discoverProcesses() {
    std::vector<pid_t> processes;
    
#ifdef __linux__
    try {
        for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
            std::string dirname = entry.path().filename().string();
            
            // Check if it's a numeric directory (process ID)
            if (std::all_of(dirname.begin(), dirname.end(), ::isdigit)) {
                pid_t pid = std::stoi(dirname);
                processes.push_back(pid);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error scanning /proc: " << e.what() << std::endl;
    }
#else
    // On non-Linux platforms, simulate some processes
    processes = {1, 2, 3, 4, 5}; // Simulate some basic processes
#endif
    
    return processes;
}

bool ProcessMonitor::isProcessAlive(pid_t pid) {
#ifdef __linux__
    return std::filesystem::exists("/proc/" + std::to_string(pid));
#else
    // On non-Linux platforms, simulate process existence
    return (pid >= 1 && pid <= 5); // Simulate some basic processes
#endif
}

bool ProcessMonitor::parseProcessStat(pid_t pid) {
#ifdef __linux__
    std::string stat_path = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream stat_file(stat_path);
    
    if (!stat_file.is_open()) {
        return false;
    }
    
    std::string line;
    if (!std::getline(stat_file, line)) {
        return false;
    }
    
    std::istringstream iss(line);
    std::string token;
    std::vector<std::string> tokens;
    
    // Parse the stat line
    while (std::getline(iss, token, ' ')) {
        tokens.push_back(token);
    }
    
    if (tokens.size() < 24) {
        return false;
    }
    
    auto& stats = process_stats_[pid];
    stats.pid = pid;
    stats.comm = tokens[1].substr(1, tokens[1].length() - 2); // Remove parentheses
    stats.state = tokens[2][0];
    stats.utime = std::stoul(tokens[13]);
    stats.stime = std::stoul(tokens[14]);
    stats.cutime = std::stoul(tokens[15]);
    stats.cstime = std::stoul(tokens[16]);
    stats.num_threads = std::stoul(tokens[19]);
    stats.vsize = std::stoul(tokens[22]);
    stats.rss = std::stoul(tokens[23]);
    stats.minflt = std::stoul(tokens[9]);
    stats.majflt = std::stoul(tokens[11]);
    stats.cminflt = std::stoul(tokens[10]);
    stats.cmajflt = std::stoul(tokens[12]);
    
    return true;
#else
    // On non-Linux platforms, simulate process stats
    auto& stats = process_stats_[pid];
    stats.pid = pid;
    stats.comm = "simulated_process_" + std::to_string(pid);
    stats.state = 'R';
    stats.utime = pid * 100;
    stats.stime = pid * 50;
    stats.cutime = 0;
    stats.cstime = 0;
    stats.num_threads = 1;
    stats.vsize = 1024 * 1024 * pid;
    stats.rss = 1024 * pid;
    stats.minflt = pid * 10;
    stats.majflt = pid;
    stats.cminflt = 0;
    stats.cmajflt = 0;
    
    return true;
#endif
}

bool ProcessMonitor::parseProcessStatus(pid_t pid) {
#ifdef __linux__
    std::string status_path = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream status_file(status_path);
    
    if (!status_file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(status_file, line)) {
        std::istringstream iss(line);
        std::string key, value;
        iss >> key >> value;
        
        if (key == "voluntary_ctxt_switches:") {
            process_stats_[pid].voluntary_ctxt_switches = std::stoul(value);
        } else if (key == "nonvoluntary_ctxt_switches:") {
            process_stats_[pid].nonvoluntary_ctxt_switches = std::stoul(value);
        }
    }
    
    return true;
#else
    // On non-Linux platforms, simulate process status
    process_stats_[pid].voluntary_ctxt_switches = pid * 10;
    process_stats_[pid].nonvoluntary_ctxt_switches = pid * 5;
    return true;
#endif
}

bool ProcessMonitor::parseProcessIO(pid_t pid) {
#ifdef __linux__
    std::string io_path = "/proc/" + std::to_string(pid) + "/io";
    std::ifstream io_file(io_path);
    
    if (!io_file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(io_file, line)) {
        std::istringstream iss(line);
        std::string key, value;
        iss >> key >> value;
        
        if (key == "rchar:") {
            process_stats_[pid].rchar = std::stoul(value);
        } else if (key == "wchar:") {
            process_stats_[pid].wchar = std::stoul(value);
        } else if (key == "syscr:") {
            process_stats_[pid].syscr = std::stoul(value);
        } else if (key == "syscw:") {
            process_stats_[pid].syscw = std::stoul(value);
        } else if (key == "read_bytes:") {
            process_stats_[pid].read_bytes = std::stoul(value);
        } else if (key == "write_bytes:") {
            process_stats_[pid].write_bytes = std::stoul(value);
        }
    }
    
    return true;
#else
    // On non-Linux platforms, simulate process I/O
    process_stats_[pid].rchar = pid * 1000;
    process_stats_[pid].wchar = pid * 500;
    process_stats_[pid].syscr = pid * 10;
    process_stats_[pid].syscw = pid * 5;
    process_stats_[pid].read_bytes = pid * 100;
    process_stats_[pid].write_bytes = pid * 50;
    return true;
#endif
}

void ProcessMonitor::calculateProcessMetrics(pid_t pid) {
    if (first_reading_) {
        return;
    }
    
    auto& current = process_stats_[pid];
    auto& previous = previous_stats_[pid];
    
    // Calculate CPU usage percentage
    unsigned long total_time = (current.utime - previous.utime) + (current.stime - previous.stime);
    current.cpu_usage_percent = total_time / 100.0; // Rough percentage
    
    // Calculate memory usage
    current.memory_usage_mb = current.rss * 4.0 / 1024.0; // RSS in pages, convert to MB
    
    // Calculate cache hit rate
    unsigned long rchar_delta = current.rchar - previous.rchar;
    unsigned long read_bytes_delta = current.read_bytes - previous.read_bytes;
    
    if (rchar_delta > 0) {
        current.cache_hit_rate = 100.0 * (rchar_delta - read_bytes_delta) / rchar_delta;
    } else {
        current.cache_hit_rate = 0.0;
    }
    
    // Calculate I/O efficiency
    unsigned long syscr_delta = current.syscr - previous.syscr;
    if (syscr_delta > 0) {
        current.io_efficiency = (double)read_bytes_delta / syscr_delta;
    } else {
        current.io_efficiency = 0.0;
    }
    
    // Calculate CPU efficiency
    unsigned long total_cpu = current.utime + current.stime;
    if (total_cpu > 0) {
        current.cpu_efficiency = 100.0 * current.utime / total_cpu;
    } else {
        current.cpu_efficiency = 0.0;
    }
    
    // Calculate rates
    current.context_switch_rate = (current.voluntary_ctxt_switches - previous.voluntary_ctxt_switches) +
                                 (current.nonvoluntary_ctxt_switches - previous.nonvoluntary_ctxt_switches);
    
    current.page_fault_rate = (current.minflt - previous.minflt) + (current.majflt - previous.majflt);
}

void ProcessMonitor::detectProcessBottlenecks(pid_t pid) {
    auto& stats = process_stats_[pid];
    
    // CPU intensive detection
    stats.is_cpu_intensive = (stats.cpu_usage_percent > 50.0);
    
    // Memory intensive detection
    stats.is_memory_intensive = (stats.memory_usage_mb > 1000.0); // 1GB threshold
    
    // I/O intensive detection
    stats.is_io_intensive = (stats.io_efficiency > 1000.0); // High I/O efficiency
    
    // Context switching heavy detection
    stats.is_context_switching_heavy = (stats.context_switch_rate > 1000);
    
    // Page faulting heavy detection
    stats.is_page_faulting_heavy = (stats.page_fault_rate > 100);
}

void ProcessMonitor::printStats() {
    if (first_reading_) {
        std::cout << "Process Stats (first reading - metrics not available yet)" << std::endl;
        return;
    }
    
    std::cout << "\n=== Process Analysis ===" << std::endl;
    std::cout << "Total Processes: " << process_stats_.size() << std::endl;
    
    // Print top processes by different metrics
    printTopProcesses(5);
}

void ProcessMonitor::printTopProcesses(int count) {
    if (process_stats_.empty()) {
        std::cout << "No process data available" << std::endl;
        return;
    }
    
    // Top CPU processes
    auto top_cpu = getTopCPUProcesses(count);
    std::cout << "\n🔥 TOP CPU PROCESSES" << std::endl;
    std::cout << std::left << std::setw(8) << "PID" 
              << std::setw(20) << "COMMAND" 
              << std::setw(10) << "CPU%" 
              << std::setw(12) << "MEMORY(MB)" 
              << std::setw(15) << "STATUS" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    for (pid_t pid : top_cpu) {
        const auto& stats = process_stats_[pid];
        std::string status = "NORMAL";
        if (stats.is_cpu_intensive) status = "CPU_INTENSIVE";
        if (stats.is_memory_intensive) status += "+MEMORY";
        if (stats.is_io_intensive) status += "+IO";
        
        std::cout << std::left << std::setw(8) << stats.pid
                  << std::setw(20) << stats.comm.substr(0, 19)
                  << std::setw(10) << std::fixed << std::setprecision(1) << stats.cpu_usage_percent
                  << std::setw(12) << std::fixed << std::setprecision(1) << stats.memory_usage_mb
                  << std::setw(15) << status << std::endl;
    }
    
    // Top Memory processes
    auto top_memory = getTopMemoryProcesses(count);
    std::cout << "\n🧠 TOP MEMORY PROCESSES" << std::endl;
    std::cout << std::left << std::setw(8) << "PID" 
              << std::setw(20) << "COMMAND" 
              << std::setw(12) << "MEMORY(MB)" 
              << std::setw(15) << "CACHE_HIT%" 
              << std::setw(15) << "STATUS" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    for (pid_t pid : top_memory) {
        const auto& stats = process_stats_[pid];
        std::string status = "NORMAL";
        if (stats.is_memory_intensive) status = "MEMORY_INTENSIVE";
        if (stats.is_page_faulting_heavy) status += "+PAGE_FAULTS";
        
        std::cout << std::left << std::setw(8) << stats.pid
                  << std::setw(20) << stats.comm.substr(0, 19)
                  << std::setw(12) << std::fixed << std::setprecision(1) << stats.memory_usage_mb
                  << std::setw(15) << std::fixed << std::setprecision(1) << stats.cache_hit_rate
                  << std::setw(15) << status << std::endl;
    }
}

void ProcessMonitor::printProcessAnalysis() {
    if (first_reading_) {
        return;
    }
    
    std::cout << "\n🔍 PROCESS-LEVEL ANALYSIS" << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
    
    // Analyze process patterns
    int cpu_intensive_count = 0;
    int memory_intensive_count = 0;
    int io_intensive_count = 0;
    int context_switching_heavy_count = 0;
    int page_faulting_heavy_count = 0;
    
    for (const auto& [pid, stats] : process_stats_) {
        if (stats.is_cpu_intensive) cpu_intensive_count++;
        if (stats.is_memory_intensive) memory_intensive_count++;
        if (stats.is_io_intensive) io_intensive_count++;
        if (stats.is_context_switching_heavy) context_switching_heavy_count++;
        if (stats.is_page_faulting_heavy) page_faulting_heavy_count++;
    }
    
    std::cout << "📊 PROCESS PATTERN ANALYSIS" << std::endl;
    std::cout << "CPU Intensive Processes: " << cpu_intensive_count << std::endl;
    std::cout << "Memory Intensive Processes: " << memory_intensive_count << std::endl;
    std::cout << "I/O Intensive Processes: " << io_intensive_count << std::endl;
    std::cout << "Context Switching Heavy: " << context_switching_heavy_count << std::endl;
    std::cout << "Page Faulting Heavy: " << page_faulting_heavy_count << std::endl;
    
    // Performance impact analysis
    if (cpu_intensive_count > 5) {
        std::cout << "🔴 HIGH CPU LOAD: " << cpu_intensive_count << " processes consuming significant CPU" << std::endl;
        std::cout << "   → Impact: CPU bottleneck, reduced throughput" << std::endl;
        std::cout << "   → Solution: Load balancing, process prioritization" << std::endl;
    }
    
    if (memory_intensive_count > 3) {
        std::cout << "🔴 HIGH MEMORY USAGE: " << memory_intensive_count << " processes using significant memory" << std::endl;
        std::cout << "   → Impact: Memory pressure, possible swapping" << std::endl;
        std::cout << "   → Solution: Memory optimization, process limits" << std::endl;
    }
    
    if (context_switching_heavy_count > 10) {
        std::cout << "🔴 HIGH CONTEXT SWITCHING: " << context_switching_heavy_count << " processes with heavy context switching" << std::endl;
        std::cout << "   → Impact: CPU overhead, reduced efficiency" << std::endl;
        std::cout << "   → Solution: Reduce thread count, optimize scheduling" << std::endl;
    }
    
    if (page_faulting_heavy_count > 5) {
        std::cout << "🔴 HIGH PAGE FAULTING: " << page_faulting_heavy_count << " processes with heavy page faulting" << std::endl;
        std::cout << "   → Impact: Memory pressure, I/O bottleneck" << std::endl;
        std::cout << "   → Solution: Increase memory, optimize memory access patterns" << std::endl;
    }
}

void ProcessMonitor::printProcessDetails(pid_t pid) {
    if (process_stats_.find(pid) == process_stats_.end()) {
        std::cout << "Process " << pid << " not found" << std::endl;
        return;
    }
    
    const auto& stats = process_stats_[pid];
    
    std::cout << "\n=== Process " << pid << " Details ===" << std::endl;
    std::cout << "Command: " << stats.comm << std::endl;
    std::cout << "State: " << stats.state << std::endl;
    std::cout << "Threads: " << stats.num_threads << std::endl;
    std::cout << "Virtual Memory: " << (stats.vsize / 1024 / 1024) << " MB" << std::endl;
    std::cout << "Resident Memory: " << std::fixed << std::setprecision(1) << stats.memory_usage_mb << " MB" << std::endl;
    std::cout << "CPU Usage: " << std::fixed << std::setprecision(1) << stats.cpu_usage_percent << "%" << std::endl;
    std::cout << "Cache Hit Rate: " << std::fixed << std::setprecision(1) << stats.cache_hit_rate << "%" << std::endl;
    std::cout << "I/O Efficiency: " << std::fixed << std::setprecision(1) << stats.io_efficiency << " bytes/syscall" << std::endl;
    std::cout << "Context Switches/sec: " << stats.context_switch_rate << std::endl;
    std::cout << "Page Faults/sec: " << stats.page_fault_rate << std::endl;
}

ProcessStats ProcessMonitor::getProcessStats(pid_t pid) const {
    auto it = process_stats_.find(pid);
    if (it != process_stats_.end()) {
        return it->second;
    }
    return ProcessStats{};
}

std::vector<pid_t> ProcessMonitor::getTopCPUProcesses(int count) const {
    std::vector<std::pair<pid_t, double>> cpu_usage;
    
    for (const auto& [pid, stats] : process_stats_) {
        cpu_usage.push_back({pid, stats.cpu_usage_percent});
    }
    
    std::sort(cpu_usage.begin(), cpu_usage.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<pid_t> result;
    for (int i = 0; i < std::min(count, (int)cpu_usage.size()); i++) {
        result.push_back(cpu_usage[i].first);
    }
    
    return result;
}

std::vector<pid_t> ProcessMonitor::getTopMemoryProcesses(int count) const {
    std::vector<std::pair<pid_t, double>> memory_usage;
    
    for (const auto& [pid, stats] : process_stats_) {
        memory_usage.push_back({pid, stats.memory_usage_mb});
    }
    
    std::sort(memory_usage.begin(), memory_usage.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<pid_t> result;
    for (int i = 0; i < std::min(count, (int)memory_usage.size()); i++) {
        result.push_back(memory_usage[i].first);
    }
    
    return result;
}

std::vector<pid_t> ProcessMonitor::getTopIOProcesses(int count) const {
    std::vector<std::pair<pid_t, double>> io_usage;
    
    for (const auto& [pid, stats] : process_stats_) {
        io_usage.push_back({pid, stats.io_efficiency});
    }
    
    std::sort(io_usage.begin(), io_usage.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<pid_t> result;
    for (int i = 0; i < std::min(count, (int)io_usage.size()); i++) {
        result.push_back(io_usage[i].first);
    }
    
    return result;
}
