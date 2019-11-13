#pragma once

#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <sstream>
#include <algorithm>

#define PROFILING

class PPROFILER
{
private:
    static std::unordered_map<std::string,double> ProfilerRoutineTimes;
    static std::unordered_map<std::string,long long> ProfilerRoutineEntries;
    static std::vector<double> AccumulatedTimes;
    std::string routine;
    std::chrono::steady_clock::time_point time_in;
    std::chrono::steady_clock::time_point time_out;
    std::chrono::steady_clock::time_point lap_timer;
    std::chrono::steady_clock::time_point oh_1;
    std::chrono::steady_clock::time_point oh_2;
    std::chrono::steady_clock::time_point oh_3;
    std::chrono::steady_clock::time_point oh_4;
    double lap_time_overhead;
    bool frozen;

public:
    PPROFILER(char const * const function_name)
    {
        oh_1 = std::chrono::steady_clock::now();
        routine = function_name;
        AccumulatedTimes.push_back(0);
        frozen = false;
        lap_time_overhead = 0;
        oh_2 = lap_timer = time_in = std::chrono::steady_clock::now();
    }

    PPROFILER(const char * class_name, char const * const function_name)
    {
        oh_1 = std::chrono::steady_clock::now();
        routine = class_name;
        routine += "::";
        routine += function_name;
        AccumulatedTimes.push_back(0);
        frozen = false;
        lap_time_overhead = 0;
        oh_2 = lap_timer = time_in = std::chrono::steady_clock::now();
    }

    ~PPROFILER()
    {
        if(!frozen) {
            Freeze();
            size_t level = AccumulatedTimes.size();
            if(level == 0) {
                std::cerr << Dump();
            }
        }
    }

    void LapTime()
    {
        std::chrono::steady_clock::time_point lt1 = std::chrono::steady_clock::now();
        std::chrono::duration<double> lap_time = std::chrono::duration_cast<std::chrono::duration<double>>(lt1-lap_timer);

        std::stringstream ss;
        ss.flags(std::ios::right | std::ios::fixed);
        ss.width(12); // sign, 999 decimal 123456
        ss.precision(6);
        ss << lap_time.count();
        std::cerr << ss.str() << std::endl;

        lap_timer = std::chrono::steady_clock::now();
        std::chrono::duration<double> overhead = std::chrono::duration_cast<std::chrono::duration<double>>(lap_timer - lt1);
        lap_time_overhead += overhead.count();
    }

    void Freeze()
    {
        oh_3 = time_out = std::chrono::steady_clock::now();
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(time_out - time_in);
        ProfilerRoutineTimes[routine] += time_span.count() - AccumulatedTimes.back();
        ProfilerRoutineEntries[routine] += 1;
        AccumulatedTimes.pop_back();
        size_t level = AccumulatedTimes.size();
        std::chrono::duration<double> overhead1 = std::chrono::duration_cast<std::chrono::duration<double>>(oh_2 - oh_1);
        double overhead = overhead1.count();
        oh_4 = std::chrono::steady_clock::now();
        std::chrono::duration<double> overhead2 = std::chrono::duration_cast<std::chrono::duration<double>>(oh_4 - oh_3);
        overhead += overhead2.count();
        overhead += lap_time_overhead;

        if(level > 0) {
            AccumulatedTimes[level-1] += time_span.count() - overhead;
        }
        frozen = true;
    }

    friend std::ostream &operator<<(std::ostream &os, PPROFILER const &m);

    std::string Dump() const
    {
        std::stringstream ss;

        size_t max_width = 0;
        std::map<std::string,double> smap;
        for(std::unordered_map<std::string,double>::const_iterator it=ProfilerRoutineTimes.begin(); it!=ProfilerRoutineTimes.end(); it++) {
            max_width = std::max(max_width,it->first.length());
            smap[it->first] = it->second;
        }
        for(std::map<std::string,double>::const_iterator it=smap.begin(); it!=smap.end(); it++) {
            ss.flags (std::ios::left);
            ss.width(max_width);
            ss << it->first;

            ss << " ";
            ss.flags(std::ios::right | std::ios::fixed);
            ss.width(12); // sign, 999 decimal 123456
            ss.precision(6);
            ss << it->second;

            ss << " ";
            ss.width(12);
            ss << ProfilerRoutineEntries[it->first];

            ss << std::endl;
        }
        return ss.str();
    }

};

#ifdef PROFILING
#define PROFILER PPROFILER Profiler(__func__);
#define CPROFILER PPROFILER Profiler(typeid(*this).name(),__func__);
#define LAPTIME Profiler.LapTime();
#else
#define PROFILER
#define CPROFILER
#define LAPTIME
#endif
