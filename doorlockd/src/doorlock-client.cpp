#include <exception>
#include <iostream>
#include <string>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include "clientmessage.h"
#include "config.h"
#include "logger.h"
#include "response.h"

// Info about doorlock-client version
const static std::string version =
        "doorlock-client-" DOORLOCK_VERSION;
const static std::string gitversion =
        DOORLOCK_GIT_BRANCH "-" DOORLOCK_GIT_COMMIT_HASH;

const static Logger &l = Logger::get();

namespace po = boost::program_options;
using boost::asio::ip::tcp;

boost::asio::io_service io_service;

const static std::string subscriptionCommand =
        "{ \"command\": \"subscribe\"}";

// The receive buffer length of the TCP socket
const int constexpr SOCKET_BUFFERLENGTH = 2048;

static void doorlock_update(const Clientmessage &msg)
{
    // TODO develop small X application to show the QR code
    char buffer[1024];
    snprintf(buffer, sizeof(buffer),
             "qrencode -t ansi %s",
             msg.token().c_str());
    system(buffer);
}

static int doorlock_client(const std::string &hostname,
                           const unsigned short port)
{
    int retval = 0;

    try {
        tcp::resolver resolver(io_service);
        tcp::socket socket(io_service);
        tcp::resolver::query query(hostname, std::to_string(port));
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::resolver::iterator end;
        boost::system::error_code error = boost::asio::error::host_not_found;
        size_t length;
        std::vector<char> data;

        while (error && endpoint_iterator != end) {
            socket.close();
            socket.connect(*endpoint_iterator++, error);
        }
        if (error)
            throw boost::system::system_error(error);

        // After connection is established, send the subscription command
        socket.write_some(boost::asio::buffer(subscriptionCommand), error);
        if (error)
            throw boost::system::system_error(error);

        data.resize(SOCKET_BUFFERLENGTH);
        length = socket.read_some(boost::asio::buffer(data), error);
        if (error)
            throw boost::system::system_error(error);

        const auto response = Response::fromJson(std::string(data.begin(), data.begin()+length));
        if (!response)
            throw std::runtime_error("Invalid response from server");

        for (;;) {
            length = socket.read_some(boost::asio::buffer(data), error);
            if (error)
                throw boost::system::system_error(error);

            const auto message = Clientmessage::fromJson(std::string(data.begin(), data.begin()+length));
            if (!response)
                throw std::runtime_error("Invalid response from server");

            doorlock_update(message);
        }
    }
    catch(const std::exception &e)
    {
        l(LogLevel::error, e.what());
        goto out;
    }

out:
    return retval;
}

int main(int argc, char** argv)
{
    int retval = -1;
    std::string hostname;
    unsigned short port;

    l((std::string)"Hello, this is " + version + " built on " + gitversion,
      LogLevel::info);

    try {
        po::options_description desc("doorlockd (" + version + " built on " + gitversion + ")");
        desc.add_options()
            ("help,h",
                "print help")
            ("port,p",
                po::value<unsigned short>(&port)->default_value(DEFAULT_PORT),
                "Port")
            ("host,c",
                po::value<std::string>(&hostname)->default_value("localhost"),
                "IP or name of host running doorlockd");

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            retval = 0;
            goto out;
        }

        po::notify(vm);
    }
    catch(const std::exception &e)
    {
        l(LogLevel::error, e.what());
        goto out;
    }

    l(LogLevel::notice, "Starting doorlock-client");
    retval = doorlock_client(hostname, port);

out:
    l(LogLevel::notice, "Stopping doorlock-client");
    return retval;
}
