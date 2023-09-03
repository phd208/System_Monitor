#include "processor.h"
#include "linux_parser.h"

// Return the aggregate CPU utilization
float Processor::Utilization() { 
    long prevTotal = total_;
    long prevIdle = idle_;

    Update();

    float totalDelta = float(total_) - float(prevTotal);
    float idleDelta = float(idle_) - float(prevIdle);

    return (totalDelta - idleDelta) / totalDelta; 
}

void Processor::Update() {
    total_ = LinuxParser::Jiffies();
    active_ = LinuxParser::ActiveJiffies();
    idle_ = LinuxParser::IdleJiffies();
}