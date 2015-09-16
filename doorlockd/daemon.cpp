#include <stdexcept>
#include <cstring>
#include <fstream>

#include <sys/resource.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "daemon.h"

using namespace std;

void daemonize(const string &dir,
               const string &stdinfile,
               const string &stdoutfile,
               const string &stderrfile)
{
    umask(0);

    rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
    {
        throw runtime_error(strerror(errno));
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
