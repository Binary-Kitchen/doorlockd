#ifndef DOOR_H
#define DOOR_H

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>

#include "logger.h"
#include "doormessage.h"

class Door final
{
public:
    using DoorCallback = std::function<void(Doormessage)>;
    enum class State {Unlocked, Locked};

    Door(const std::string &serDev,
         unsigned int baudrate,
         const std::string &logfile_scripts);
    ~Door();

    State state() const;
    void setDoorCallback(DoorCallback doorCallback);

    void lock();
    void unlock();

private:

    using Milliseconds = std::chrono::milliseconds;

    const unsigned int _baudrate;

    // To prevent concurrent writes
    std::mutex _serialMutex = { };

    boost::asio::io_service _ioService = { };
    boost::asio::serial_port _port;

    const std::string _logfile_scripts;

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

    DoorCallback _doorCallback = { };

    Logger &_logger;

    // Writed command to AVR board
    bool _writeCMD(char c);
    // Receives one byte and returns true or returns false on timeout
    bool _readByte(char &byte, Milliseconds timeout);

    void _exec_and_log(const std::string &filename);
};

#endif
