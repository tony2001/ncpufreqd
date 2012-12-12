
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
	NC_OPT("temp_high", "0", OPT_INT, temp_high)
	NC_OPT("temp_low", "0", OPT_INT, temp_low)
	NC_OPT("verbose", "0", OPT_INT, verbosity_level)
	NC_OPT("sleep", "1", OPT_INT, sleep_delay)
	NC_OPT("fifo", "0", OPT_BOOL, create_fifo)
	NC_OPT("wheel_write", "0", OPT_BOOL, wheel_write)
	NC_OPT("default_mode", "0", OPT_INT, default_mode)
	NC_OPT("thermal_sensor_path", NC_THERMAL_SENSOR_PATH, OPT_STRING, thermal_sensor_path)
	NC_OPT("cpu_freq_list_path", NC_CPU_FREQ_LIST_PATH, OPT_STRING, cpu_freq_path)
	NC_OPT("cpu_max_freq_path", NC_CPU_MAX_FREQ_PATH, OPT_STRING, cpu_max_freq_path)
	NC_OPT("cpu_current_freq_path", NC_CPU_CURRENT_FREQ_PATH, OPT_STRING, cpu_current_freq_path)
	NC_OPT("cpu_governor_path", NC_CPU_GOVERNOR_PATH, OPT_STRING, cpu_governor_path)
	NC_OPT("ac_state_path", NC_AC_STATE_PATH, OPT_STRING, ac_state_path)
	NC_OPT_LAST
};

unsigned char mask[256];
int mask_initialized = 0;

static inline void nc_rtrim(unsigned char *str) /* {{{ */
{
	unsigned char chars[] = " \n\r\t\v\0";
	unsigned char *end;
	int i;

	if (!mask_initialized) {
		memset(mask, 0, sizeof(mask));
		for (i = 0; i < sizeof(chars); i++) {
			mask[chars[i]] = 1;
		}
		mask_initialized = 1;
	}

	end = str + strlen((const char *)str);
	while (end > str) {
		if (mask[*end]) {
			*end = '\0';
		}
		end--;
	}
}
/* }}} */

static inline int nc_config_value_set(nc_config_t *config, ncpufreqd_config_opt_t *opt, const char *value) /* {{{ */
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
				unsigned char *val = (unsigned char *)(base + opt->field_offset);
				strncpy((char *)val, value, MPATH);
				nc_rtrim(val);
			}
			break;

		default:
			syslog(LOG_ERR, "invalid type specified for config option %s \"\"", opt->name);
			break;
	}
	return 0;
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

int nc_config_default_values_set(nc_config_t *config) /* {{{ */
{
	ncpufreqd_config_opt_t *opts = options;

	while (opts->name) {
		nc_config_value_set(config, opts, opts->default_value);
		opts++;
	}
	return 0;
}
/* }}} */

int nc_config_read(nc_config_t *config) /* {{{ */
{

	FILE *fd = NULL;
	char line[1024];
	char *ptr = NULL;
	int currLine = 0;
	struct group *wheelGroup;

	syslog(LOG_INFO, "reading /etc/ncpufreqd.conf");

	nc_config_default_values_set(config);

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
		nc_config_value_set(config, opt, ptr);
	}

	fclose(fd);

	/* Sanity checks */

	if (config->sleep_delay > 30) {
		syslog(LOG_ERR, "sleep value in /etc/ncpufreqd.conf is too high");
		return 0;
	}

	if (config->sleep_delay == 0) {
		syslog(LOG_ERR, "sleep value in /etc/ncpufreqd.conf equals 0");
		return 0;
	}

	if (config->temp_high <= config->temp_low) {
		syslog(LOG_ERR, "temp_high is lower or equal to temp_low");
		return 0;
	}

	if (config->verbosity_level >= 3) {
		syslog(LOG_WARNING, "verbosity level clamped to 2");
		config->verbosity_level = 2;
	}

	if (config->default_mode > 2) {
		syslog(LOG_WARNING, "default_mode value invalid, defaulting to auto (0)");
		config->default_mode = 0;
	}

	if (config->verbosity_level == 2) {
		syslog(LOG_INFO, \
			"config read: th=%u, tl=%u, sl=%u, vl=%u, dm=%u", \
			config->temp_high, config->temp_low, config->sleep_delay, config->verbosity_level, \
			config->default_mode \
		);
		/*
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
		*/
	}

	unlink("/dev/ncpufreqd");

	if (config->create_fifo) {

		if (mkfifo("/dev/ncpufreqd", 0222) == 0) {

			syslog(LOG_NOTICE, "fifo created at /dev/ncpufreqd");
			chmod("/dev/ncpufreqd", S_IWUSR);

			if (config->wheel_write) {

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


