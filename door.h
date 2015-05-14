#ifndef DOOR_H
#define DOOR_H

#include <string>
#include <thread>

#include "logger.h"

class Door {

public:
    static Door &get();

    ~Door();

    void lock();
    void unlock();

private:

    Door();

    const Logger &_l;

    bool _open = { false };
    std::thread _heartbeat = { };

    bool _schnapper = { false };

    static constexpr int _LOCKPIN = 10;
    static constexpr int _SCHNAPPER = 7;
};

#endif
