#ifndef EPAPER_H
#define EPAPER_H

#include <string>
#include <array>

#include "logger.h"

/*
 * The "Epaper" class.
 *
 * Wrapper for the epaper third Party library.
 *
 * Exists as singleton, as only one display may exist.
 */
class Epaper {
public:

    static Epaper &get();
    ~Epaper();

    // This function will draw template.png to the display and
    // convert the uri to a QR-Code and paste it on top of template.png
    // using imagemagick
    void draw(const std::string &uri);

private:

    constexpr static int _HEIGHT = 176; // In Pixel
    constexpr static int _WIDTH  = 33; // In Byte
    constexpr static int _ARRAY_SIZE = _HEIGHT * _WIDTH;

    Epaper();

    // The old image is needed when updating the Epaper display
    // It informs the display which pixels have to be flipped
    uint8_t _prevImg[_ARRAY_SIZE];

    const Logger &_logger;
};

#endif
