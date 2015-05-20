#include <iostream>

#include <wiringPi.h>
#include <unistd.h>

#include "logger.h"
#include "door.h"

using namespace std;

Door::Door() :
    _l(Logger::get())
{
    _l(LogLevel::info, "Initializing Raspberry Pi GPIOs");
    wiringPiSetup();
    pinMode(_LOCKPIN, OUTPUT);
    pinMode(_SCHNAPPER, OUTPUT);
    lock();
}

Door::~Door()
{
    lock();
}

Door &Door::get()
{
    static Door d;
    return d;
}

void Door::lock()
{
    digitalWrite(_SCHNAPPER, HIGH);
    _l(LogLevel::info, "Door closed");
    if (_open == true)
    {
        _open = false;
        _heartbeat.join();
    }

    system("wget -O /dev/null --timeout 3 \"http://homer.binary.kitchen:8080/set?color=000000\" > /dev/null 2>&1");
}

void Door::unlock()
{
    _schnapper = true;

    if (_open == true) 
    {
        return;
    }

    _open = true;
    _heartbeat = std::thread([this] () {
        auto beat = [] () {
            digitalWrite(_LOCKPIN, HIGH);
            usleep(10000);
            digitalWrite(_LOCKPIN, LOW);
            usleep(10000);
        };

        digitalWrite(_SCHNAPPER, HIGH);
        while (_open) {
            if (_schnapper == true)
            {
                for (int i = 0; i < 32 ; i++)
                {
                    digitalWrite(_SCHNAPPER, LOW);
                    beat();
                    digitalWrite(_SCHNAPPER, HIGH);
                    beat();
                }
                digitalWrite(_SCHNAPPER, HIGH);
                _schnapper = false;
            }
            beat();
        }
    });

    _l(LogLevel::info, "Door opened");
}
