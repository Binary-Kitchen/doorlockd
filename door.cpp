#include <iostream>
#include <wiringPi.h>

#include "logger.h"
#include "door.h"

using namespace std;


constexpr int LOCKPIN = 10;

Door::Door() : _l(Logger::get())
{
    _l(LogLevel::info, "Initializing Raspberry Pi GPIOs");
    wiringPiSetup();
    pinMode(LOCKPIN, OUTPUT);

    lock();
}

Door::~Door()
{

}

Door &Door::get()
{
    static Door d;
    return d;
}

void Door::lock()
{
    _l(LogLevel::info, "Door closed");
    digitalWrite(LOCKPIN, HIGH);
}

void Door::unlock()
{
    _l(LogLevel::info, "Door opened");
    digitalWrite(LOCKPIN, LOW);
}
