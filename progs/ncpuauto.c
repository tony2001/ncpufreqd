
/**************************************************************************

Copyright (c) 2003-2005 Krzysiek 'Nelchael' Pawlik <krzysiek.pawlik@people.pl>

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

int main(int argc, char **argv) {

	FILE *file;
	char *me;
	char text[32];

	if (argc) {} /* Just to remove the warning :) */
	me = (char*)basename(argv[0]);

	memset(text, 0, 32);

	if (strcmp(me, "ncpuauto") == 0)
		strcat(text, "auto");
	else if (strcmp(me, "ncpupowersave") == 0)
		strcat(text, "powersave");
	else if (strcmp(me, "ncpuperformance") == 0)
		strcat(text, "performance");
	else {
		printf("Wrong symlink.\n");
		return -1;
	}

	file = fopen("/dev/ncpufreqd", "w");
	if (file == NULL) {
		printf("Can't open /dev/ncpufreqd - pipe missing?\n");
		return -1;
	}

	fprintf(file, "%s\n", text);

	fclose(file);

	return 0;

}
