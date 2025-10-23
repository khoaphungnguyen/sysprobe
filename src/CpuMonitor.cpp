#include "CpuMonitor.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

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
    
    // Parse interrupts
    parseProcInterrupts();
    
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
    //std::cout << "Nice:   " << std::setw(6) << current_.nice_percent << "%" << std::endl;
    std::cout << "System: " << std::setw(6) << current_.system_percent << "%" << std::endl;
    std::cout << "Idle:   " << std::setw(6) << current_.idle_percent << "%" << std::endl;
    std::cout << "IOWait: " << std::setw(6) << current_.iowait_percent << "%" << std::endl;
    std::cout << "IRQ:    " << std::setw(6) << current_.irq_percent << "%" << std::endl;
    std::cout << "SoftIRQ:" << std::setw(6) << current_.softirq_percent << "%" << std::endl;
    //std::cout << "Steal:  " << std::setw(6) << current_.steal_percent << "%" << std::endl;
    //std::cout << "Guest:  " << std::setw(6) << current_.guest_percent << "%" << std::endl;
    //std::cout << "GuestNice:" << std::setw(4) << current_.guest_nice_percent << "%" << std::endl;
}

void CpuMonitor::printInterruptStats() {
    std::cout << "\n=== Interrupt Analysis ===" << std::endl;
    
    // Collect and analyze all interrupts
    std::vector<std::tuple<std::string, unsigned long, unsigned long, size_t, double>> irq_analysis;
    
    for (const auto& [irq_name, counts] : interrupt_counts_) {
        if (counts.empty()) continue;
        
        // Calculate total interrupts for this IRQ
        unsigned long total = 0;
        for (unsigned long count : counts) {
            total += count;
        }
        
        // Skip very low activity interrupts
        if (total < 10000) continue;
        
        // Find CPU with highest count
        unsigned long max_count = 0;
        size_t max_cpu = 0;
        for (size_t i = 0; i < counts.size(); i++) {
            if (counts[i] > max_count) {
                max_count = counts[i];
                max_cpu = i;
            }
        }
        
        // Calculate distribution balance
        double balance = (double)max_count / total;
        
        irq_analysis.push_back({irq_name, total, max_count, max_cpu, balance});
    }
    
    // Sort by severity: storms first, then by total interrupts
    std::sort(irq_analysis.begin(), irq_analysis.end(), 
              [](const auto& a, const auto& b) {
                  double balance_a = std::get<4>(a);
                  double balance_b = std::get<4>(b);
                  unsigned long total_a = std::get<1>(a);
                  unsigned long total_b = std::get<1>(b);
                  
                  // Storms (balance > 0.8) come first
                  bool storm_a = balance_a > 0.8;
                  bool storm_b = balance_b > 0.8;
                  
                  if (storm_a && !storm_b) return true;
                  if (!storm_a && storm_b) return false;
                  
                  // If both storms or both not storms, sort by total interrupts
                  return total_a > total_b;
              });
    
    // Show only the most critical interrupts (top 10)
    std::cout << "Critical Interrupts (Top 10):" << std::endl;
    for (size_t i = 0; i < std::min(irq_analysis.size(), size_t(10)); i++) {
        const auto& [irq_name, total, max_count, max_cpu, balance] = irq_analysis[i];
        
        std::string status;
        if (balance > 0.8) {
            status = "🔴 STORM";
        } else if (balance > 0.5) {
            status = "🟡 UNBALANCED";
        } else {
            status = "🟢 BALANCED";
        }
        
        std::cout << std::left << std::setw(8) << irq_name 
                  << std::setw(12) << ("Total: " + std::to_string(total))
                  << std::setw(15) << ("Max: CPU" + std::to_string(max_cpu) + "(" + std::to_string(max_count) + ")")
                  << std::setw(20) << ("Balance: " + std::to_string((int)(balance * 100)) + "%")
                  << status << std::endl;
    }
    
    // Show storm summary
    int storm_count = 0;
    int unbalanced_count = 0;
    unsigned long total_storm_interrupts = 0;
    
    for (const auto& [irq_name, total, max_count, max_cpu, balance] : irq_analysis) {
        if (balance > 0.8) {
            storm_count++;
            total_storm_interrupts += total;
        } else if (balance > 0.5) {
            unbalanced_count++;
        }
    }
    
    std::cout << "\nStorm Summary:" << std::endl;
    std::cout << "  🔴 Storms: " << storm_count << " IRQs (" << total_storm_interrupts << " interrupts)" << std::endl;
    std::cout << "  🟡 Unbalanced: " << unbalanced_count << " IRQs" << std::endl;
    
    if (storm_count > 0) {
        std::cout << "  ⚠️  CRITICAL: " << storm_count << " interrupt storms detected!" << std::endl;
    }
}

std::map<std::string, std::vector<unsigned long>> CpuMonitor::getInterruptCounts() const {
    return interrupt_counts_;
}

bool CpuMonitor::parseProcInterrupts() {
    std::ifstream interrupts_file("/proc/interrupts");
    if (!interrupts_file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(interrupts_file, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string irq_name;
        iss >> irq_name;
        
        if (irq_name.empty() || irq_name == "CPU0") continue;
        
        std::vector<unsigned long> counts;
        unsigned long count;
        while (iss >> count) {
            counts.push_back(count);
        }
        
        if (!counts.empty()) {
            interrupt_counts_[irq_name] = counts;
        }
    }
    
    return true;
}