#ifndef UTIL_H
#define UTIL_H

#include <algorithm>
#include <json/json.h>

template <typename T>
T getJson(const Json::Value &root, const std::string &key);

template <typename T>
T getJsonOrFail(const Json::Value &root, const std::string &key)
{
    const auto members = root.getMemberNames();
    if (std::find(members.begin(), members.end(), key) == members.end())
    {
        throw "Json key not existing";
    }

    return getJson<T>(root, key);
}

std::string toHexString(uint64_t c);
uint64_t toUint64(const std::string &s);

#endif
