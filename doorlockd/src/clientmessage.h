#ifndef CLIENTMESSAGE_H
#define CLIENTMESSAGE_H

#include <string>

#include "doormessage.h"

class Clientmessage
{

public:

    Clientmessage(std::string token,
                  Doormessage doormessage);

    std::string toJson() const;

private:

    std::string _token;
    Doormessage _doormessage;
};

#endif
