#include "AdvancedTUI.h"
#include "CpuMonitor.h"
#include "MemoryMonitor.h"
#include "StorageMonitor.h"
#include "PerfMonitor.h"
#include "NumaMonitor.h"
#include "ProcessMonitor.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

AdvancedTUI::AdvancedTUI() : 
    cpu_monitor_(nullptr), memory_monitor_(nullptr), storage_monitor_(nullptr),
    perf_monitor_(nullptr), numa_monitor_(nullptr), process_monitor_(nullptr),
    main_window_(nullptr), header_window_(nullptr), content_window_(nullptr), footer_window_(nullptr),
    current_view_(OVERVIEW), running_(false) {
    
    // Initialize time series data
    cpu_usage_history_ = TimeSeriesData(60);
    memory_usage_history_ = TimeSeriesData(60);
    storage_iops_history_ = TimeSeriesData(60);
    perf_ipc_history_ = TimeSeriesData(60);
    perf_cache_hit_history_ = TimeSeriesData(60);
}

AdvancedTUI::~AdvancedTUI() {
    cleanup();
}

bool AdvancedTUI::initialize() {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    // Setup colors
    if (!setupColors()) {
        return false;
    }
    
    // Create windows
    createWindows();
    
    running_ = true;
    last_update_ = std::chrono::steady_clock::now();
    
    return true;
}

bool AdvancedTUI::setupColors() {
    if (!has_colors()) {
        return false;
    }
    
    start_color();
    
    // Define color pairs
    COLOR_PAIR_NORMAL = 1;
    COLOR_PAIR_WARNING = 2;
    COLOR_PAIR_CRITICAL = 3;
    COLOR_PAIR_SUCCESS = 4;
    COLOR_PAIR_HEADER = 5;
    COLOR_PAIR_BORDER = 6;
    
    init_pair(COLOR_PAIR_NORMAL, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_PAIR_WARNING, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_PAIR_CRITICAL, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_PAIR_SUCCESS, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_PAIR_HEADER, COLOR_CYAN, COLOR_BLUE);
    init_pair(COLOR_PAIR_BORDER, COLOR_WHITE, COLOR_BLUE);
    
    return true;
}

void AdvancedTUI::createWindows() {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // Create main window
    main_window_ = newwin(max_y, max_x, 0, 0);
    wbkgd(main_window_, COLOR_PAIR(COLOR_PAIR_NORMAL));
    
    // Create header window (3 lines)
    header_window_ = newwin(3, max_x, 0, 0);
    wbkgd(header_window_, COLOR_PAIR(COLOR_PAIR_HEADER));
    
    // Create content window (max_y - 6 lines)
    content_window_ = newwin(max_y - 6, max_x - 2, 3, 1);
    wbkgd(content_window_, COLOR_PAIR(COLOR_PAIR_NORMAL));
    
    // Create footer window (3 lines)
    footer_window_ = newwin(3, max_x, max_y - 3, 0);
    wbkgd(footer_window_, COLOR_PAIR(COLOR_PAIR_BORDER));
}

void AdvancedTUI::destroyWindows() {
    if (header_window_) {
        delwin(header_window_);
        header_window_ = nullptr;
    }
    if (content_window_) {
        delwin(content_window_);
        content_window_ = nullptr;
    }
    if (footer_window_) {
        delwin(footer_window_);
        footer_window_ = nullptr;
    }
    if (main_window_) {
        delwin(main_window_);
        main_window_ = nullptr;
    }
}

