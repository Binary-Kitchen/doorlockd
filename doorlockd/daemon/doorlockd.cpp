#include <csignal>
#include <iostream>

#include <boost/program_options.hpp>
#include <boost/asio.hpp>

#include <json/json.h>

#include "../lib/logic.h"

#include "config.h"

namespace po = boost::program_options;
namespace ba = boost::asio;
using ba::ip::tcp;

// Info about doorlockd version
const static std::string version =
        "doorlockd-" DOORLOCK_VERSION;
const static std::string gitversion =
        DOORLOCK_GIT_BRANCH "-" DOORLOCK_GIT_COMMIT_HASH;

// The receive buffer length of the TCP socket
const int constexpr SOCKET_BUFFERLENGTH = 2048;

static Logger &l = Logger::get();

static std::unique_ptr<Logic> logic = nullptr;
static ba::io_service io_service;

static std::mutex mutex;
static std::condition_variable onClientMessage;
static volatile bool run = true;

static void signal_handler(int signum)
{
    l((std::string)"Received Signal " + std::to_string(signum),
      LogLevel::warning);
    io_service.stop();
    run = false;
    onClientMessage.notify_all();
}

static Response subscribe(tcp::socket &sock)
{
    sock.write_some(ba::buffer(logic->getClientMessage().toJson()));
    while (run) {
        std::unique_lock<std::mutex> lock(mutex);
        onClientMessage.wait(lock);
        if (run) {
            sock.write_some(ba::buffer(logic->getClientMessage().toJson()));
        }
    };
    return Response(Response::Code::Success);
}

static void session(tcp::socket &&sock)
{
    ba::ip::address remoteAddress;
    unsigned short remotePort = 0;
    Response response;

    try {
        std::vector<char> data;
        data.resize(SOCKET_BUFFERLENGTH);

        remoteAddress = sock.remote_endpoint().address();
        remotePort = sock.remote_endpoint().port();

        l("Incoming TCP connection from " + remoteAddress.to_string() + "("
          + std::to_string(remotePort) + ")",
          LogLevel::notice);

        size_t length = sock.read_some(ba::buffer(data));

        // Get Request
        const std::string requestString(data.begin(), data.begin()+length);
        l("   Parsing request...", LogLevel::info);
        Request request = Request::fromString(requestString);

        switch (request.command) {
            case Request::Command::Lock:
            case Request::Command::Unlock:
                response = logic->request(request);
                break;

            case Request::Command::Subscribe:
                if (remoteAddress.is_loopback() == false) {
                    response.code = Response::Code::AccessDenied;
                    response.message = "Subscriptions are only allowed from localhost";
                } else {
                    response = subscribe(sock);
                }
                break;

            case Request::Command::Unknown:
            default:
                response.code = Response::Code::UnknownCommand;
                response.message = "Received unknown command ";
            break;
        }

        throw response;
    }
    catch (const Response &err) {
        response = err;
    }
    catch (const std::exception &err) {
        response.code = Response::Code::Fail;
        response.message = "Exception in session " + remoteAddress.to_string()
                + ":" + std::to_string(remotePort) + " : " + err.what();
    }
    catch (...) {
        response.code = Response::Code::Fail;
        response.message = "Unhandled doorlockd error";
    }

    if (!response) {
        l(response.message, LogLevel::warning);
    }

    if (sock.is_open()) {
        boost::system::error_code ec;
        sock.write_some(ba::buffer(response.toJson()), ec);
    }

    l("Closing TCP connection from " + remoteAddress.to_string(), LogLevel::notice);
}

static void server(unsigned short port)
{
    l(LogLevel::info, "Starting TCP Server");

    const auto endpoint = tcp::endpoint(ba::ip::address::from_string("127.0.0.1"), port);
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
    unsigned int tokenLength;
    std::string serDev;
    unsigned int baudrate;

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
            ("tokenLength,t",
                po::value<unsigned int>(&tokenLength)->default_value(DEFAULT_TOKEN_LENGTH),
                "Token length")
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
            exit(0);
        }

        po::notify(vm);

        tokenTimeout = std::chrono::seconds(timeout);

        l.setLogFile(logfile);
        l.logFile(true);
    }
    catch(const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        exit(-1);
    }

    l((std::string)"Hello, this is " + version + " built on " + gitversion,
      LogLevel::info);
    l(LogLevel::notice, "Starting doorlockd");

    signal(SIGINT, signal_handler);
    signal(SIGKILL, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);

    l(LogLevel::info, "Starting Doorlock Logic");

    retval = 0;
    try {
        logic = std::unique_ptr<Logic>(new Logic(tokenTimeout,
                                                 ldapUri,
                                                 bindDN,
                                                 lockPagePrefix,
                                                 tokenLength,
                                                 serDev,
                                                 baudrate,
                                                 onClientMessage));
        server(port);
    }
    catch (...) {
        l(LogLevel::error, "Fatal error, shutting down");
        retval = -1;
    }

    if (logic) {
        l(LogLevel::info, "Stopping Doorlock Logic");
        logic.reset();
    }
    l(LogLevel::notice, "Doorlockd stopped");
    return retval;
}
