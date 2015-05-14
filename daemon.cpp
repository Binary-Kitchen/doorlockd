#include <stdexcept>
#include <cstring>
#include <fstream>

#include <sys/resource.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "daemon.h"

using namespace std;

void daemonize(const bool daemonize,
               const string &dir,
               const string &stdinfile,
               const string &stdoutfile,
               const string &stderrfile,
               const string &pidFile)
{
    umask(0);

    rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
    {
        throw runtime_error(strerror(errno));
    }

    if (daemonize == true)
    {
        pid_t pid;
        if ((pid = fork()) < 0)
        {
            throw runtime_error(strerror(errno));
        } else if (pid != 0) {
            exit(0);
        }

        pid = setsid();

        ofstream pidStream;
        pidStream.exceptions(ofstream::failbit | ofstream::badbit);
        pidStream.open(pidFile, ofstream::trunc);
        pidStream << pid << endl;
    }

    if (!dir.empty() && chdir(dir.c_str()) < 0)
    {
        throw runtime_error(strerror(errno));
    }

    if (rl.rlim_max == RLIM_INFINITY)
    {
        rl.rlim_max = 1024;
    }

    for (unsigned int i = 0; i < rl.rlim_max; i++)
    {
        close(i);
    }

    int fd0 = open(stdinfile.c_str(), O_RDONLY);
    int fd1 = open(stdoutfile.c_str(),
      O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
    int fd2 = open(stderrfile.c_str(),
      O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);

    if (fd0 != STDIN_FILENO || fd1 != STDOUT_FILENO || fd2 != STDERR_FILENO)
    {
        throw runtime_error("new standard file descriptors were not opened as expected");
    }
}
