#include <iostream>

#include <wiringPi.h>
#include <unistd.h>

#include "config.h"
#include "logger.h"
#include "door.h"

using namespace std;

Door::Door() :
    _l(Logger::get())
{
    _l(LogLevel::info, "Initializing Raspberry Pi GPIOs");
    wiringPiSetup();
    pinMode(_HEARTBEATPIN, OUTPUT);
    pinMode(_SCHNAPPERPIN, OUTPUT);
    pinMode(_LOCKPIN, INPUT);
    pullUpDnControl(_LOCKPIN, PUD_UP);
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
    _l(LogLevel::notice, "Executing Pre Lock Script");
    system(PRE_LOCK_SCRIPT);

    digitalWrite(_SCHNAPPERPIN, HIGH);
    _l(LogLevel::info, "Door closed");

    if (_state == State::Unlocked)
    {
        // Stop the Heartbeat Thread
        _state = State::Locked;
        _heartbeat.join();
    }

    _l(LogLevel::notice, "Executing Post Lock Script");
    system(POST_LOCK_SCRIPT);
}

void Door::unlock()
{
    _l(LogLevel::notice, "Executing Pre Unlock Script");
    system(PRE_UNLOCK_SCRIPT);

    // In any case, klacker the schnapper
    _schnapper = true;

    // If heartbeat is already running, return
    if (_state == State::Unlocked)
    {
        return;
    }

    // If not, first set state to unlocked
    _state = State::Unlocked;

    // Start the Heartbeat Thread
    _heartbeat = std::thread([this] () {

        // One "beat" is one complete cycle of the heartbeat clock
        auto beat = [this] () {
            digitalWrite(_HEARTBEATPIN, HIGH);
            usleep(10000);
            digitalWrite(_HEARTBEATPIN, LOW);
            usleep(10000);

            if (!digitalRead(_LOCKPIN)) {
                std::thread([this] () {
                    lock();
                }).detach();
            }
        };

        // The default of the Schnapperpin: always high
        digitalWrite(_SCHNAPPERPIN, HIGH);

        // Heartbeat while the state is unlocked
        while (_state == State::Unlocked) {

            // In case of schnapper, send 0x55 resp. 0xaa to the schnapperpin
            if (_schnapper == true)
            {
                for (int i = 0; i < 32 ; i++)
                {
                    // Set '0'
                    digitalWrite(_SCHNAPPERPIN, LOW);
                    // cycle and send
                    beat();
                    // Set '1'
                    digitalWrite(_SCHNAPPERPIN, HIGH);
                    // cycle and send
                    beat();
                }

                // Reset schnapperpin
                digitalWrite(_SCHNAPPERPIN, HIGH);
                // and deactivate schnapper for the next round
                _schnapper = false;
            }

            // Heartbeat
            beat();
        }
    });

    _l(LogLevel::info, "Door opened");

    _l(LogLevel::notice, "Executing Post Unlock Script");
    system(POST_UNLOCK_SCRIPT);
}
