#include "CpuMonitor.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    std::cout << "Tiny Monitor - Phase 1: Foundation" << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;
    
    CpuMonitor cpu_monitor;
    
    // Main monitoring loop
    while (true) {
        // Update CPU statistics
        if (!cpu_monitor.update()) {
            std::cerr << "Failed to update CPU stats" << std::endl;
            break;
        }
        
        // Print statistics
        cpu_monitor.printStats();
        
        // Wait 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}