
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
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "defs.h"

extern SConfig config;

struct stat cpu1;
unsigned int lstatedCPU1 = 0;
unsigned int setAlsoCPU1 = 0;

int setGovernor(int gov) {

	FILE *fd = NULL;

	if (gov != GOVERNOR_PERFORMANCE && gov != GOVERNOR_POWERSAVE) {
		syslog(LOG_ERR, "invalid governor requested (%d)", gov);
		return 0;
	}

	if (lstatedCPU1 == 0) {

		lstatedCPU1 = 1;
		setAlsoCPU1 = 0;

		memset(&cpu1, 0, sizeof(struct stat));

		if (lstat("/sys/devices/system/cpu/cpu1", &cpu1) == 0) {
			/* This should work like so:
			 *  - cpu1 exists -> lstat returns 0
			 *  - cpu1 is a directiry (not a symlink!!) -> set governor for cpu1 too
			 */
			if (S_ISDIR(cpu1.st_mode) && !S_ISLNK(cpu1.st_mode)) {
				syslog(LOG_INFO,
					"detected second processor at \"/sys/devices/system/cpu/cpu1\""
				);
				setAlsoCPU1 = 1;
			}
		}

	}

	fd = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "w");
	if (fd == NULL) {
		syslog(LOG_ERR,
			"failed to open \"/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor\" (%s)",
			strerror(errno)
		);
		return 0;
	}

	if (gov == GOVERNOR_PERFORMANCE) {
		fprintf(fd, "performance");
		syslog(LOG_INFO, "governor set to performance for cpu0");
	} else {
		fprintf(fd, "powersave");
		syslog(LOG_INFO, "governor set to powersave for cpu0");
	}

	fclose(fd);
	fd = NULL;

	if (setAlsoCPU1) {

		fd = fopen("/sys/devices/system/cpu/cpu1/cpufreq/scaling_governor", "w");
		if (fd == NULL) {
			syslog(LOG_ERR,
				"failed to open \"/sys/devices/system/cpu/cpu1/cpufreq/scaling_governor\" (%s)",
				strerror(errno)
			);
			return 0;
		}

		if (gov == GOVERNOR_PERFORMANCE) {
			fprintf(fd, "performance");
			syslog(LOG_INFO, "governor set to performance for cpu1");
		} else {
			fprintf(fd, "powersave");
			syslog(LOG_INFO, "governor set to powersave for cpu1");
		}

		fclose(fd);

	}

	return 1;

}

unsigned int getProcessorKHz(void) {

	unsigned int khz = 0;
	FILE *fd = NULL;

	fd = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
	if (fd == NULL) {
		syslog(LOG_ERR, "failed to open \"/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq\" (%s) - do you have cpufreq (CPU frequency scaling) in your kernel?", strerror(errno));
		return 0;
	}

	fscanf(fd, "%u", &khz);
	fclose(fd);

	return khz;

}

int setThrottling(unsigned int level) {

	FILE *fd = NULL;

	fd = fopen((const char*)config.acpiProcessorPath, "w");
	if (fd == NULL) {
		syslog(LOG_ERR, "failed to open \"%s\" (%s)", config.acpiProcessorPath, strerror(errno));
		return 0;
	}

	fprintf(fd, "%u", level);
	fclose(fd);

	/* syslog(LOG_INFO, "throttling level set to %u", level); */

	return 1;

}

unsigned int getTemperature(void) {

	unsigned int i = 0;
	char content[100];
	unsigned int conlen = 0;
	char *ptr = NULL;
	FILE *fd = NULL;
	int read = 0;

	fd = fopen((const char*)config.acpiThermalZonePath, "r");
	if (fd == NULL) {
		syslog(LOG_ERR, "failed to open \"%s\" (%s) - do you have ACPI Thermal Zone enabled in your kernel and correct path in config?", config.acpiThermalZonePath, strerror(errno));
		return 0;
	}

	memset(content, 0, 100);

	read = fread(content, sizeof(char), 99, fd);

	if (read == 0) {
		syslog(LOG_ERR, "failed to read \"%s\" (%s)", config.acpiThermalZonePath, strerror(errno));
		fclose(fd);
		return 0;
	}

	fclose(fd);

	conlen = strlen(content);

	for (i = 0; i < conlen; i++) {	/* Nasty !! ;) */
		if (content[i] >= '0' && content[i] <= '9') {
			ptr = content + i;
			break;
		}
	}

	return (unsigned int)(strtol(ptr, NULL, 10));

}

int acOnline(void) {

	FILE *fd = NULL;
	char content[100];
	int read = 0;

	fd = fopen((const char*)config.acpiACAdapterPath, "r");
	if (fd == NULL) {
		syslog(LOG_ERR, "failed to open \"%s\" (%s) - do you have ACPI AC Adapter enabled in you kernel and correct path in config?", config.acpiACAdapterPath, strerror(errno));
		return -1;
	}

	memset(content, 0, 100);

	read = fread(content, sizeof(char), 99, fd);

	if (read == 0) {
		syslog(LOG_ERR, "failed to read \"%s\" (%s)", config.acpiACAdapterPath, strerror(errno));
		fclose(fd);
		return -1;
	}

	fclose(fd);

	if (strstr(content, "on-line") == NULL)
		return 0;
	else
		return 1;

}
