#ifndef EPAPER_H
#define EPAPER_H

#include <string>
#include <array>

#include "logger.h"

class Epaper {
public:

    constexpr static int HEIGHT = 176; // In Pixel
    constexpr static int WIDTH  = 33; // In Byte

    constexpr static int ARRAY_SIZE = HEIGHT * WIDTH;

    static Epaper &get();
    ~Epaper();

    void draw(const std::string &uri);

private:

    Epaper();

    uint8_t _prevImg[ARRAY_SIZE];

    const Logger &_logger;
};

#endif
