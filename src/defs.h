
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

#ifndef __defs_h__
#define __defs_h__

#include "ncpufreqd_config.h"

#define GOVERNOR_PERFORMANCE	0x00
#define GOVERNOR_POWERSAVE		0xFF

#define MODE_AUTO			0x00000001
#define MODE_POWERSAVE		0x00000010
#define MODE_PERFORMANCE	0x00000100

#define MPATH		512

typedef struct {
	unsigned int temp_high;
	unsigned int temp_low;
	unsigned int sleep_delay;
	unsigned int verbosity_level;
	unsigned int create_fifo;
	unsigned int mode;
	unsigned int wheel_write;
	unsigned int default_mode;
	unsigned char thermal_sensor_path[MPATH];
	unsigned char cpu_freq_list_path[MPATH];
	unsigned char cpu_max_freq_path[MPATH];
	unsigned char cpu_current_freq_path[MPATH];
	unsigned char ac_state_path[MPATH];
} nc_config_t;

#endif
