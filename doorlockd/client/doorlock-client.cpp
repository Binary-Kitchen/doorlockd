#include <exception>
#include <iostream>
#include <string>
#include <thread>

#include <boost/program_options.hpp>

#include <QApplication>

#include "config.h"
#include "mainwindow.h"

// Info about doorlock-client version
const static std::string version =
        "doorlock-client-" DOORLOCK_VERSION;
const static std::string gitversion =
        DOORLOCK_GIT_BRANCH "-" DOORLOCK_GIT_COMMIT_HASH;

namespace po = boost::program_options;

int main(int argc, char** argv)
{
    int retval = 0;
    Logger &l = Logger::get();
    l((std::string)"Hello, this is " + version + " built on " + gitversion,
      LogLevel::info);

    try {
        QApplication app(argc, argv);
        std::string hostname;
        unsigned short port;
        po::variables_map vm;

        qRegisterMetaType<Clientmessage>("Clientmessage");
        app.setOrganizationName("Binary Kitchen");
        app.setApplicationName("doorlock-client");

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

        po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            exit(-1);
        }

        po::notify(vm);
        l(LogLevel::notice, "Starting doorlock-client");

        // Start main GUI
        MainWindow mainWindow(hostname, port);
        mainWindow.showFullScreen();

        // This routine will never return under normal conditions
        retval = app.exec();

        mainWindow.hide();
        mainWindow.close();
    }
    catch(const std::exception &e)
    {
        l(LogLevel::error, e.what());
        retval = -1;
    }

    l(LogLevel::notice, "Stopping doorlock-client");
    return retval;
}
