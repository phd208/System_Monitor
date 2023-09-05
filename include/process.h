#ifndef PROCESS_H
#define PROCESS_H

#include <string>
using std::string;

// Basic class for Process representation
class Process {
 public:
  Process(int pid);
  int Pid();                               
  string User();                      
  string Command();                   
  float CpuUtilization() const;            
  string Ram();                       
  long int UpTime();                       
  bool operator<(Process const& a) const;  

 private:
    int pid_;
    float cpu_;
};

#endif