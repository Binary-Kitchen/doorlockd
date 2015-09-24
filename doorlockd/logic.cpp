#include <chrono>
#include <functional>

#include <cstdlib>
#include <json/json.h>

#define LDAP_DEPRECATED 1
#include <ldap.h>

#include <errno.h>

#include "util.h"
#include "logic.h"

using namespace std;

Logic::Logic(const chrono::seconds tokenTimeout,
             const string &ldapUri,
             const string &bindDN,
             const string &webPrefix,
             const string &serDev,
             const unsigned int baudrate,
             condition_variable &onClientUpdate) :
    _logger(Logger::get()),
    _door(serDev, baudrate),
    _tokenTimeout(tokenTimeout),
    _onClientUpdate(onClientUpdate),
    _ldapUri(ldapUri),
    _bindDN(bindDN),
    _webPrefix(webPrefix)
{
    srand(time(NULL));
    _createNewToken(false);

    _door.setDoorCallback(std::bind(&Logic::_doorCallback,
                                    this,
                                    std::placeholders::_1));

    _tokenUpdater = thread([this] () {
        while (_run)
        {
            unique_lock<mutex> l(_mutex);
            _tokenCondition.wait_for(l, _tokenTimeout);
            if (_run == false)
            {
                break;
            } else {
                _createNewToken(true);
            }
        }
    });
}

Logic::~Logic()
{
    _run = false;
    _tokenCondition.notify_one();
    _tokenUpdater.join();
}

Response Logic::parseRequest(const Json::Value &root)
{
    unique_lock<mutex> l(_mutex);

    _logger(LogLevel::info, "Incoming request...");
    Response response;
    string command, user, password, ip, token;

    try {
        command = getJsonOrFail<string>(root, "command");
        ip = getJsonOrFail<string>(root, "ip");
        user = getJsonOrFail<string>(root, "user");
        password = getJsonOrFail<string>(root, "password");
        token = getJsonOrFail<string>(root, "token");
    }
    catch (...)
    {
        _logger(LogLevel::warning, "Error parsing JSON");
        response.code = Response::Code::JsonError;
        response.message = "Error parsing JSON";
        goto out;
    }

    _logger("  User   : " + user, LogLevel::notice);
    _logger("  IP     : " + ip, LogLevel::notice);
    _logger("  Token  : " + token, LogLevel::notice);

    if (_checkToken(token) == false)
    {
        _logger(LogLevel::error, "User provided invalid token");
        response.code = Response::Code::InvalidToken;
        response.message = "User provided invalid token";
        goto out;
    }

    response = _checkLDAP(user,password);
    if (!response)
    {
        _logger(LogLevel::error, "Ldap error");
        goto out;
    }

    if (command == "lock")
    {
        response = _lock();
    } else if (command == "unlock") {
        response = _unlock();
    } else {
        response.code = Response::Code::UnknownCommand;
        response.message = "Unknown Command: " + command;
        _logger(response.message, LogLevel::error);
    }

out:
    return response;
}

Response Logic::_lock()
{
    Response response;
    if (_door.state() == Door::State::Locked)
    {
        response.code = Response::Code::AlreadyLocked;
        response.message = "Unable to lock: already closed";
        _logger(response.message, LogLevel::warning);
    } else {
        _door.lock();
        _createNewToken(false);

        response.code = Response::Code::Success;
    }

    return response;
}

Response Logic::_unlock()
{
    Response response;

    const auto oldState = _door.state();
    _door.unlock();
    _createNewToken(false);

    if (oldState == Door::State::Unlocked)
    {
        response.code = Response::Code::AlreadyUnlocked;
        response.message = "Unable to unlock: already unlocked";
        _logger(response.message, LogLevel::warning);
    } else {
        response.code = Response::Code::Success;
    }

    return response;
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

Response Logic::_checkLDAP(const string &user, const string &password)
{
    constexpr int BUFFERSIZE = 1024;
    char buffer[BUFFERSIZE];
    Response retval;

    int rc = -1;
    LDAP* ld = nullptr;
    unsigned long version = LDAP_VERSION3;

    _logger(LogLevel::notice, "Trying to authenticate as user \"%s\"", user.c_str());
    snprintf(buffer, BUFFERSIZE, _bindDN.c_str(), user.c_str());

    rc = ldap_initialize(&ld, _ldapUri.c_str());
    if(rc != LDAP_SUCCESS)
    {
        retval.message = (string)"LDAP initialize error: "
                       + ldap_err2string(rc);
        retval.code = Response::Code::LDAPInit;
        _logger(retval.message, LogLevel::error);
        goto out2;
    }

    rc = ldap_set_option(ld,
                         LDAP_OPT_PROTOCOL_VERSION,
                         (void*)&version);
    if (rc != LDAP_SUCCESS)
    {
        retval.code = Response::Code::LDAPInit;
        retval.message = "LDAP set version failed";
        _logger(retval.message, LogLevel::error);
        goto out;
    }

    rc = ldap_simple_bind_s(ld, buffer, password.c_str());
    if (rc != LDAP_SUCCESS)
    {
        retval = Response::Code::InvalidCredentials;
        retval.message = "Credential check for user \"" + user
                       + "\" failed: " + ldap_err2string(rc);
        _logger(retval.message, LogLevel::error);
        goto out;
    }

    _logger(LogLevel::notice, "user \"%s\" successfully authenticated", user.c_str());
    retval = Response::Code::Success;

out:
    ldap_unbind(ld);
    ld = nullptr;
out2:
    return retval;
}

void Logic::_createNewToken(const bool stillValid)
{
    // Todo Mutex einführen

    _prevToken = _curToken;
    _prevValid = stillValid;

    _curToken = (((uint64_t)rand())<<32) | ((uint64_t)rand());

    ostringstream message;
    message << "New Token generated: " << toHexString(_curToken) << " old Token: " << toHexString(_prevToken) << " is " << (_prevValid?"still":"not") << " valid";
    _logger(message, LogLevel::info);

    _onClientUpdate.notify_all();
}

std::string Logic::getClientMessage()
{
    std::lock_guard<std::mutex> l(_mutex);
    Json::Value message;
    Json::StyledWriter writer;

    message["token"] = _webPrefix + toHexString(_curToken);
    message["unlockButton"] = _doormessage.isUnlockButton;
    message["lockButton"] = _doormessage.isLockButton;
    message["emergencyUnlock"] = _doormessage.isEmergencyUnlock;

    // Reset doormessage
    _doormessage = Door::Doormessage();

    return writer.write(message);
}

void Logic::_doorCallback(Door::Doormessage doormessage)
{
    std::lock_guard<std::mutex> l(_mutex);
    _doormessage = doormessage;
}
