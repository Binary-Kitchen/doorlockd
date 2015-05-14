#include <iostream>
#include <string>
#include <cstdlib>
#include <memory>
#include <utility>
#include <csignal>

#include <boost/program_options.hpp>
#include <boost/asio.hpp>

#include "daemon.h"
#include "config.h"
#include "logic.h"

using namespace std;

namespace po = boost::program_options;
using boost::asio::ip::tcp;

const static Logger &l = Logger::get();

static unique_ptr<Logic> logic = nullptr;

boost::asio::io_service io_service;

void signal_handler(int signum)
{
    (void)signum;
    io_service.stop();
    logic.reset();
}

class session
  : public std::enable_shared_from_this<session>
{

public:

    session(tcp::socket socket)
        : _socket(std::move(socket))
    {
    }

    void start()
    {
        auto self(shared_from_this());
        _socket.async_read_some(boost::asio::buffer(_data, _maxLen),
            [this, self](boost::system::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    const string payload(_data, length);
                    const auto rc = logic->parseRequest(payload);
                    boost::asio::write(_socket, boost::asio::buffer(to_string(rc) + "\n"));
                }
            });
    }

private:

    tcp::socket _socket;
    static constexpr int _maxLen = { 2048 };
    char _data[_maxLen];
};

class server
{

public:

  server(boost::asio::io_service& io_service, short port)
    : _acceptor(io_service, tcp::endpoint(tcp::v4(), port)),
      _socket(io_service)
  {
    do_accept();
  }

private:

    void do_accept()
    {

      _acceptor.async_accept(_socket,
          [this](boost::system::error_code ec)
          {
              if (!ec)
              {
                  std::make_shared<session>(std::move(_socket))->start();
              }

              do_accept();
          });

    }

    tcp::acceptor _acceptor;
    tcp::socket _socket;
};


int main(int argc, char** argv)
{
    int retval = -1;
    short port;
    std::chrono::seconds tokenTimeout;
    string ldapServer;
    string bindDN;
    string lockPagePrefix;
    string allowedIpPrefix;
    string logfile;
    string pidFile;
    bool foreground = false;

    l(LogLevel::notice, "Starting doorlockd");

    system("/usr/bin/gpio load spi");
    system("/usr/bin/gpio load i2c");

    try {
        unsigned int timeout;
        po::options_description desc("usage: doorlockd");
        desc.add_options()
            ("help,h", "print help")
            ("tokentimeout,t", po::value<unsigned int>(&timeout)->default_value(DEFAULT_TOKEN_TIMEOUT), "Token timeout in seconds")
            ("port,p", po::value<short>(&port)->default_value(DEFAULT_PORT), "Port")
            ("ldap,s", po::value<string>(&ldapServer)->default_value(DEFAULT_LDAP_SERVER), "Ldap Server")
            ("bidndn,b", po::value<string>(&bindDN)->default_value(DEFAULT_BINDDN), "Bind DN, %s means username")
            ("web,w", po::value<string>(&lockPagePrefix)->default_value(DEFAULT_WEB_PREFIX), "Prefix of the webpage")
            ("ip,i", po::value<string>(&allowedIpPrefix)->default_value(DEFAULT_ALLOWED_IP_PREFIX), "Default allowed IP Prefix")
            ("foreground,f", po::bool_switch(&foreground)->default_value(false), "Run in foreground")
            ("logfile,l", po::value<string>(&logfile)->default_value(DEFAULT_LOG_FILE), "Log file")
            ("pid,z", po::value<string>(&pidFile)->default_value(DEFAULT_PID_FILE), "PID file");

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            retval = 0;
            goto out;
        }

        po::notify(vm);

        tokenTimeout = std::chrono::seconds(timeout>3 ? timeout-3 : timeout); // Epaper refresh takes ~3 seconds
    }
    catch(const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        goto out;
    }

    daemonize(!foreground,
              "/",
              "/dev/null",
              logfile,
              logfile,
              pidFile);

    signal(SIGINT, signal_handler);
    signal(SIGKILL, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);

    logic = unique_ptr<Logic>(new Logic(tokenTimeout,
                                        ldapServer,
                                        bindDN,
                                        lockPagePrefix,
                                        allowedIpPrefix));

    try {
        server s(io_service, port);
        io_service.run();
    }
    catch (const char* const &ex) {
        ostringstream str;
        str << "FATAL ERROR: " << ex;
        l(str, LogLevel::error);
        retval = -1;
        goto out;
    }

    retval = 0;

out:
    Door::get().lock();
    l(LogLevel::notice, "Doorlockd stopped");
    return retval;
}
