#!/usr/bin/env python3
"""
Analyze ZED camera timing logs to identify performance patterns
"""

import re
import sys
from datetime import datetime
from statistics import mean, stdev

class TimingAnalyzer:
    def __init__(self):
        self.frame_data = []
        self.delay_events = []
        self.grab_failures = []
        self.slow_grabs = []
        self.system_events = []
        
    def parse_log_file(self, filename):
        print(f"Analyzing log file: {filename}")
        
        with open(filename, 'r') as f:
            for line_num, line in enumerate(f, 1):
                line = line.strip()
                
                # Parse frame timing
                if '] Frame ' in line and 'interval=' in line:
                    self._parse_frame_line(line, line_num)
                
                # Parse long delays
                elif '*** LONG DELAY ***' in line:
                    self._parse_long_delay(line, line_num)
                
                # Parse grab failures
                elif '*** GRAB FAILED ***' in line:
                    self._parse_grab_failure(line, line_num)
                
                # Parse slow grabs
                elif '*** SLOW GRAB ***' in line:
                    self._parse_slow_grab(line, line_num)
                
                # Parse system monitor events
                elif '[SYSMON' in line:
                    self._parse_system_event(line, line_num)
    
    def _parse_frame_line(self, line, line_num):
        """Parse normal frame timing line"""
        pattern = r'\[([^\]]+)\] Frame (\d+): interval=(\d+)ms, grab=(\d+)ms'
        match = re.search(pattern, line)
        if match:
            camera, frame_num, interval, grab_time = match.groups()
            self.frame_data.append({
                'line': line_num,
                'camera': camera,
                'frame': int(frame_num),
                'interval': int(interval),
                'grab_time': int(grab_time)
            })
    
    def _parse_long_delay(self, line, line_num):
        """Parse long delay events"""
        pattern = r'Frame (\d+): interval=(\d+)ms, grab=(\d+)ms'
        match = re.search(pattern, line)
        if match:
            frame_num, interval, grab_time = match.groups()
            self.delay_events.append({
                'line': line_num,
                'frame': int(frame_num),
                'interval': int(interval),
                'grab_time': int(grab_time)
            })
    
    def _parse_grab_failure(self, line, line_num):
        """Parse grab failure events"""
        pattern = r'Status: ([^,]+), grab_duration=(\d+)ms'
        match = re.search(pattern, line)
        if match:
            status, duration = match.groups()
            self.grab_failures.append({
                'line': line_num,
                'status': status,
                'duration': int(duration)
            })
    
    def _parse_slow_grab(self, line, line_num):
        """Parse slow grab events"""
        pattern = r'Duration: (\d+)ms'
        match = re.search(pattern, line)
        if match:
            duration = match.groups()[0]
            self.slow_grabs.append({
                'line': line_num,
                'duration': int(duration)
            })
    
    def _parse_system_event(self, line, line_num):
        """Parse system monitoring events"""
        # Extract system metrics
        mem_match = re.search(r'MEM: ([\d.]+)%', line)
        load_match = re.search(r'LOAD: ([\d.]+)', line)
        usb_match = re.search(r'USB: (\d+) devs', line)
        context_match = re.search(r'\| ([^\]]+)\]', line)
        
        event = {'line': line_num}
        if mem_match:
            event['mem_usage'] = float(mem_match.group(1))
        if load_match:
            event['cpu_load'] = float(load_match.group(1))
        if usb_match:
            event['usb_devices'] = int(usb_match.group(1))
        if context_match:
            event['context'] = context_match.group(1)
        
        self.system_events.append(event)
    
    def generate_report(self):
        """Generate comprehensive analysis report"""
        print("\n" + "="*60)
        print("ZED CAMERA TIMING ANALYSIS REPORT")
        print("="*60)
        
        # Basic statistics
        if self.frame_data:
            intervals = [f['interval'] for f in self.frame_data]
            grab_times = [f['grab_time'] for f in self.frame_data]
            
            print(f"\n📊 BASIC STATISTICS:")
            print(f"  Total frames analyzed: {len(self.frame_data)}")
            print(f"  Frame interval - Mean: {mean(intervals):.1f}ms, StdDev: {stdev(intervals):.1f}ms")
            print(f"  Grab time - Mean: {mean(grab_times):.1f}ms, StdDev: {stdev(grab_times):.1f}ms")
            print(f"  Expected interval: ~33ms (30fps), Expected grab: ~16-33ms")
        
        # Long delay analysis
        if self.delay_events:
            print(f"\n⚠️  LONG DELAY EVENTS: {len(self.delay_events)}")
            delay_intervals = [d['interval'] for d in self.delay_events]
            delay_grabs = [d['grab_time'] for d in self.delay_events]
            
            print(f"  Delay interval - Max: {max(delay_intervals)}ms, Mean: {mean(delay_intervals):.1f}ms")
            print(f"  Grab time during delays - Max: {max(delay_grabs)}ms, Mean: {mean(delay_grabs):.1f}ms")
            
            # Check if delays are in grab vs elsewhere
            grab_caused = sum(1 for d in self.delay_events if d['grab_time'] > 100)
            print(f"  Delays caused by slow grab(): {grab_caused}/{len(self.delay_events)}")
            
            # Show worst cases
            worst_delays = sorted(self.delay_events, key=lambda x: x['interval'], reverse=True)[:5]
            print(f"  🔥 Worst delays:")
            for d in worst_delays:
                print(f"    Frame {d['frame']}: interval={d['interval']}ms, grab={d['grab_time']}ms (line {d['line']})")
        
        # Grab failure analysis
        if self.grab_failures:
            print(f"\n❌ GRAB FAILURES: {len(self.grab_failures)}")
            failure_durations = [f['duration'] for f in self.grab_failures]
            print(f"  Failure duration - Max: {max(failure_durations)}ms, Mean: {mean(failure_durations):.1f}ms")
        
        # Slow grab analysis
        if self.slow_grabs:
            print(f"\n🐌 SLOW GRABS: {len(self.slow_grabs)}")
            slow_durations = [g['duration'] for g in self.slow_grabs]
            print(f"  Duration - Max: {max(slow_durations)}ms, Mean: {mean(slow_durations):.1f}ms")
        
        # System resource correlation
        if self.system_events:
            print(f"\n🖥️  SYSTEM RESOURCE EVENTS: {len(self.system_events)}")
            
            # Analyze system state during problems
            problem_events = [e for e in self.system_events if 'LONG_DELAY' in e.get('context', '') or 'SLOW_GRAB' in e.get('context', '')]
            baseline_events = [e for e in self.system_events if 'BASELINE' in e.get('context', '')]
            
            if problem_events and baseline_events:
                print(f"  Problem events: {len(problem_events)}")
                print(f"  Baseline events: {len(baseline_events)}")
                
                # Compare system metrics
                for metric in ['mem_usage', 'cpu_load', 'usb_devices']:
                    problem_values = [e[metric] for e in problem_events if metric in e]
                    baseline_values = [e[metric] for e in baseline_events if metric in e]
                    
                    if problem_values and baseline_values:
                        problem_avg = mean(problem_values)
                        baseline_avg = mean(baseline_values)
                        diff_pct = ((problem_avg - baseline_avg) / baseline_avg) * 100
                        
                        print(f"  {metric}: Problem={problem_avg:.1f}, Baseline={baseline_avg:.1f}, Diff={diff_pct:+.1f}%")
        
        print(f"\n💡 RECOMMENDATIONS:")
        
        if self.delay_events:
            grab_caused = sum(1 for d in self.delay_events if d['grab_time'] > 100)
            if grab_caused > len(self.delay_events) * 0.7:
                print(f"  • Most delays are caused by slow grab() calls - investigate camera/USB issues")
            else:
                print(f"  • Delays occur between grab() calls - investigate CPU scheduling/threading")
        
        if self.grab_failures:
            print(f"  • {len(self.grab_failures)} grab failures detected - check camera connection")
        
        if len(self.delay_events) > len(self.frame_data) * 0.1:
            print(f"  • High delay event rate ({len(self.delay_events)}/{len(self.frame_data)}) - system resource issue likely")
        
        print(f"  • Monitor USB bandwidth usage during peak delays")
        print(f"  • Consider reducing resolution/fps if delays persist")
        print(f"  • Check for thermal throttling on long recordings")

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 analyze_timing.py <log_file>")
        sys.exit(1)
    
    analyzer = TimingAnalyzer()
    analyzer.parse_log_file(sys.argv[1])
    analyzer.generate_report()

if __name__ == "__main__":
    main()

