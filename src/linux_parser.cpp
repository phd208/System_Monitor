#include <dirent.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <numeric>
#include <algorithm>

#include "linux_parser.h"

using std::stof;
using std::stol;
using std::string;
using std::to_string;
using std::vector;

// const std::string kProcDirectory{"/proc/"};
// const std::string kCmdlineFilename{"/cmdline"};
// const std::string kCpuinfoFilename{"/cpuinfo"};
// const std::string kStatusFilename{"/status"};
// const std::string kStatFilename{"/stat"};
// const std::string kUptimeFilename{"/uptime"};
// const std::string kMeminfoFilename{"/meminfo"};
// const std::string kVersionFilename{"/version"};
// const std::string kOSPath{"/etc/os-release"};
// const std::string kPasswordPath{"/etc/passwd"};

namespace {
  long AccumulateJiffies(std::vector<LinuxParser::CPUStates> states) {
    auto jiffies = LinuxParser::CpuUtilization();
    long jiffiesSum;
    for (LinuxParser::CPUStates state : states) {
      jiffiesSum += stol(jiffies[state]);
    }
    return jiffiesSum;
  }

  std::istringstream CreateStringStream(const string & path) {
    string line;
    std::ifstream stream(path);
    if (stream.is_open()) {
      std::getline(stream, line);
    }
    return std::istringstream(line);
  }

  string FindKeyInStream(const string & path, const string & searchKey, string valueType) {
    string line, key, value;
    std::ifstream stream(path);
    if (stream.is_open()) {
      while (std::getline(stream, line)) {
        std::istringstream linestream(line);
        linestream >> key;
        if (key == searchKey) {
          if (valueType == "string") {
              linestream >> value;
          } else if (valueType == "int") {
              int iValue;
              linestream >> iValue;
              value = to_string(iValue); 
          } else if (valueType == "long") {
              long lValue;
              linestream >> lValue;
              if (searchKey == "VmSize:")
                lValue /= 1000;
              value = to_string(lValue);
          } else {
              float fValue;
              linestream >> fValue;
              value = to_string(fValue);
          }
          break;
        }

      }
    }
    return value;
  }


}

// DONE: An example of how to read data from the filesystem
string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

// DONE: An example of how to read data from the filesystem
string LinuxParser::Kernel() {
  string os, kernel, version;

  // string line;
  // std::ifstream stream(kProcDirectory + kVersionFilename);
  // if (stream.is_open()) {
  //   std::getline(stream, line);
  //   std::istringstream linestream(line);
  //   linestream >> os >> version >> kernel;
  // }

  std::istringstream linestream = CreateStringStream(kProcDirectory + kVersionFilename);
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
  float total = stof(FindKeyInStream(kProcDirectory + kMeminfoFilename, "MemTotal:", "float"));
  float available = stof(FindKeyInStream(kProcDirectory + kMeminfoFilename, "MemAvailable:", "float"));
  return (total - available) / total; 

}

// Read and return the system uptime
long LinuxParser::UpTime() { 
  string uptime;
  std::istringstream linestream = CreateStringStream(kProcDirectory + kUptimeFilename);
  linestream >> uptime;
  return stol(uptime);
}

// Read and return the number of jiffies for the system
long LinuxParser::Jiffies() { 
  return LinuxParser::ActiveJiffies() + LinuxParser::IdleJiffies(); 
}

// Read and return the number of active jiffies for a PID
long LinuxParser::ActiveJiffies(int pid) {
  std::istringstream linestream = CreateStringStream(kProcDirectory + std::to_string(pid) + kStatFilename);
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
  std::istringstream linestream = CreateStringStream(kProcDirectory + kStatFilename);
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
  return stoi(FindKeyInStream(kProcDirectory + kStatFilename, "processes", "int")); 
}

// Read and return the number of running processes
int LinuxParser::RunningProcesses() { 
  return stoi(FindKeyInStream(kProcDirectory + kStatFilename, "procs_running", "int"));; 
}

// Read and return the command associated with a process
string LinuxParser::Command(int pid) { 
  string command;
  std::ifstream stream(kProcDirectory + to_string(pid) + kCmdlineFilename);
  if (stream.is_open()) {
    std::getline(stream, command);
  }
  return command; 
}

// Read and return the memory used by a process
string LinuxParser::Ram(int pid) { 
 return FindKeyInStream(kProcDirectory + to_string(pid) + kStatusFilename, "VmSize:", "long");
}

// Read and return the user ID associated with a process
string LinuxParser::Uid(int pid) { 
  return FindKeyInStream(kProcDirectory + to_string(pid) + kStatusFilename, "Uid:", "string"); 
}

// Read and return the user associated with a process
string LinuxParser::User(int pid) { 
  string uid = Uid(pid);
  string id, x, temp, line, name;
  std::ifstream stream(kPasswordPath);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      linestream >> temp >> x >> id;
      if (id == uid) {
        name = temp;
        break;
      }
    }
  }
  return name;
}

// Read and return the uptime of a process
long LinuxParser::UpTime(int pid) { 
  string value;
  vector<string> values;
  long startTime;
  std::istringstream linestream = CreateStringStream(kProcDirectory + std::to_string(pid) + kStatFilename);
  while (linestream >> value) {
    values.push_back(value);
  }

  try {
    startTime = stol(values[21]) / sysconf(_SC_CLK_TCK);
  } catch (...) {
    startTime = 0;
  }

  return startTime; 
  
}