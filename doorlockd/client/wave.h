#ifndef WAVE_H
#define WAVE_H

#include <mutex>
#include <string>

class Wave {
public:
    Wave(const std::string &filename);
    Wave(const Wave &rhs);
    ~Wave();

    Wave &operator=(const Wave &rhs);

    void play() const;
    void playAsync() const;

private:
    static std::mutex _playMutex;

    const std::string _filename;
    const std::string _command;
};

#endif
