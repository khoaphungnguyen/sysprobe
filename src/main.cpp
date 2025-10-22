#include "CpuMonitor.h"
#include "MemoryMonitor.h" 
#include "StorageMonitor.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    std::cout << "Tiny Monitor - Phase 2: Storage Deep Dive" << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;
    
    CpuMonitor cpu_monitor;
    MemoryMonitor memory_monitor;
    StorageMonitor storage_monitor;  
    
    // Main monitoring loop
    while (true) {
        // Update CPU statistics
        if (!cpu_monitor.update()) {
            std::cerr << "Failed to update CPU stats" << std::endl;
            break;
        }
        
        // Update memory statistics
        if (!memory_monitor.update()) {
            std::cerr << "Failed to update memory stats" << std::endl;
            break;
        }
        
        // Update storage statistics
        if (!storage_monitor.update()) {
            std::cerr << "Failed to update storage stats" << std::endl;
            break;
        }
        
        // Print statistics
        cpu_monitor.printStats();
        memory_monitor.printStats(); 
        storage_monitor.printStats();
        storage_monitor.printHotDevices();
        storage_monitor.printQueueAnalysis();
        storage_monitor.printPerformanceSummary();
        
        // Wait 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}