#include "clientmessage.h"
#include "util.h"
#include "response.h"

const std::string Clientmessage::_tokenKey = "token";
const std::string Clientmessage::_unlockButtonKey = "unlockButton";
const std::string Clientmessage::_lockButtonKey = "lockButton";
const std::string Clientmessage::_emergencyUnlockKey = "emergencyUnlock";
const std::string Clientmessage::_isOpenKey = "isOpen";

Clientmessage::Clientmessage(std::string token,
                             Doormessage doormessage) :
    _token(token),
    _doormessage(doormessage)
{
}

std::string Clientmessage::toJson() const
{
    Json::StyledWriter writer;
    Json::Value message;

    message[_tokenKey] = _token;
    message[_unlockButtonKey] = _doormessage.isUnlockButton;
    message[_lockButtonKey] = _doormessage.isLockButton;
    message[_emergencyUnlockKey] = _doormessage.isEmergencyUnlock;
    message[_isOpenKey] = _doormessage.isOpen;

    return writer.write(message);
}

const std::string& Clientmessage::token() const
{
    return _token;
}

const Doormessage& Clientmessage::doormessage() const
{
    return _doormessage;
}

Clientmessage Clientmessage::fromJson(const Json::Value &root)
{
    std::string token;
    Doormessage doormessage;

    try {
        token = getJsonOrFail<std::string>(root, _tokenKey);
        doormessage.isLockButton = getJsonOrFail<bool>(root, _lockButtonKey);
        doormessage.isUnlockButton = getJsonOrFail<bool>(root, _unlockButtonKey);
        doormessage.isEmergencyUnlock = getJsonOrFail<bool>(root, _emergencyUnlockKey);
        doormessage.isOpen = getJsonOrFail<bool>(root, _isOpenKey);
    }
    catch (const std::exception &ex) {
        throw Response(Response::Code::JsonError, ex.what());
    }

    return Clientmessage(token, doormessage);
}

Clientmessage Clientmessage::fromString(const std::string &string)
{
    Json::Reader reader;
    Json::Value root;

    if (reader.parse(string, root) == false)
        throw Response(Response::Code::NotJson,
                       "No valid JSON");

    return fromJson(root);
}
