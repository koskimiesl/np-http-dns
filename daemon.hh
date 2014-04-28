
#ifndef NETPROG_DAEMON_HH
#define NETPROG_DAEMON_HH

/*
 * Start a daemon process originating from current process
 *
 * progname: program identification used in system logger
 * return: 0 if daemonization succeeded, -1 on error
 */
int daemon_init(const char* progname);

#endif
