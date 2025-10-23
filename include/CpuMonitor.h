#pragma once

#include <string>
#include <fstream>
#include <map>
#include <vector>

struct CpuTimes {
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
    unsigned long iowait;
    unsigned long irq;
    unsigned long softirq;
    unsigned long steal;
    unsigned long guest;
    unsigned long guest_nice;

    // Calculated percentages
    double user_percent;
    double nice_percent;
    double system_percent;
    double idle_percent;
    double iowait_percent;
    double irq_percent;
    double softirq_percent;
    double steal_percent;
    double guest_percent;
    double guest_nice_percent;
};

class CpuMonitor {
public:
    CpuMonitor();
    ~CpuMonitor() = default;
    
    bool update();
    void printStats();
    
           // Add these getter methods
           double getCpuUsage() const { return 100.0 - current_.idle_percent; }
           double getUserUsage() const { return current_.user_percent; }
           double getSystemUsage() const { return current_.system_percent; }
           double getIOWait() const { return current_.iowait_percent; }
           double getHardIRQ() const { return current_.irq_percent; }
           double getSoftIRQ() const { return current_.softirq_percent; }
    void printInterruptStats();
    std::map<std::string, std::vector<unsigned long>> getInterruptCounts() const;
    std::string getInterruptDescription(const std::string& irq_name) const;
    
private:
    bool parseProcStat();
    void calculatePercentages();
    bool parseProcInterrupts();
    std::map<std::string, std::vector<unsigned long>> interrupt_counts_;
    std::map<std::string, std::vector<unsigned long>> previous_interrupt_counts_;
    
    std::ifstream proc_stat_file_;
    CpuTimes current_;
    CpuTimes previous_;
    bool first_reading_;
};