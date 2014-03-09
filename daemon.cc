#include <cstdio>
#include <iostream>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "daemon.hh"

#define	MAXFD 64

int daemon_init(const char* progname, int facility)
{
	pid_t pid;

	// create child process, terminate parent
	if ((pid = fork()) < 0)
	{
		perror("fork");
		return -1;
	}
	else if (pid)
		exit(0);

	// become session and process group leader, detach from terminal
	if (setsid() < 0)
	{
		perror("setsid");
		return -1;
	}

	// ignore SIGHUP when session leader terminates
	signal(SIGHUP, SIG_IGN);

	// create child again, terminate parent (i.e. session leader)
	if ((pid = fork()) < 0)
	{
		perror("fork");
		return -1;
	}
	else if (pid)
		exit(0);

	// change to safe working directory
	chdir("/");

	// close inherited file descriptors (including standard streams)
	int	i;
	for (i = 0; i < MAXFD; i++)
		close(i);

	// redirect standard streams
	open("/dev/null", O_RDONLY); // stdin
	open("/dev/null", O_RDWR); // stdout
	open("/dev/null", O_RDWR); // stderr

	// open connection to the system logger
	openlog(progname, LOG_PID, facility);

	return 0;
}
