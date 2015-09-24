#ifndef CLIENTMESSAGE_H
#define CLIENTMESSAGE_H

#include <string>

#include "doormessage.h"

class Clientmessage
{

public:

    Clientmessage(std::string token,
                  Doormessage doormessage);

    static Clientmessage fromJson(const std::string &json);
    std::string toJson() const;

    const std::string& token() const;
    const Doormessage& doormessage() const;

private:

    std::string _token;
    Doormessage _doormessage;

    static const std::string _tokenKey;
    const static std::string _unlockButtonKey;
    const static std::string _lockButtonKey;
    const static std::string _emergencyUnlockKey;
};

#endif