void AdvancedTUI::run() {
    while (running_) {
        // Update data
        if (cpu_monitor_) cpu_monitor_->update();
        if (memory_monitor_) memory_monitor_->update();
        if (storage_monitor_) storage_monitor_->update();
        if (perf_monitor_) perf_monitor_->update();
        if (numa_monitor_) numa_monitor_->update();
        if (process_monitor_) process_monitor_->update();
        
        // Update time series data
        if (cpu_monitor_) {
            cpu_usage_history_.addPoint(cpu_monitor_->getCpuUsage());
        }
        if (memory_monitor_) {
            memory_usage_history_.addPoint(memory_monitor_->getMemoryUsage());
        }
        if (storage_monitor_) {
            storage_iops_history_.addPoint(storage_monitor_->getTotalIOPS());
        }
        if (perf_monitor_) {
            perf_ipc_history_.addPoint(perf_monitor_->getIPC());
            perf_cache_hit_history_.addPoint(perf_monitor_->getCacheHitRate());
        }
        
        // Clear and redraw
        werase(main_window_);
        werase(header_window_);
        werase(content_window_);
        werase(footer_window_);
        
        // Draw current view
        drawHeader();
        switch (current_view_) {
            case OVERVIEW:
                drawOverview();
                break;
            case STORAGE_DETAIL:
                drawStorageDetail();
                break;
            case PERFORMANCE_COUNTERS:
                drawPerformanceCounters();
                break;
            case PROCESS_DRILLDOWN:
                drawProcessDrillDown();
                break;
            case NUMA_VIEW:
                drawNUMAView();
                break;
        }
        drawFooter();
        
        // Refresh all windows
        wrefresh(main_window_);
        wrefresh(header_window_);
        wrefresh(content_window_);
        wrefresh(footer_window_);
        
        // Handle input
        handleInput();
        
        // Small delay
        napms(100);
    }
}

void AdvancedTUI::cleanup() {
    destroyWindows();
    endwin();
}

void AdvancedTUI::drawHeader() {
    wattron(header_window_, COLOR_PAIR(COLOR_PAIR_HEADER));
    
    // Title
    mvwprintw(header_window_, 0, 2, "🚀 Advanced System Monitor - Phase 6 TUI");
    
    // View indicator
    std::string view_name;
    switch (current_view_) {
        case OVERVIEW: view_name = "Overview"; break;
        case STORAGE_DETAIL: view_name = "Storage Detail"; break;
        case PERFORMANCE_COUNTERS: view_name = "Performance Counters"; break;
        case PROCESS_DRILLDOWN: view_name = "Process Drill-Down"; break;
        case NUMA_VIEW: view_name = "NUMA View"; break;
    }
    mvwprintw(header_window_, 0, 50, "View: %s", view_name.c_str());
    
    // System status
    if (cpu_monitor_ && memory_monitor_ && storage_monitor_) {
        double cpu_usage = cpu_monitor_->getCpuUsage();
        double mem_usage = memory_monitor_->getMemoryUsage();
        double storage_iops = storage_monitor_->getTotalIOPS();
        
        mvwprintw(header_window_, 1, 2, "CPU: %.1f%% | Memory: %.1f%% | Storage: %.0f IOPS", 
                  cpu_usage, mem_usage, storage_iops);
    }
    
    // Navigation hints
    mvwprintw(header_window_, 2, 2, "1-5: Switch Views | Q: Quit | R: Refresh");
    
    wattroff(header_window_, COLOR_PAIR(COLOR_PAIR_HEADER));
}

