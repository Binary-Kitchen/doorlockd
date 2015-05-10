#include <cstring>
#include <unistd.h>
#include <fcntl.h>

#include "logger.h"

extern "C" {
#include "epaper/bsp.h"
#include "epaper/Display_Controller.h"
}

#include "epaper.h"

#include "template.h"

using namespace std;

Epaper::Epaper() :
    _logger(Logger::get())
{
    memset(_prevImg, 0xFF, ARRAY_SIZE);
    bsp_init();
}

Epaper::~Epaper()
{
}

Epaper &Epaper::get()
{
    static Epaper e;
    return e;
}

void Epaper::draw(const string &uri)
{
    unsigned char buffer[ARRAY_SIZE];
    snprintf((char*)buffer, ARRAY_SIZE, "qrencode -l M -d 100 -s 5 \"%s\" -o /tmp/qr.png", uri.c_str());
    system((char*)buffer);

    FILE* f = popen("composite -geometry +90+0 /tmp/qr.png /tmp/template.png -colorspace gray -depth 1 gray:-", "r");
    int i = fread(buffer, ARRAY_SIZE, 1, f);
    if (i != 1)
    {
        _logger(LogLevel::error, "Image format error");
        pclose(f);
        return;
    }
    pclose(f);

    epd_DisplayImg(EPDType_270, buffer, _prevImg);
    memcpy(_prevImg, buffer, ARRAY_SIZE);
}
