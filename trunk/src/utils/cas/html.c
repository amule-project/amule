/*
 *  Name:         HTML creation functions 
 *
 *  Purpose:      Create a nice HTML page with all the statistics
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
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#include "html.h"
#include "functions.h"
#include "version.h"
#ifdef __APPLE__
	#define CAS_DIR_SEPARATOR	"/"
#elif defined(__WIN32__)
	#define CAS_DIR_SEPARATOR	"\\"
#else
	#define CAS_DIR_SEPARATOR	"/"
#endif

int create_html(char *stats[20], char *lines[6], char template[120], char *path_for_html)
{
	/* Strings */
	char *path = NULL;
	char version[25], upload[25], download[25];
	char *search[] = {"#VERSION#", "#CLIENT#", "#NICK#", "#UPLOADRATE#" ,
		"#DOWNLOADRATE#" , "#QUEUE#" , "#NUMSHARE#" , "#SESSIONUP#" ,
		"#SESSIONDOWN#" , "#TOTALUP#", "#TOTALDOWN#" , "#SERVER#" , "#IP#",
		"#PORT#" };

	snprintf(version, 25, "cas %s", CAS_VERSION);
	snprintf(upload, 25, "%s kB/s", stats[7]);
	snprintf(download, 25, "%s kB/s", stats[6]);

	char *repl[] = { version , lines[0] , stats[10] , upload , download ,
		stats[8] , stats[9] , stats[15] , stats[14] , stats[12] , stats[11] ,
		stats[1] , stats[2] , stats[3] };

	/* get some memory to read the template into */
	int fdTmpl;
	if ((fdTmpl = open(template, O_RDONLY)) < 0)
	{
		printf("\n\n%s\n",template);
		perror("Could not open file");
		exit (43);
	}
	
	struct stat sb;
	if (fstat(fdTmpl, &sb) < 0)
	{
		perror("Could not stat file");
		exit(43);
	}
	close(fdTmpl);

	/* 2 times the size of the template should be enough */
	/* st_size is defined as off_t, but size_t seems more reasonable */
	size_t size = sb.st_size*2;
	char *mem = calloc(size, 1);
	if (NULL == mem)
	{
		perror("Could not calloc\n");
		exit(44);
	}

	/* read the template into the memory */
	size_t len = 0;
	int ler;
	FILE *fTmpl = fopen(template,"r");
	while ((ler=fgetc(fTmpl)) != EOF && len+1 < size)
	{
		mem[len++] = ler;
	}
	fclose(fTmpl);
	
	/* printf ("HTML: %s\n", mem); */

	int t;
	for (t=0; t<=13; t++)
	{
		/* replace the special tags */
		replace(mem, search[t], repl[t]);
	}

	/* printf("FINAL: %s\n",mem); */

	if (path_for_html == NULL) {
		path = get_path("aMule-online-sign.html");
	} else {
		if (path_for_html[strlen(path_for_html)-1] != CAS_DIR_SEPARATOR[0]) {
			strcat(path_for_html, CAS_DIR_SEPARATOR);
		}
		strcat(path_for_html, "aMule-online-sign.html");
		path = path_for_html;
	}

	if (NULL == path)
	{
		perror("could not get the HTML path\n");
		free(mem);
		return 0;
	}
	
	FILE *fHTML = NULL;
	if ((fHTML = fopen(path, "w")) == NULL)
	{
		perror("Unable to create file\n");
		free(path);
		free(mem);
		exit(44);
	}
	free(path);

	fprintf(fHTML, "%s", mem);
	fclose(fHTML);
	free(mem);
	
	printf("HTML file created.\n");

	return 1;
}
