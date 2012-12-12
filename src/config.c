
/**************************************************************************

Copyright (c) 2003-2007 Krzysiek 'Nelchael' Pawlik <krzysiek.pawlik@people.pl>

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
#include <stddef.h>

#include "defs.h"
#include "config.h"

ncpufreqd_config_opt_t options[] = {
	NC_OPT("temp_high", "0", OPT_INT, tempHigh)
	NC_OPT("temp_low", "0", OPT_INT, tempLow)
	NC_OPT("verbose", "0", OPT_INT, verbosityLevel)
	NC_OPT("sleep", "1", OPT_INT, sleepDelay)
	NC_OPT("fifo", "0", OPT_BOOL, createFifo)
	NC_OPT("wheel_write", "0", OPT_BOOL, wheelWrite)
	NC_OPT("default_mode", "0", OPT_INT, defaultMode)
	NC_OPT("use_cpufreq", "0", OPT_BOOL, useCpufreq)
	NC_OPT("throttling_states", "0",  OPT_INT, thrStates)
	NC_OPT("throttling_offline", "0", OPT_INT, thrOffline)
	NC_OPT("acpi_processor_path", "/proc/acpi/processor/CPU/throttling", OPT_STRING, acpiProcessorPath)
	NC_OPT("acpi_ac_adapter_path", "/proc/acpi/ac_adapter/AC/state", OPT_STRING, acpiACAdapterPath)
	NC_OPT("acpi_thermal_zone_path", "/proc/acpi/thermal_zone/THM0/temperature", OPT_STRING, acpiThermalZonePath)
	NC_OPT_LAST
};

void stripPaths(SConfig *config)  /* {{{ */
{

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
/* }}} */

static inline int setConfigValue(SConfig *config, ncpufreqd_config_opt_t *opt, const char *value) /* {{{ */
{
	char *base = (char *)config;

	switch (opt->type) {
		case OPT_INT:
			{
				unsigned int *val = (unsigned int *)(base + opt->field_offset);
				*val = (unsigned int)strtol(value, NULL, 10);
			}
			break;

		case OPT_BOOL:
			{
				unsigned int *val = (unsigned int *)(base + opt->field_offset);
				*val = (unsigned int)strtol(value, NULL, 10) ? 1 : 0;
			}
			break;

		case OPT_STRING:
			{
				char *val = (char *)(base + opt->field_offset);
				strncpy(val, value, MPATH);
			}
			break;

		default:
			syslog(LOG_ERR, "invalid type specified for config option %s \"\"", opt->name);
			break;
	}
}
/* }}} */

static inline ncpufreqd_config_opt_t *findConfigOption(const char *name) /* {{{ */
{
	ncpufreqd_config_opt_t *opts = options;

	while (opts->name) {
		if (strncmp(opts->name, name, opts->name_len) == 0) {
			return opts;
		}
		opts++;
	}
	return NULL;
}
/* }}} */

int setConfigDefaultValues(SConfig *config) /* {{{ */
{
	ncpufreqd_config_opt_t *opts = options;

	while (opts->name) {
		setConfigValue(config, opts, opts->default_value);
		opts++;
	}
	return 0;
}
/* }}} */

int readConfig(SConfig *config) /* {{{ */
{

	FILE *fd = NULL;
	char line[1024];
	char *ptr = NULL;
	int currLine = 0;
	struct group *wheelGroup;

	syslog(LOG_INFO, "reading /etc/ncpufreqd.conf");

	setConfigDefaultValues(config);

	/* Open file: */
	fd = fopen("/etc/ncpufreqd.conf", "r");
	if (fd == NULL) {
		syslog(LOG_ERR, "failed to open \"/etc/ncpufreqd.conf\"");
		return 0;
	}

	while (!feof(fd)) {
		ncpufreqd_config_opt_t *opt;

		memset(line, 0, 1024);
		if (fgets(line, 1022, fd) == NULL)
			break;

		currLine++;

		/* Ignore comments (lines starting with # or ;) and empty lines */
		if (line[0] == '#' || line[0] == ';' || line[0] == '\n' || line[0] == '\r')
			continue;

		opt = findConfigOption(line);

		if (!opt) {
			syslog(LOG_WARNING, "garbage in /etc/ncpufreqd.conf at line %d", currLine);
		}

		ptr = line + opt->name_len;
		while (ptr[0] != '=') {
			ptr++;
		}
		setConfigValue(config, opt, ptr);
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
/* }}} */


