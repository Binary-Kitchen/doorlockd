#include <iostream>
#include <sstream>
#include <string>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "logic.h"

using namespace std;

const static Logger &l = Logger::get();

int main(void)
{
    int retval = -1;

    l(LogLevel::notice, "Starting doorlockd");

    int fifoHandle = -1;

    l(LogLevel::debug, "Creating Fifo file");
    if (access(FIFO_LOCATION, F_OK) == 0)
    {
        l(LogLevel::warning, "Fifo file aready existing, trying to delete");
        if (unlink(FIFO_LOCATION) != 0)
        {
            fprintf(stderr, "Unable to delete Fifo file");
            goto out;
        }
    }

    umask(0);

    if (mkfifo(FIFO_LOCATION, 0770) != 0)
    {
        fprintf(stderr, "Unable to create Fifo");
        goto out;
    }

    fifoHandle = open(FIFO_LOCATION, O_RDWR | O_NONBLOCK);
    if (fifoHandle == -1)
    {
        fprintf(stderr, "Unable to open Fifo");
        goto out;
    }

    if (fchown(fifoHandle, 0, 1001) != 0)
    {
        fprintf(stderr, "Fifo chown failed");
        goto out1;
    }


    try {
        Logic &logic = Logic::get();
        struct timeval tv;
        fd_set set;
    
        for (;;)
        {
            FD_ZERO(&set);
            FD_SET(fifoHandle, &set);
            tv.tv_sec = TOKEN_TIMEOUT;
            tv.tv_usec = 0;
    
            int i = select(fifoHandle+1, &set, nullptr, nullptr, &tv);
            if (i == 0)
            {
                logic.createNewToken(true);
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
    
            int rc = logic.parseRequest(payload);
        }

        retval = 0;
    }
    catch (const char* const &ex) {
        ostringstream str;
        str << "FATAL ERROR: " << ex;
        l(str, LogLevel::error);
        retval = -1;
    }

out1:
    if (fifoHandle != -1)
    {
        close(fifoHandle);
    }

    l(LogLevel::debug, "Removing Fifo file");
    if (unlink(FIFO_LOCATION) != 0)
    {
        throw("Unable to delete Fifo file");
    }

out:
    Door::get().lock();
    l(LogLevel::notice, "Doorlockd stopped");
    return retval;
}