void AdvancedTUI::drawOverview() {
    int y = 0;
    
    // System Overview
    mvwprintw(content_window_, y++, 2, "📊 SYSTEM OVERVIEW");
    mvwprintw(content_window_, y++, 2, "─────────────────────────────────────────────────────────────────────");
    
    if (cpu_monitor_) {
        double cpu_usage = cpu_monitor_->getCpuUsage();
        drawProgressBar(content_window_, y++, 2, 50, cpu_usage, 100.0, "CPU Usage");
        
        // CPU breakdown
        mvwprintw(content_window_, y++, 2, "  User: %.1f%% | System: %.1f%% | IO Wait: %.1f%% | IRQ: %.1f%%", 
                  cpu_monitor_->getUserUsage(), cpu_monitor_->getSystemUsage(),
                  cpu_monitor_->getIOWait(), cpu_monitor_->getHardIRQ());
    }
    
    if (memory_monitor_) {
        double mem_usage = memory_monitor_->getMemoryUsage();
        drawProgressBar(content_window_, y++, 2, 50, mem_usage, 100.0, "Memory Usage");
        
        // Memory breakdown
        mvwprintw(content_window_, y++, 2, "  Available: %.0f MB | Cache: %.1f%% | Buffer: %.1f%%", 
                  memory_monitor_->getAvailableMemory() / 1024.0,
                  memory_monitor_->getCacheUsage(), memory_monitor_->getBufferUsage());
    }
    
    if (storage_monitor_) {
        double storage_iops = storage_monitor_->getTotalIOPS();
        drawProgressBar(content_window_, y++, 2, 50, storage_iops, 10000.0, "Storage IOPS");
        
        // Storage breakdown
        mvwprintw(content_window_, y++, 2, "  Hot Devices: %d | Bottlenecks: %d", 
                  storage_monitor_->getHotDeviceCount(), storage_monitor_->getBottleneckCount());
    }
    
    y += 2;
    
    // Historical Trends
    mvwprintw(content_window_, y++, 2, "📈 HISTORICAL TRENDS (Last 60 seconds)");
    mvwprintw(content_window_, y++, 2, "─────────────────────────────────────────────────────────────────────");
    
    if (cpu_monitor_) {
        drawSparkline(content_window_, y++, 2, 50, cpu_usage_history_, "CPU Usage");
    }
    if (memory_monitor_) {
        drawSparkline(content_window_, y++, 2, 50, memory_usage_history_, "Memory Usage");
    }
    if (storage_monitor_) {
        drawSparkline(content_window_, y++, 2, 50, storage_iops_history_, "Storage IOPS");
    }
}

void AdvancedTUI::drawStorageDetail() {
    int y = 0;
    
    mvwprintw(content_window_, y++, 2, "💾 STORAGE DETAIL ANALYSIS");
    mvwprintw(content_window_, y++, 2, "─────────────────────────────────────────────────────────────────────");
    
    if (storage_monitor_) {
        // Storage summary
        mvwprintw(content_window_, y++, 2, "Total IOPS: %.0f | Total Throughput: %.2f MB/s", 
                  storage_monitor_->getTotalIOPS(), storage_monitor_->getTotalThroughput());
        
        y += 2;
        
        // Device table header
        mvwprintw(content_window_, y++, 2, "%-12s %-10s %-12s %-10s %-12s %-10s", 
                  "Device", "IOPS", "Throughput", "Latency", "Queue Depth", "Status");
        mvwprintw(content_window_, y++, 2, "─────────────────────────────────────────────────────────────────────");
        
        // This would need access to individual device stats
        // For now, show summary
        mvwprintw(content_window_, y++, 2, "Hot Devices: %d | Bottlenecks: %d", 
                  storage_monitor_->getHotDeviceCount(), storage_monitor_->getBottleneckCount());
    }
}

void AdvancedTUI::drawPerformanceCounters() {
    int y = 0;
    
    mvwprintw(content_window_, y++, 2, "⚡ HARDWARE PERFORMANCE COUNTERS");
    mvwprintw(content_window_, y++, 2, "─────────────────────────────────────────────────────────────────────");
    
    if (perf_monitor_) {
        // IPC and Cache Performance
        mvwprintw(content_window_, y++, 2, "IPC (Instructions/Cycle): %.2f", perf_monitor_->getIPC());
        mvwprintw(content_window_, y++, 2, "Cache Hit Rate: %.1f%%", perf_monitor_->getCacheHitRate());
        mvwprintw(content_window_, y++, 2, "Branch Miss Rate: %.1f%%", perf_monitor_->getBranchMissRate());
        
        y += 2;
        
        // Performance analysis
        mvwprintw(content_window_, y++, 2, "🔍 PERFORMANCE ANALYSIS");
        mvwprintw(content_window_, y++, 2, "─────────────────────────────────────────────────────────────────────");
        
        if (perf_monitor_->isCacheThrashing()) {
            drawAlert(content_window_, y++, 2, "🔴 CACHE THRASHING DETECTED - Memory bandwidth bottleneck", COLOR_PAIR_CRITICAL);
        }
        
        if (perf_monitor_->isBranchMispredicting()) {
            drawAlert(content_window_, y++, 2, "🔴 HIGH BRANCH MISPREDICTION - CPU pipeline stalls", COLOR_PAIR_CRITICAL);
        }
        
        // Historical trends
        y += 2;
        mvwprintw(content_window_, y++, 2, "📈 PERFORMANCE TRENDS");
        drawSparkline(content_window_, y++, 2, 50, perf_ipc_history_, "IPC Trend");
        drawSparkline(content_window_, y++, 2, 50, perf_cache_hit_history_, "Cache Hit Rate Trend");
    }
}

