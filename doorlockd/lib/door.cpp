#include "config.h"
#include "door.h"

#include "../../doorcmds.h"

Door::Door(const std::string &serDev,
           unsigned int baudrate,
           const std::string &logfile_scripts) :
    _baudrate(baudrate),
    _port(_ioService, serDev),
    _logfile_scripts(logfile_scripts),
    _logger(Logger::get())
{
    // Configure serial port
    _port.set_option(boost::asio::serial_port_base::baud_rate(baudrate));
    _port.set_option(boost::asio::serial_port_base::character_size(8));
    _port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
    _port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    _port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

    _asyncRead();

    _ioThread = std::thread([this] () {
        _ioService.run();
    });

    // TODO Ping device
}

Door::~Door()
{
    lock();

    _ioService.stop();
    _ioService.reset();

    _port.cancel();
    _port.close();

    _ioThread.join();
}

bool Door::_readByte(char &byte, std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(_receiveLock);
    _receivedCondition.wait_for(lock, timeout);
    if (_byteReady) {
        byte = recvBuf;
        _byteReady = false;
        return true;
    }
    return false;
}

void Door::_asyncRead()
{
    _port.async_read_some(
        boost::asio::buffer(&recvBuf, sizeof(recvBuf)),
        [this] (const boost::system::error_code &ec, size_t bytes_transferred) {
            if (ec) {
                // Operation canceled occurs on system shutdown
                // So we return without invoking an additional asyncRead()
                if (ec == boost::system::errc::operation_canceled)
                    return;

                _logger(LogLevel::error, "Serialport error: %s", ec.message().c_str());
                goto out;
            }

            if (bytes_transferred != 1) {
                _logger(LogLevel::error, "Fatal serial error");
                goto out;
            }

            if (recvBuf == DOOR_BUTTON_UNLOCK) {
                // In case that someone pushed the unlock button - just log it.
                // No further actions required
                _logger(LogLevel::notice, "Someone pushed the unlock button");
                if (_doorCallback) {
                    _doorCallback(Doormessage(true, false, false));
                }
                goto out;
            } else if (recvBuf == DOOR_BUTTON_LOCK) {
                _logger(LogLevel::notice, "Someone pushed the lock button");
                _logger(LogLevel::notice, "Locking...");
                lock();
                if (_doorCallback) {
                    _doorCallback(Doormessage(false, true, false));
                }
                goto out;
            } else if (recvBuf == DOOR_EMERGENCY_UNLOCK) {
                _logger(LogLevel::warning, "Someone did an emergency unlock!");
                _exec_and_log(EMERGENCY_UNLOCK_SCRIPT);
                if (_doorCallback) {
                    _doorCallback(Doormessage(false, false, true));
                }
                goto out;
            }

            _byteReady = true;
            _receivedCondition.notify_one();

        out:
            _asyncRead();
        });
}

Door::State Door::state() const
{
    return _state;
}

void Door::lock()
{
    _stateMutex.lock();


    _logger(LogLevel::notice, "Executing Pre Lock Script");
    _exec_and_log(PRE_LOCK_SCRIPT);

    if (_state == State::Locked) {
        _stateMutex.unlock();
        _logger(LogLevel::info, "Door already closed");
        goto out;
    }

    _state = State::Locked;
    _stateMutex.unlock();
    _heartbeatCondition.notify_one();
    _heartbeatThread.join();

    _logger(LogLevel::notice , "Door closed");

out:
    _logger(LogLevel::notice, "Executing Post Lock Script");
    _exec_and_log(POST_LOCK_SCRIPT);
}

void Door::unlock()
{
    _stateMutex.lock();
    _schnapper = true;

    _logger(LogLevel::notice, "Executing Pre Unlock Script");
    _exec_and_log(PRE_UNLOCK_SCRIPT);

    if(_state == State::Unlocked) {
        _stateMutex.unlock();
        _logger(LogLevel::info, "Door already opened");
        goto out;
    }

    _state = State::Unlocked;
    _stateMutex.unlock();

    _heartbeatThread = std::thread([this] () {
        std::unique_lock<std::mutex> lock(_heartbeatMutex);

        while (_state == State::Unlocked) {
            if (_state == State::Unlocked) {
                _writeCMD(DOOR_CMD_UNLOCK);

                if (_schnapper) {
                    _schnapper = false;
                    _writeCMD(DOOR_CMD_SCHNAPER);
                }
            }

            _heartbeatCondition.wait_for(lock, Milliseconds(400));
        }
        _writeCMD(DOOR_CMD_LOCK);
    });

    _logger(LogLevel::notice, "Door opened");

    out:
    _logger(LogLevel::notice, "Executing Post Unlock Script");
    _exec_and_log(POST_UNLOCK_SCRIPT);
}

bool Door::_writeCMD(char c)
{
    std::lock_guard<std::mutex> l(_serialMutex);

    _port.write_some(boost::asio::buffer(&c, sizeof(c)));
    char response;
    if (_readByte(response, Milliseconds(100)))
    {
        if (c != response) {
            _logger(LogLevel::error, "Sent command '%c' but got response '%c'", c, response);
            return false;
        }
        return true;
    }
    _logger(LogLevel::error, "Sent Serial command, but got no response!");
    return false;
}

void Door::setDoorCallback(DoorCallback doorCallback)
{
    _doorCallback = doorCallback;
}

void Door::_exec_and_log(const std::string &filename)
{
    const std::string cmd = filename + " &";
    ::system(cmd.c_str());
}
