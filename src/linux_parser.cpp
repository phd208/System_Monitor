#include <dirent.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <numeric>

#include "linux_parser.h"

using std::stof;
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
      jiffiesSum += std::stol(jiffies[state]);
    }
    return jiffiesSum;
  }

  std::istringstream CreateStringStream(const std::string & path) {
    string line;
    std::ifstream stream(path);
    if (stream.is_open()) {
      std::getline(stream, line);
    }
    return std::istringstream(line);
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

// BONUS: Update this to use std::filesystem
// vector<int> LinuxParser::Pids() {
//   vector<int> pids;
//   DIR* directory = opendir(kProcDirectory.c_str());
//   struct dirent* file;
//   while ((file = readdir(directory)) != nullptr) {
//     // Is this a directory?
//     if (file->d_type == DT_DIR) {
//       // Is every character of the name a digit?
//       string filename(file->d_name);
//       if (std::all_of(filename.begin(), filename.end(), isdigit)) {
//         int pid = stoi(filename);
//         pids.push_back(pid);
//       }
//     }
//   }
//   closedir(directory);

//   for (auto p : pids) {
//     std::cout << p << "\n";
//   }

//   return pids;
// }


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


// TODO: Read and return the system memory utilization
float LinuxParser::MemoryUtilization() { 
  float total = -1, available = -1;
  
  string line;
  std::ifstream stream(kProcDirectory + kMeminfoFilename);
  if (stream.is_open()) {
    while (std::getline(stream,line)) {
      std::istringstream linestream(line);
      string key;
      linestream >> key;
      if (key == "MemTotal:") {
        linestream >> total;
      } else if (key == "MemAvailable:") {
        linestream >> available;
      } 

      if (total != -1 && available != -1) {
        break;
      }
    }
  }
  
  return (total - available) / total; 

}

// TODO: Read and return the system uptime
long LinuxParser::UpTime() { 
  // string line, uptime;
  // std::ifstream stream(kProcDirectory + kUptimeFilename);
  // if (stream.is_open()) {
  //   std::getline(stream, line);
  //   std::istringstream uptimeStream(line);
  //   uptimeStream >> uptime;
  // }
  // return std::stol(uptime); 

  string uptime;
  std::istringstream linestream = CreateStringStream(kProcDirectory + kUptimeFilename);
  linestream >> uptime;
  return std::stol(uptime);
}

// TODO: Read and return the number of jiffies for the system
long LinuxParser::Jiffies() { 
  return LinuxParser::ActiveJiffies() + LinuxParser::IdleJiffies(); 
}

// Read and return the number of active jiffies for a PID
long LinuxParser::ActiveJiffies(int pid) {
  // string line;
  // std::ifstream stream(kProcDirectory + std::to_string(pid) + kStatFilename);
  // std::getline(stream, line);
  // std::istringstream linestream(line);

  std::istringstream linestream = CreateStringStream(kProcDirectory + std::to_string(pid) + kStatFilename);

  std::vector<std::string> values(std::istream_iterator<std::string>{linestream}, std::istream_iterator<std::string>());

  if (values.size() < 17) {
    return 0; // Ensure we have enough values
  }

  long utime = std::all_of(values[13].begin(), values[13].end(), isdigit) ? std::stol(values[13]) : 0;
  long stime = std::all_of(values[14].begin(), values[14].end(), isdigit) ? std::stol(values[14]) : 0;
  long cutime = std::all_of(values[15].begin(), values[15].end(), isdigit) ? std::stol(values[15]) : 0;
  long cstime = std::all_of(values[16].begin(), values[16].end(), isdigit) ? std::stol(values[16]) : 0;
  
  return (utime + stime + cutime + cstime) / sysconf(_SC_CLK_TCK); 
}

// TODO: Read and return the number of active jiffies for the system
long LinuxParser::ActiveJiffies() { 
  std::vector<CPUStates> activeStates = { CPUStates::kUser_, CPUStates::kNice_, CPUStates::kSystem_, CPUStates::kIRQ_, CPUStates::kSoftIRQ_, CPUStates::kSteal_};
  return AccumulateJiffies(activeStates);
}

// TODO: Read and return the number of idle jiffies for the system
long LinuxParser::IdleJiffies() { 
  std::vector<CPUStates> idleStates = { CPUStates::kIdle_, CPUStates::kIOwait_};
  return AccumulateJiffies(idleStates);
}


// TODO: Read and return CPU utilization
vector<string> LinuxParser::CpuUtilization() { 
  // std::vector<string> jiffies;
  // string line;
  // std::ifstream stream(kProcDirectory + kStatFilename);
  // if (stream.is_open() && std::getline(stream,line)) {
  //   std::istringstream linestream(line);

  //   // discard the 'cpu' prefix
  //   linestream.ignore(std::numeric_limits<std::streamsize>::max(), ' ');

  //   string value;
  //   while (linestream >> value) {
  //     jiffies.push_back(value);
  //   }
  // }
  // return jiffies; 

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

// TODO: Read and return the total number of processes
int LinuxParser::TotalProcesses() { return 0; }

// TODO: Read and return the number of running processes
int LinuxParser::RunningProcesses() { return 0; }

// TODO: Read and return the command associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Command(int pid[[maybe_unused]]) { return string(); }

// TODO: Read and return the memory used by a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Ram(int pid[[maybe_unused]]) { return string(); }

// TODO: Read and return the user ID associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Uid(int pid[[maybe_unused]]) { return string(); }

// TODO: Read and return the user associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::User(int pid[[maybe_unused]]) { return string(); }

// TODO: Read and return the uptime of a process
// REMOVE: [[maybe_unused]] once you define the function
long LinuxParser::UpTime(int pid[[maybe_unused]]) { return 0; }