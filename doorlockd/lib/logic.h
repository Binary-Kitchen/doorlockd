#ifndef LOGIC_H
#define LOGIC_H

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include "config.h"
#include "clientmessage.h"
#include "door.h"
#include "logger.h"
#include "request.h"
#include "response.h"

/* The "Logic" class
 *
 * This class is initilized by all settings.
 *
 * It handles incoming requests and allows modifications of the door
 */
class Logic
{
public:

    Logic(const std::chrono::seconds tokenTimeout,
          const std::string &ldapUri,
          const std::string &bindDN,
          const std::string &webPrefix,
          const std::string &serDev,
          const unsigned int baudrate,
          std::condition_variable &onClientUpdate);
    ~Logic();

    // Parse incoming JSON Requests
    Response request(const Request &request);

    // Send direct command to door without credential checks
    enum class DoorCommand { Lock, Unlock };
    Response processDoor(const DoorCommand &doorCommand);

    // Returns the current Token
    Clientmessage getClientMessage();

private:

    // Internal lock wrapper
    Response _lock();
    // Internal unlock wrapper
    Response _unlock();

    // Checks if the incoming token is valid
    Response _checkToken(std::string token) const;

    // Checks if incoming credentials against LDAP
    Response _checkLDAP(const std::string &user,
                        const std::string &password);

    // Creates a new random token and draws it on the epaper.
    // stillValid indicates whether the old (previous) token is still valid
    void _createNewToken(const bool stillValid);

    void _doorCallback(Doormessage doormessage);

    Logger &_logger;

    // The door
    Door _door;

    // The current token
    std::string _curToken = { "0000000000000000" };
    // The previous token
    std::string _prevToken = { "0000000000000000" };
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
    // General mutex for concurrent data access
    mutable std::mutex _mutex = {};

    Doormessage _doormessage = {};

    // This variable gets notified on token updates
    std::condition_variable &_onClientUpdate;

    // The URI of the ldap server
    const std::string _ldapUri;
    // LDAP bindDN
    const std::string _bindDN;
    // Prefix of the website
    const std::string _webPrefix;
};

#endif
