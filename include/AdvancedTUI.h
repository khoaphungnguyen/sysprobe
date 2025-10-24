#pragma once

#include <ncurses.h>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <deque>

// Forward declarations
class CpuMonitor;
class MemoryMonitor;
class StorageMonitor;
class PerfMonitor;
class NumaMonitor;
class ProcessMonitor;

struct TimeSeriesData {
    std::deque<double> values;
    std::deque<std::chrono::steady_clock::time_point> timestamps;
    size_t max_points;
    double min_value;
    double max_value;
    
    TimeSeriesData(size_t max_pts = 60) : max_points(max_pts), min_value(0.0), max_value(100.0) {}
    
    void addPoint(double value);
    void drawSparkline(WINDOW* win, int y, int x, int width, int height);
};

class AdvancedTUI {
public:
    AdvancedTUI();
    ~AdvancedTUI();
    
    bool initialize();
    void run();
    void cleanup();
    
    // View management
    void showOverview();
    void showStorageDetail();
    void showPerformanceCounters();
    void showProcessDrillDown();
    void showNUMAView();
    
    // Data integration
    void setMonitors(CpuMonitor* cpu, MemoryMonitor* mem, StorageMonitor* storage,
                    PerfMonitor* perf, NumaMonitor* numa, ProcessMonitor* process);
    
private:
    // NCurses setup
    bool setupColors();
    void createWindows();
    void destroyWindows();
    
    // Drawing functions
    void drawHeader();
    void drawOverview();
    void drawStorageDetail();
    void drawPerformanceCounters();
    void drawProcessDrillDown();
    void drawNUMAView();
    void drawFooter();
    
    // Helper functions
    void drawProgressBar(WINDOW* win, int y, int x, int width, double value, double max, const std::string& label);
    void drawSparkline(WINDOW* win, int y, int x, int width, const TimeSeriesData& data, const std::string& label);
    void drawTable(WINDOW* win, int y, int x, const std::vector<std::vector<std::string>>& data);
    void drawAlert(WINDOW* win, int y, int x, const std::string& message, int color_pair);
    
    // Input handling
    void handleInput();
    
    // Data storage
    TimeSeriesData cpu_usage_history_;
    TimeSeriesData memory_usage_history_;
    TimeSeriesData storage_iops_history_;
    TimeSeriesData perf_ipc_history_;
    TimeSeriesData perf_cache_hit_history_;
    
    // Monitor references
    CpuMonitor* cpu_monitor_;
    MemoryMonitor* memory_monitor_;
    StorageMonitor* storage_monitor_;
    PerfMonitor* perf_monitor_;
    NumaMonitor* numa_monitor_;
    ProcessMonitor* process_monitor_;
    
    // NCurses windows
    WINDOW* main_window_;
    WINDOW* header_window_;
    WINDOW* content_window_;
    WINDOW* footer_window_;
    
    // UI state
    enum ViewMode {
        OVERVIEW,
        STORAGE_DETAIL,
        PERFORMANCE_COUNTERS,
        PROCESS_DRILLDOWN,
        NUMA_VIEW
    } current_view_;
    
    bool running_;
    std::chrono::steady_clock::time_point last_update_;
    
    // Color pairs
    int COLOR_PAIR_NORMAL;
    int COLOR_PAIR_WARNING;
    int COLOR_PAIR_CRITICAL;
    int COLOR_PAIR_SUCCESS;
    int COLOR_PAIR_HEADER;
    int COLOR_PAIR_BORDER;
};
