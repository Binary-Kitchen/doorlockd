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

    void run();

private:

    Logic();
    void _parseRequest(const std::string &str);

    void _lock();
    void _unlock();

    bool _checkToken(const std::string &token);
    bool _checkLDAP(const std::string &user, const std::string &password);

    void _createNewToken(const bool stillValid);

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

    int _fifoHandle = {-1};

    static constexpr int _tokenTimeout = TOKEN_TIMEOUT;

    enum {LOCKED, UNLOCKED} _state = { LOCKED };
};

#endif
