#include "CpuMonitor.h"
#include <iostream>
#include <sstream>
#include <iomanip>

CpuMonitor::CpuMonitor() : first_reading_(true) {
    // Open /proc/stat for reading
    proc_stat_file_.open("/proc/stat");
    if (!proc_stat_file_.is_open()) {
        std::cerr << "Failed to open /proc/stat" << std::endl;
    }
}

bool CpuMonitor::update() {
    if (!proc_stat_file_.is_open()) {
        return false;
    }
    
    // Store previous reading
    previous_ = current_;
    
    // Parse current reading
    if (!parseProcStat()) {
        return false;
    }
    
    // Calculate percentages (skip first reading)
    if (!first_reading_) {
        calculatePercentages();
    } else {
        first_reading_ = false;
    }
    
    return true;
}

bool CpuMonitor::parseProcStat() {
    // Reset file position to beginning
    proc_stat_file_.seekg(0);
    proc_stat_file_.clear();
    
    std::string line;
    if (!std::getline(proc_stat_file_, line)) {
        return false;
    }
    
    // Parse the first line (cpu line)
    std::istringstream iss(line);
    std::string cpu_label;
    iss >> cpu_label; // Skip "cpu" label
    
    if (cpu_label != "cpu") {
        return false;
    }
    
    // Parse CPU times
    iss >> current_.user >> current_.nice >> current_.system >> current_.idle
        >> current_.iowait >> current_.irq >> current_.softirq >> current_.steal
        >> current_.guest >> current_.guest_nice;
    
    return true;
}

void CpuMonitor::calculatePercentages() {
    // Calculate total time differences
    unsigned long total_time = (current_.user - previous_.user) +
                              (current_.nice - previous_.nice) +
                              (current_.system - previous_.system) +
                              (current_.idle - previous_.idle) +
                              (current_.iowait - previous_.iowait) +
                              (current_.irq - previous_.irq) +
                              (current_.softirq - previous_.softirq) +
                              (current_.steal - previous_.steal) +
                              (current_.guest - previous_.guest) +
                              (current_.guest_nice - previous_.guest_nice);
    
    if (total_time == 0) {
        // Avoid division by zero
        return;
    }
    
    // Calculate percentages
    current_.user_percent = 100.0 * (current_.user - previous_.user) / total_time;
    current_.nice_percent = 100.0 * (current_.nice - previous_.nice) / total_time;
    current_.system_percent = 100.0 * (current_.system - previous_.system) / total_time;
    current_.idle_percent = 100.0 * (current_.idle - previous_.idle) / total_time;
    current_.iowait_percent = 100.0 * (current_.iowait - previous_.iowait) / total_time;
    current_.irq_percent = 100.0 * (current_.irq - previous_.irq) / total_time;
    current_.softirq_percent = 100.0 * (current_.softirq - previous_.softirq) / total_time;
    current_.steal_percent = 100.0 * (current_.steal - previous_.steal) / total_time;
    current_.guest_percent = 100.0 * (current_.guest - previous_.guest) / total_time;
    current_.guest_nice_percent = 100.0 * (current_.guest_nice - previous_.guest_nice) / total_time;
}

void CpuMonitor::printStats() {
    if (first_reading_) {
        std::cout << "CPU Stats (first reading - percentages not available yet)" << std::endl;
        return;
    }
    
    std::cout << "\n=== CPU Statistics ===" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "User:   " << std::setw(6) << current_.user_percent << "%" << std::endl;
    std::cout << "Nice:   " << std::setw(6) << current_.nice_percent << "%" << std::endl;
    std::cout << "System: " << std::setw(6) << current_.system_percent << "%" << std::endl;
    std::cout << "Idle:   " << std::setw(6) << current_.idle_percent << "%" << std::endl;
    std::cout << "IOWait: " << std::setw(6) << current_.iowait_percent << "%" << std::endl;
    std::cout << "IRQ:    " << std::setw(6) << current_.irq_percent << "%" << std::endl;
    std::cout << "SoftIRQ:" << std::setw(6) << current_.softirq_percent << "%" << std::endl;
    std::cout << "Steal:  " << std::setw(6) << current_.steal_percent << "%" << std::endl;
    std::cout << "Guest:  " << std::setw(6) << current_.guest_percent << "%" << std::endl;
    std::cout << "GuestNice:" << std::setw(4) << current_.guest_nice_percent << "%" << std::endl;
}