
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

#include <syslog.h>

#include "defs.h"
#include "functions.h"

static int lastGovernor = 0x7F;

int cpufreq_handle_online(SConfig *config, unsigned int temp) {

	if (config->mode == MODE_AUTO) {

		if (temp > config->tempHigh && lastGovernor != GOVERNOR_POWERSAVE) {

			syslog(LOG_NOTICE, "processor temperature %d exceeded maximum (%d)", temp, config->tempHigh);

			if (setGovernor(GOVERNOR_POWERSAVE) == 0) {
				syslog(LOG_ERR, "can't set processor state, terminating");
				return -1;
			}

			lastGovernor = GOVERNOR_POWERSAVE;

			if (config->verbosityLevel >= 1)
				syslog(LOG_INFO, "processor has %d KHz, and temperature %d", getProcessorKHz(), getTemperature());

		}

		if (temp < config->tempLow && lastGovernor != GOVERNOR_PERFORMANCE) {

			syslog(LOG_NOTICE, "processor temperature %d dropped below low temperature (%d)", temp, config->tempLow);

			if (setGovernor(GOVERNOR_PERFORMANCE) == 0) {
				syslog(LOG_ERR, "can't set processor state, terminating");
				return -1;
			}

			lastGovernor = GOVERNOR_PERFORMANCE;

			if (config->verbosityLevel >= 1)
				syslog(LOG_INFO, "processor has %d KHz, and temperature %d", getProcessorKHz(), getTemperature());

		}

	} else {

		if (config->mode == MODE_PERFORMANCE && lastGovernor != GOVERNOR_PERFORMANCE) {

			if (setGovernor(GOVERNOR_PERFORMANCE) == 0) {
				syslog(LOG_ERR, "can't set processor state, terminating");
				return -1;
			}

			lastGovernor = GOVERNOR_PERFORMANCE;

			if (config->verbosityLevel >= 1)
				syslog(LOG_INFO, "processor has %d KHz, and temperature %d", getProcessorKHz(), getTemperature());

		}

		if (config->mode == MODE_POWERSAVE && lastGovernor != GOVERNOR_POWERSAVE) {

			if (setGovernor(GOVERNOR_POWERSAVE) == 0) {
				syslog(LOG_ERR, "can't set processor state, terminating");
				return -1;
			}

			lastGovernor = GOVERNOR_POWERSAVE;

			if (config->verbosityLevel >= 1)
				syslog(LOG_INFO, "processor has %d KHz, and temperature %d", getProcessorKHz(), getTemperature());

		}

	}

	return 0;

}

int cpufreq_handle_offline(SConfig *config) {

	if (lastGovernor != GOVERNOR_POWERSAVE) {

		syslog(LOG_NOTICE, "AC offline");

		if (setGovernor(GOVERNOR_POWERSAVE) == 0) {
			syslog(LOG_ERR, "can't set processor state, terminating");
			return -1;
		}

		lastGovernor = GOVERNOR_POWERSAVE;

		if (config->verbosityLevel >= 1)
			syslog(LOG_INFO, "processor has %d KHz, and temperature %d", getProcessorKHz(), getTemperature());

	}

	return 0;

}

void cpufreq_dump_info(unsigned int acState, unsigned int temp) {

	if (lastGovernor == GOVERNOR_PERFORMANCE)
		syslog(LOG_INFO, "AC: %s, processor has %d KHz, temperature %d, governor: performance", \
			(acState ? ("online") : ("offline")), getProcessorKHz(), temp);
	else
		syslog(LOG_INFO, "AC: %s, processor has %d KHz, temperature %d, governor: powersave", \
			(acState ? ("online") : ("offline")), getProcessorKHz(), temp);

}
