#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <cstdlib>
#include <json/json.h>

#include "util.h"
#include "logic.h"

using namespace std;

const string Logic::_lockPagePrefix = LOCKPAGE_PREFIX;
const string Logic::_fifoLocation = FIFO_LOCATION;

Logic &Logic::get()
{
    static Logic l;
    return l;
}

Logic::Logic() :
    _logger(Logger::get()),
    _door(Door::get()),
    _epaper(Epaper::get())
{
    srand(time(NULL));

    _logger(LogLevel::debug, "Creating Fifo file");
    if (access(_fifoLocation.c_str(), F_OK) == 0)
    {
        _logger(LogLevel::warning, "Fifo file aready existing, trying to delete");
        if (unlink(_fifoLocation.c_str()) != 0)
        {
            throw("Unable to delete Fifo file");
        }
    }
    if (mkfifo(_fifoLocation.c_str(), 0660) != 0)
    {
        throw("Unable to create Fifo");
    }

    _fifoHandle = open(_fifoLocation.c_str(), O_RDWR | O_NONBLOCK);
    if (_fifoHandle == -1)
    {
        throw("Unable to open Fifo");
    }

    _createNewToken(false);
}

Logic::~Logic()
{
    if (_fifoHandle != -1)
    {
        close(_fifoHandle);
    }

    _logger(LogLevel::debug, "Removing Fifo file");
    if (unlink(_fifoLocation.c_str()) != 0)
    {
        throw("Unable to delete Fifo file");
    }
}

void Logic::_parseRequest(const string &str)
{
    _logger("Parsing request...");
    Json::Reader reader;
    Json::Value root;

    bool suc = reader.parse(str, root, false);
    if (!suc)
    {
        _logger(LogLevel::error, "Request ist not valid JSON!");
        return;
    }

    string action, user, password, host, token;
    bool authenticated;
    try {
        action = getJsonOrFail<string>(root, "action");
        host = getJsonOrFail<string>(root, "host");
        authenticated = getJsonOrFail<bool>(root, "authenticated");
        string user, password;
        if (authenticated == true)
        {
            user = getJsonOrFail<string>(root, "user");
            password = getJsonOrFail<string>(root, "password");
            token = getJsonOrFail<string>(root, "token");
        }
    }
    catch (...)
    {
        _logger(LogLevel::warning, "Error parsing JSON");
        return;
    }

    printf("Action: %s\nAuthenticated: %d\nHost: %s\n",action.c_str(), authenticated, host.c_str());
    printf("User: %s\nPassword: %s\nToken: %s\n",user.c_str(), password.c_str(), token.c_str());

    if (authenticated == true)
    {
        if (_checkToken(token) == false)
        {
            _logger(LogLevel::error, "User provided invalid token");
            return;
        }

        if (_checkLDAP(user, password) == false)
        {
            _logger(LogLevel::error, "invalid LDAP credentials");
            return;
        }
    }

    if (action == "lock")
    {
        _lock();
    } else if (action == "unlock") {
        _unlock();
    } else {
        _logger(LogLevel::error, "Unknown Action: %s", action.c_str());
    }
}

void Logic::_lock()
{
    if (_state == LOCKED)
    {
        _logger(LogLevel::warning, "Unable to lock: already locked");
        return;
    }
    _door.lock();
    _state = LOCKED;
    _createNewToken(false);
}

void Logic::_unlock()
{
    if (_state == UNLOCKED)
    {
       _logger(LogLevel::warning, "Unable to unlock: already unlocked");
       return;
   }
   _door.unlock();
   _state = UNLOCKED;
   _createNewToken(false);
}

void Logic::run()
{
    struct timeval tv;
    fd_set set;

    for (;;)
    {
        FD_ZERO(&set);
        FD_SET(_fifoHandle, &set);
        tv.tv_sec = _tokenTimeout;
        tv.tv_usec = 0;

        int i = select(_fifoHandle+1, &set, nullptr, nullptr, &tv);
        if (i == 0)
        {
            _createNewToken(true);
            continue;
        } else if (i == -1) {
            throw "Fifo select() failed";
        }

        if (!FD_ISSET(_fifoHandle, &set))
        {
            _logger(LogLevel::warning, "select(): Not my fd");
            continue;
        }

        string payload;
        for (;;)
        {
            constexpr int BUFSIZE = 2;
            char tmp[BUFSIZE];
            i = read(_fifoHandle, tmp, BUFSIZE);
            if (i > 0) {
                payload += string(tmp, i);
            } else {
                if (errno == EWOULDBLOCK)
                {
                    break;
                }
                throw "read() fifo failed";
            }
        }
        _parseRequest(payload);
    }
}

bool Logic::_checkToken(const string &strToken)
{
    try {
        uint64_t token = toUint64(strToken);
        if (token == _curToken || (_prevValid == true && token == _prevToken))
        {
            _logger(LogLevel::info, "Token check successful");
            return true;
        }
    }
    catch (const char* const &ex)
    {
        _logger(LogLevel::error, "Check Token failed for token \"%s\" (expected %s): %s", strToken.c_str(), toHexString(_curToken).c_str(), ex);
    }
    return false;
}

bool Logic::_checkLDAP(const string &user, const string &password)
{
    return true;
}

void Logic::_createNewToken(const bool stillValid)
{
    _prevToken = _curToken;
    _prevValid = stillValid;

    _curToken = (((uint64_t)rand())<<32) | ((uint64_t)rand());

    _epaper.draw(_lockPagePrefix + toHexString(_curToken));

    ostringstream message;
    message << "New Token generated: " << toHexString(_curToken) << " old Token: " << toHexString(_prevToken) << " is " << (_prevValid?"still":"not") << " valid";
    _logger(message, LogLevel::info);
}
