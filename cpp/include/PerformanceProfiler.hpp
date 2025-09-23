#ifndef __PERFORMANCE_PROFILER_HPP__
#define __PERFORMANCE_PROFILER_HPP__

#include <chrono>
#include <iostream>
#include <string>
#include <map>
#include <mutex>

class PerformanceProfiler {
public:
    static PerformanceProfiler& getInstance() {
        static PerformanceProfiler instance;
        return instance;
    }

    class ScopedTimer {
    public:
        ScopedTimer(const std::string& name) : name_(name) {
            start_ = std::chrono::high_resolution_clock::now();
        }
        
        ~ScopedTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
            PerformanceProfiler::getInstance().recordTiming(name_, duration.count());
        }
        
    private:
        std::string name_;
        std::chrono::high_resolution_clock::time_point start_;
    };

    void recordTiming(const std::string& name, long microseconds) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto& stats = timings_[name];
        stats.count++;
        stats.total_time += microseconds;
        stats.last_time = microseconds;
        
        if (microseconds > stats.max_time) {
            stats.max_time = microseconds;
        }
        if (stats.min_time == 0 || microseconds < stats.min_time) {
            stats.min_time = microseconds;
        }
    }

    void printReport() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::cout << "\n=== PERFORMANCE REPORT ===" << std::endl;
        std::cout << "Operation                    | Count |    Avg  |   Last  |    Min  |    Max  | Total " << std::endl;
        std::cout << "----------------------------|-------|---------|---------|---------|---------|-------" << std::endl;
        
        for (const auto& pair : timings_) {
            const std::string& name = pair.first;
            const TimingStats& stats = pair.second;
            
            double avg = static_cast<double>(stats.total_time) / stats.count / 1000.0; // ms
            double last = stats.last_time / 1000.0; // ms
            double min = stats.min_time / 1000.0; // ms  
            double max = stats.max_time / 1000.0; // ms
            double total = stats.total_time / 1000.0; // ms
            
            std::printf("%-28s | %5ld | %6.2fms | %6.2fms | %6.2fms | %6.2fms | %6.0fms\n",
                       name.substr(0, 28).c_str(), stats.count, avg, last, min, max, total);
        }
        
        std::cout << "=============================" << std::endl;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        timings_.clear();
    }

private:
    struct TimingStats {
        long count = 0;
        long total_time = 0;  // microseconds
        long last_time = 0;
        long min_time = 0;
        long max_time = 0;
    };

    std::map<std::string, TimingStats> timings_;
    std::mutex mutex_;
};

// Convenience macro for scoped timing
#define PROFILE_SCOPE(name) PerformanceProfiler::ScopedTimer timer(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)

#endif // __PERFORMANCE_PROFILER_HPP__
