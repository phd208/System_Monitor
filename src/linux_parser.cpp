#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>

#include "linux_parser.h"

using std::stof;
using std::stol;
using std::string;
using std::to_string;
using std::vector;

namespace {
  long AccumulateJiffies(std::vector<LinuxParser::CPUStates> states) {
    auto jiffies = LinuxParser::CpuUtilization();
    long jiffiesSum;
    for (LinuxParser::CPUStates state : states) {
      jiffiesSum += stol(jiffies[state]);
    }
    return jiffiesSum;
  }

  string GetLine(const string & path, const string & searchKey = "") {
    string line = " ";
    std::ifstream stream(path);
    if (stream.is_open()) {
      if (searchKey.empty()) {
        // stream has just one line; no search needed
        std::getline(stream, line);
      } else {
        // stream has multiple lines; searchKey provided
        while (std::getline(stream, line)) {
          if (line.find(searchKey) != std::string::npos) {
            break;
          }
        }
      }
    }
    return line;
  }

}

// An example of how to read data from the filesystem
string LinuxParser::OperatingSystem() {
  string key = "PRETTY_NAME";
  string os;
  string line = GetLine(kOSPath, key);
  
  std::replace(line.begin(), line.end(), ' ', '_');
  std::replace(line.begin(), line.end(), '=', ' ');
  std::replace(line.begin(), line.end(), '"', ' ');
  std::istringstream linestream(line);
  linestream >> key >> os;
  std::replace(os.begin(), os.end(), '_', ' ');

  return os;
}

// Read data from the filesystem
string LinuxParser::Kernel() {
  string os, kernel, version;
  std::istringstream linestream(GetLine(kProcDirectory + kVersionFilename));
  linestream >> os >> version >> kernel;
  return kernel;
}

vector<int> LinuxParser::Pids() {
  std::vector<int> pids;
  for (const auto &entry : std::filesystem::directory_iterator(kProcDirectory)) {
    if (std::filesystem::is_directory(entry)) {
      string filename = entry.path().filename().string();
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  } 
  return pids;
}       

// Read and return the system memory utilization
float LinuxParser::MemoryUtilization() { 
  string path = kProcDirectory + kMeminfoFilename;
  string key;
  float total, free;

  std::istringstream linestreamTotal(GetLine(path, "MemTotal:"));
  linestreamTotal >> key >> total;

  std::istringstream linestreamFree(GetLine(path, "MemFree:"));
  linestreamFree >> key >> free;
  
  return (total - free) / total; 
}

// Read and return the system uptime
long LinuxParser::UpTime() { 
  string uptime;
  std::istringstream linestream(GetLine(kProcDirectory + kUptimeFilename));
  linestream >> uptime;
  return stol(uptime);
}

// Read and return the number of jiffies for the system
long LinuxParser::Jiffies() { 
  return LinuxParser::ActiveJiffies() + LinuxParser::IdleJiffies(); 
}

// Read and return the number of active jiffies for a PID
long LinuxParser::ActiveJiffies(int pid) {
  std::istringstream linestream(GetLine(kProcDirectory + std::to_string(pid) + kStatFilename));
  std::vector<std::string> values(std::istream_iterator<std::string>{linestream}, std::istream_iterator<std::string>());

  if (values.size() < 17) {
    return 0; // Ensure we have enough values
  }

  long utime = std::all_of(values[13].begin(), values[13].end(), isdigit) ? stol(values[13]) : 0;
  long stime = std::all_of(values[14].begin(), values[14].end(), isdigit) ? stol(values[14]) : 0;
  long cutime = std::all_of(values[15].begin(), values[15].end(), isdigit) ? stol(values[15]) : 0;
  long cstime = std::all_of(values[16].begin(), values[16].end(), isdigit) ? stol(values[16]) : 0;
  
  return (utime + stime + cutime + cstime) / sysconf(_SC_CLK_TCK); 
}

// Read and return the number of active jiffies for the system
long LinuxParser::ActiveJiffies() { 
  std::vector<CPUStates> activeStates = { CPUStates::kUser_, CPUStates::kNice_, CPUStates::kSystem_, CPUStates::kIRQ_, CPUStates::kSoftIRQ_, CPUStates::kSteal_};
  return AccumulateJiffies(activeStates);
}

// Read and return the number of idle jiffies for the system
long LinuxParser::IdleJiffies() { 
  std::vector<CPUStates> idleStates = { CPUStates::kIdle_, CPUStates::kIOwait_};
  return AccumulateJiffies(idleStates);
}

// Read and return CPU utilization
vector<string> LinuxParser::CpuUtilization() { 
  std::vector<string> jiffies;
  std::istringstream linestream(GetLine(kProcDirectory + kStatFilename));
  // discard the 'cpu' prefix
  linestream.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
  string value;
  while (linestream >> value) {
    jiffies.push_back(value);
  }
  return jiffies;
}

// Read and return the total number of processes
int LinuxParser::TotalProcesses() { 
  string key;
  int processes;
  std::istringstream linestream(GetLine(kProcDirectory + kStatFilename, "processes"));
  linestream >> key >> processes;
  return processes; 
}

// Read and return the number of running processes
int LinuxParser::RunningProcesses() { 
  string key;
  int processes;
  std::istringstream linestream(GetLine(kProcDirectory + kStatFilename, "procs_running"));
  linestream >> key >> processes;
  return processes; 
}

// Read and return the command associated with a process
string LinuxParser::Command(int pid) { 
  return GetLine(kProcDirectory + to_string(pid) + kCmdlineFilename);
}

// Read and return the memory used by a process
string LinuxParser::Ram(int pid) { 
 
  string path = kProcDirectory + to_string(pid) + kStatusFilename;
  long ram;
  
  string key, line, mem;
  std::ifstream stream(path);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      linestream >> key;
      if (key == "VmSize:") {
        linestream >> ram;
        ram /= 1024;
        mem = to_string(ram);
      }
    }
  }

  return mem;
}

// Read and return the user ID associated with a process
string LinuxParser::Uid(int pid) { 
  string key = "Uid:";
  string path = kProcDirectory + to_string(pid) + kStatusFilename;
  string uid;
  std::istringstream linestream(GetLine(path, key));
  linestream >> key >> uid;
  return uid; 
}

// Read and return the user associated with a process
string LinuxParser::User(int pid) { 
  string name, x, id;
  string line = GetLine(kPasswordPath, Uid(pid));
  std::replace(line.begin(), line.end(), ':', ' ');
  std::istringstream linestream(line);
  linestream >> name >> x >> id;
  return name;
}

// Read and return the uptime of a process
long LinuxParser::UpTime(int pid) { 
  string value;
  long startTime = 0;

  std::istringstream linestream(GetLine(kProcDirectory + to_string(pid) + kStatFilename));

  // Skipping the first 21 values
  for (int i = 0; i < 21 && (linestream >> value); i++) {};

  // Reading the 22nd value
  if (linestream >> value) {
    try {
      startTime = stol(value) / sysconf(_SC_CLK_TCK);
    } catch (const std::invalid_argument&) {
      std::cerr << "Error: Invalid data format in /proc file." << std::endl;
    } catch (const std::out_of_range&) {
      std::cerr << "Error: Number out of range in /proc file." << std::endl;
    } catch (...) {
      std:: cerr << "Error: Unkown exception while parsing /proc file." << std::endl;
    }
  }

  return LinuxParser::UpTime() - startTime;
  
}