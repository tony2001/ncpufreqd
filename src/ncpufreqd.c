
/**************************************************************************

Copyright (c) 2003-2006 Krzysiek 'Nelchael' Pawlik <krzysiek.pawlik@people.pl>

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the
use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.

	2. Altered source versions must be plainly marked as such, and must not
	   be misrepresented as being the original software.

	3. This notice may not be removed or altered from any source distribution.

**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <sys/poll.h>
#include <fcntl.h>

#include "defs.h"
#include "config.h"
#include "functions.h"

#include "handler_cpufreq.h"
#include "handler_acpithr.h"

int AllowedToRun = 1;
SConfig config;

void sighandler(int sig) {

	if (sig == SIGTERM) {	/* Should be always true :) */

		AllowedToRun = 0;
		return;

	}

}

void handleFifo(FILE *fifo) {

	char buffer[1024];
	memset(buffer, 0, 1024);

	if (fgets(buffer, 1023, fifo) == NULL) {
		syslog(LOG_ERR, "error reading from fifo - disabling (%s)", strerror(errno));
		return;
	}

	if (strncmp(buffer, "powersave", 9) == 0) {
		config.mode = MODE_POWERSAVE;
		syslog(LOG_INFO, "mode set to powersave");
		return;
	}

	if (strncmp(buffer, "performance", 11) == 0) {
		config.mode = MODE_PERFORMANCE;
		syslog(LOG_INFO, "mode set to performance");
		return;
	}

	if (strncmp(buffer, "auto", 4) == 0) {
		config.mode = MODE_AUTO;
		syslog(LOG_INFO, "mode set to auto");
		return;
	}

	syslog(LOG_INFO, "garbage in fifo - ignored");

}

void daemon_func(void) {

	int acState = 0;
	unsigned int temp = 0;
	FILE *fifo;
	int fd;
	struct pollfd pollFifo;

	signal(SIGTERM, sighandler);
	if (readConfig(&config) == 0) {
		return;
	}

	/* Used only with cpufreq: */
	switch (config.defaultMode) {
		case 1:
			config.mode = MODE_POWERSAVE;
			syslog(LOG_INFO, "mode set to powersave");
			break;
		case 2:
			config.mode = MODE_PERFORMANCE;
			syslog(LOG_INFO, "mode set to performance");
			break;
		default:
			config.mode = MODE_AUTO;
			syslog(LOG_INFO, "mode set to auto");
	}

	if (config.useCpufreq)
		cpufreq_handle_online(&config, 0);
	else
		acpithr_handle_online(&config, 0);

	while (AllowedToRun) {

		acState = acOnline();
		temp = getTemperature();

		if (acState == -1) {

			syslog(LOG_ERR, "can't read ACPI AC state, terminating");
			break;

		}

		if (temp == 0) {

			syslog(LOG_ERR, "can't read ACPI temperature, terminating");
			break;

		}

		if (acState == 1) {

			/* We're online! */

			if (config.useCpufreq) {
				if (cpufreq_handle_online(&config, temp) != 0)
					break;
			} else {
				if (acpithr_handle_online(&config, temp) != 0)
					break;
			}

		} else {

			/* We're offline! */

			if (config.useCpufreq) {
				if (cpufreq_handle_offline(&config) != 0)
					break;
			} else {
				if (acpithr_handle_offline(&config) != 0)
					break;
			}

		}

		if (config.verbosityLevel == 2) {

			if (config.useCpufreq)
				/* Dump info about governor */
				cpufreq_dump_info(acState, temp);
			else
				/* Dump info about throttling state */
				acpithr_dump_info(acState, temp);

		}

		/* Check fifo */
		if (config.createFifo) {

			fd = open("/dev/ncpufreqd", O_NONBLOCK);
			if (fd < 0) {
				syslog(LOG_ERR, "can't open fifo - disabling (%s)", strerror(errno));
				config.createFifo = 0;
			} else {

				pollFifo.fd = fd;
				pollFifo.events = POLLIN | POLLPRI;
				pollFifo.revents = 0;

				switch (poll(&pollFifo, 1, 100)) {

					case 1:
						fifo = fdopen(fd, "r");
						if (fifo == NULL) {
							syslog(LOG_ERR, "can't open fifo - disabling (%s)", strerror(errno));
							config.createFifo = 0;
						} else {
							handleFifo(fifo);
							fclose(fifo);
						}
						break;

					case 0:
						break;

					default:
						syslog(LOG_ERR, "poll() on fifo returned error - disabling (%s)", strerror(errno));
						config.createFifo = 0;

				}

				close(fd);

			}

		}

		sleep(config.sleepDelay);

	}

	if (config.createFifo)
		unlink("/dev/ncpufreqd");

}

int main(int argc, char **argv) {

	if (argc != 1) {

		if (strcmp(argv[1], "--version") == 0) {

			printf("ncpufreqd %s compiled %s %s with GCC %s\n", PACKAGE_VERSION, __DATE__, __TIME__, __VERSION__);
			return 0;

		} else {

			printf("ncpufreqd doesn't accept any command line argument except --version\n");
			return 0;

		}

	}

	if (getuid() != 0 || getgid() != 0) {

		printf("You have to be root to run this daemon\n");
		return -2;

	}

	if (daemon(0, 0) == -1) {

		fprintf(stderr, "failed to create child process\n");
		return -1;

	}

	syslog(LOG_INFO, "ncpufreqd started");
	daemon_func();
	syslog(LOG_INFO, "ncpufreqd terminated");

	return 0;

}
