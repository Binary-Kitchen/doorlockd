#ifndef LOGIC_H
#define LOGIC_H

#include <cstdint>
#include <string>
#include <fstream>

#include "config.h"
#include "epaper.h"
#include "door.h"
#include "logger.h"

class Logic
{
public:

    static Logic &get();
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
    void createNewToken(const bool stillValid);

private:

    Logic();

    Response _lock();
    Response _unlock();

    bool _checkToken(const std::string &token);
    Response _checkLDAP(const std::string &user, const std::string &password);
    bool _checkIP(const std::string &ip);


    const Logger &_logger;
    Door &_door;
    Epaper &_epaper;

    using Token = uint64_t;

    Token _curToken = { 0x0000000000000000 };
    bool _prevValid = { false };
    Token _prevToken = { 0x0000000000000000 };


    const static std::string _lockPagePrefix;
    const static std::string _bindDN;
    const static std::string _ldapServer;
    const static std::string _allowedIpPrefix;

    static constexpr int _tokenTimeout = TOKEN_TIMEOUT;

    enum {LOCKED, UNLOCKED} _state = { LOCKED };
};

#endif
