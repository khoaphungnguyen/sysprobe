#include "StorageMonitor.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>

StorageMonitor::StorageMonitor() : first_reading_(true) {
    // Open /proc/diskstats for reading
    diskstats_file_.open("/proc/diskstats");
    if (!diskstats_file_.is_open()) {
        std::cerr << "Failed to open /proc/diskstats (Linux only)" << std::endl;
        return;
    }
    
    // Discover NVMe devices
    if (!discoverDevices()) {
        std::cerr << "Failed to discover NVMe devices" << std::endl;
        return;
    }
}

// Update StorageMonitor to support all device types
bool StorageMonitor::discoverDevices() {
    devices_.clear();
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator("/sys/block")) {
            std::string device_name = entry.path().filename().string();
            
            // Support all block devices, not just NVMe
            if (device_name.substr(0, 4) == "nvme" ||  // NVMe
                device_name.substr(0, 2) == "sd" ||    // SATA/SAS
                device_name.substr(0, 3) == "md" ||    // VROC drive
                device_name.substr(0, 3) == "gdg" ||    // GRAID drive
                device_name.substr(0, 3) == "sxl" ) {   // SMARTRAID drive
                
                devices_.push_back(device_name);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error scanning /sys/block: " << e.what() << std::endl;
        return false;
    }
    
    return true;
}

bool StorageMonitor::update() {
    if (!diskstats_file_.is_open()) {
        return false;
    }
    
    // Store previous reading
    previous_stats_ = disk_stats_;
    
    // Parse current reading
    if (!parseDiskStats()) {
        return false;
    }
    
    // Calculate performance metrics
    calculatePerformance();
    
    // Detect hot devices AFTER we have data
    detectHotDevices();
    
    // Calculate queue statistics
    calculateQueueStats();
    
    return true;
}

