#ifndef WAVE_H
#define WAVE_H

#include <string>
#include <vector>
#include <memory>

#include <ao/ao.h>

class Wave {
public:

    using Raw = std::vector<char>;

    static Wave fromFile(const std::string &filename);
    Wave(int bits,
         int channels,
         int rate,
         Raw data);
    Wave(const Wave &rhs);
    ~Wave();

    Wave &operator=(const Wave &rhs);

    void play() const;

private:

    std::unique_ptr<ao_sample_format> _format;
    ao_device* _device = { nullptr };

    const Raw _data;
};

#endif
