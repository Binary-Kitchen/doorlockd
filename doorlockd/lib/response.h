#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>

struct Response
{
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

    Response() :
        code(Fail),
        message("General failure")
    {
    }

    Response(Response::Code code, const std::string &message = "") :
        code(code),
        message(message)
    {
    }

    static Response fromJson(const std::string &json);
    std::string toJson() const;

    // Returns true if code is success
    operator bool() const;

private:

    const static std::string _codeKey;
    const static std::string _messageKey;
};

#endif // RESPONSE_H
