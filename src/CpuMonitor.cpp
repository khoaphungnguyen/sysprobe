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
    std::cout << "Nice:   " << std::setw(6) << current_.nice_percent << "%" << std::endl;
    std::cout << "System: " << std::setw(6) << current_.system_percent << "%" << std::endl;
    std::cout << "Idle:   " << std::setw(6) << current_.idle_percent << "%" << std::endl;
    std::cout << "IOWait: " << std::setw(6) << current_.iowait_percent << "%" << std::endl;
    std::cout << "IRQ:    " << std::setw(6) << current_.irq_percent << "%" << std::endl;
    std::cout << "SoftIRQ:" << std::setw(6) << current_.softirq_percent << "%" << std::endl;
}

void CpuMonitor::printInterruptStats() {
    std::cout << "🔍 INTERRUPT ANALYSIS" << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────────────" << std::endl;
    
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
    
    // Show only critical interrupts (storms and high activity)
    int critical_count = 0;
    for (size_t i = 0; i < irq_analysis.size() && critical_count < 3; i++) {
        const auto& [irq_name, total, max_count, max_cpu, balance] = irq_analysis[i];
        
        // Only show storms or very high activity interrupts
        if (balance > 0.8 || total > 100000) {
            std::string status;
            if (balance > 0.8) {
                status = "🔴 STORM";
            } else if (balance > 0.5) {
                status = "🟡 UNBALANCED";
            } else {
                status = "🟢 HIGH ACTIVITY";
            }
            
            // Get interrupt description
            std::string description = getInterruptDescription(irq_name);
            
            std::cout << "IRQ " << irq_name << ": " << std::to_string(total) << " interrupts";
            if (!description.empty()) {
                std::cout << " (" << description << ")";
            }
            std::cout << " - " << status << std::endl;
            critical_count++;
        }
    }
    
    if (critical_count == 0) {
        std::cout << "No critical interrupt issues detected" << std::endl;
    }
    
    // Show storm summary only if there are issues
    int storm_count = 0;
    int unbalanced_count = 0;
    
    for (const auto& [irq_name, total, max_count, max_cpu, balance] : irq_analysis) {
        if (balance > 0.8) {
            storm_count++;
        } else if (balance > 0.5) {
            unbalanced_count++;
        }
    }
    
    if (storm_count > 0 || unbalanced_count > 0) {
        std::cout << std::endl;
        if (storm_count > 0) {
            std::cout << "⚠️  CRITICAL: " << storm_count << " interrupt storms detected!" << std::endl;
            std::cout << "   → Impact: CPU overwhelmed by interrupts, I/O performance severely degraded" << std::endl;
            std::cout << "   → Solution: Check device drivers, consider interrupt affinity tuning" << std::endl;
        }
        if (unbalanced_count > 0) {
            std::cout << "⚠️  WARNING: " << unbalanced_count << " unbalanced interrupts" << std::endl;
            std::cout << "   → Impact: Some CPU cores overloaded, others idle - poor scaling" << std::endl;
            std::cout << "   → Solution: Use irqbalance or manual IRQ affinity to distribute load" << std::endl;
        }
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

std::string CpuMonitor::getInterruptDescription(const std::string& irq_name) const {
    // Common interrupt mappings
    static const std::map<std::string, std::string> interrupt_descriptions = {
        {"0", "Timer"},
        {"1", "Keyboard"},
        {"2", "Cascade"},
        {"3", "Serial"},
        {"4", "Serial"},
        {"5", "Parallel"},
        {"6", "Floppy"},
        {"7", "Parallel"},
        {"8", "RTC"},
        {"9", "ACPI"},
        {"10", "Network"},
        {"11", "USB"},
        {"12", "PS/2 Mouse"},
        {"13", "FPU"},
        {"14", "Primary IDE"},
        {"15", "Secondary IDE"},
        {"16", "SATA"},
        {"17", "USB"},
        {"18", "USB"},
        {"19", "USB"},
        {"20", "USB"},
        {"21", "USB"},
        {"22", "USB"},
        {"23", "USB"},
        {"24", "USB"},
        {"25", "USB"},
        {"26", "USB"},
        {"27", "USB"},
        {"28", "USB"},
        {"29", "USB"},
        {"30", "USB"},
        {"31", "USB"},
        {"32", "Local APIC Timer"},
        {"33", "Local APIC Thermal"},
        {"34", "Local APIC Performance"},
        {"35", "Local APIC Error"},
        {"36", "Local APIC Spurious"},
        {"37", "Local APIC"},
        {"38", "Local APIC"},
        {"39", "Local APIC"},
        {"40", "Local APIC"},
        {"41", "Local APIC"},
        {"42", "Local APIC"},
        {"43", "Local APIC"},
        {"44", "Local APIC"},
        {"45", "Local APIC"},
        {"46", "PCIe"},
        {"47", "PCIe"},
        {"48", "PCIe"},
        {"49", "PCIe"},
        {"50", "PCIe"},
        {"51", "PCIe"},
        {"52", "PCIe"},
        {"53", "PCIe"},
        {"54", "PCIe"},
        {"55", "GPU"},
        {"56", "Audio"},
        {"57", "Audio"},
        {"58", "Audio"},
        {"59", "Audio"},
        {"60", "Audio"},
        {"61", "Audio"},
        {"62", "Audio"},
        {"63", "Audio"},
        {"64", "Audio"},
        {"65", "Audio"},
        {"66", "Audio"},
        {"67", "Audio"},
        {"68", "Audio"},
        {"69", "Audio"},
        {"70", "Audio"},
        {"71", "Audio"},
        {"72", "Audio"},
        {"73", "Audio"},
        {"74", "Audio"},
        {"75", "Audio"},
        {"76", "Audio"},
        {"77", "Audio"},
        {"78", "Audio"},
        {"79", "Audio"},
        {"80", "Audio"},
        {"81", "Audio"},
        {"82", "Audio"},
        {"83", "Audio"},
        {"84", "Audio"},
        {"85", "Audio"},
        {"86", "Audio"},
        {"87", "Audio"},
        {"88", "Audio"},
        {"89", "Audio"},
        {"90", "Audio"},
        {"91", "Audio"},
        {"92", "Audio"},
        {"93", "Audio"},
        {"94", "Audio"},
        {"95", "Audio"},
        {"96", "Audio"},
        {"97", "Audio"},
        {"98", "Audio"},
        {"99", "Audio"},
        {"100", "Audio"},
        {"101", "Audio"},
        {"102", "Audio"},
        {"103", "Audio"},
        {"104", "Audio"},
        {"105", "Audio"},
        {"106", "Audio"},
        {"107", "Audio"},
        {"108", "Audio"},
        {"109", "Audio"},
        {"110", "Audio"},
        {"111", "Audio"},
        {"112", "Audio"},
        {"113", "Audio"},
        {"114", "Audio"},
        {"115", "Audio"},
        {"116", "Audio"},
        {"117", "Audio"},
        {"118", "Audio"},
        {"119", "Audio"},
        {"120", "Audio"},
        {"121", "Audio"},
        {"122", "Audio"},
        {"123", "Audio"},
        {"124", "Audio"},
        {"125", "Audio"},
        {"126", "Audio"},
        {"127", "Audio"},
        {"128", "Audio"},
        {"129", "Audio"},
        {"130", "Audio"},
        {"131", "Audio"},
        {"132", "Audio"},
        {"133", "Audio"},
        {"134", "Audio"},
        {"135", "Audio"},
        {"136", "Audio"},
        {"137", "Audio"},
        {"138", "Audio"},
        {"139", "Audio"},
        {"140", "Audio"},
        {"141", "Audio"},
        {"142", "Audio"},
        {"143", "Audio"},
        {"144", "Audio"},
        {"145", "Audio"},
        {"146", "Audio"},
        {"147", "Audio"},
        {"148", "Audio"},
        {"149", "Audio"},
        {"150", "Audio"},
        {"151", "Audio"},
        {"152", "Audio"},
        {"153", "Audio"},
        {"154", "Audio"},
        {"155", "Audio"},
        {"156", "Audio"},
        {"157", "Audio"},
        {"158", "Audio"},
        {"159", "Audio"},
        {"160", "Audio"},
        {"161", "Audio"},
        {"162", "Audio"},
        {"163", "Audio"},
        {"164", "Audio"},
        {"165", "Audio"},
        {"166", "Audio"},
        {"167", "Audio"},
        {"168", "Audio"},
        {"169", "Audio"},
        {"170", "Audio"},
        {"171", "Audio"},
        {"172", "Audio"},
        {"173", "Audio"},
        {"174", "Audio"},
        {"175", "Audio"},
        {"176", "Audio"},
        {"177", "Audio"},
        {"178", "Audio"},
        {"179", "Audio"},
        {"180", "Audio"},
        {"181", "Audio"},
        {"182", "Audio"},
        {"183", "Audio"},
        {"184", "Audio"},
        {"185", "Audio"},
        {"186", "Audio"},
        {"187", "Audio"},
        {"188", "Audio"},
        {"189", "Audio"},
        {"190", "Audio"},
        {"191", "Audio"},
        {"192", "Audio"},
        {"193", "Audio"},
        {"194", "Audio"},
        {"195", "Audio"},
        {"196", "Audio"},
        {"197", "Audio"},
        {"198", "Audio"},
        {"199", "Audio"},
        {"200", "Audio"},
        {"201", "Audio"},
        {"202", "Audio"},
        {"203", "Audio"},
        {"204", "Audio"},
        {"205", "Audio"},
        {"206", "Audio"},
        {"207", "Audio"},
        {"208", "Audio"},
        {"209", "Audio"},
        {"210", "Audio"},
        {"211", "Audio"},
        {"212", "Audio"},
        {"213", "Audio"},
        {"214", "Audio"},
        {"215", "Audio"},
        {"216", "Audio"},
        {"217", "Audio"},
        {"218", "Audio"},
        {"219", "Audio"},
        {"220", "Audio"},
        {"221", "Audio"},
        {"222", "Audio"},
        {"223", "Audio"},
        {"224", "Audio"},
        {"225", "Audio"},
        {"226", "Audio"},
        {"227", "Audio"},
        {"228", "Audio"},
        {"229", "Audio"},
        {"230", "Audio"},
        {"231", "Audio"},
        {"232", "Audio"},
        {"233", "Audio"},
        {"234", "Audio"},
        {"235", "Audio"},
        {"236", "Audio"},
        {"237", "Audio"},
        {"238", "Audio"},
        {"239", "Audio"},
        {"240", "Audio"},
        {"241", "Audio"},
        {"242", "Audio"},
        {"243", "Audio"},
        {"244", "Audio"},
        {"245", "Audio"},
        {"246", "Audio"},
        {"247", "Audio"},
        {"248", "Audio"},
        {"249", "Audio"},
        {"250", "Audio"},
        {"251", "Audio"},
        {"252", "Audio"},
        {"253", "Audio"},
        {"254", "Audio"},
        {"255", "Audio"}
    };
    
    auto it = interrupt_descriptions.find(irq_name);
    if (it != interrupt_descriptions.end()) {
        return it->second;
    }
    
    // If not found, return empty string
    return "";
}