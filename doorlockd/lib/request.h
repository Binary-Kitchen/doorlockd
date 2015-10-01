#ifndef REQUEST_H
#define REQUEST_H

#include <string>

#include <json/json.h>

#include "response.h"

class Request
{
public:
    static Request fromJson(const Json::Value &root);
    static Request fromString(const std::string &string);

    enum class Command { Lock, Unlock, Subscribe, Unknown }
                       command = { Command::Unknown };
    std::string user = { };
    std::string password = { };
    std::string token = { };

private:
    static Command _commandFromString(const std::string &command);

    const static std::string _commandKey;
};

#endif // REQUEST_H

