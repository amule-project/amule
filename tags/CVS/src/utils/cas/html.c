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
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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


int create_html(char stats[20][80], char *lines[6], char template[120])
{
	// strings
	char version[25], upload[25], download[25];
	// ficheiro
	int fd,ler,t,size;
	char *mem;
	struct stat sb;
	FILE *temp;


	snprintf(version, 25, "cas %s", CAS_VERSION);
	snprintf(upload, 25, "%s kB/s", stats[6]);
	snprintf(download, 25, "%s kB/s", stats[5]);


	char *search[] = {"#VERSION#", "#CLIENT#", "#NICK#", "#UPLOADRATE#" ,
		"#DOWNLOADRATE#" , "#QUEUE#" , "#NUMSHARE#" , "#SESSIONUP#" ,
		"#SESSIONDOWN#" , "#TOTALUP#", "#TOTALDOWN#" , "#SERVER#" , "#IP#",
		"#PORT#" };

	char *repl[] = { version , lines[0] , stats[9] , upload , download ,
		stats[7] , stats[8] , stats[14] , stats[13] , stats[11] , stats[10] ,
		stats[1] , stats[2] , stats[3] };

	// get some memory to read the template into
	if ((fd = open (template, O_RDONLY)) < 0)
	{
		printf("\n\n%s\n",template);
		perror("Could not open file");
		exit (43);
	}

	if(fstat(fd, &sb) < 0){
		perror("Could not stat file");
		exit(43);
	}

	size = sb.st_size*2;

	mem = malloc(size);

	if (mem==NULL)
	{
		printf("coulnt malloc\n");
		exit(44);
	}

	memset(mem, '\0', (size));
	close(fd);

	// read the template to the memory
	temp = fopen(template,"r");
	while ((ler=fgetc(temp)) != EOF)
	{
		sprintf(mem,"%s%c",mem,ler);
	}

	for (t=0;t<=13;t++)
	{
		char *mem2 = mem;
		mem = replace(search[t],repl[t],mem2,size);
		free(mem2);
	}
	//		mem = replace(search[t],repl[t],mem,size);


	printf("%s",mem);

	fclose(temp);
	free(mem);

	return 0;
}

