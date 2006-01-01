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
 *  51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
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
 * 2005.12.10 - fulgas: added kad info support
 * 2005,12,16 - stefanero: fixed Kad related stuff and some other things
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
	char *stats[20];
	char *lines[6];
	long lSize;
	char * buffer;
	int i;
	CONF config;

	if ((argc == 2) && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
		usage(argv[0]);
		exit(0);
	}

	/* get amulesig path */
	path = get_path("amulesig.dat");

	if (path == NULL) {
		perror("Unable to get aMule settings path\n");
		exit(1);
	}

	/* open the file and if not exists exit with an error */
	if ((amulesig = fopen(path, "r")) == NULL) {
		fprintf(stderr, "Unable to open file %s\nCheck if you have amule online signature enabled.\n", path);
		exit(2);
	} 
	/* i believe this shouldnt be here. 
	The freq of update could be higher than 60 seconds.
	And it doesn't mean that the amule is not running.
	*/
	/*
	else {
		struct stat s_file;
		if ( stat(path, &s_file) == 0 ) {
			time_t t_now = time(0);
			if ( (t_now - s_file.st_mtime) > 60 ) {
				perror("aMule online signature last updated more then 60 sec ago\n");
				perror("Check that your aMule is running\n");
			}
		}
	}*/
	free(path);

	/* initialize all the strings with nothing */
	for (i = 0; i <= 19; i++)
		stats[i] = 0;

	/* start reading the stuff from amulesign to the stats array */
	// obtain file size.
	fseek (amulesig , 0 , SEEK_END);
	lSize = ftell (amulesig);
	rewind (amulesig);
	buffer = (char*) malloc (lSize);
	if (buffer == NULL) {
		perror("Could not create buffer\n");
		exit (2);
	}
	fread (buffer,1,lSize,amulesig);
	fclose(amulesig);

	stats[0] = strtok (buffer,"\n"); /* ed2k status*/
	stats[1] = strtok (NULL,"\n"); /* server name */
	stats[2] = strtok (NULL,"\n"); /* server ip */
	stats[3] = strtok (NULL,"\n"); /* server port */
	stats[4] = strtok (NULL,"\n"); /* high or low id */
	stats[5] = strtok (NULL,"\n"); /* kad status */
	stats[6] = strtok (NULL,"\n"); /* dl */
	stats[7] = strtok (NULL,"\n"); /* ul*/
	stats[8] = strtok (NULL,"\n"); /* queue*/
	stats[9] = strtok (NULL,"\n"); /* clients*/
	stats[10] = strtok (NULL,"\n"); /* nick*/
	stats[11] = strtok (NULL,"\n"); /* total download */
	stats[12] = strtok (NULL,"\n"); /* total upload */
	stats[13] = strtok (NULL,"\n"); /* version */
	stats[14] = strtok (NULL,"\n"); /*Session download*/
	stats[15] = strtok (NULL,"\n"); /* Session upload */
	stats[16] = strtok (NULL,"\n"); /* aMule running Time */

	/* if amule isnt running say that and exit else print out the stuff
	 * [ToDo] States 0 & 2 mean offline/connecting not "not running"...
	 */
	
	//if amule uptime is 0, then its not running...
	if (strncmp(stats[16],"0",1) == 0 ) {
		perror("aMule is not running\n");
		exit(3);
	}
	
	
	if (strncmp(stats[0],"2",1) == 0)
		CreateLine(lines, 0 ,"aMule %s is connecting\n", stats[13]);
	else
		CreateLine(lines, 0, "aMule %s has been running for %s\n",
				stats[13], timeconv(stats[16]));

	
	
	if (strncmp(stats[0],"0",1) == 0 && strncmp(stats[5],"0",1) == 0)
		CreateLine(lines, 1, "%s is not connected ", stats[10]);
	else if (strncmp(stats[0],"0",1) == 0 && strncmp(stats[5],"0",1) != 0)
		CreateLine(lines, 1, "%s is connected to ", stats[10]);
	else
		CreateLine(lines, 1, "%s is connnnected to %s [%s:%s] with ", stats[10],
			stats[1], stats[2], stats[3]);
	
	
	if (strncmp(stats[5],"2",1) == 0) {
		if (strncmp(stats[4],"H",1) == 0)
			AppendToLine(lines, 1, "HighID | Kad: ok \n");
		else if (strncmp(stats[4],"L",1) == 0)
                        AppendToLine(lines, 1, "LowID | Kad: ok \n");
		else
			AppendToLine(lines, 1, "Kad: ok \n");
	} else if (strncmp(stats[5],"1",1) == 0) {
		if (strncmp(stats[4],"H",1) == 0)
			AppendToLine(lines, 1, "HighID | Kad: firewalled \n");
		else if (strncmp(stats[4],"L",1) == 0)
                        AppendToLine(lines, 1, "LowID | Kad: firewalled \n");
        	else
			AppendToLine(lines, 1, "Kad: firewalled \n");
	} else {
		if (strncmp(stats[4],"H",1) == 0)
			AppendToLine(lines, 1, "HighID | Kad: off \n");
		else if (strncmp(stats[4],"L",1) == 0)
                        AppendToLine(lines, 1, "LowID | Kad: off \n");
		else
			AppendToLine(lines, 1, "but running\n");
	}

	stats[11] = strdup(convbytes(stats[11]));
	stats[12] = strdup(convbytes(stats[12]));

	CreateLine(lines, 2, "Total Download: %s, Upload: %s\n",stats[11] , stats[12]);

	stats[15] = strdup(convbytes(stats[15]));
	stats[14] = strdup(convbytes(stats[14]));

	CreateLine(lines, 3, "Session Download: %s, Upload: %s\n",stats[14], stats[15]);

	CreateLine(lines, 4, "Download: %s kB/s, Upload: %s kB/s\n", stats[6], stats[7]);

	CreateLine(lines, 5, "Sharing: %s file(s), Clients on queue: %s\n", stats[9] , stats[8]);

#ifdef __GD__
	if (argc == 2 && strcmp(argv[1], "-o") == 0) {
		if (!readconfig(&config)) {
			perror("Could not read config file\n");
			exit(4);
		}

		if (!createimage(&config, lines)) {
			perror("Could not create image!\n");
			exit(5);
		}
		exit(0);
	}
#endif

	if (argc == 2 && strcmp(argv[1], "-p") == 0) {
		
		if (!readconfig(&config)) {
			perror("Could not read config file\n");
			exit(4);
		}

		if (!create_html(stats,lines,config.template)) {
			perror("Could not create the HTML Page.\n");
		}

#ifdef __GD__
		if (!createimage(&config, lines)) {
			perror("Could not create image!\n");
			exit(5);
		}
#endif


		exit(0);
	}
	for (i = 0; i <= 5; i++) {
		printf("%s", lines[i]);
		free(lines[i]);
	}
	free(buffer);
	exit(0);
}

