#pragma once 

#include <string>
#include <fstream>

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
        ~CpuMonitor() = default;  // Destructor
        
        bool update();       // Call this every second
        void printStats();   // Print current stats
        
    private:
        bool parseProcStat();
        void calculatePercentages();
        
        std::ifstream proc_stat_file_;
        CpuTimes current_;
        CpuTimes previous_;
        bool first_reading_;
};