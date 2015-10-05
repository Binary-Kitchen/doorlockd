#ifndef CLIENTMESSAGE_H
#define CLIENTMESSAGE_H

#include <string>
#include <json/json.h>

#include "doormessage.h"

class Clientmessage
{
public:

    Clientmessage(std::string token,
                  bool isOpen,
                  Doormessage doormessage);

    static Clientmessage fromJson(const Json::Value &root);
    static Clientmessage fromString(const std::string &json);
    std::string toJson() const;

    const std::string& token() const;
    bool isOpen() const;
    const Doormessage& doormessage() const;

private:

    const std::string _token;
    const bool _isOpen;
    const Doormessage _doormessage;

    static const std::string _tokenKey;
    const static std::string _unlockButtonKey;
    const static std::string _lockButtonKey;
    const static std::string _emergencyUnlockKey;
    const static std::string _isOpenKey;
};

#endif
