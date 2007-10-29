
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

#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include "defs.h"

void stripPaths(SConfig *config) {

	while (config->acpiProcessorPath[strlen((const char*)config->acpiProcessorPath) - 1] == '\n')
		config->acpiProcessorPath[strlen((const char*)config->acpiProcessorPath) - 1] = 0;

	while (config->acpiACAdapterPath[strlen((const char*)config->acpiACAdapterPath) - 1] == '\n')
		config->acpiACAdapterPath[strlen((const char*)config->acpiACAdapterPath) - 1] = 0;

	while (config->acpiThermalZonePath[strlen((const char*)config->acpiThermalZonePath) - 1] == '\n')
		config->acpiThermalZonePath[strlen((const char*)config->acpiThermalZonePath) - 1] = 0;

	while (config->acpiProcessorPath[strlen((const char*)config->acpiProcessorPath) - 1] == ' ')
		config->acpiProcessorPath[strlen((const char*)config->acpiProcessorPath) - 1] = 0;

	while (config->acpiACAdapterPath[strlen((const char*)config->acpiACAdapterPath) - 1] == ' ')
		config->acpiACAdapterPath[strlen((const char*)config->acpiACAdapterPath) - 1] = 0;

	while (config->acpiThermalZonePath[strlen((const char*)config->acpiThermalZonePath) - 1] == ' ')
		config->acpiThermalZonePath[strlen((const char*)config->acpiThermalZonePath) - 1] = 0;

}

