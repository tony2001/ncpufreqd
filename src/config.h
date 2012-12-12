
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

extern int nc_config_read(nc_config_t *config);

typedef enum {
	OPT_BOOL=0,
	OPT_INT,
	OPT_STRING
} ncpufreqd_config_opt_type_t;

typedef struct {
	char *name;
	size_t name_len;
	char *default_value;
	size_t field_offset;
	ncpufreqd_config_opt_type_t type;
} ncpufreqd_config_opt_t;

#define NC_OPT(name, default_value, type, field) \
	{name, sizeof("name")-1, default_value, offsetof(nc_config_t, field), type},

#define NC_OPT_LAST \
	{NULL, 0, 0, 0}

