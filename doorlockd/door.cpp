#include "config.h"
#include "door.h"

Door::Door(const std::string &serDev,
           unsigned int baudrate) :
    _baudrate(baudrate),
    _port(_ioService, serDev),
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

bool Door::readByte(char &byte, std::chrono::milliseconds timeout)
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

            if (recvBuf == 'U') {
                _logger(LogLevel::notice, "Someone pushed the unlock button");
            }
            if (recvBuf == 'L') {
                _logger(LogLevel::notice, "Someone pushed the lock button");
                _logger(LogLevel::notice, "Locking...");
                lock();
                goto out;
            }
            // TODO EMERGENCY DOOR BUTTON

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
    system(PRE_LOCK_SCRIPT);

    if (_state == State::Locked) {
        _stateMutex.unlock();
        _logger(LogLevel::info, "Door already closed");
        goto out;
    }

    _state = State::Locked;
    _stateMutex.unlock();
    _heartbeatCondition.notify_one();
    _heartbeatThread.join();

    _logger(LogLevel::info, "Door closed");

out:
    _logger(LogLevel::notice, "Executing Post Lock Script");
    system(POST_LOCK_SCRIPT);
}

void Door::unlock()
{
    _stateMutex.lock();
    _schnapper = true;

    _logger(LogLevel::notice, "Executing Pre Unlock Script");
    system(PRE_UNLOCK_SCRIPT);

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
                writeCMD('u');

                if (_schnapper) {
                    _schnapper = false;
                    writeCMD('s');
                }
            }

            _heartbeatCondition.wait_for(lock, Milliseconds(400));
        }
        writeCMD('l');
    });

    _logger(LogLevel::info, "Door opened");

    out:
    _logger(LogLevel::notice, "Executing Post Unlock Script");
    system(POST_UNLOCK_SCRIPT);
}

bool Door::writeCMD(char c)
{
    std::lock_guard<std::mutex> l(_serialMutex);

    _port.write_some(boost::asio::buffer(&c, sizeof(c)));
    char response;
    if (readByte(response, Milliseconds(100)))
    {
        if (c != response) {
            _logger(LogLevel::error, "Sent command '%c' but gor response '%c'", c, response);
            return false;
        }
        return true;
    }
    _logger(LogLevel::error, "Sent Serial command, but got no response!");
    return false;
}

