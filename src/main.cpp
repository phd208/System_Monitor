#include "ncurses_display.h"
#include "system.h"
#include "format.h"
#include "linux_parser.h"

#include <iostream>
#include <curses.h>
#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <typeinfo>

using std::string;
using std::to_string;

int main() {
  System system;
  NCursesDisplay::Display(system);
}


