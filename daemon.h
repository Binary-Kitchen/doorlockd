#ifndef DAEMON_H
#define DAEMON_H

#include <string>

void daemonize(const bool daemonize,
               const std::string &dir,
               const std::string &stdinfile,
               const std::string &stdoutfile,
               const std::string &stderrfile,
               const std::string &pidFile);

#endif
