#include "NumaMonitor.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <algorithm>

NumaMonitor::NumaMonitor() : first_reading_(true) {
    vmstat_file_.open("/proc/vmstat");
    if (!vmstat_file_.is_open()) {
        std::cerr << "Failed to open /proc/vmstat" << std::endl;
    }
    
    // Discover NUMA topology
    discoverNumaTopology();
}

bool NumaMonitor::update() {
    if (!vmstat_file_.is_open()) {
        return false;
    }
    
    // Store previous reading
    previous_vmstat_ = current_vmstat_;
    
    // Parse vmstat
    if (!parseVmstat()) {
        return false;
    }
    
    // Update NUMA node information
    for (auto& [node_id, node] : numa_nodes_) {
        parseNumaNode(node_id);
    }
    
    // Calculate memory pressure (skip first reading)
    if (!first_reading_) {
        calculateMemoryPressure();
        detectBottlenecks();
    } else {
        first_reading_ = false;
    }
    
    return true;
}

bool NumaMonitor::parseVmstat() {
#ifdef __linux__
    vmstat_file_.seekg(0);
    vmstat_file_.clear();
    
    std::string line;
    while (std::getline(vmstat_file_, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string key, value;
        iss >> key >> value;
        
        // Parse key vmstat metrics
        if (key == "pgfault") {
            current_vmstat_.pgfault = std::stoul(value);
        } else if (key == "pgmajfault") {
            current_vmstat_.pgmajfault = std::stoul(value);
        } else if (key == "pgpgin") {
            current_vmstat_.pgpgin = std::stoul(value);
        } else if (key == "pgpgout") {
            current_vmstat_.pgpgout = std::stoul(value);
        } else if (key == "pswpin") {
            current_vmstat_.pswpin = std::stoul(value);
        } else if (key == "pswpout") {
            current_vmstat_.pswpout = std::stoul(value);
        } else if (key == "pgsteal") {
            current_vmstat_.pgsteal = std::stoul(value);
        } else if (key == "pgscan_kswapd") {
            current_vmstat_.pgscan_kswapd = std::stoul(value);
        } else if (key == "pgscan_direct") {
            current_vmstat_.pgscan_direct = std::stoul(value);
        } else if (key == "nr_dirty") {
            current_vmstat_.nr_dirty = std::stoul(value);
        } else if (key == "nr_writeback") {
            current_vmstat_.nr_writeback = std::stoul(value);
        } else if (key == "nr_unstable") {
            current_vmstat_.nr_unstable = std::stoul(value);
        } else if (key == "nr_slab_reclaimable") {
            current_vmstat_.nr_slab_reclaimable = std::stoul(value);
        } else if (key == "nr_slab_unreclaimable") {
            current_vmstat_.nr_slab_unreclaimable = std::stoul(value);
        }
    }
    
    return true;
#else
    // On non-Linux platforms, simulate vmstat data
    static unsigned long counter = 0;
    counter += 100;
    
    current_vmstat_.pgfault = counter;
    current_vmstat_.pgmajfault = counter / 100;
    current_vmstat_.pgpgin = counter / 10;
    current_vmstat_.pgpgout = counter / 10;
    current_vmstat_.pswpin = 0; // No swapping in simulation
    current_vmstat_.pswpout = 0;
    current_vmstat_.pgsteal = counter / 50;
    current_vmstat_.pgscan_kswapd = counter / 20;
    current_vmstat_.pgscan_direct = counter / 100;
    current_vmstat_.nr_dirty = counter / 10;
    current_vmstat_.nr_writeback = counter / 20;
    current_vmstat_.nr_unstable = 0;
    current_vmstat_.nr_slab_reclaimable = counter / 5;
    current_vmstat_.nr_slab_unreclaimable = counter / 10;
    
    return true;
#endif
}

bool NumaMonitor::discoverNumaTopology() {
    numa_nodes_.clear();
    
#ifdef __linux__
    try {
        // Check if NUMA is available
        if (!std::filesystem::exists("/sys/devices/system/node")) {
            std::cout << "NUMA not available on this system" << std::endl;
            return false;
        }
        
        // Discover NUMA nodes
        for (const auto& entry : std::filesystem::directory_iterator("/sys/devices/system/node")) {
            std::string node_path = entry.path().string();
            std::string node_name = entry.path().filename().string();
            
            if (node_name.substr(0, 4) == "node") {
                int node_id = std::stoi(node_name.substr(4));
                numa_nodes_[node_id] = NumaNode{node_id, 0, 0, 0, 0.0, {}, false};
            }
        }
        
        std::cout << "Discovered " << numa_nodes_.size() << " NUMA nodes" << std::endl;
        return true;
        
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error discovering NUMA topology: " << e.what() << std::endl;
        return false;
    }
#else
    // On non-Linux platforms, simulate a single NUMA node
    numa_nodes_[0] = NumaNode{0, 8ULL * 1024 * 1024 * 1024, 4ULL * 1024 * 1024 * 1024, 4ULL * 1024 * 1024 * 1024, 50.0, {0, 1, 2, 3}, true};
    std::cout << "NUMA simulation mode - single node with 8GB memory" << std::endl;
    return true;
#endif
}

bool NumaMonitor::parseNumaNode(int node_id) {
    if (numa_nodes_.find(node_id) == numa_nodes_.end()) {
        return false;
    }
    
    auto& node = numa_nodes_[node_id];
    
    // Parse node memory info
    std::string meminfo_path = "/sys/devices/system/node/node" + std::to_string(node_id) + "/meminfo";
    std::ifstream meminfo_file(meminfo_path);
    
    if (meminfo_file.is_open()) {
        std::string line;
        while (std::getline(meminfo_file, line)) {
            std::istringstream iss(line);
            std::string key, value, unit;
            iss >> key >> value >> unit;
            
            if (key == "Node" + std::to_string(node_id) + "MemTotal:") {
                node.mem_total = std::stoul(value);
            } else if (key == "Node" + std::to_string(node_id) + "MemFree:") {
                node.mem_free = std::stoul(value);
            }
        }
        
        node.mem_used = node.mem_total - node.mem_free;
        if (node.mem_total > 0) {
            node.usage_percent = 100.0 * node.mem_used / node.mem_total;
        }
    }
    
    // Parse CPU cores for this node
    std::string cpulist_path = "/sys/devices/system/node/node" + std::to_string(node_id) + "/cpulist";
    std::ifstream cpulist_file(cpulist_path);
    
    if (cpulist_file.is_open()) {
        std::string cpulist;
        std::getline(cpulist_file, cpulist);
        
        // Parse CPU list (e.g., "0-3,8-11")
        std::istringstream iss(cpulist);
        std::string range;
        while (std::getline(iss, range, ',')) {
            size_t dash_pos = range.find('-');
            if (dash_pos != std::string::npos) {
                int start = std::stoi(range.substr(0, dash_pos));
                int end = std::stoi(range.substr(dash_pos + 1));
                for (int cpu = start; cpu <= end; cpu++) {
                    node.cpu_cores.push_back(cpu);
                }
            } else {
                node.cpu_cores.push_back(std::stoi(range));
            }
        }
    }
    
    return true;
}

void NumaMonitor::calculateMemoryPressure() {
    // Calculate page fault rates
    unsigned long pgfault_delta = current_vmstat_.pgfault - previous_vmstat_.pgfault;
    unsigned long pgmajfault_delta = current_vmstat_.pgmajfault - previous_vmstat_.pgmajfault;
    
    current_vmstat_.page_fault_rate = pgfault_delta;
    current_vmstat_.major_fault_rate = pgmajfault_delta;
    
    // Calculate swap activity
    unsigned long swap_in_delta = current_vmstat_.pswpin - previous_vmstat_.pswpin;
    unsigned long swap_out_delta = current_vmstat_.pswpout - previous_vmstat_.pswpout;
    
    current_vmstat_.swap_rate = swap_in_delta + swap_out_delta;
    current_vmstat_.is_swapping = (swap_in_delta > 0 || swap_out_delta > 0);
    
    // Calculate memory pressure score
    double pressure_score = 0.0;
    
    // Factor in dirty pages
    if (current_vmstat_.nr_dirty > 1000) pressure_score += 20.0;
    if (current_vmstat_.nr_writeback > 500) pressure_score += 15.0;
    
    // Factor in page scanning activity
    unsigned long scan_delta = (current_vmstat_.pgscan_kswapd - previous_vmstat_.pgscan_kswapd) +
                               (current_vmstat_.pgscan_direct - previous_vmstat_.pgscan_direct);
    if (scan_delta > 1000) pressure_score += 25.0;
    
    // Factor in major page faults
    if (pgmajfault_delta > 10) pressure_score += 30.0;
    
    // Factor in swap activity
    if (current_vmstat_.is_swapping) pressure_score += 40.0;
    
    current_vmstat_.memory_pressure = pressure_score;
    current_vmstat_.is_memory_pressured = (pressure_score > 50.0);
}

void NumaMonitor::detectBottlenecks() {
    // Additional bottleneck detection logic
    if (current_vmstat_.nr_dirty > 5000) {
        std::cout << "⚠️  High dirty page count: " << current_vmstat_.nr_dirty << std::endl;
    }
    
    if (current_vmstat_.nr_writeback > 1000) {
        std::cout << "⚠️  High writeback activity: " << current_vmstat_.nr_writeback << std::endl;
    }
}

void NumaMonitor::printStats() {
    if (first_reading_) {
        std::cout << "NUMA/Memory Stats (first reading - metrics not available yet)" << std::endl;
        return;
    }
    
    std::cout << "\n=== NUMA Memory Analysis ===" << std::endl;
    std::cout << "NUMA Nodes: " << numa_nodes_.size() << std::endl;
    
    // Print per-node statistics
    for (const auto& [node_id, node] : numa_nodes_) {
        std::cout << "Node " << node_id << ": " 
                  << std::fixed << std::setprecision(1) << node.usage_percent << "% used"
                  << " (" << (node.mem_used / 1024) << "MB/" << (node.mem_total / 1024) << "MB)"
                  << " CPUs: " << node.cpu_cores.size() << std::endl;
    }
    
    std::cout << "\n=== Memory Pressure Indicators ===" << std::endl;
    std::cout << "Page Faults/sec:     " << std::setw(8) << current_vmstat_.page_fault_rate << std::endl;
    std::cout << "Major Faults/sec:     " << std::setw(8) << current_vmstat_.major_fault_rate << std::endl;
    std::cout << "Swap Activity/sec:     " << std::setw(8) << current_vmstat_.swap_rate << std::endl;
    std::cout << "Dirty Pages:          " << std::setw(8) << current_vmstat_.nr_dirty << std::endl;
    std::cout << "Writeback Pages:      " << std::setw(8) << current_vmstat_.nr_writeback << std::endl;
    std::cout << "Memory Pressure:      " << std::setw(8) << std::fixed << std::setprecision(1) 
              << current_vmstat_.memory_pressure << "%" << std::endl;
}

void NumaMonitor::printNumaTopology() {
    std::cout << "\n🔍 NUMA TOPOLOGY ANALYSIS" << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
    
    if (numa_nodes_.empty()) {
        std::cout << "NUMA not available on this system" << std::endl;
        return;
    }
    
    std::cout << "📊 NUMA NODE DETAILS" << std::endl;
    for (const auto& [node_id, node] : numa_nodes_) {
        std::string status = "🟢 BALANCED";
        if (node.usage_percent > 90) {
            status = "🔴 OVERLOADED";
        } else if (node.usage_percent > 80) {
            status = "🟡 HIGH USAGE";
        }
        
        std::cout << "Node " << node_id << ": " << std::fixed << std::setprecision(1) 
                  << node.usage_percent << "% memory usage (" 
                  << (node.mem_used / 1024) << "MB/" << (node.mem_total / 1024) << "MB) - " 
                  << status << std::endl;
        std::cout << "  CPU Cores: ";
        for (size_t i = 0; i < node.cpu_cores.size(); i++) {
            if (i > 0) std::cout << ",";
            std::cout << node.cpu_cores[i];
        }
        std::cout << std::endl;
    }
    
    // NUMA imbalance detection
    if (numa_nodes_.size() > 1) {
        std::vector<double> usage_rates;
        for (const auto& [node_id, node] : numa_nodes_) {
            usage_rates.push_back(node.usage_percent);
        }
        
        double max_usage = *std::max_element(usage_rates.begin(), usage_rates.end());
        double min_usage = *std::min_element(usage_rates.begin(), usage_rates.end());
        double imbalance = max_usage - min_usage;
        
        if (imbalance > 30.0) {
            std::cout << std::endl;
            std::cout << "🔴 NUMA IMBALANCE DETECTED: " << std::fixed << std::setprecision(1) 
                      << imbalance << "% difference between nodes" << std::endl;
            std::cout << "   → Impact: Some NUMA nodes overloaded, others underutilized" << std::endl;
            std::cout << "   → Solution: Use numactl to bind processes to specific nodes" << std::endl;
        }
    }
}

void NumaMonitor::printMemoryPressureAnalysis() {
    if (first_reading_) {
        return;
    }
    
    std::cout << "\n🔍 MEMORY PRESSURE ANALYSIS" << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
    
    // Page fault analysis
    std::cout << "📊 PAGE FAULT ANALYSIS" << std::endl;
    std::cout << "Minor Faults/sec: " << current_vmstat_.page_fault_rate;
    if (current_vmstat_.page_fault_rate > 10000) {
        std::cout << " 🔴 HIGH - Memory pressure detected" << std::endl;
    } else if (current_vmstat_.page_fault_rate > 5000) {
        std::cout << " 🟡 ELEVATED - Monitor memory usage" << std::endl;
    } else {
        std::cout << " ✅ NORMAL" << std::endl;
    }
    
    std::cout << "Major Faults/sec: " << current_vmstat_.major_fault_rate;
    if (current_vmstat_.major_fault_rate > 100) {
        std::cout << " 🔴 CRITICAL - Severe memory pressure, possible swapping" << std::endl;
    } else if (current_vmstat_.major_fault_rate > 10) {
        std::cout << " 🟡 WARNING - Memory pressure increasing" << std::endl;
    } else {
        std::cout << " ✅ NORMAL" << std::endl;
    }
    
    // Swap analysis
    std::cout << "📊 SWAP ANALYSIS" << std::endl;
    if (current_vmstat_.is_swapping) {
        std::cout << "🔴 SWAPPING DETECTED: " << current_vmstat_.swap_rate << " pages/sec" << std::endl;
        std::cout << "   → Impact: Severe performance degradation, I/O bottleneck" << std::endl;
        std::cout << "   → Solution: Increase physical memory, reduce memory usage" << std::endl;
    } else {
        std::cout << "✅ NO SWAPPING - Memory pressure under control" << std::endl;
    }
    
    // Memory pressure assessment
    std::cout << "📊 MEMORY PRESSURE ASSESSMENT" << std::endl;
    std::cout << "Pressure Score: " << std::fixed << std::setprecision(1) 
              << current_vmstat_.memory_pressure << "%";
    
    if (current_vmstat_.memory_pressure > 80.0) {
        std::cout << " 🔴 CRITICAL - Severe memory pressure" << std::endl;
        std::cout << "   → Impact: System performance severely degraded" << std::endl;
        std::cout << "   → Solution: Immediate memory increase required" << std::endl;
    } else if (current_vmstat_.memory_pressure > 60.0) {
        std::cout << " 🟡 WARNING - Elevated memory pressure" << std::endl;
        std::cout << "   → Impact: Performance may be affected" << std::endl;
        std::cout << "   → Solution: Monitor closely, consider memory upgrade" << std::endl;
    } else {
        std::cout << " ✅ NORMAL - Memory pressure under control" << std::endl;
    }
    
    // Dirty page analysis
    if (current_vmstat_.nr_dirty > 1000) {
        std::cout << "🔴 HIGH DIRTY PAGES: " << current_vmstat_.nr_dirty << " pages" << std::endl;
        std::cout << "   → Impact: Storage write bottleneck, memory pressure" << std::endl;
        std::cout << "   → Solution: Tune dirty page ratios, optimize write patterns" << std::endl;
    }
    
    if (current_vmstat_.nr_writeback > 500) {
        std::cout << "🔴 HIGH WRITEBACK: " << current_vmstat_.nr_writeback << " pages" << std::endl;
        std::cout << "   → Impact: Storage I/O bottleneck, performance degradation" << std::endl;
        std::cout << "   → Solution: Optimize storage performance, reduce write load" << std::endl;
    }
}

double NumaMonitor::getTotalMemoryUsage() const {
    double total_usage = 0.0;
    int node_count = 0;
    
    for (const auto& [node_id, node] : numa_nodes_) {
        total_usage += node.usage_percent;
        node_count++;
    }
    
    return node_count > 0 ? total_usage / node_count : 0.0;
}
