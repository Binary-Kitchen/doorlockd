#include <cstring>
#include <fstream>
#include <stdexcept>
#include <thread>

#include <sndfile.h>

#include "wave.h"

Wave::Wave(int bits,
           int channels,
           int rate,
           Raw data) :
    _format(new ao_sample_format),
    _data(data)
{
    _format->bits = bits;
    _format->rate = rate;
    _format->channels = channels;
    _format->byte_format = AO_FMT_LITTLE;
    _format->matrix = nullptr;

    int default_driver;
    default_driver = ao_default_driver_id();
    _device = ao_open_live(default_driver,
                           _format.get(),
                           nullptr);
}

Wave::~Wave()
{
    if (_device != nullptr)
        ao_close(_device);
}

Wave Wave::fromFile(const std::string &filename)
{
    SF_INFO sfinfo;
    SNDFILE *file = sf_open(filename.c_str(), SFM_READ, &sfinfo);
    if (file == nullptr)
        throw std::runtime_error("Unable to open soundfile " + filename);

    size_t rawSize = sfinfo.channels * sfinfo.frames * sizeof(short);
    Raw data;
    data.resize(rawSize);

    sf_read_raw(file, &data.front(), data.size());
    sf_close(file);

    int bits;
    switch (sfinfo.format & SF_FORMAT_SUBMASK) {
        case SF_FORMAT_PCM_16:
            bits = 16;
            break;
        case SF_FORMAT_PCM_24:
            bits = 24;
            break;
        case SF_FORMAT_PCM_32:
            bits = 32;
            break;
        case SF_FORMAT_PCM_S8:
            bits = 8;
            break;
        case SF_FORMAT_PCM_U8:
            bits = 8;
            break;
        default:
            bits = 16;
            break;
    }

    return Wave(bits,
                sfinfo.channels,
                sfinfo.samplerate,
                data);
}

void Wave::play() const
{
    std::lock_guard<std::mutex> l(_playMutex);
    if (_device == nullptr) {
        return;
    }

    ao_play(_device,
            (char*)&_data.front(),
            _data.size());
}

void Wave::playAsync() const
{
    std::thread([this] () {
        play();
    }).detach();
}
