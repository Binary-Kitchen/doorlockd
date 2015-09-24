#include <exception>

#include <json/json.h>

#include "response.h"
#include "util.h"

const std::string Response::_codeKey = "code";
const std::string Response::_messageKey = "message";

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

Response Response::fromJson(const std::string &json)
{
    Json::Reader reader;
    Json::Value root;
    Response retval;

    if (reader.parse(json, root) == false)
        throw std::runtime_error("Error parsing JSON");

    retval.message = getJsonOrFail<std::string>(root, _messageKey);

    const auto code = getJsonOrFail<int>(root, _codeKey);
    if (code > Code::RESPONSE_NUM_ITEMS)
        throw std::runtime_error("Error code out of range");
    retval.code = static_cast<Code>(code);

    return retval;
}
