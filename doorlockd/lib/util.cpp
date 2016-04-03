#include "util.h"

template <>
int getJson(const Json::Value &root, const std::string &key)
{
    auto val = root.get(key, Json::Value());
    if (val.isInt())
        return val.asInt();
    throw std::runtime_error("Json Type error");
}

template <>
std::string getJson(const Json::Value &root, const std::string &key)
{
    auto val = root.get(key, Json::Value());
    if (val.isString())
        return val.asString();
    throw std::runtime_error("Json Type error");
}

template <>
size_t getJson(const Json::Value &root, const std::string &key)
{
    auto val = root.get(key, Json::Value());
    if (val.isInt())
        return val.asUInt64();
    throw std::runtime_error("Json Type error");
}

template <>
bool getJson(const Json::Value &root, const std::string &key)
{
    auto val = root.get(key, Json::Value());
    if (val.isBool())
        return val.asBool();
    throw std::runtime_error("Json Type error");
}

template <>
Json::Value getJson(const Json::Value &root, const std::string &key)
{
    auto val = root.get(key, Json::Value());
    return val;
}

static char nibble2hex(unsigned char input)
{
    input &= 0xf;

    if(input <= 9)
    {
        return input + '0';
    }

    return input - 0xA + 'A';
}

std::string randHexString(unsigned int len)
{
    std::string retval;
    while (len--)
        retval += nibble2hex(rand() & 0xF);
    return retval;
}

unsigned char hex2uchar(const char input)
{
    if(input >= '0' && input <= '9') {
        return input - '0';
    } else if(input >= 'A' && input <= 'F') {
        return input - 'A' + 10;
    } else if(input >= 'a' && input <= 'f') {
        return input - 'a' + 10;
    }
    throw std::runtime_error("Malformed hexadecimal input");
}
