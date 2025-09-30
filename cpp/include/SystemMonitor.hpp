#ifndef __SYSTEM_MONITOR_HPP__
#define __SYSTEM_MONITOR_HPP__

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <chrono>
#include <cstdio>

class SystemMonitor {
public:
    static void logSystemStatus(const std::string& context) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::cout << "[SYSMON " << std::put_time(std::localtime(&time_t), "%H:%M:%S") 
                  << " | " << context << "] ";
        
        // Memory usage
        logMemoryUsage();
        
        // CPU load average
        logCpuLoad();
        
        // USB device status
        logUsbDevices();
        
        std::cout << std::endl;
    }

private:
    static void logMemoryUsage() {
        std::ifstream meminfo("/proc/meminfo");
        if (meminfo.is_open()) {
            std::string line;
            long total = 0, available = 0;
            
            while (std::getline(meminfo, line)) {
                if (line.find("MemTotal:") == 0) {
                    sscanf(line.c_str(), "MemTotal: %ld kB", &total);
                } else if (line.find("MemAvailable:") == 0) {
                    sscanf(line.c_str(), "MemAvailable: %ld kB", &available);
                    break;
                }
            }
            
            if (total > 0 && available > 0) {
                double usage_percent = 100.0 * (total - available) / total;
                std::cout << "MEM: " << std::fixed << std::setprecision(1) 
                          << usage_percent << "% | ";
            }
            meminfo.close();
        }
    }
    
    static void logCpuLoad() {
        std::ifstream loadavg("/proc/loadavg");
        if (loadavg.is_open()) {
            std::string load1, load5, load15;
            loadavg >> load1 >> load5 >> load15;
            std::cout << "LOAD: " << load1 << " | ";
            loadavg.close();
        }
    }
    
    static void logUsbDevices() {
        // Simple USB device count - could be enhanced
        std::ifstream lsusb_proc("/proc/bus/usb/devices");
        if (lsusb_proc.is_open()) {
            std::string line;
            int device_count = 0;
            while (std::getline(lsusb_proc, line)) {
                if (line.find("T:") == 0) { // Device entry
                    device_count++;
                }
            }
            std::cout << "USB: " << device_count << " devs";
            lsusb_proc.close();
        }
    }
};

#endif // __SYSTEM_MONITOR_HPP__
