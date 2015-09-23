#include <iostream>
#include <string>
#include <cstdlib>
#include <memory>
#include <utility>
#include <csignal>

#include <boost/program_options.hpp>
#include <boost/asio.hpp>

#include <json/json.h>

#include "daemon.h"
#include "config.h"
#include "logic.h"
#include "util.h"

namespace po = boost::program_options;
using boost::asio::ip::tcp;

// Info about doorlockd version
const static std::string version =
        "doorlockd-" DOORLOCKD_VERSION;
const static std::string gitversion =
        DOORLOCKD_GIT_BRANCH  "-" DOORLOCKD_GIT_COMMIT_HASH;

// The receive buffer length of the TCP socket
const int constexpr SOCKET_BUFFERLENGTH = 2048;

const static Logger &l = Logger::get();

static std::unique_ptr<Logic> logic = nullptr;
static boost::asio::io_service io_service;

static std::condition_variable onTokenUpdate;

static void signal_handler(int signum)
{
    l((std::string)"Received Signal " + std::to_string(signum),
      LogLevel::warning);
    io_service.stop();
}

static void session(tcp::socket &&sock)
{
    boost::system::error_code error;
    const auto remoteIP = sock.remote_endpoint().address();
    std::vector<char> data;
    data.resize(SOCKET_BUFFERLENGTH);

    l("Incoming TCP connection from " + remoteIP.to_string(), LogLevel::notice);

    try {
        size_t length = sock.read_some(boost::asio::buffer(data),
                                       error);
        if (error == boost::asio::error::eof)
            return;
        else if (error)
            throw boost::system::system_error(error);

        const std::string request(data.begin(), data.begin()+length);

        Json::Reader reader;
        Json::Value root;
        Response response;
        std::string command;

        if (!reader.parse(request, root, false))
        {
            response.message = "Request is no valid JSON";
            response.code = Response::Code::JsonError;
            l(response.message, LogLevel::warning);
            goto out;
        }

        try {
            command = getJsonOrFail<std::string>(root, "command");
        }
        catch (...)
        {
            response.code = Response::Code::JsonError;
            response.message = "Error parsing JSON";
            l(response.message, LogLevel::warning);
            goto out;
        }

        l("  Command: " + command, LogLevel::notice);
        if (command == "lock" || command == "unlock") {
            response = logic->parseRequest(root);
        } else {
            response.code = Response::Code::UnknownCommand;
            response.message = "Received unknown command " + command;
            l(response.message, LogLevel::warning);
        }

    out:
        sock.write_some(boost::asio::buffer(response.toJson()),
                        error);
        if (error == boost::asio::error::eof)
            return;
        else if (error)
            throw boost::system::system_error(error);
    }
    catch (std::exception& e) {
      std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}

static void server(unsigned short port)
{
    l(LogLevel::info, "Starting TCP Server");

    const auto endpoint = tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port);
    tcp::acceptor a(io_service, endpoint);

    tcp::socket sock(io_service);

    std::function<void(void)> accept_connection = [&] () {
        a.async_accept(sock,
                       [&] (boost::system::error_code ec) {
            if (ec)
            {
                return;
            }
            std::thread(session, std::move(sock)).detach();
            accept_connection();
        });
    };

    accept_connection();

    io_service.run();
    l(LogLevel::info, "Stopped TCP Server");
}

int main(int argc, char** argv)
{
    int retval = -1;
    short port;
    std::chrono::seconds tokenTimeout;
    std::string ldapUri;
    std::string bindDN;
    std::string lockPagePrefix;
    std::string logfile;
    std::string serDev;
    unsigned int baudrate;

    l((std::string)"Hello, this is " + version + " built on " + gitversion,
      LogLevel::info);
    l(LogLevel::notice, "Starting doorlockd");

    try {
        unsigned int timeout;
        po::options_description desc("doorlockd (" + version + " built on " + gitversion + ")");
        desc.add_options()
            ("help,h",
                "print help")
            ("tokentimeout,t",
                po::value<unsigned int>(&timeout)->default_value(DEFAULT_TOKEN_TIMEOUT),
                "Token timeout in seconds")
            ("port,p",
                po::value<short>(&port)->default_value(DEFAULT_PORT),
                "Port")
            ("ldap,s",
                po::value<std::string>(&ldapUri)->default_value(DEFAULT_LDAP_URI),
                "Ldap Server")
            ("bidndn,b",
                po::value<std::string>(&bindDN)->default_value(DEFAULT_BINDDN),
                "Bind DN, %s means username")
            ("web,w",
                po::value<std::string>(&lockPagePrefix)->default_value(DEFAULT_WEB_PREFIX),
                "Prefix of the webpage")
            ("logfile,l",
                po::value<std::string>(&logfile)->default_value(DEFAULT_LOG_FILE),
                "Log file")
            ("serial,i",
                po::value<std::string>(&serDev)->default_value(DEFAULT_SERIAL_DEVICE),
                "Serial port")

            ("baud,r",
                po::value<unsigned int>(&baudrate)->default_value((DEFAULT_SERIAL_BAUDRATE)),
                "Serial baudrate");

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            retval = 0;
            goto out;
        }

        po::notify(vm);

        tokenTimeout = std::chrono::seconds(timeout);
    }
    catch(const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        goto out;
    }

    daemonize("/",
              "/dev/null",
              logfile,
              logfile);
    // Resend version string after redirection stdout
    l((std::string)"Hello, this is " + version + " built on " + gitversion,
      LogLevel::info);

    signal(SIGINT, signal_handler);
    signal(SIGKILL, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);

    l(LogLevel::info, "Starting Doorlock Logic");
    try {
        logic = std::unique_ptr<Logic>(new Logic(tokenTimeout,
                                                 ldapUri,
                                                 bindDN,
                                                 lockPagePrefix,
                                                 serDev,
                                                 baudrate,
                                                 onTokenUpdate));
        server(port);
    }
    catch (...) {
        l(LogLevel::error, "Fatal error, shutting down");
        retval = -1;
        goto out;
    }

    retval = 0;

out:
    if (logic) {
        l(LogLevel::info, "Stopping Doorlock Logic");
        logic.reset();
    }
    l(LogLevel::notice, "Doorlockd stopped");
    return retval;
}
