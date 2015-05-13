#ifndef LOGIC_H
#define LOGIC_H

#include <cstdint>
#include <string>
#include <thread>
#include <condition_variable>
#include <mutex>

#include "config.h"
#include "epaper.h"
#include "door.h"
#include "logger.h"

class Logic
{
public:

    Logic(const std::chrono::seconds tokenTimeout);
    ~Logic();

    enum Response {
        Success = 0, // Request successful
        Fail, // General non-specified error
        AlreadyUnlocked, // Authentication successful, but door is already unlocked
        AlreadyLocked, // Authentication successful, but door is already locked
        NotJson, // Request is not a valid JSON object
        JsonError, // Request is valid JSON, but does not contain necessary material
        InvalidToken, // Request contains invalid token
        InvalidCredentials, // Invalid LDAP credentials
        InvalidIP, // IP check failure
        UnknownAction, // Unknown action
        LDAPInit, // Ldap initialization failed
    };

    Response parseRequest(const std::string &str);

private:

    Response _lock();
    Response _unlock();

    bool _checkToken(const std::string &token);
    Response _checkLDAP(const std::string &user, const std::string &password);
    bool _checkIP(const std::string &ip);

    void _createNewToken(const bool stillValid);

    const Logger &_logger;
    Door &_door;
    Epaper &_epaper;

    using Token = uint64_t;

    Token _curToken = { 0x0000000000000000 };
    bool _prevValid = { false };
    Token _prevToken = { 0x0000000000000000 };

    const std::chrono::seconds _tokenTimeout;

    const static std::string _lockPagePrefix;
    const static std::string _bindDN;
    const static std::string _ldapServer;
    const static std::string _allowedIpPrefix;

    std::thread _tokenUpdater = {};
    std::condition_variable _c = {};
    std::mutex _mutex = {};
    bool _run = true;

    enum {LOCKED, UNLOCKED} _state = { LOCKED };
};

#endif
