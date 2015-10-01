#include <errno.h>
#define LDAP_DEPRECATED 1
#include <ldap.h>

#include "logic.h"
#include "util.h"

Logic::Logic(const std::chrono::seconds tokenTimeout,
             const std::string &ldapUri,
             const std::string &bindDN,
             const std::string &webPrefix,
             const std::string &serDev,
             const unsigned int baudrate,
             std::condition_variable &onClientUpdate) :
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

    _tokenUpdater = std::thread([this] () {
        while (_run)
        {
            std::unique_lock<std::mutex> l(_mutex);
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

Response Logic::processDoor(const DoorCommand &doorCommand)
{
    Response response;

    switch (doorCommand) {
        case DoorCommand::Lock:
            response = _lock();
            break;
        case DoorCommand::Unlock:
            response = _unlock();
            break;
        default:
            response.code = Response::Code::UnknownCommand;
            response.message = "Unknown DoorCommand";
            break;
    }

    return response;
}

Response Logic::request(const Request &request)
{
    std::unique_lock<std::mutex> l(_mutex);
    Response response;

    DoorCommand doorCommand;

    switch (request.command) {
        case Request::Command::Lock:
            doorCommand = DoorCommand::Lock;
            break;
        case Request::Command::Unlock:
            doorCommand = DoorCommand::Unlock;
            break;
        default:
            response.code = Response::Code::UnknownCommand;
            response.message = "Unknown Command";
            goto out;
    }

    response = _checkToken(request.token);
    if (!response) {
        goto out;
    }
    _logger(LogLevel::info, "   -> Token check successful");

    response = _checkLDAP(request.user, request.password);
    if (!response) {
        goto out;
    }
    _logger(LogLevel::info, "   -> LDAP check successful");

    response = processDoor(doorCommand);
    _logger(LogLevel::info, "   -> Door Command successful");

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
    } else {
        _createNewToken(false);

        response.code = Response::Code::Success;
        response.message = "Success";
    }

    _door.lock();

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
        _logger(response.message, LogLevel::info);
    } else {
        response.code = Response::Code::Success;
        response.message = "Success";
    }

    return response;
}

Response Logic::_checkToken(std::string token) const
{
    std::transform(token.begin(),
                   token.end(),
                   token.begin(),
                   ::toupper);

    if (token == _curToken)
        return Response(Response::Code::Success);

    if (_prevValid == true && token == _prevToken)
        return Response(Response::Code::Success);

    _logger("Check Token failed: got \"" + token
            + "\", expected \"" + _curToken +"\"",
            LogLevel::error);

    return Response(Response::InvalidToken,
                    "User provided invalid token");
}

Response Logic::_checkLDAP(const std::string &user,
                           const std::string &password)
{
    constexpr int BUFFERSIZE = 1024;
    char buffer[BUFFERSIZE];
    Response retval;

    int rc = -1;
    LDAP* ld = nullptr;
    unsigned long version = LDAP_VERSION3;

    _logger(LogLevel::info, "      Trying to authenticate as user \"%s\"", user.c_str());
    snprintf(buffer, BUFFERSIZE, _bindDN.c_str(), user.c_str());

    rc = ldap_initialize(&ld, _ldapUri.c_str());
    if(rc != LDAP_SUCCESS)
    {
        retval.message = (std::string)"LDAP initialize error: "
                + ldap_err2string(rc);
        retval.code = Response::Code::LDAPInit;
        goto out2;
    }

    rc = ldap_set_option(ld,
                         LDAP_OPT_PROTOCOL_VERSION,
                         (void*)&version);
    if (rc != LDAP_SUCCESS)
    {
        retval.code = Response::Code::LDAPInit;
        retval.message = "LDAP set version failed";
        goto out;
    }

    rc = ldap_simple_bind_s(ld, buffer, password.c_str());
    if (rc != LDAP_SUCCESS)
    {
        retval = Response::Code::InvalidCredentials;
        retval.message = "Credential check for user \"" + user
                       + "\" failed: " + ldap_err2string(rc);
        goto out;
    }

    retval.code = Response::Code::Success;
    retval.message = "";

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

    _curToken = toHexString((((uint64_t)rand())<<32) | ((uint64_t)rand()));

    std::ostringstream message;
    message << "New token: " << _curToken
            << " old token: " << _prevToken << " is "
            << (_prevValid?"still":"not") << " valid";
    _logger(message, LogLevel::notice);

    _onClientUpdate.notify_all();
}

Clientmessage Logic::getClientMessage()
{
    std::lock_guard<std::mutex> l(_mutex);
    Clientmessage retval(_webPrefix + _curToken,
                         _doormessage);

    // Reset doormessage
    _doormessage = Doormessage();

    return retval;
}

void Logic::_doorCallback(Doormessage doormessage)
{
    std::lock_guard<std::mutex> l(_mutex);
    _doormessage = doormessage;
    _onClientUpdate.notify_all();
}
