
#ifndef NP_DAEMON_HH
#define NP_DAEMON_HH

/* Starts a daemon process originating from current process
 *
 * param progname: program identification used in system logger
 * param facility: default facility used in system logger
 * returns: 0 if daemonization succeeded, -1 on error */
int daemon_init(const char* progname, int facility);

#endif
