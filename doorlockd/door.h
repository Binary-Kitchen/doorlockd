#ifndef DOOR_H
#define DOOR_H

#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>

#include "logger.h"

class Door final
{
public:

    Door(const std::string &serDev,
         unsigned int baudrate);
    ~Door();

    enum class State {Unlocked, Locked};

    State state() const;

    void lock();
    void unlock();

private:

    using Milliseconds = std::chrono::milliseconds;

    const unsigned int _baudrate;

    // To prevent concurrent writes
    std::mutex _serialMutex = { };

    boost::asio::io_service _ioService = { };
    boost::asio::serial_port _port;

    std::mutex _stateMutex = { };
    volatile State _state = { State::Locked };

    std::thread _heartbeatThread = { };
    std::mutex _heartbeatMutex = { };
    std::condition_variable _heartbeatCondition = { };

    std::thread _ioThread = { };
    void _asyncRead();

    volatile bool _schnapper = { false };

    // Indicates if recvBuf contains a valid response from AVR Board
    volatile bool _byteReady = { false };
    // Actual response
    char recvBuf = { };

    std::condition_variable _receivedCondition = { };
    std::mutex _receiveLock = { };

    const Logger &_logger;

    // Writed command to AVR board
    bool writeCMD(char c);
    // Receives one byte and returns true or returns false on timeout
    bool readByte(char &byte, Milliseconds timeout);
};

#endif
