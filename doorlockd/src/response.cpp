#include <json/json.h>

#include "response.h"

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