void AdvancedTUI::drawProcessDrillDown() {
    int y = 0;
    
    mvwprintw(content_window_, y++, 2, "🔍 PROCESS-LEVEL ANALYSIS");
    mvwprintw(content_window_, y++, 2, "─────────────────────────────────────────────────────────────────────");
    
    if (process_monitor_) {
        const auto& process_stats = process_monitor_->getProcessStats();
        mvwprintw(content_window_, y++, 2, "Total Processes: %zu", process_stats.size());
        
        y += 2;
        
        // Top processes table
        mvwprintw(content_window_, y++, 2, "%-8s %-20s %-10s %-12s %-15s", 
                  "PID", "COMMAND", "CPU%", "MEMORY(MB)", "STATUS");
        mvwprintw(content_window_, y++, 2, "─────────────────────────────────────────────────────────────────────");
        
        // Get top processes
        auto top_cpu = process_monitor_->getTopCPUProcesses(10);
        for (pid_t pid : top_cpu) {
            auto stats = process_monitor_->getProcessStats(pid);
            std::string status = "NORMAL";
            if (stats.is_cpu_intensive) status = "CPU_INT";
            if (stats.is_memory_intensive) status += "+MEM";
            if (stats.is_io_intensive) status += "+IO";
            
            mvwprintw(content_window_, y++, 2, "%-8d %-20s %-10.1f %-12.1f %-15s", 
                      stats.pid, stats.comm.substr(0, 19).c_str(), 
                      stats.cpu_usage_percent, stats.memory_usage_mb, status.c_str());
        }
    }
}

void AdvancedTUI::drawNUMAView() {
    int y = 0;
    
    mvwprintw(content_window_, y++, 2, "🏗️  NUMA TOPOLOGY & MEMORY ANALYSIS");
    mvwprintw(content_window_, y++, 2, "─────────────────────────────────────────────────────────────────────");
    
    if (numa_monitor_) {
        mvwprintw(content_window_, y++, 2, "NUMA Nodes: %d", numa_monitor_->getNumaNodeCount());
        mvwprintw(content_window_, y++, 2, "Total Memory Usage: %.1f%%", numa_monitor_->getTotalMemoryUsage());
        
        if (numa_monitor_->isMemoryPressured()) {
            drawAlert(content_window_, y++, 2, "🔴 MEMORY PRESSURE DETECTED", COLOR_PAIR_CRITICAL);
        }
        
        if (numa_monitor_->isSwapping()) {
            drawAlert(content_window_, y++, 2, "🔴 SWAPPING DETECTED - Performance severely degraded", COLOR_PAIR_CRITICAL);
        }
        
        mvwprintw(content_window_, y++, 2, "Memory Pressure: %.1f%%", numa_monitor_->getMemoryPressure());
    }
}

void AdvancedTUI::drawFooter() {
    wattron(footer_window_, COLOR_PAIR(COLOR_PAIR_BORDER));
    
    // Status indicators
    mvwprintw(footer_window_, 0, 2, "Status: ");
    
    bool has_issues = false;
    if (cpu_monitor_ && cpu_monitor_->getCpuUsage() > 90) {
        waddstr(footer_window_, "🔴 CPU ");
        has_issues = true;
    }
    if (memory_monitor_ && memory_monitor_->getMemoryUsage() > 90) {
        waddstr(footer_window_, "🔴 MEM ");
        has_issues = true;
    }
    if (storage_monitor_ && storage_monitor_->getBottleneckCount() > 0) {
        waddstr(footer_window_, "🔴 STORAGE ");
        has_issues = true;
    }
    
    if (!has_issues) {
        waddstr(footer_window_, "🟢 HEALTHY");
    }
    
    // Update time
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_update_).count();
    mvwprintw(footer_window_, 0, 50, "Uptime: %lds", elapsed);
    
    wattroff(footer_window_, COLOR_PAIR(COLOR_PAIR_BORDER));
}

