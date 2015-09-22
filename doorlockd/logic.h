#ifndef LOGIC_H
#define LOGIC_H

#include <cstdint>
#include <string>
#include <thread>
#include <condition_variable>
#include <mutex>

#include "config.h"
#include "door.h"
#include "logger.h"
#include "response.h"

/* The "Logic" class
 *
 * This class is initilized by all settings.
 *
 * It parses incoming JSON-Requests and returns the Response Code.
 */
class Logic
{
public:

    Logic(const std::chrono::seconds tokenTimeout,
          const std::string &ldapUri,
          const std::string &bindDN,
          const std::string &webPrefix,
          const std::string &serDev,
          std::condition_variable &onTokenUpdate);
    ~Logic();

    // Parse incoming JSON Requests
    Response parseRequest(const Json::Value &root);

    // Returns the current Token
    std::string getCurrentToken() const;

private:

    // Internal lock wrapper
    Response _lock();
    // Internal unlock wrapper
    Response _unlock();

    // Checks if the incoming token is valid
    bool _checkToken(const std::string &token);

    // Checks if incoming credentials against LDAP
    Response _checkLDAP(const std::string &user,
                        const std::string &password);

    // Creates a new random token and draws it on the epaper.
    // stillValid indicates whether the old (previous) token is still valid
    void _createNewToken(const bool stillValid);

    const Logger &_logger;

    // The door
    Door _door;

    // Tokens are 64-bit hexadecimal values
    using Token = uint64_t;

    // The current token
    Token _curToken = { 0x0000000000000000 };
    // The previous token
    Token _prevToken = { 0x0000000000000000 };
    // Indicates whether the previous token is valid
    bool _prevValid = { false };

    // Tokens are refreshed all tokenTimout seconds
    const std::chrono::seconds _tokenTimeout;
    // Thread for asynchronosly updating tokens
    std::thread _tokenUpdater = {};
    // Thread can be force-triggered for updates using the condition variable
    std::condition_variable _tokenCondition = {};
    // stop indicator for the thread
    bool _run = true;
    // Token mutex
    std::mutex _mutex = {};

    // This variable gets notified on token updates
    std::condition_variable &_onTokenUpdate;

    // The URI of the ldap server
    const std::string _ldapUri;
    // LDAP bindDN
    const std::string _bindDN;
    // Prefix of the website
    const std::string _webPrefix;
};

#endif
