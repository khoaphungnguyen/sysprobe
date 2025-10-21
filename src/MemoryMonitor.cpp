#include "MemoryMonitor.h"
#include <iostream>
#include <sstream>
#include <iomanip>

MemoryMonitor::MemoryMonitor() {
    // Open /proc/meminfo for reading
    meminfo_file_.open("/proc/meminfo");
    if (!meminfo_file_.is_open()) {
        std::cerr << "Failed to open /proc/meminfo" << std::endl;
    }
}

bool MemoryMonitor::update() {
    if (!meminfo_file_.is_open()) {
        return false;
    }
    
    // Parse current reading
    if (!parseMemInfo()) {
        return false;
    }
    
    // Calculate percentages and detect bottlenecks
    calculatePercentages();
    detectBottlenecks();
    
    return true;
}

bool MemoryMonitor::parseMemInfo() {
    // Reset file position to beginning
    meminfo_file_.seekg(0);
    meminfo_file_.clear();
    
    std::string line;
    while (std::getline(meminfo_file_, line)) {
        std::istringstream iss(line);
        std::string key, value, unit;
        iss >> key >> value >> unit;
        
        // Parse key memory metrics
        if (key == "MemTotal:") {
            current_.mem_total = std::stoul(value);
        } else if (key == "MemFree:") {
            current_.mem_free = std::stoul(value);
        } else if (key == "MemAvailable:") {
            current_.mem_available = std::stoul(value);
        } else if (key == "Buffers:") {
            current_.buffers = std::stoul(value);
        } else if (key == "Cached:") {
            current_.cached = std::stoul(value);
        } else if (key == "SwapCached:") {
            current_.swap_cached = std::stoul(value);
        } else if (key == "Active:") {
            current_.active = std::stoul(value);
        } else if (key == "Inactive:") {
            current_.inactive = std::stoul(value);
        } else if (key == "Dirty:") {
            current_.dirty = std::stoul(value);
        } else if (key == "Writeback:") {
            current_.writeback = std::stoul(value);
        }
    }
    
    return true;
}

void MemoryMonitor::calculatePercentages() {
    if (current_.mem_total == 0) {
        return; // Avoid division by zero
    }
    
    // Calculate memory usage percentage
    current_.memory_usage_percent = 100.0 * (current_.mem_total - current_.mem_available) / current_.mem_total;
    
    // Calculate available percentage
    current_.available_percent = 100.0 * current_.mem_available / current_.mem_total;
    
    // Calculate buffer efficiency (buffers vs total cache)
    unsigned long total_cache = current_.buffers + current_.cached;
    if (total_cache > 0) {
        current_.buffer_efficiency = 100.0 * current_.buffers / total_cache;
        current_.cache_efficiency = 100.0 * current_.cached / total_cache;
    } else {
        current_.buffer_efficiency = 0.0;
        current_.cache_efficiency = 0.0;
    }
}

void MemoryMonitor::detectBottlenecks() {
    // Memory pressure: low available memory
    current_.memory_pressure = (current_.available_percent < 10.0);

    // Compute helper percentages (kB-based values in meminfo)
    const double dirty_percent = current_.mem_total > 0
        ? (100.0 * static_cast<double>(current_.dirty) / static_cast<double>(current_.mem_total))
        : 0.0;
    const double writeback_percent = current_.mem_total > 0
        ? (100.0 * static_cast<double>(current_.writeback) / static_cast<double>(current_.mem_total))
        : 0.0;
    const unsigned long total_cache_kb = current_.buffers + current_.cached;
    const double total_cache_percent = current_.mem_total > 0
        ? (100.0 * static_cast<double>(total_cache_kb) / static_cast<double>(current_.mem_total))
        : 0.0;

    // Storage bottleneck (heuristic):
    //  - elevated dirty or writeback percentages, OR
    //  - low available memory combined with small total cache footprint
    current_.storage_bottleneck =
        (dirty_percent > 2.0) ||
        (writeback_percent > 1.0) ||
        (current_.memory_pressure && total_cache_percent < 15.0);

    // Write bottleneck: dirty growing large relative to total memory
    current_.write_bottleneck = (dirty_percent > 5.0);
}

void MemoryMonitor::printStats() {
    std::cout << "\n=== Memory Statistics ===" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    // Core memory metrics
    std::cout << "Total Memory:    " << std::setw(8) << (current_.mem_total / 1024) << " MB" << std::endl;
    std::cout << "Available:       " << std::setw(8) << (current_.mem_available / 1024) << " MB (" 
              << std::setw(5) << current_.available_percent << "%)" << std::endl;
    std::cout << "Memory Usage:    " << std::setw(8) << current_.memory_usage_percent << "%" << std::endl;
    
    // Buffer and cache metrics
    std::cout << "Buffers:         " << std::setw(8) << (current_.buffers / 1024) << " MB (" 
              << std::setw(5) << current_.buffer_efficiency << "%)" << std::endl;
    std::cout << "Cached:          " << std::setw(8) << (current_.cached / 1024) << " MB (" 
              << std::setw(5) << current_.cache_efficiency << "%)" << std::endl;
    
    // I/O metrics
    const double dirty_percent = current_.mem_total > 0
        ? (100.0 * static_cast<double>(current_.dirty) / static_cast<double>(current_.mem_total))
        : 0.0;
    const double writeback_percent = current_.mem_total > 0
        ? (100.0 * static_cast<double>(current_.writeback) / static_cast<double>(current_.mem_total))
        : 0.0;
    std::cout << "Dirty:           " << std::setw(8) << (current_.dirty / 1024) << " MB ("
              << std::setw(5) << dirty_percent << "%)" << std::endl;
    std::cout << "Writeback:       " << std::setw(8) << (current_.writeback / 1024) << " MB ("
              << std::setw(5) << writeback_percent << "%)" << std::endl;
    
    // Status indicators
    std::cout << "\n=== Status Indicators ===" << std::endl;
    std::cout << "Memory Pressure: " << (current_.memory_pressure ? "⚠️  YES" : "✅ NO") << std::endl;
    std::cout << "Storage Bottleneck: " << (current_.storage_bottleneck ? "⚠️  YES" : "✅ NO") << std::endl;
    std::cout << "Write Bottleneck:   " << (current_.write_bottleneck ? "⚠️  YES" : "✅ NO") << std::endl;
}