void AdvancedTUI::drawProgressBar(WINDOW* win, int y, int x, int width, double value, double max, const std::string& label) {
    mvwprintw(win, y, x, "%s: ", label.c_str());
    
    int filled = (int)((value / max) * (width - label.length() - 10));
    filled = std::max(0, std::min(filled, width - (int)label.length() - 10));
    
    waddch(win, '[');
    for (int i = 0; i < width - (int)label.length() - 10; i++) {
        if (i < filled) {
            if (value > 80) {
                waddch(win, '#');
            } else if (value > 50) {
                waddch(win, '=');
            } else {
                waddch(win, '-');
            }
        } else {
            waddch(win, ' ');
        }
    }
    waddch(win, ']');
    mvwprintw(win, y, x + width - 8, " %.1f%%", value);
}

void AdvancedTUI::drawSparkline(WINDOW* win, int y, int x, int width, const TimeSeriesData& data, const std::string& label) {
    mvwprintw(win, y, x, "%s: ", label.c_str());
    
    if (data.values.empty()) {
        mvwprintw(win, y, x + label.length() + 2, "No data");
        return;
    }
    
    // Find min/max for scaling
    double min_val = *std::min_element(data.values.begin(), data.values.end());
    double max_val = *std::max_element(data.values.begin(), data.values.end());
    
    if (max_val == min_val) {
        mvwprintw(win, y, x + label.length() + 2, "Flat line");
        return;
    }
    
    // Draw sparkline
    int spark_width = width - (int)label.length() - 15;
    for (size_t i = 0; i < std::min(data.values.size(), (size_t)spark_width); i++) {
        double normalized = (data.values[i] - min_val) / (max_val - min_val);
        int height = (int)(normalized * 8);
        
        char ch;
        if (height >= 7) ch = '#';
        else if (height >= 6) ch = '=';
        else if (height >= 5) ch = '+';
        else if (height >= 4) ch = '*';
        else if (height >= 3) ch = 'o';
        else if (height >= 2) ch = '.';
        else if (height >= 1) ch = ',';
        else ch = ' ';
        
        mvwaddch(win, y, x + label.length() + 2 + i, ch);
    }
    
    // Show current value
    mvwprintw(win, y, x + width - 8, " %.1f", data.values.back());
}

void AdvancedTUI::drawAlert(WINDOW* win, int y, int x, const std::string& message, int color_pair) {
    wattron(win, COLOR_PAIR(color_pair));
    mvwprintw(win, y, x, "%s", message.c_str());
    wattroff(win, COLOR_PAIR(color_pair));
}

void AdvancedTUI::handleInput() {
    int ch = getch();
    
    switch (ch) {
        case '1':
            current_view_ = OVERVIEW;
            break;
        case '2':
            current_view_ = STORAGE_DETAIL;
            break;
        case '3':
            current_view_ = PERFORMANCE_COUNTERS;
            break;
        case '4':
            current_view_ = PROCESS_DRILLDOWN;
            break;
        case '5':
            current_view_ = NUMA_VIEW;
            break;
        case 'q':
        case 'Q':
            running_ = false;
            break;
        case 'r':
        case 'R':
            // Force refresh
            break;
    }
}

void AdvancedTUI::setMonitors(CpuMonitor* cpu, MemoryMonitor* mem, StorageMonitor* storage,
                             PerfMonitor* perf, NumaMonitor* numa, ProcessMonitor* process) {
    cpu_monitor_ = cpu;
    memory_monitor_ = mem;
    storage_monitor_ = storage;
    perf_monitor_ = perf;
    numa_monitor_ = numa;
    process_monitor_ = process;
}

// TimeSeriesData implementation
void TimeSeriesData::addPoint(double value) {
    values.push_back(value);
    timestamps.push_back(std::chrono::steady_clock::now());
    
    if (values.size() > max_points) {
        values.pop_front();
        timestamps.pop_front();
    }
    
    // Update min/max
    if (value < min_value) min_value = value;
    if (value > max_value) max_value = value;
}
