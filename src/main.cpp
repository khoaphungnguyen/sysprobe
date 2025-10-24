#include "CpuMonitor.h"
#include "MemoryMonitor.h"
#include "StorageMonitor.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>

// Function declarations
void printProgressBar(double current, double max, int width = 30);
void printSystemDashboard(CpuMonitor& cpu, MemoryMonitor& mem, StorageMonitor& storage);
void clearScreen();

void printProgressBar(double current, double max, int width) {
    int filled = (int)((current / max) * width);
    
    std::cout << "[";
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            if (current > 80) {
                std::cout << "█";  // Red for high usage
            } else if (current > 50) {
                std::cout << "▓";  // Yellow for medium usage
            } else {
                std::cout << "░";  // Green for low usage
            }
        } else {
            std::cout << " ";
        }
    }
    std::cout << "]";
}

void clearScreen() {
    // Clear screen using ANSI escape codes
    std::cout << "\033[2J\033[1;1H";
}

void printSystemDashboard(CpuMonitor& cpu, MemoryMonitor& mem, StorageMonitor& storage) {
    // Compact Header
    std::cout << "╔═══════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    🚀 Tiny Monitor - Quick Issue Detection 🚀         ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    // Get metrics
    double cpu_usage = cpu.getCpuUsage();
    double mem_usage = mem.getMemoryUsage();
    double total_iops = storage.getTotalIOPS();
    int hot_devices = storage.getHotDeviceCount();
    int bottlenecks = storage.getBottleneckCount();
    
    // Compact System Overview
    std::cout << "📊 SYSTEM OVERVIEW" << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
    
    // CPU - Enhanced with detailed breakdown
    std::cout << "🖥️  CPU:    ";
    printProgressBar(cpu_usage, 100.0);
    std::cout << " " << std::fixed << std::setprecision(1) << cpu_usage << "%";
    
    // Show detailed breakdown if there's significant activity
    if (cpu_usage > 5.0) {
        std::cout << " [User:" << std::fixed << std::setprecision(1) << cpu.getUserUsage() 
                  << "% Sys:" << cpu.getSystemUsage() 
                  << "% IO:" << cpu.getIOWait() 
                  << "% IRQ:" << cpu.getHardIRQ() 
                  << "% SoftIRQ:" << cpu.getSoftIRQ() << "%]";
    }
    
    if (cpu.getIOWait() > 10) std::cout << " ⚠️  High IOWait";
    if (cpu.getHardIRQ() > 5) std::cout << " ⚠️  High HardIRQ";
    if (cpu.getSoftIRQ() > 5) std::cout << " ⚠️  High SoftIRQ";
    std::cout << std::endl;
    
    // Memory - Enhanced with detailed breakdown
    std::cout << "🧠 Memory: ";
    printProgressBar(mem_usage, 100.0);
    std::cout << " " << std::fixed << std::setprecision(1) << mem_usage << "%";
    
    // Show detailed breakdown
    std::cout << " [Used:" << std::fixed << std::setprecision(1) << mem_usage 
              << "% Cache:" << mem.getCacheUsage() 
              << "% Buffer:" << mem.getBufferUsage() 
              << "% Avail:" << std::fixed << std::setprecision(0) << (mem.getAvailableMemory() / 1024.0) << "MB]";
    
    if (mem_usage > 80) std::cout << " ⚠️  Low Available";
    if (mem.getCacheUsage() > 50) std::cout << " ⚠️  High Cache";
    std::cout << std::endl;
    
    // Storage - Enhanced with better explanations
    std::cout << "💾 Storage: ";
    printProgressBar(total_iops, 10000.0);
    std::cout << " " << std::fixed << std::setprecision(0) << total_iops << " IOPS";
    
    // Show detailed storage info
    if (hot_devices > 0) {
        std::cout << " ⚠️  " << hot_devices << " hot devices (high activity)";
    }
    if (bottlenecks > 0) {
        std::cout << " ⚠️  " << bottlenecks << " bottlenecks (100% queue)";
    }
    std::cout << std::endl;
    
    std::cout << std::endl;
    
    // Quick Issue Detection
    std::cout << "🚨 ISSUE DETECTION" << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
    
    bool has_issues = false;
    
    // CPU Issues
    if (cpu_usage > 90) {
        std::cout << "🔴 CRITICAL: CPU overload (" << std::fixed << std::setprecision(1) << cpu_usage << "%)" << std::endl;
        has_issues = true;
    } else if (cpu_usage > 80) {
        std::cout << "🟡 WARNING: High CPU usage (" << std::fixed << std::setprecision(1) << cpu_usage << "%)" << std::endl;
        has_issues = true;
    }
    
    if (cpu.getIOWait() > 20) {
        std::cout << "🔴 CRITICAL: High IOWait (" << std::fixed << std::setprecision(1) << cpu.getIOWait() << "%) - Storage bottleneck" << std::endl;
        has_issues = true;
    } else if (cpu.getIOWait() > 10) {
        std::cout << "🟡 WARNING: Elevated IOWait (" << std::fixed << std::setprecision(1) << cpu.getIOWait() << "%)" << std::endl;
        has_issues = true;
    }
    
    // Memory Issues
    if (mem_usage > 95) {
        std::cout << "🔴 CRITICAL: Memory exhaustion (" << std::fixed << std::setprecision(1) << mem_usage << "%)" << std::endl;
        has_issues = true;
    } else if (mem_usage > 85) {
        std::cout << "🟡 WARNING: High memory usage (" << std::fixed << std::setprecision(1) << mem_usage << "%)" << std::endl;
        has_issues = true;
    }
    
    // Storage Issues - Enhanced with explanations
    if (hot_devices > 3) {
        std::cout << "🔴 CRITICAL: Multiple hot storage devices (" << hot_devices << " devices) - High I/O activity may cause overheating" << std::endl;
        has_issues = true;
    } else if (hot_devices > 1) {
        std::cout << "🟡 WARNING: Hot storage devices detected (" << hot_devices << " devices) - Monitor temperature" << std::endl;
        has_issues = true;
    }
    
    if (bottlenecks > 2) {
        std::cout << "🔴 CRITICAL: Storage bottlenecks (" << bottlenecks << " devices at 100% queue) - I/O requests queued, high latency" << std::endl;
        has_issues = true;
    } else if (bottlenecks > 0) {
        std::cout << "🟡 WARNING: Storage bottlenecks detected (" << bottlenecks << " devices) - I/O queue full, performance degraded" << std::endl;
        has_issues = true;
    }
    
    // Interrupt Analysis (only if there are issues)
    if (cpu_usage > 50 || cpu.getIOWait() > 5 || cpu.getHardIRQ() > 5 || cpu.getSoftIRQ() > 5) {
        std::cout << std::endl;
        cpu.printInterruptStats();
    }
    
    // Performance Impact Analysis
    if (has_issues) {
        std::cout << std::endl;
        std::cout << "🎯 PERFORMANCE IMPACT ANALYSIS" << std::endl;
        std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
        
        // CPU Impact
        if (cpu.getIOWait() > 10) {
            std::cout << "🔴 CPU IOWait " << std::fixed << std::setprecision(1) << cpu.getIOWait() 
                      << "% - Storage is bottleneck, CPU waiting for I/O" << std::endl;
        }
        if (cpu.getHardIRQ() > 5) {
            std::cout << "🔴 Hard IRQ " << std::fixed << std::setprecision(1) << cpu.getHardIRQ() 
                      << "% - Hardware interrupts consuming CPU, limiting I/O throughput" << std::endl;
        }
        if (cpu.getSoftIRQ() > 5) {
            std::cout << "🔴 Soft IRQ " << std::fixed << std::setprecision(1) << cpu.getSoftIRQ() 
                      << "% - Deferred interrupt processing, may limit scaling" << std::endl;
        }
        
        // Storage Impact
        if (bottlenecks > 0) {
            std::cout << "🔴 Storage bottlenecks prevent I/O scaling - " << bottlenecks 
                      << " devices at 100% queue depth" << std::endl;
        }
        if (hot_devices > 3) {
            std::cout << "🔴 Multiple hot devices may cause thermal throttling and performance degradation" << std::endl;
        }
        
        // Memory Impact
        if (mem_usage > 90) {
            std::cout << "🔴 High memory usage may cause swapping, severely impacting I/O performance" << std::endl;
        }
    }
    
    std::cout << std::endl;
    
    // System Status
    std::cout << "🎯 SYSTEM STATUS" << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
    
    if (!has_issues) {
        std::cout << "Status: 🟢 SYSTEM HEALTHY - No issues detected" << std::endl;
    } else {
        std::cout << "Status: 🔴 ATTENTION REQUIRED - Issues detected above" << std::endl;
    }
}

int main() {
    std::cout << "Tiny Monitor - Quick Issue Detection" << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;
    std::cout << std::endl;
    
    CpuMonitor cpu_monitor;
    MemoryMonitor memory_monitor;
    StorageMonitor storage_monitor;
    
    // Main monitoring loop
    while (true) {
        // Update all statistics
        cpu_monitor.update();
        memory_monitor.update();
        storage_monitor.update();
        
        // Clear screen and print dashboard
        clearScreen();
        printSystemDashboard(cpu_monitor, memory_monitor, storage_monitor);
        
        // Wait 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}