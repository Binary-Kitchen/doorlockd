#include "request.h"
#include "util.h"
#include "logger.h"

const std::string Request::_commandKey = "command";

Request::Command Request::_commandFromString(const std::string &command)
{
    Command retval = Command::Unknown;

    if (command == "lock")
        retval = Command::Lock;
    else if (command == "unlock")
        retval = Command::Unlock;
    else if (command == "subscribe")
        retval = Command::Subscribe;
    else
        retval = Command::Unknown;

    return retval;
}

Request Request::fromJson(const Json::Value &root)
{
    Request retval;
    const auto &l = Logger::get();

    try {
        const auto commandStr =
                getJsonOrFail<std::string>(root, _commandKey);
        l("      command: " + commandStr, LogLevel::info);
        retval.command = _commandFromString(commandStr);

        // Stop parsing, if command is unknown
        if (retval.command == Command::Unknown)
            return retval;

        if (retval.command == Command::Lock ||
                retval.command == Command::Unlock) {
            retval.user = getJsonOrFail<std::string>(root, "user");
            l("      user: " + retval.user, LogLevel::info);
            retval.password = getJsonOrFail<std::string>(root, "password");
            l("      password: XXX", LogLevel::info);
            retval.token = getJsonOrFail<std::string>(root, "token");
            l("      token: " + retval.token, LogLevel::info);
        }

        if (retval.command == Command::Subscribe) {
            // Nothing to do in this case
        }
    }
    catch (const std::exception &ex) {
        throw Response(Response::Code::JsonError, ex.what());
    }

    return retval;
}

Request Request::fromString(const std::string &string)
{
    Json::Reader reader;
    Json::Value root;

    if (reader.parse(string, root) == false)
        throw Response(Response::Code::NotJson,
                       "No valid JSON");

    return fromJson(root);
}
