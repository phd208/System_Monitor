#include "ncurses_display.h"
#include "system.h"
#include "format.h"

#include <iostream>
#include <curses.h>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using std::string;
using std::to_string;

int main() {
  System system;
 // NCursesDisplay::Display(system);
 std::cout << "OS: " + system.OperatingSystem() << std::endl;
 std::cout << "Kernel: " + system.Kernel() << std::endl;
 std::cout << "CPU util: " + to_string(system.Cpu().Utilization()) << std::endl;
 std::cout << "Memory: " + to_string(system.MemoryUtilization()) << std::endl;
 std::cout << "Total Processes: " + to_string(system.TotalProcesses()) << std::endl;
 std::cout << "Running Processes: " + to_string(system.RunningProcesses()) << std::endl;
 std::cout << "Up Time: " + to_string(system.UpTime()) << std::endl;
}