bool StorageMonitor::parseDiskStats() {    
    // Reset file position to beginning
    diskstats_file_.seekg(0);
    diskstats_file_.clear();
    
    std::string line;
    
    while (std::getline(diskstats_file_, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string device_name;
        int major, minor;
        
        // Parse major, minor, then device name
        iss >> major >> minor >> device_name;
        
        // Check if it's one of our NVMe devices
        if (std::find(devices_.begin(), devices_.end(), device_name) != devices_.end()) {
            DiskStats stats;
            stats.device_name = device_name;
            
            // Parse disk statistics
            if (iss >> stats.reads >> stats.read_merges >> stats.read_sectors >> stats.read_time
                >> stats.writes >> stats.write_merges >> stats.write_sectors >> stats.write_time
                >> stats.io_in_progress >> stats.io_time >> stats.weighted_io_time) {
                
                disk_stats_[device_name] = stats;
            }
        }
    }
    
    return true;
}

void StorageMonitor::calculatePerformance() {
    if (first_reading_) {
        first_reading_ = false;
        return;
    }
    
    for (auto& [device_name, current_stats] : disk_stats_) {
        if (previous_stats_.find(device_name) == previous_stats_.end()) {
            continue;
        }
        
        const auto& prev_stats = previous_stats_[device_name];
        
        // Calculate IOPS (operations per second)
        unsigned long read_ops = current_stats.reads - prev_stats.reads;
        unsigned long write_ops = current_stats.writes - prev_stats.writes;
        
        current_stats.read_iops = read_ops;
        current_stats.write_iops = write_ops;
        current_stats.total_iops = read_ops + write_ops;
        
        // Calculate throughput (MB/s)
        unsigned long read_sectors = current_stats.read_sectors - prev_stats.read_sectors;
        unsigned long write_sectors = current_stats.write_sectors - prev_stats.write_sectors;
        
        current_stats.read_mbps = (read_sectors * 512.0) / (1024.0 * 1024.0);  // 512 bytes per sector
        current_stats.write_mbps = (write_sectors * 512.0) / (1024.0 * 1024.0);
        current_stats.total_mbps = current_stats.read_mbps + current_stats.write_mbps;
        
        // Calculate average latency (ms)
        unsigned long io_time = current_stats.io_time - prev_stats.io_time;
        unsigned long total_ops = read_ops + write_ops;
        
        if (total_ops > 0) {
            current_stats.avg_latency = (double)io_time / total_ops;
        } else {
            current_stats.avg_latency = 0.0;
        }
        
        // Current queue depth
        current_stats.queue_depth = current_stats.io_in_progress;
    }
}

void StorageMonitor::detectHotDevices() {
     // Check if we have any data to work with
     if (disk_stats_.empty()) {
        return;
    }

    // Sort devices by IOPS to find hot devices
    std::vector<std::pair<std::string, double>> device_iops;
    
    for (const auto& [device_name, stats] : disk_stats_) {
        device_iops.push_back({device_name, stats.total_iops});
    }
    
    // Sort by IOPS (descending)
    std::sort(device_iops.begin(), device_iops.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Mark top 25% as hot devices
    size_t hot_count = std::max(1UL, device_iops.size() / 4);
    
    for (size_t i = 0; i < hot_count; ++i) {
        disk_stats_[device_iops[i].first].is_hot_device = true;
    }
}

void StorageMonitor::calculateQueueStats() {
    for (const auto& [device_name, stats] : disk_stats_) {
        QueueStats queue_stats;
        queue_stats.device_name = device_name;
        queue_stats.queue_depth = stats.io_in_progress;
        queue_stats.max_queue_depth = 128;  // Typical NVMe queue depth
        queue_stats.avg_queue_depth = stats.queue_depth;
        queue_stats.queue_utilization = (double)stats.io_in_progress / 128.0 * 100.0;
        
        queue_stats_[device_name] = queue_stats;
    }
}

void StorageMonitor::printStats() {
    if (first_reading_) {
        std::cout << "Storage Stats (first reading - metrics not available yet)" << std::endl;
        return;
    }

     // Check if we have any data
     if (disk_stats_.empty()) {
        std::cout << "No storage data available" << std::endl;
        return;
    }
    
    std::cout << "\n=== Storage Performance Analysis ===" << std::endl;
    std::cout << "Total Devices: " << devices_.size() << " NVMe drives" << std::endl;
    
    double total_iops = 0.0;
    double total_mbps = 0.0;
    
    for (const auto& [device_name, stats] : disk_stats_) {
        total_iops += stats.total_iops;
        total_mbps += stats.total_mbps;
    }
    
    std::cout << "Total IOPS: " << std::fixed << std::setprecision(0) << total_iops << std::endl;
    std::cout << "Total Throughput: " << std::fixed << std::setprecision(2) << total_mbps << " MB/s" << std::endl;
    
    // Print individual device stats
    std::cout << "\nPer-Device Statistics:" << std::endl;
    std::cout << std::left << std::setw(12) << "Device" 
              << std::setw(10) << "IOPS" 
              << std::setw(12) << "Throughput" 
              << std::setw(10) << "Latency" 
              << std::setw(12) << "Queue Depth" 
              << std::setw(10) << "Status" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    for (const auto& [device_name, stats] : disk_stats_) {
        std::string status = stats.is_hot_device ? "HOT" : "NORMAL";
        if (stats.queue_depth > 100) {
            status = "BOTTLENECK";
        } else if (stats.queue_depth > 50) {
            status = "WARNING";
        }
        
        std::cout << std::left << std::setw(12) << device_name
                  << std::setw(10) << std::fixed << std::setprecision(0) << stats.total_iops
                  << std::setw(12) << std::fixed << std::setprecision(2) << stats.total_mbps << " MB/s"
                  << std::setw(10) << std::fixed << std::setprecision(2) << stats.avg_latency << " ms"
                  << std::setw(12) << std::to_string((int)stats.queue_depth) + "/128"
                  << std::setw(10) << status << std::endl;
    }
}



void StorageMonitor::printHotDevices() {
    std::cout << "\n=== Hot Devices Analysis ===" << std::endl;
    
    std::vector<std::pair<std::string, DiskStats>> hot_devices;
    
    for (const auto& [device_name, stats] : disk_stats_) {
        if (stats.is_hot_device) {
            hot_devices.push_back({device_name, stats});
        }
    }
    
    if (hot_devices.empty()) {
        std::cout << "No hot devices detected" << std::endl;
        return;
    }
    
    // Sort by IOPS
    std::sort(hot_devices.begin(), hot_devices.end(),
              [](const auto& a, const auto& b) { return a.second.total_iops > b.second.total_iops; });
    
    for (const auto& [device_name, stats] : hot_devices) {
        std::string status = "HOT";
        if (stats.queue_depth > 100) {
            status = "BOTTLENECK";
        } else if (stats.queue_depth > 50) {
            status = "WARNING";
        }
        
        std::cout << device_name << ": " << std::fixed << std::setprecision(0) << stats.total_iops 
                  << " IOPS, Queue: " << (int)stats.queue_depth << "/128 (" 
                  << std::fixed << std::setprecision(1) << (stats.queue_depth/128.0*100.0) << "% full) - " 
                  << status << std::endl;
    }
}

void StorageMonitor::printQueueAnalysis() {
    std::cout << "\n=== Queue Depth Analysis ===" << std::endl;
    
    int bottleneck_count = 0;
    int warning_count = 0;
    int normal_count = 0;
    
    for (const auto& [device_name, stats] : disk_stats_) {
        if (stats.queue_depth > 100) {
            bottleneck_count++;
        } else if (stats.queue_depth > 50) {
            warning_count++;
        } else {
            normal_count++;
        }
    }
    
    std::cout << bottleneck_count << " devices at 100% queue depth" << std::endl;
    std::cout << warning_count << " devices at 50-99% queue depth" << std::endl;
    std::cout << normal_count << " devices at normal queue depth" << std::endl;
    
    if (bottleneck_count > 0) {
        std::cout << "\nRecommendation: Implement interrupt grouping for bottlenecked devices" << std::endl;
    }
}

void StorageMonitor::printPerformanceSummary() {
    if (first_reading_) {
        return;
    }
    
    double total_iops = 0.0;
    int hot_count = 0;
    int bottleneck_count = 0;
    
    for (const auto& [device_name, stats] : disk_stats_) {
        total_iops += stats.total_iops;
        if (stats.is_hot_device) hot_count++;
        if (stats.queue_depth > 100) bottleneck_count++;
    }
    
    std::cout << "\n=== Performance Summary ===" << std::endl;
    std::cout << "Total IOPS: " << std::fixed << std::setprecision(0) << total_iops << std::endl;
    std::cout << "Hot Devices: " << hot_count << "/" << devices_.size() << std::endl;
    std::cout << "Bottlenecked Devices: " << bottleneck_count << "/" << devices_.size() << std::endl;
    
    if (bottleneck_count > 0) {
        std::cout << "Performance Impact: " << std::fixed << std::setprecision(1) 
                  << (100.0 - (total_iops / (devices_.size() * 3000.0)) * 100.0) << "% performance loss" << std::endl;
    }
}



double StorageMonitor::getTotalIOPS() const {
    double total = 0.0;
    for (const auto& [device_name, stats] : disk_stats_) {
        total += stats.total_iops;
    }
    return total;
}

double StorageMonitor::getTotalThroughput() const {
    double total = 0.0;
    for (const auto& [device_name, stats] : disk_stats_) {
        total += stats.total_mbps;
    }
    return total;
}

int StorageMonitor::getHotDeviceCount() const {
    int count = 0;
    for (const auto& [device_name, stats] : disk_stats_) {
        if (stats.is_hot_device) count++;
    }
    return count;
}

int StorageMonitor::getBottleneckCount() const {
    int count = 0;
    for (const auto& [device_name, stats] : disk_stats_) {
        if (stats.queue_depth > 100) count++;
    }
    return count;
}

bool StorageMonitor::parseDeviceStats() {
    for (const auto& device : devices_) {
        std::string stat_path = "/sys/block/" + device + "/stat";
        std::ifstream stat_file(stat_path);
        
        if (stat_file.is_open()) {
            std::string line;
            std::getline(stat_file, line);
            std::istringstream iss(line);
            
            // Parse /sys/block/{device}/stat format
            unsigned long reads, read_merges, read_sectors, read_time;
            unsigned long writes, write_merges, write_sectors, write_time;
            unsigned long io_in_progress, io_time, weighted_io_time;
            
            if (iss >> reads >> read_merges >> read_sectors >> read_time
                >> writes >> write_merges >> write_sectors >> write_time
                >> io_in_progress >> io_time >> weighted_io_time) {
                
                // Calculate latency metrics
                double total_ios = reads + writes;
                if (total_ios > 0) {
                    device_details_[device].avg_latency = (double)weighted_io_time / total_ios;
                    device_details_[device].service_time = (double)(read_time + write_time) / total_ios;
                }
            }
        }
    }
    return true;
}

bool StorageMonitor::parseQueueStats() {
    for (const auto& device : devices_) {
        std::string queue_path = "/sys/block/" + device + "/queue";
        
        // Read queue depth
        std::ifstream depth_file(queue_path + "/nr_requests");
        if (depth_file.is_open()) {
            depth_file >> device_details_[device].max_queue_depth;
        }
        
        // Read current queue depth from /proc/diskstats
        if (disk_stats_.find(device) != disk_stats_.end()) {
            device_details_[device].queue_depth = disk_stats_[device].io_in_progress;
        }
    }
    return true;
}

bool StorageMonitor::parseSchedulerInfo() {
    for (const auto& device : devices_) {
        std::string scheduler_path = "/sys/block/" + device + "/queue/scheduler";
        std::ifstream scheduler_file(scheduler_path);
        
        if (scheduler_file.is_open()) {
            std::string line;
            std::getline(scheduler_file, line);
            
            // Parse scheduler info (e.g., "mq-deadline kyber bfq [none]")
            size_t start = line.find('[');
            size_t end = line.find(']');
            if (start != std::string::npos && end != std::string::npos) {
                device_details_[device].scheduler = line.substr(start + 1, end - start - 1);
            }
        }
    }
    return true;
}

void StorageMonitor::printDetailedDeviceStats() {
    std::cout << "Detailed Device Statistics:" << std::endl;
    for (const auto& [device_name, stats] : disk_stats_) {
        std::cout << "Device: " << device_name << std::endl;
        std::cout << "  IOPS: " << stats.total_iops << std::endl;
        std::cout << "  Throughput: " << stats.total_mbps << " MB/s" << std::endl;
        std::cout << "  Latency: " << stats.avg_latency << " ms" << std::endl;
        std::cout << "  Queue Depth: " << stats.queue_depth << std::endl;
    }
}

void StorageMonitor::printSchedulerInfo() {
    std::cout << "I/O Scheduler Info:" << std::endl;
    for (const auto& [device_name, details] : device_details_) {
        std::cout << "Device: " << device_name << " - Scheduler: " << details.scheduler << std::endl;
    }
}