#include <exception>

#include <json/json.h>

#include "response.h"
#include "util.h"

const std::string Response::_codeKey = "code";
const std::string Response::_messageKey = "message";

Response::Response():
    code(Fail),
    message("General failure")
{
}

Response::Response(Response::Code code, const std::string &what) :
    code(code),
    message(what)
{
}

Response::operator bool() const
{
    return code == Response::Code::Success;
}

std::string Response::toJson() const
{
    Json::Value response;
    Json::StyledWriter writer;

    response[_codeKey] = code;
    response[_messageKey] = message;

    return writer.write(response);
}

Response Response::fromJson(const Json::Value &root)
{
    Response retval;

    try {
        retval.message = getJsonOrFail<std::string>(root, _messageKey);

        const auto code = getJsonOrFail<int>(root, _codeKey);
        if (code > Code::RESPONSE_NUM_ITEMS)
            throw std::runtime_error("Error code out of range");

        retval.code = static_cast<Code>(code);
    }
    catch (const std::exception &ex) {
        throw Response(Response::Code::JsonError, ex.what());
    }

    return retval;
}

Response Response::fromString(const std::string &json)
{
    Json::Reader reader;
    Json::Value root;

    if (reader.parse(json, root) == false)
        throw Response(Response::Code::NotJson,
                       "No valid JSON");

    return fromJson(root);
}
