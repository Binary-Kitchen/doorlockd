#ifndef UTIL_H
#define UTIL_H

#include <algorithm>
#include <exception>
#include <json/json.h>

template <typename T>
T getJson(const Json::Value &root, const std::string &key);

template <typename T>
static T getJsonOrFail(const Json::Value &root, const std::string &key)
{
    if (root.isObject() == false)
    {
        throw std::runtime_error("Invalid Json Object");
    }

    if (root.isMember(key) == false) {
        throw std::runtime_error("Json key \"" + key + "\" not existing");
    }

    return getJson<T>(root, key);
}

std::string randHexString(unsigned int len);

#endif
