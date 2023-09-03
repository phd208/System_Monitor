#include <unistd.h>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "process.h"
#include "linux_parser.h"

using std::string;
using std::to_string;
using std::vector;

Process::Process(int pid) {
    id = pid;
    command = LinuxParser::Command(pid);
    ram = LinuxParser::Ram(pid);
    user = LinuxParser::User(pid);
    uptime = LinuxParser::UpTime(pid);

    long seconds = LinuxParser::UpTime() - uptime;
    long total = LinuxParser::ActiveJiffies(pid);
    if (seconds != 0) {
        cpu = static_cast<float>(total) / static_cast<float>(seconds);
    } else {
        cpu = 0.0f;
    }
}

// Return this process's ID
int Process::Pid() { return id; }

// Return this process's CPU utilization
float Process::CpuUtilization() const { return cpu; }

// Return the command that generated this process
string Process::Command() { return command; }

// Return this process's memory utilization
string Process::Ram() { return ram; }

// Return the user (name) that generated this process
string Process::User() { return user; }

// Return the age of this process (in seconds)
long int Process::UpTime() { return uptime; }

// Overload the "less than" comparison operator for Process objects
bool Process::operator<(Process const& a) const { 
    return CpuUtilization() < a.CpuUtilization(); 
}