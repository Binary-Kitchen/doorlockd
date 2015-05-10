#include "util.h"

using namespace std;

template <>
int getJson(const Json::Value &root, const string &key)
{
    auto val = root.get(key, Json::Value());
    if (val.isInt())
        return val.asInt();
    throw "Json Type error";
}

template <>
string getJson(const Json::Value &root, const string &key)
{
    auto val = root.get(key, Json::Value());
    if (val.isString())
        return val.asString();
    throw "Json Type error";
}

template <>
size_t getJson(const Json::Value &root, const string &key)
{
    auto val = root.get(key, Json::Value());
    if (val.isInt())
        return val.asUInt64();
    throw "Json Type error";
}

template <>
bool getJson(const Json::Value &root, const string &key)
{
    auto val = root.get(key, Json::Value());
    if (val.isBool())
        return val.asBool();
    throw "Json Type error";
}

template <>
Json::Value getJson(const Json::Value &root, const string &key)
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

string toHexString(const uint64_t c)
{
    string retval;

    retval  = nibble2hex((c>>60) & 0xF);
    retval += nibble2hex((c>>56) & 0xF);
    retval += nibble2hex((c>>52) & 0xF);
    retval += nibble2hex((c>>48) & 0xF);
    retval += nibble2hex((c>>44) & 0xF);
    retval += nibble2hex((c>>40) & 0xF);
    retval += nibble2hex((c>>36) & 0xF);
    retval += nibble2hex((c>>32) & 0xF);
    retval += nibble2hex((c>>28) & 0xF);
    retval += nibble2hex((c>>24) & 0xF);
    retval += nibble2hex((c>>20) & 0xF);
    retval += nibble2hex((c>>16) & 0xF);
    retval += nibble2hex((c>>12) & 0xF);
    retval += nibble2hex((c>> 8) & 0xF);
    retval += nibble2hex((c>> 4) & 0xF);
    retval += nibble2hex((c    ) & 0xF);

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
    throw("Malformed hexadecimal input");
}

uint64_t toUint64(const string &s)
{
    if (s.length() != (64/4))
    {
        throw("Hex string has invalid length");
    }

    uint64_t retval = 0;

    for (int i = 0 ; i < (64/4) ; i++)
    {
        retval <<= 4;
        retval |= hex2uchar(s.at(i))&0xf;
    }

    return retval;
}
