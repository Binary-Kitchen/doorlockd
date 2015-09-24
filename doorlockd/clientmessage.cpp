#include <json/json.h>

#include "clientmessage.h"

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

    message["token"] = _token;
    message["unlockButton"] = _doormessage.isUnlockButton;
    message["lockButton"] = _doormessage.isLockButton;
    message["emergencyUnlock"] = _doormessage.isEmergencyUnlock;

    return writer.write(message);
}
