/*
 *  Name:         Main cas file 
 *
 *  Purpose:      aMule Statistics
 *
 *  Author:       Pedro de Oliveira <falso@rdk.homeip.net>
 *
 *  Copyright (C) 2004 by Pedro de Oliveira
 * 
 *  This file is part of aMule.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the
 *  Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "version.h"
#include "configfile.h"
#include "functions.h"
#include "graphics.h"
#include "html.h"

/*
 * History:
 *
 * ????.??.?? - falso: creation of cas.
 * ????.??.?? - Jacobo221: Detect connecting state
 * ????.??.?? - falso: HTML page generation
 * 2004.08.27 - GonoszTopi: New line handling routines, to cope with lines
 *		longer than 80 characters. Fixes buffer overflow.
 */

#include "lines.h"

void usage(char *myname)
{
	printf	("   ___    _ _   ___    c aMule statistics\n"
			" /'___) /'_` )/',__)   by Pedro de Oliveira\n"
			"( (___ ( (_| |\\__, \\   <falso@rdk.homeip.net>\n"
			"`\\____)`\\__,_)(____/   Version %s\n\n"

			"Usage: %s [OPTION]\n"
			"If run without any option prints stats to stdout\n\n"
			"OPTIONS:\n"
#ifdef __GD__
			"-o\tWrites the online signature picture\n"
#endif
			"-p\tHTML Page with stats and picture\n"
			"-h\tThis help youre reading\n", CAS_VERSION, myname);
}

int main(int argc, char *argv[])
{
	/* Declaration of variables */
	FILE *amulesig;
	char *path;
	char stats[20][80];
	char *lines[6];
	int ler, i;
	CONF config;

	if ((argc == 2) && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
		usage(argv[0]);
		exit(0);
	}

	/* get amulesig path */
	path = get_path("amulesig.dat");
	if (path == NULL) {
		printf("Unable to get aMule settings path\n");
		exit(1);
	}

	/* open the file and if not exists exit with an error */
	if ((amulesig = fopen(path, "r")) == NULL) {
		printf("Unable to open file %s\nCheck if you have amule online signature enabled.\n", path);
		exit(2);
	} else {
		struct stat s_file;
		if ( stat(path, &s_file) == 0 ) {
			time_t t_now = time(0);
			if ( (t_now - s_file.st_mtime) > 60 ) {
				printf("aMule online signature last updated more then 60 sec ago\n");
				printf("Check that your aMule is running\n");
			}
		}
	}
	free(path);

	/* initialize all the strings with nothing */
	for (i = 0; i <= 19; i++)
		stats[i][0] = 0;

	/* start reading the stuff from amulesign to the stats array */
	i = 0;
	while (!feof(amulesig)) {
		ler = fgetc(amulesig);

		if (!feof(amulesig)) {

			/* Make it DOS compatible.*/
			if (ler == 13) ler = fgetc(amulesig);
			if (ler != 10) {
				if (strlen(stats[i]) < 80)
					sprintf(stats[i], "%s%c", stats[i],
							ler);
			} else
				i++;
		}

	}
	fclose(amulesig);

	/* if amule isnt running say that and exit else print out the stuff
	 * [ToDo] States 0 & 2 mean offline/connecting not "not running"...
	 */
	if (stats[0][0] == '0') {
		printf("aMule is not running\n");
		exit(3);
	}

	if (stats[0][0] == '2')
		CreateLine(lines, 0 ,"aMule %s is connecting\n", stats[12]);
	else
		CreateLine(lines, 0, "aMule %s has been running for %s\n",
				stats[12], timeconv(stats[15]));

	CreateLine(lines, 1, "%s is on %s [%s:%s] with ", stats[9],
			stats[1], stats[2], stats[3]);
	if (stats[4][0] == 'H')
		AppendToLine(lines, 1, "HighID\n");
	else
		AppendToLine(lines, 1, "LowID\n");


	strcpy(stats[10], convbytes(stats[10])); /* total download */
	strcpy(stats[11], convbytes(stats[11])); /* total upload */
	CreateLine(lines, 2, "Total Download: %s, Upload: %s\n",
			stats[10], stats[11]);

	strcpy(stats[13], convbytes(stats[13])); /* sess. download */
	strcpy(stats[14], convbytes(stats[14])); /* sess. upload */
	CreateLine(lines, 3, "Session Download: %s, Upload: %s\n",
			stats[13], stats[14]);

	CreateLine(lines, 4, "Download: %s kB/s, Upload: %s kB/s\n",
			stats[5], stats[6]);
	CreateLine(lines, 5,
			"Sharing: %s file(s), Clients on queue: %s\n",
			stats[8], stats[7]);
#ifdef __GD__
	if (argc == 2 && strcmp(argv[1], "-o") == 0) {
		if (!readconfig(&config)) {
			printf("Could not read config file\n");
			exit(4);
		}

		if (!createimage(&config, lines)) {
			printf("Could not create image!\n");
			exit(5);
		}
		exit(0);
	}
#endif

	if (argc == 2 && strcmp(argv[1], "-p") == 0) {
		
		if (!readconfig(&config)) {
			printf("Could not read config file\n");
			exit(4);
		}

		if (!create_html(stats,lines,config.template)) {
			printf("Could not create the HTML Page.\n");
		}

#ifdef __GD__
		if (!createimage(&config, lines)) {
			printf("Could not create image!\n");
			exit(5);
		}
#endif


		exit(0);
	}

	for (i = 0; i <= 5; i++) {
		printf("%s", lines[i]);
		free(lines[i]);
	}

	exit(0);
}

