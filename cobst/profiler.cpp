#include "profiler.h"

// These are static variables for the PPROFILER class
std::unordered_map<std::string,double> PPROFILER::ProfilerRoutineTimes;
std::unordered_map<std::string,LONGLONG> PPROFILER::ProfilerRoutineEntries;
std::vector<double> PPROFILER::AccumulatedTimes;

std::ostream &operator<<(std::ostream &os, PPROFILER const &m)
{
    return os << m.Dump();
}
