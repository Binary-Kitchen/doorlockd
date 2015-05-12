#include <stdio.h>
#include <errno.h>

#include <cstdlib>
#include <json/json.h>

#define LDAP_DEPRECATED 1
#include <ldap.h>

#include "util.h"
#include "logic.h"

using namespace std;

const string Logic::_lockPagePrefix = LOCKPAGE_PREFIX;

const string Logic::_ldapServer = LDAP_SERVER;
const string Logic::_bindDN = BINDDN;
const string Logic::_allowedIpPrefix = ALLOWEDIPPREFIX;

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
    createNewToken(false);
}

Logic::~Logic()
{
}

Logic::Response Logic::parseRequest(const string &str)
{
    _logger("Parsing request...");
    Json::Reader reader;
    Json::Value root;
    Response retval = Fail;
    string action, user, password, ip, token;
    bool authenticate;

    bool suc = reader.parse(str, root, false);
    if (!suc)
    {
        _logger(LogLevel::error, "Request ist not valid JSON!");
        retval = NotJson;
        goto out;
    }

    try {
        action = getJsonOrFail<string>(root, "action");
        ip = getJsonOrFail<string>(root, "ip");
        authenticate = getJsonOrFail<bool>(root, "authenticate");
        if (authenticate == true)
        {
            user = getJsonOrFail<string>(root, "user");
            password = getJsonOrFail<string>(root, "password");
            token = getJsonOrFail<string>(root, "token");
        }
    }
    catch (...)
    {
        _logger(LogLevel::warning, "Error parsing JSON");
        retval = JsonError;
        goto out;
    }

    printf("Action: %s\nAuthenticate: %d\nIP: %s\n",action.c_str(), authenticate, ip.c_str());
    printf("User: %s\nPassword: XXXXXXXXXX\nToken: %s\n",user.c_str(), token.c_str());

    if (authenticate == true)
    {
        if (_checkToken(token) == false)
        {
            _logger(LogLevel::error, "User provided invalid token");
            retval = InvalidToken;
            goto out;
        }

        retval = _checkLDAP(user,password);
        if (retval != Success)
        {
            _logger(LogLevel::error, "Ldap error");
            goto out;
        }
    } else {
        if (_checkIP(ip) == false)
        {
            _logger(LogLevel::error, "IP check for non-authentication failed");
            retval = InvalidIP;
            goto out;
        }
    }

    if (action == "lock")
    {
        retval = _lock();
    } else if (action == "unlock") {
        retval = _unlock();
    } else {
        _logger(LogLevel::error, "Unknown Action: %s", action.c_str());
        retval = UnknownAction;
    }

out:
    return retval;
}

Logic::Response Logic::_lock()
{
    if (_state == LOCKED)
    {
        _logger(LogLevel::warning, "Unable to lock: already closed");
        return AlreadyLocked;
    }
 
    _door.lock();
    _state = LOCKED;
    createNewToken(false);

    return Success;
}

Logic::Response Logic::_unlock()
{
    if (_state == UNLOCKED)
    {
       _logger(LogLevel::warning, "Unable to unlock: already unlocked");
       return AlreadyUnlocked;
   }

   _door.unlock();
   _state = UNLOCKED;
   createNewToken(false);

   return Success;
}

bool Logic::_checkIP(const string &ip)
{
    return true;
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

Logic::Response Logic::_checkLDAP(const string &user, const string &password)
{
    constexpr int BUFFERSIZE = 1024;
    char buffer[BUFFERSIZE];
    Response retval = Fail;
    int rc = -1;
    LDAP* ld = nullptr;
    unsigned long version = LDAP_VERSION3;

    _logger(LogLevel::notice, "Trying to authenticate as user \"%s\"", user.c_str());
    snprintf(buffer, BUFFERSIZE, _bindDN.c_str(), user.c_str());

    rc = ldap_initialize(&ld, _ldapServer.c_str());
    if(rc != LDAP_SUCCESS)
    {   
        _logger(LogLevel::error, "LDAP initialize error: %s", ldap_err2string(rc));
        retval = LDAPInit;
        goto out2;
    }

    rc = ldap_set_option(ld,
                         LDAP_OPT_PROTOCOL_VERSION,
                         (void*)&version);
    if (rc != LDAP_SUCCESS)
    {
        _logger(LogLevel::error, "LDAP set version failed");
        retval = LDAPInit;
        goto out;
    }

    rc = ldap_simple_bind_s(ld, buffer, password.c_str());
    if (rc != LDAP_SUCCESS)
    {   
        _logger(LogLevel::error, "Credential check for user \"%s\" failed: %s", user.c_str(), ldap_err2string(rc));
        retval = InvalidCredentials;
        goto out;
    }

    _logger(LogLevel::notice, "user \"%s\" successfully authenticated", user.c_str());
    retval = Success;

out:
    ldap_unbind(ld);
    ld = nullptr;
out2:
    return retval;
}

void Logic::createNewToken(const bool stillValid)
{
    _prevToken = _curToken;
    _prevValid = stillValid;

    _curToken = (((uint64_t)rand())<<32) | ((uint64_t)rand());

    _epaper.draw(_lockPagePrefix + toHexString(_curToken));

    ostringstream message;
    message << "New Token generated: " << toHexString(_curToken) << " old Token: " << toHexString(_prevToken) << " is " << (_prevValid?"still":"not") << " valid";
    _logger(message, LogLevel::info);
}
