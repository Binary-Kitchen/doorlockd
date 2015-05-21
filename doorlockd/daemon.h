#ifndef DAEMON_H
#define DAEMON_H

#include <string>

// Daemonizes the process if daemonize is true.
// If daemonize is true, it will write the new PID to the file "pidFile"
//
// This function will also redirect stdin, out and err to the
// specified files
void daemonize(const bool daemonize,
               const std::string &dir,
               const std::string &stdinfile,
               const std::string &stdoutfile,
               const std::string &stderrfile,
               const std::string &pidFile);

#endif
