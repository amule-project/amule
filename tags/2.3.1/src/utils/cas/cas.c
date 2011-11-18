/*
 *  Name:         Main cas file 
 *
 *  Purpose:      aMule Statistics
 *
 *  Author:       Pedro de Oliveira <falso@rdk.homeip.net>
 *
 *  Copyright (c) 2004-2011 Pedro de Oliveira ( falso@rdk.homeip-net )
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
#include <getopt.h>

#include "version.h"
#include "configfile.h"
#include "functions.h"
#include "graphics.h"
#include "html.h"
#include "lines.h"

#ifdef HAVE_CONFIG_H
	#include "config.h"	// For HAVE_GETOPT_LONG
#endif

#ifndef HAVE_GETOPT_LONG
/* Code from getopt_long.h - getopt_long() for systems that lack it
  Copyright (c) 2001-2011 Arthur de Jong, GPL 2 and later */
# define no_argument 0
# define required_argument 1
# define optional_argument 2
 
struct option {
	const char *name;
	int has_arg;
	int *flag;
	int val;
};
 
int getopt_long(int argc,
		char * const argv[],
		const char *optstring,
		const struct option *longopts,
		int *longindex);
#endif

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

static struct option long_options[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "html", optional_argument, NULL, 'p' },
	{ "picture", optional_argument, NULL, 'o' },
	{ "config-dir", required_argument, NULL, 'c' },
	{ NULL, 0, NULL, 0 }
};

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
			"-o, --picture, -P\tWrites the online signature picture\n"
#endif
			"-p, --html, -H\t\tHTML Page with stats and picture\n"
			"-c, --config-dir\tSpecifies a config-dir different from home\n"
			"-h, --help\t\tThis help you're reading\n", CAS_VERSION, myname);
}

#ifndef HAVE_GETOPT_LONG

/* Code from getopt_long.c - getopt_long() for systems that lack it
  Copyright (c) 2001-2011 Arthur de Jong, GPL 2 and later
  Slightly edited for the sake of clarity by Gaznevada */

int getopt_long(int argc,
                char * const argv[],
                const char *optstring,
                const struct option *longopts,
                int *longindex)
{
int i;
int length;
if ( (optind > 0) && (optind < argc) &&
     (strncmp(argv[optind],"--",2) == 0) &&
     (argv[optind][2] != '\0') ) {
        for (i = 0; longopts[i].name != NULL; i++) {
                length = strlen(longopts[i].name);
                if (strncmp(argv[optind]+2,longopts[i].name,length) == 0) {
                        if ( (longopts[i].has_arg == no_argument) && (argv[optind][2+length] == '\0') ) {
                                optind++;
                                return longopts[i].val;
                        }
                else if ( (longopts[i].has_arg == required_argument) && (argv[optind][2+length] == '=') ) {
                        optarg=argv[optind]+3+length;
                        optind++;
                        return longopts[i].val;
                        }
                else if ( (longopts[i].has_arg == required_argument) && (argv[optind][2+length] == '\0') ) {
                        optarg=argv[optind+1];
                        optind+=2;
                        return longopts[i].val;
                        }
                else if ( (longopts[i].has_arg == optional_argument) && (argv[optind][2+length] == '=') ) {
                        optarg=argv[optind]+3+length;
                        optind++;
                        return longopts[i].val;
                        }
                else if ( (longopts[i].has_arg==optional_argument) && (argv[optind][2+length] == '\0') ) {
                        optind++;
                        return longopts[i].val;
                        }
                }
        }
}
return getopt(argc,argv,optstring);
}

#endif // HAVE_GETOPT_LONG

int main(int argc, char *argv[])
{
	/* Declaration of variables */
	FILE *amulesig;
	int use_out_pic = 0;
	int use_page = 0;
	char *config_path=NULL;
	char *path;
	char *stats[20];
	char *lines[IMG_TEXTLINES];
	long lSize;
	char * buffer;
	int i;
	int c;
	int errflag = 0;
	char *path_for_picture=NULL;
	char *path_for_html=NULL;
	CONF config;
	time_t lt;
	struct tm *ltp;
	char arr[20];

	while ((c = getopt_long (argc, argv, "c:P:H:hpo", long_options, NULL)) != -1)

		switch (c)
		{
		case 'c':
			config_path=optarg;
			break;
		case 'h':
			usage(argv[0]);
			exit(0);
		case 'H':
		case 'p':
			use_page=1;
			if (optarg != NULL) {
				path_for_html = optarg;
			}
			break;
		case 'P':
		case 'o':
			use_out_pic=1;
			if (optarg != NULL) {
				path_for_picture = optarg;
			}
			break;
		case '?':
			errflag++;
		}
		if (errflag) {
			usage(argv[0]);
               		exit (2);
          	}

	/* get amulesig path */

	path = get_amule_path("amulesig.dat", 1, config_path);

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
	if (0 == lSize) {
		perror("aMule signature file is 0 Byte, exiting.\n");
		exit(2);
	}
	rewind (amulesig);
	buffer = (char*) malloc (lSize);
	if (buffer == NULL) {
		perror("Could not create buffer\n");
		exit (2);
	}
	if (fread(buffer,1,lSize,amulesig)){}	// // prevent GCC warning
	fclose(amulesig);

	stats[0] = strtok (buffer,"\n");
	for (i=1;i<17;i++) {
		stats[i] = strtok (NULL,"\n");
		if (NULL == stats[i]) {
			perror("Too few fields in aMule signature file, exiting.\n");
			exit(2);
		}
	}

	// local time stored as stats[17]
	lt = time(NULL);
	ltp = localtime(&lt);
	strftime(arr, 20, "%b %d %Y, %H:%M", ltp);

	// if amule isn't running say that and exit else print out the stuff
	
	// if amule uptime is 0, then its not running...
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
		CreateLine(lines, 1, "%s is connected to %s [%s:%s] with ", stats[10],
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

	CreateLine(lines, 6, "Time: %s\n", arr);

#ifdef __GD__
	if (use_out_pic == 1) {
		if (!readconfig(&config)) {
			perror("Could not read config file\n");
			exit(4);
		}

		if (!createimage(&config, lines, path_for_picture)) {
			perror("Could not create image!\n");
			exit(5);
		}
		exit(0);
	}
#endif

	if (use_page == 1) {
		
		if (!readconfig(&config)) {
			perror("Could not read config file\n");
			exit(4);
		}

		if (!create_html(stats,lines,config.template, path_for_html)) {
			perror("Could not create the HTML Page.\n");
		}

#ifdef __GD__
		if (!createimage(&config, lines, path_for_picture)) {
			perror("Could not create image!\n");
			exit(5);
		}
#endif


		exit(0);
	}
	for (i = 0; i <= 6; i++) {
		printf("%s", lines[i]);
		free(lines[i]);
	}
	free(buffer);
	exit(0);
}