int readConfig(SConfig *config) {

	FILE *fd = NULL;
	char line[1024];
	char *ptr = NULL;
	int currLine = 0;
	struct group *wheelGroup;

	syslog(LOG_INFO, "reading /etc/ncpufreqd.conf");

	/* Pull in defaults: */
	config->useCpufreq = 1;
	config->tempHigh = 0;
	config->tempLow = 0;
	config->thrStates = 0;
	config->thrOffline = 0;
	config->sleepDelay = 0;
	config->verbosityLevel = 0;
	config->wheelWrite = 0;
	config->defaultMode = 0;

	config->acpiProcessorPath[0] = 0;
	config->acpiACAdapterPath[0] = 0;
	config->acpiThermalZonePath[0] = 0;

	strcpy((char*)config->acpiProcessorPath, (const char*)"/proc/acpi/processor/CPU/throttling");
	strcpy((char*)config->acpiACAdapterPath, (const char*)"/proc/acpi/ac_adapter/AC/state");
	strcpy((char*)config->acpiThermalZonePath, (const char*)"/proc/acpi/thermal_zone/THM0/temperature");

	/* Open file: */
	fd = fopen("/etc/ncpufreqd.conf", "r");
	if (fd == NULL) {
		syslog(LOG_ERR, "failed to open \"/etc/ncpufreqd.conf\"");
		return 0;
	}

	while (!feof(fd)) {

		memset(line, 0, 1024);
		if (fgets(line, 1022, fd) == NULL)
			break;

		currLine++;

		/* Ignore comments (lines starting with # or ;) and empty lines */
		if (line[0] == '#' || line[0] == ';' || line[0] == '\n' || line[0] == '\r')
			continue;

		if (strncmp(line, "temp_high", 9) == 0) {

			ptr = line + 9;
			while (ptr[0] != '=') ptr++;
			ptr++;
			config->tempHigh = (unsigned int)strtol(ptr, NULL, 10);
			continue;

		}

		if (strncmp(line, "temp_low", 8) == 0) {

			ptr = line + 8;
			while (ptr[0] != '=') ptr++;
			ptr++;
			config->tempLow = (unsigned int)strtol(ptr, NULL, 10);
			continue;

		}

		if (strncmp(line, "verbose", 7) == 0) {

			ptr = line + 7;
			while (ptr[0] != '=') ptr++;
			ptr++;
			config->verbosityLevel = (unsigned int)strtol(ptr, NULL, 10);
			continue;

		}

		if (strncmp(line, "sleep", 5) == 0) {

			ptr = line + 5;
			while (ptr[0] != '=') ptr++;
			ptr++;
			config->sleepDelay = (unsigned int)strtol(ptr, NULL, 10);
			continue;

		}

		if (strncmp(line, "fifo", 4) == 0) {

			ptr = line + 4;
			while (ptr[0] != '=') ptr++;
			ptr++;
			config->createFifo = (unsigned int)(strtol(ptr, NULL, 10) != 0 ? 1 : 0);
			continue;

		}

		if (strncmp(line, "wheel_write", 11) == 0) {

			ptr = line + 11;
			while (ptr[0] != '=') ptr++;
			ptr++;
			config->wheelWrite = (unsigned int)(strtol(ptr, NULL, 10) != 0 ? 1 : 0);
			continue;

		}

		if (strncmp(line, "use_cpufreq", 11) == 0) {

			ptr = line + 11;
			while (ptr[0] != '=') ptr++;
			ptr++;
			config->useCpufreq = (unsigned int)(strtol(ptr, NULL, 10) != 0 ? 1 : 0);
			continue;

		}

		if (strncmp(line, "throttling_states", 17) == 0) {

			ptr = line + 17;
			while (ptr[0] != '=') ptr++;
			ptr++;
			config->thrStates = (unsigned int)strtol(ptr, NULL, 10);
			continue;

		}

		if (strncmp(line, "throttling_offline", 18) == 0) {

			ptr = line + 18;
			while (ptr[0] != '=') ptr++;
			ptr++;
			config->thrOffline = (unsigned int)strtol(ptr, NULL, 10);
			continue;

		}

		if (strncmp(line, "acpi_processor_path", 19) == 0) {

			ptr = line + 19;
			while (ptr[0] != '/') ptr++;
			strncpy((char*)config->acpiProcessorPath, ptr, MPATH);
			continue;

		}

		if (strncmp(line, "acpi_ac_adaper_path", 19) == 0) {

			ptr = line + 19;
			while (ptr[0] != '/') ptr++;
			strncpy((char*)config->acpiACAdapterPath, ptr, MPATH);
			continue;

		}

		if (strncmp(line, "acpi_thermal_zone_path", 22) == 0) {

			ptr = line + 22;
			while (ptr[0] != '/') ptr++;
			strncpy((char*)config->acpiThermalZonePath, ptr, MPATH);
			continue;

		}

		if (strncmp(line, "default_mode", 12) == 0) {

			ptr = line + 12;
			while (ptr[0] != '=') ptr++;
			ptr++;
			config->defaultMode = (unsigned int)strtol(ptr, NULL, 10);
			continue;

		}

		syslog(LOG_WARNING, "garbage in /etc/ncpufreqd.conf at line %d", currLine);

	}

	fclose(fd);

	/* Sanity checks */

	stripPaths(config);

	if (config->sleepDelay > 30) {
		syslog(LOG_ERR, "sleep value in /etc/ncpufreqd.conf is too high");
		return 0;
	}

	if (config->sleepDelay == 0) {
		syslog(LOG_ERR, "sleep value in /etc/ncpufreqd.conf equals 0");
		return 0;
	}

	if (config->tempHigh <= config->tempLow) {
		syslog(LOG_ERR, "temp_high is lower or equal to temp_low");
		return 0;
	}

	if (config->verbosityLevel >= 3) {
		syslog(LOG_WARNING, "verbosity level clamped to 2");
		config->verbosityLevel = 2;
	}

	if (config->useCpufreq != 1 && config->createFifo) {
		syslog(LOG_INFO, "can't use fifo when using ACPI throttling, fifo disabled");
		config->createFifo = 0;
	}

	if (config->defaultMode > 2) {
		syslog(LOG_WARNING, "default_mode value invalid, defaulting to auto (0)");
		config->defaultMode = 0;
	}

	if (config->verbosityLevel == 2) {
		syslog(LOG_INFO, \
			"config read: uc=%u, ts=%u, to=%u, th=%u, tl=%u, sl=%u, vl=%u, dm=%u", \
			config->useCpufreq, config->thrStates, config->thrOffline, \
			config->tempHigh, config->tempLow, config->sleepDelay, config->verbosityLevel, \
			config->defaultMode \
		);
		syslog(LOG_INFO, \
			"config read: pp=\"%s\"", \
			config->acpiProcessorPath
		);
		syslog(LOG_INFO, \
			"config read: acap=\"%s\"", \
			config->acpiACAdapterPath
		);
		syslog(LOG_INFO, \
			"config read: tzp=\"%s\"", \
			config->acpiThermalZonePath
		);
	}

	unlink("/dev/ncpufreqd");

	if (config->createFifo) {

		if (mkfifo("/dev/ncpufreqd", 0222) == 0) {

			syslog(LOG_NOTICE, "fifo created at /dev/ncpufreqd");
			chmod("/dev/ncpufreqd", S_IWUSR);

			if (config->wheelWrite) {

				chmod("/dev/ncpufreqd", S_IWUSR | S_IWGRP);
				wheelGroup = getgrnam("wheel");

				if (wheelGroup == NULL)
					syslog(LOG_WARNING, "group \"wheel\" not found");
				else
					chown("/dev/ncpufreqd", 0, wheelGroup->gr_gid);

			}

		} else
			syslog(LOG_ERR, "failed to create fifo at /dev/ncpufreqd (%s)", strerror(errno));

	}

	return 1;

}

