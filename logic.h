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

    int parseRequest(const std::string &str);
    void createNewToken(const bool stillValid);

private:

    Logic();

    void _lock();
    void _unlock();

    bool _checkToken(const std::string &token);
    bool _checkLDAP(const std::string &user, const std::string &password);
    bool _checkIP(const std::string &ip);


    const Logger &_logger;
    Door &_door;
    Epaper &_epaper;

    using Token = uint64_t;

    Token _curToken;
    bool _prevValid = { false };
    Token _prevToken;


    const static std::string _lockPagePrefix;
    const static std::string _fifoLocation;
    const static std::string _bindDN;
    const static std::string _ldapServer;
    const static std::string _allowedIpPrefix;

    static constexpr int _tokenTimeout = TOKEN_TIMEOUT;

    enum {LOCKED, UNLOCKED} _state = { LOCKED };
};

#endif
