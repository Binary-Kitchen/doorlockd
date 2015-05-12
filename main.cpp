#include <iostream>
#include <sstream>
#include <string>

#include <boost/program_options.hpp>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "logic.h"

using namespace std;

namespace po = boost::program_options;

const static Logger &l = Logger::get();

int createFifo(const string &fifoLocation)
{
    int handle = -1;
    l(LogLevel::debug, "Creating Fifo file");
    if (access(fifoLocation.c_str(), F_OK) == 0)
    {
        l(LogLevel::warning, "Fifo file aready existing, trying to delete");
        if (unlink(fifoLocation.c_str()) != 0)
        {
            fprintf(stderr, "Unable to delete Fifo file");
            goto out;
        }
    }

    umask(0);

    if (mkfifo(fifoLocation.c_str(), 0770) != 0)
    {
        fprintf(stderr, "Unable to create Fifo");
        goto out;
    }

    handle = open(fifoLocation.c_str(), O_RDWR | O_NONBLOCK);
    if (handle == -1)
    {
        fprintf(stderr, "Unable to open Fifo");
        goto out;
    }

    if (fchown(handle, 0, 1001) != 0)
    {
        fprintf(stderr, "Fifo chown failed");
        close(handle);
        handle = -1;
        goto out;
    }

out:
    return handle;
}

int closeFifo(int handle)
{
    int retval = -1;

    if (handle != -1)
    {
        close(handle);
    }

    l(LogLevel::debug, "Removing Fifo file");
    if (unlink(FIFO_LOCATION) != 0)
    {
        l(LogLevel::error, "Unable to delete Fifo file");
        retval = -1;
        goto out;
    }

    retval = 0;

out:
    return retval;
}

int main(int argc, char** argv)
{
    int retval = -1;
    int fifoHandle = -1;
    std::chrono::seconds tokenTimeout;

    try {
        unsigned int timeout;
        po::options_description desc("usage: doorlockd");
        desc.add_options()
            ("help,h", "print help")
            ("tokentimeout,t", po::value<unsigned int>(&timeout)->required(), "tokentimeout in seconds");

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

    l(LogLevel::notice, "Starting doorlockd");

    fifoHandle = createFifo(FIFO_LOCATION);
    if (fifoHandle == -1)
    {
        goto out;
    }

    try {
        Logic &logic = Logic::get(tokenTimeout);
        fd_set set;
    
        for (;;)
        {
            FD_ZERO(&set);
            FD_SET(fifoHandle, &set);
    
            int i = select(fifoHandle+1, &set, nullptr, nullptr, nullptr);
            if (i == 0)
            {
                continue;
            } else if (i == -1) {
                throw "Fifo select() failed";
            }
    
            if (!FD_ISSET(fifoHandle, &set))
            {
                l(LogLevel::warning, "select(): Not my fd");
                continue;
            }
    
            string payload;
            for (;;)
            {
                constexpr int BUFSIZE = 2;
                char tmp[BUFSIZE];
                i = read(fifoHandle, tmp, BUFSIZE);
                if (i > 0) {
                    payload += string(tmp, i);
                } else {
                    if (errno == EWOULDBLOCK)
                    {
                        break;
                    }
                    throw "read() fifo failed";
                }
            }
    
            const auto rc = logic.parseRequest(payload);
        }

        retval = 0;
    }
    catch (const char* const &ex) {
        ostringstream str;
        str << "FATAL ERROR: " << ex;
        l(str, LogLevel::error);
        retval = -1;
        goto out1;
    }

    retval = 0;

out1:
    retval = closeFifo(fifoHandle);
out:
    Door::get().lock();
    l(LogLevel::notice, "Doorlockd stopped");
    return retval;
}
