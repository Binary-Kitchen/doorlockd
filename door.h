#ifndef DOOR_H
#define DOOR_H

#include <string>

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
};

#endif
