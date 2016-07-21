#ifndef NETWORK_H
#define NETWORK_H

#include <string>

#include <boost/asio.hpp>
#include <QThread>

#include "../lib/clientmessage.h"
#include "../lib/logger.h"

class NetworkThread : public QThread {
    Q_OBJECT

public:
    NetworkThread(const std::string &hostname,
                  const unsigned short port);

private:
    void run();

    Logger &_l;
    const std::string _hostname;
    const unsigned short _port;
    
    boost::asio::io_service _io_service;

    const static std::string _subscription_command;

    // The receive buffer length of the TCP socket
    constexpr static int _SOCKET_BUFFERLENGTH = {2048};
    
signals:
    void new_clientmessage(Clientmessage msg);
};

#endif
