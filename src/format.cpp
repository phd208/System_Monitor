#include <string>
#include <iomanip>
#include <sstream>

#include "format.h"

using std::string;
using std::setw;
using std::setfill;

// INPUT: Long int measuring seconds
// OUTPUT: HH:MM:SS
string Format::ElapsedTime(long seconds) { 
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    seconds %= 60;

    std::ostringstream timeStream;
    timeStream  <<  setw(2) << setfill('0') << hours << ":"
                <<  setw(2) << setfill('0') << minutes << ":"
                <<  setw(2) << setfill('0') << seconds;
    
    return timeStream.str(); 
}