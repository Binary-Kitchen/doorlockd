#ifndef CLIENTMESSAGE_H
#define CLIENTMESSAGE_H

#include <string>
#include <json/json.h>

#include "doormessage.h"

class Clientmessage
{
public:

    Clientmessage(std::string web_address,
                  bool isOpen,
                  Doormessage doormessage);
    Clientmessage();

    Clientmessage &operator=(const Clientmessage& rhs);

    static Clientmessage fromJson(const Json::Value &root);
    static Clientmessage fromString(const std::string &json);
    std::string toJson() const;

    const std::string& web_address() const;
    bool isOpen() const;
    const Doormessage& doormessage() const;

private:

    std::string _web_address;
    bool _isOpen;
    Doormessage _doormessage;

    const static std::string _tokenKey;
    const static std::string _unlockButtonKey;
    const static std::string _lockButtonKey;
    const static std::string _emergencyUnlockKey;
    const static std::string _isOpenKey;
};

#endif
