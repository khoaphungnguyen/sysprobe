#include "CpuMonitor.h"
#include "MemoryMonitor.h"
#include "StorageMonitor.h"
#include "PerfMonitor.h"
#include "NumaMonitor.h"
#include "ProcessMonitor.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>
#include <memory>

// Global variables for signal handling
bool g_running = true;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutting down gracefully..." << std::endl;
        g_running = false;
        exit(0);
    }
}

void printUsage() {
    std::cout << "Advanced System Monitor - Phases 3-5" << std::endl;
    std::cout << "Usage: ./sysprobe-advanced [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --perf, -p         Enable hardware performance counters (Phase 3)" << std::endl;
    std::cout << "  --numa, -n         Enable NUMA analysis (Phase 4)" << std::endl;
    std::cout << "  --process, -r      Enable process monitoring (Phase 5)" << std::endl;
    std::cout << "  --help, -h         Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  ./sysprobe-advanced --perf --numa --process    # Full advanced monitoring" << std::endl;
    std::cout << "  ./sysprobe-advanced --perf                    # Performance counters only" << std::endl;
    std::cout << "  ./sysprobe-advanced --numa --process          # NUMA and process analysis" << std::endl;
}

void runTextMode(bool enable_perf, bool enable_numa, bool enable_process) {
    std::cout << "🚀 Advanced System Monitor - Text Mode" << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;
    std::cout << std::endl;
    
    // Initialize monitors
    CpuMonitor cpu_monitor;
    MemoryMonitor memory_monitor;
    StorageMonitor storage_monitor;
    
    std::unique_ptr<PerfMonitor> perf_monitor;
    std::unique_ptr<NumaMonitor> numa_monitor;
    std::unique_ptr<ProcessMonitor> process_monitor;
    
    if (enable_perf) {
        perf_monitor = std::make_unique<PerfMonitor>();
        if (!perf_monitor->initialize()) {
            std::cout << "⚠️  Warning: Hardware performance counters not available" << std::endl;
            perf_monitor.reset();
        }
    }
    
    if (enable_numa) {
        numa_monitor = std::make_unique<NumaMonitor>();
    }
    
    if (enable_process) {
        process_monitor = std::make_unique<ProcessMonitor>();
    }
    
    // Main monitoring loop
    while (g_running) {
        // Update all statistics
        cpu_monitor.update();
        memory_monitor.update();
        storage_monitor.update();
        
        if (perf_monitor) {
            perf_monitor->update();
        }
        if (numa_monitor) {
            numa_monitor->update();
        }
        if (process_monitor) {
            process_monitor->update();
        }
        
        // Clear screen
        std::cout << "\033[2J\033[1;1H";
        
        // Print comprehensive dashboard
        std::cout << "╔═══════════════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║                    🚀 Advanced System Monitor - All Phases 🚀         ║" << std::endl;
        std::cout << "╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
        std::cout << std::endl;
        
        // Phase 1-2: Basic monitoring
        std::cout << "📊 BASIC SYSTEM MONITORING (Phases 1-2)" << std::endl;
        std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
        cpu_monitor.printStats();
        memory_monitor.printStats();
        storage_monitor.printStats();
        
        // Phase 3: Hardware performance counters
        if (perf_monitor) {
            std::cout << "\n⚡ HARDWARE PERFORMANCE COUNTERS (Phase 3)" << std::endl;
            std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
            perf_monitor->printStats();
            perf_monitor->printAdvancedAnalysis();
        }
        
        // Phase 4: NUMA and advanced memory analysis
        if (numa_monitor) {
            std::cout << "\n🏗️  NUMA & ADVANCED MEMORY ANALYSIS (Phase 4)" << std::endl;
            std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
            numa_monitor->printStats();
            numa_monitor->printNumaTopology();
            numa_monitor->printMemoryPressureAnalysis();
        }
        
        // Phase 5: Process-level analysis
        if (process_monitor) {
            std::cout << "\n🔍 PROCESS-LEVEL ANALYSIS (Phase 5)" << std::endl;
            std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
            process_monitor->printStats();
            process_monitor->printProcessAnalysis();
            process_monitor->printTopProcesses(10);
        }
        
        // Advanced correlation analysis
        std::cout << "\n🎯 ADVANCED CORRELATION ANALYSIS" << std::endl;
        std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
        
        // CPU bottleneck analysis
        if (cpu_monitor.getCpuUsage() > 90) {
            std::cout << "🔴 CRITICAL: CPU overload (" << std::fixed << std::setprecision(1) 
                      << cpu_monitor.getCpuUsage() << "%)" << std::endl;
        }
        
        if (cpu_monitor.getIOWait() > 20) {
            std::cout << "🔴 CRITICAL: High IOWait (" << std::fixed << std::setprecision(1) 
                      << cpu_monitor.getIOWait() << "%) - Storage bottleneck" << std::endl;
        }
        
        // Memory pressure analysis
        if (memory_monitor.getMemoryUsage() > 95) {
            std::cout << "🔴 CRITICAL: Memory exhaustion (" << std::fixed << std::setprecision(1) 
                      << memory_monitor.getMemoryUsage() << "%)" << std::endl;
        }
        
        // Storage bottleneck analysis
        if (storage_monitor.getBottleneckCount() > 0) {
            std::cout << "🔴 CRITICAL: Storage bottlenecks (" << storage_monitor.getBottleneckCount() 
                      << " devices at 100% queue) - I/O requests queued" << std::endl;
        }
        
        // Performance counter analysis
        if (perf_monitor) {
            if (perf_monitor->isCacheThrashing()) {
                std::cout << "🔴 CRITICAL: Cache thrashing detected - Memory bandwidth bottleneck" << std::endl;
            }
            if (perf_monitor->isBranchMispredicting()) {
                std::cout << "🔴 CRITICAL: High branch misprediction - CPU pipeline stalls" << std::endl;
            }
        }
        
        // NUMA analysis
        if (numa_monitor) {
            if (numa_monitor->isMemoryPressured()) {
                std::cout << "🔴 CRITICAL: Memory pressure detected - Performance degraded" << std::endl;
            }
            if (numa_monitor->isSwapping()) {
                std::cout << "🔴 CRITICAL: Swapping detected - Severe performance impact" << std::endl;
            }
        }
        
        // Process analysis
        if (process_monitor) {
            const auto& process_stats = process_monitor->getProcessStats();
            int cpu_intensive = 0;
            int memory_intensive = 0;
            
            for (const auto& [pid, stats] : process_stats) {
                if (stats.is_cpu_intensive) cpu_intensive++;
                if (stats.is_memory_intensive) memory_intensive++;
            }
            
            if (cpu_intensive > 5) {
                std::cout << "🔴 CRITICAL: " << cpu_intensive << " CPU-intensive processes detected" << std::endl;
            }
            if (memory_intensive > 3) {
                std::cout << "🔴 CRITICAL: " << memory_intensive << " memory-intensive processes detected" << std::endl;
            }
        }
        
        std::cout << std::endl;
        std::cout << "🎯 SYSTEM STATUS: ";
        
        bool has_critical_issues = false;
        if (cpu_monitor.getCpuUsage() > 90 || cpu_monitor.getIOWait() > 20 ||
            memory_monitor.getMemoryUsage() > 95 || storage_monitor.getBottleneckCount() > 0) {
            has_critical_issues = true;
        }
        
        if (has_critical_issues) {
            std::cout << "🔴 CRITICAL ISSUES DETECTED - Immediate attention required" << std::endl;
        } else {
            std::cout << "🟢 SYSTEM HEALTHY - All metrics within normal ranges" << std::endl;
        }
        
        std::cout << std::endl;
        std::cout << "Press Ctrl+C to exit" << std::endl;
        
        // Wait 2 seconds
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}


int main(int argc, char* argv[]) {
    // Setup signal handling
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Parse command line arguments
    bool enable_perf = false;
    bool enable_numa = false;
    bool enable_process = false;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--perf" || arg == "-p") {
            enable_perf = true;
        } else if (arg == "--numa" || arg == "-n") {
            enable_numa = true;
        } else if (arg == "--process" || arg == "-r") {
            enable_process = true;
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else {
            std::cout << "Unknown option: " << arg << std::endl;
            printUsage();
            return 1;
        }
    }
    
    // Show configuration
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Performance Counters: " << (enable_perf ? "Enabled (Phase 3)" : "Disabled") << std::endl;
    std::cout << "  NUMA Analysis: " << (enable_numa ? "Enabled (Phase 4)" : "Disabled") << std::endl;
    std::cout << "  Process Monitoring: " << (enable_process ? "Enabled (Phase 5)" : "Disabled") << std::endl;
    std::cout << std::endl;
    
    try {
        runTextMode(enable_perf, enable_numa, enable_process);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
