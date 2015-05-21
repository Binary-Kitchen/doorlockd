#ifndef DOOR_H
#define DOOR_H

#include <string>
#include <thread>

#include "logger.h"

/*
 * The Door class.
 *
 * This class exists as a singleton as only one door and hence one object may exist.
 * Available via the get() method.
 *
 * This class is responsible for opening and closing the door by sending
 * the heartbeat to the AVR board via Raspberry Pi GPIOs
 *
 * Whenever the unlock() is called, this class also sends a 0x55 resp. 0xaa
 * to the AVR in order to klacker the schnapper.
 */
class Door {

public:

    // Returns the singleton
    static Door &get();
    ~Door();

    enum class State {Locked, Unlocked};

    // Lock the door
    void lock();
    // Unlock the door
    void unlock();

private:

    Door();

    // used for logging
    const Logger &_l;

    // Indicates the internal state: Door is open or locked
    State _state = { Door::State::Locked };

    // A Heartbeat thread is started when the door is unlocked
    std::thread _heartbeat = { };

    // Read by the Heartbeat thread if it should klacker the schnapper or not
    bool _schnapper = { false };

    // WiringPi GPIO Pins
    static constexpr int _HEARTBEATPIN = 10;
    static constexpr int _SCHNAPPERPIN = 7;
};

#endif
