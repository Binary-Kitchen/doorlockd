#include <cstring>
#include <stdexcept>
#include <thread>

#include "wave.h"

std::mutex Wave::_playMutex = {};

Wave::Wave(const std::string &filename):
    _filename(filename),
    _command("aplay " + _filename)
{
}

Wave::~Wave()
{
}

void Wave::play() const
{
    std::lock_guard<std::mutex> l(_playMutex);
    system(_command.c_str());
}

void Wave::playAsync() const
{
    std::thread([this] () {
        play();
    }).detach();
}
