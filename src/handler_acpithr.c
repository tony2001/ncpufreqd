
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

#include <syslog.h>

#include "defs.h"
#include "functions.h"

static unsigned int throttling_level = 0xFFFFFF;

int acpithr_handle_online(SConfig *config, unsigned int temp) {

	float value;
	unsigned int tmp;

	if (temp < config->tempLow) {

		if (throttling_level != 0) {
			syslog(LOG_NOTICE, "processor temperature below %d -> setting throttling to 0", temp);

			if (setThrottling(0) == 0) {
				syslog(LOG_ERR, "can't set throttling, terminating");
				return -1;
			}

			throttling_level = 0;

			if (config->verbosityLevel == 1)
				syslog(LOG_INFO, "processor has %d KHz, and temperature %d", getProcessorKHz(), getTemperature());

		}

		return 0;

	}

	if (temp > config->tempHigh) {

		if (throttling_level != config->thrStates - 1) {

			syslog(LOG_NOTICE, "processor temperature %d exceeded maximum -> setting throttling to %d", temp, config->thrStates - 1);

			if (setThrottling(config->thrStates - 1) == 0) {
				syslog(LOG_ERR, "can't set throttling, terminating");
				return -1;
			}

			throttling_level = config->thrStates - 1;

			if (config->verbosityLevel == 1)
				syslog(LOG_INFO, "processor has %d KHz, and temperature %d", getProcessorKHz(), getTemperature());

		}

		return 0;

	}

	/* temp is between Low and High -> interpolate */

	value = (float)(config->thrStates - 2) / (float)(config->tempHigh - config->tempLow);
	tmp = (unsigned int)((float)(temp - config->tempLow) * value);

	if (tmp != throttling_level) {

		syslog(LOG_NOTICE, "processor temperature %d mapped to %u throttling level", temp, tmp);

		if (setThrottling(tmp) == 0) {
			syslog(LOG_ERR, "can't set throttling, terminating");
			return -1;
		}

		throttling_level = tmp;

		if (config->verbosityLevel == 1)
			syslog(LOG_INFO, "processor has %d KHz, and temperature %d", getProcessorKHz(), getTemperature());

	}

	return 0;

}

int acpithr_handle_offline(SConfig *config) {

	if (throttling_level != config->thrOffline) {

		syslog(LOG_NOTICE, "AC offline");

		if (setThrottling(config->thrOffline) == 0) {
			syslog(LOG_ERR, "can't set throttling level, terminating");
			return -1;
		}

		throttling_level = config->thrOffline;

		if (config->verbosityLevel == 1)
			syslog(LOG_INFO, "processor has %d KHz, and temperature %d", getProcessorKHz(), getTemperature());

	}

	return 0;

}

void acpithr_dump_info(unsigned int acState, unsigned int temp) {

	syslog(LOG_INFO, "AC: %s, processor has %d KHz, temperature %d, throttling level %u", \
			(acState ? ("online") : ("offline")), getProcessorKHz(), temp, \
			throttling_level);

}
