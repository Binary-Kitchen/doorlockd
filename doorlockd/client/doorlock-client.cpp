#include <exception>
#include <iostream>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <QApplication>

#include "config.h"

#include "../lib/clientmessage.h"
#include "../lib/logger.h"
#include "../lib/response.h"

#include "mainwindow.h"

// Info about doorlock-client version
const static std::string version =
        "doorlock-client-" DOORLOCK_VERSION;
const static std::string gitversion =
        DOORLOCK_GIT_BRANCH "-" DOORLOCK_GIT_COMMIT_HASH;

const static Logger &l = Logger::get();

namespace po = boost::program_options;
namespace ba = boost::asio;
using ba::ip::tcp;

ba::io_service io_service;

const static std::string subscriptionCommand =
        "{ \"command\": \"subscribe\"}";

// The receive buffer length of the TCP socket
const int constexpr SOCKET_BUFFERLENGTH = 2048;

static volatile bool run = true;

std::unique_ptr<MainWindow> mainWindow = nullptr;

static void onDoorlockUpdate(const Clientmessage &msg)
{
    const auto& doormessage = msg.doormessage();
    l("Received message", LogLevel::info);
    l((std::string)"  token: " + msg.token(),
      LogLevel::info);
    l((std::string)"  open: " + std::to_string(doormessage.isOpen),
      LogLevel::info);
    l((std::string)"  button lock: " + std::to_string(doormessage.isLockButton),
      LogLevel::info);
    l((std::string)"  button unlock: " + std::to_string(doormessage.isUnlockButton),
      LogLevel::info);
    l((std::string)"  emergency open: " + std::to_string(doormessage.isEmergencyUnlock),
      LogLevel::info);
    if (mainWindow) {
        mainWindow->setQRCode(msg.token());
    }
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
        boost::system::error_code error = ba::error::host_not_found;
        size_t length;
        std::vector<char> data;

        while (error && endpoint_iterator != end) {
            socket.close();
            socket.connect(*endpoint_iterator++, error);
        }
        if (error)
            throw boost::system::system_error(error);

        // After connection is established, send the subscription command
        socket.write_some(ba::buffer(subscriptionCommand), error);
        if (error)
            throw boost::system::system_error(error);

        data.resize(SOCKET_BUFFERLENGTH);

        std::function<void(void)> receiveMessage = [&] () {
            socket.async_read_some(ba::buffer(data),
                                   [&] (const boost::system::error_code &ec,
                                        const size_t length)
            {
                if (ec) {
                    throw boost::system::system_error(ec);
                }

                const auto message = Clientmessage::fromJson(std::string(data.begin(), data.begin()+length));
                onDoorlockUpdate(message);

                receiveMessage();
            });
        };

        receiveMessage();
        io_service.run();
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
            exit(-1);
        }

        po::notify(vm);
    }
    catch(const std::exception &e)
    {
        l(LogLevel::error, e.what());
        exit(-1);
    }

    l(LogLevel::notice, "Starting doorlock-client");

    QApplication app(argc, argv);
    app.setOrganizationName("Binary Kitchen");
    app.setApplicationName("doorlock-client");

    mainWindow = std::unique_ptr<MainWindow>(new MainWindow);
    mainWindow->showFullScreen();

    // Start the TCP client as thread
    std::thread clientThread = std::thread([&] () {
        // If the TCP client returns, an error has occured
        // In normal operation, it never returns
        doorlock_client(hostname, port);

        // This will stop the Qapplication
        mainWindow->hide();
        mainWindow->close();
    });

    // This routine will never return in normal operation
    app.exec();

    // Stop the IO service
    io_service.stop();

    clientThread.join();

    if (mainWindow)
        mainWindow.reset();

    l(LogLevel::notice, "Stopping doorlock-client");
    return 0;
}
