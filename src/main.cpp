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
    // Enhanced Header with Phase Info
    std::cout << "╔═══════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    🚀 Tiny Monitor - Phase 2 Dashboard 🚀           ║" << std::endl;
    std::cout << "║                        Storage Deep Dive & Interrupt Analysis         ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    // Get metrics
    double cpu_usage = cpu.getCpuUsage();
    double mem_usage = mem.getMemoryUsage();
    double total_iops = storage.getTotalIOPS();
    double total_throughput = storage.getTotalThroughput();
    int hot_devices = storage.getHotDeviceCount();
    int bottlenecks = storage.getBottleneckCount();
    
    // System Overview with enhanced progress bars
    std::cout << "📊 SYSTEM OVERVIEW" << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
    
    // CPU with Hardware IRQ
    std::cout << "🖥️  CPU Usage:    ";
    printProgressBar(cpu_usage, 100.0);
    std::cout << " " << std::fixed << std::setprecision(1) << cpu_usage << "%" << std::endl;
    
    // Memory with pressure indicator
    std::cout << "🧠 Memory:       ";
    printProgressBar(mem_usage, 100.0);
    std::cout << " " << std::fixed << std::setprecision(1) << mem_usage << "%" << std::endl;
    
    // Storage with IOPS and throughput
    std::cout << "💾 Storage:      ";
    printProgressBar(total_iops, 10000.0); // Higher scale for high-end systems
    std::cout << " " << std::fixed << std::setprecision(0) << total_iops << " IOPS";
    std::cout << " (" << std::fixed << std::setprecision(1) << total_throughput << " MB/s)" << std::endl;
    
    std::cout << std::endl;
    
    // Enhanced Detailed Breakdown
    std::cout << "📈 DETAILED BREAKDOWN" << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
    
    // CPU Breakdown with Hardware IRQ
    std::cout << "🖥️  CPU Breakdown:" << std::endl;
    std::cout << "   User:   " << std::setw(6) << std::fixed << std::setprecision(1) << cpu.getUserUsage() << "%";
    std::cout << "    System: " << std::setw(6) << std::fixed << std::setprecision(1) << cpu.getSystemUsage() << "%" << std::endl;
    std::cout << "   IOWait: " << std::setw(6) << std::fixed << std::setprecision(1) << cpu.getIOWait() << "%";
    std::cout << "    HardIRQ:" << std::setw(6) << std::fixed << std::setprecision(1) << cpu.getHardIRQ() << "%" << std::endl;
    std::cout << "   SoftIRQ:" << std::setw(6) << std::fixed << std::setprecision(1) << cpu.getSoftIRQ() << "%" << std::endl;
    
    std::cout << std::endl;
    
    // Memory Breakdown
    std::cout << "🧠 Memory Breakdown:" << std::endl;
    std::cout << "   Available: " << std::setw(8) << std::fixed << std::setprecision(0) << (mem.getAvailableMemory() / 1024.0) << " MB";
    std::cout << "    Buffers: " << std::setw(8) << std::fixed << std::setprecision(0) << (mem.getBufferUsage() / 1024.0) << " MB" << std::endl;
    std::cout << "   Cache:    " << std::setw(8) << std::fixed << std::setprecision(0) << (mem.getCacheUsage() / 1024.0) << " MB" << std::endl;
    
    std::cout << std::endl;
    
    // Storage Breakdown
    std::cout << "💾 Storage Breakdown:" << std::endl;
    std::cout << "   Total IOPS:    " << std::setw(8) << std::fixed << std::setprecision(0) << total_iops;
    std::cout << "    Throughput: " << std::setw(8) << std::fixed << std::setprecision(1) << total_throughput << " MB/s" << std::endl;
    std::cout << "   Hot Devices:   " << std::setw(8) << hot_devices;
    std::cout << "    Bottlenecks: " << std::setw(8) << bottlenecks << std::endl;
    
    std::cout << std::endl;

    // Enhanced Interrupt Analysis
    cpu.printInterruptStats();
    
    std::cout << std::endl;
    
    // Enhanced Status with more detailed indicators
    std::cout << "🎯 SYSTEM STATUS" << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
    std::cout << "Status: ";
    
    bool cpu_stress = cpu_usage > 80;
    bool mem_stress = mem_usage > 80;
    bool storage_stress = total_iops > 10000 || hot_devices > 5;
    bool interrupt_storm = bottlenecks > 10;
    
    if (cpu_stress || mem_stress || storage_stress || interrupt_storm) {
        std::cout << "🔴 SYSTEM UNDER STRESS";
        if (cpu_stress) std::cout << " (High CPU)";
        if (mem_stress) std::cout << " (High Memory)";
        if (storage_stress) std::cout << " (Storage Issues)";
        if (interrupt_storm) std::cout << " (Interrupt Storms)";
    } else if (cpu_usage > 50 || mem_usage > 50 || total_iops > 1000) {
        std::cout << "🟡 SYSTEM MODERATE LOAD";
    } else {
        std::cout << "🟢 SYSTEM HEALTHY";
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "Tiny Monitor - Phase 2: Storage Deep Dive" << std::endl;
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