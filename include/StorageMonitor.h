#pragma once

#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <unordered_map>

// Disk statistics from /proc/diskstats
struct DiskStats {
    std::string device_name;        // nvme0n1, nvme1n1, etc.
    unsigned long reads;            // Number of read operations
    unsigned long read_merges;      // Number of read merges
    unsigned long read_sectors;     // Sectors read
    unsigned long read_time;        // Time spent reading (ms)
    unsigned long writes;           // Number of write operations
    unsigned long write_merges;     // Number of write merges
    unsigned long write_sectors;    // Sectors written
    unsigned long write_time;       // Time spent writing (ms)
    unsigned long io_in_progress;   // Current I/O operations
    unsigned long io_time;          // Total I/O time
    unsigned long weighted_io_time; // Weighted I/O time
    
    // Calculated metrics
    double read_iops;               // Read operations per second
    double write_iops;              // Write operations per second
    double total_iops;              // Total IOPS
    double read_mbps;               // Read throughput (MB/s)
    double write_mbps;              // Write throughput (MB/s)
    double total_mbps;              // Total throughput (MB/s)
    double avg_latency;             // Average I/O latency (ms)
    double queue_depth;             // Current queue depth
    bool is_hot_device;             // High I/O activity detected
};

// Device queue statistics
struct QueueStats {
    std::string device_name;
    unsigned long queue_depth;       // Current queue depth
    unsigned long max_queue_depth;  // Maximum queue depth
    double avg_queue_depth;          // Average queue depth
    double queue_utilization;       // Queue utilization percentage
};

class StorageMonitor {
    public:
        StorageMonitor();
        ~StorageMonitor() = default;
        
        bool update();
        void printStats();
        void printHotDevices();
        void printQueueAnalysis();
        void printPerformanceSummary();
        
        // Add these getter methods
        double getTotalIOPS() const;
        double getTotalThroughput() const;
        int getHotDeviceCount() const;
        int getBottleneckCount() const;
        void printDetailedDeviceStats();
        void printSchedulerInfo();
        
    private:
        bool discoverDevices();
        bool parseDiskStats();
        bool parseDeviceStats();  // Parse /sys/block/{device}/stat
        bool parseQueueStats();   // Parse queue depth info
        bool parseSchedulerInfo(); // Parse I/O scheduler
        void calculatePerformance();
        void detectHotDevices();
        void calculateQueueStats();
        void calculateLatencyMetrics();

        struct DeviceDetails {
            std::string scheduler;
            unsigned long queue_depth;
            unsigned long max_queue_depth;
            double avg_latency;
            double service_time;
        };
        std::map<std::string, DeviceDetails> device_details_;
        std::ifstream diskstats_file_;
        std::map<std::string, DiskStats> disk_stats_;
        std::map<std::string, DiskStats> previous_stats_;
        std::vector<std::string> devices_;
        std::unordered_map<std::string, QueueStats> queue_stats_;
        bool first_reading_;
    };