#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>

#include <json/json.h>

class Response
{
public:
    enum Code {
        Success = 0, // Request successful
        Fail, // General non-specified error
        AlreadyUnlocked, // Authentication successful, but door is already unlocked
        AlreadyLocked, // Authentication successful, but door is already locked
        NotJson, // Request is not a valid JSON object
        JsonError, // Request is valid JSON, but does not contain necessary material
        InvalidToken, // Request contains invalid token
        InvalidCredentials, // Invalid LDAP credentials
        InvalidIP, // IP check failure
        UnknownCommand, // Unknown action
        LDAPInit, // Ldap initialization failed
        AccessDenied, // Access denied
        RESPONSE_NUM_ITEMS
    } code;
    std::string message;

    Response();
    Response(Response::Code code, const std::string &message = "");

    static Response fromJson(const Json::Value &root);
    static Response fromString(const std::string &json);
    std::string toJson() const;

    // Returns true if code is success
    operator bool() const;

private:

    const static std::string _codeKey;
    const static std::string _messageKey;
};

#endif // RESPONSE_H
