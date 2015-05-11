#include <iostream>
#include <sstream>
#include <string>

#include <unistd.h>

#include "logic.h"

using namespace std;

const static Logger &l = Logger::get();

int main(void)
{
    l(LogLevel::notice, "Starting doorlockd");

    try {
        Logic &logic = Logic::get();
        logic.run();
    }
    catch (const char* const &ex) {
        ostringstream str;
        str << "FATAL ERROR: " << ex;
        l(str, LogLevel::error);
    }

    Door::get().lock();
    l(LogLevel::notice, "Doorlockd stopped");
    return 0;
}
