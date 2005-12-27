/*
 *  Name:         Config file functions
 *
 *  Purpose:      Read info from casrc ou create one if it doesnt exist
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

#include "configfile.h"
#include "functions.h"

int writeconfig(void)
{
	FILE *config;
	char *path;
	unsigned int i;
	char *def[] = {
		"# cas config file\n",
		"#\n",
		"# font - full path to a ttf font\n",
		"# font_size - size the font\n",
		"# source_image - image where the text will be writen\n",
		"# *_line - x,y,[1/0] enabled or disabled\n\n",
		"font /usr/share/fonts/corefonts/times.ttf\n",
		"font_size 10.5\n",
		"source_image /usr/share/pixmaps/stat.png\n",
		"first_line 23,19,1\n",
		"second_line 23,36,1\n",
		"third_line 23,54,1\n",
		"fourth_line 23,72,1\n",
		"fifth_line 23,89,1\n",
		"sixth_line 23,106,1\n",
		"template /usr/share/pixmaps/tmp.html\n"
	};

	path = get_path("casrc");
	if (path == NULL)
		return 0;

	if ( (config = fopen(path, "w")) == NULL)
		return 0;

	for (i = 0; i < sizeof(def) / sizeof(char *); i++)
		fprintf(config, "%s", def[i]);

	fclose(config);

	printf("%s created, please edit it and then rerun cas\n", path);
	free(path);

	return 1;
}

/* Jacobo221 - [ToDo] There should be a check for corrupt config files! */
int readconfig(CONF *config)
{
	char buffer[120], option[15], *path;
	FILE *conf;
	int i = 0, ler;
	char lines[IMG_TEXTLINES][12] = {
		"first_line",
		"second_line",
		"third_line",
		"fourth_line",
		"fifth_line",
		"sixth_line"
	};

	path = get_path("casrc");
	if (path == NULL)
		return 0;

	if ((conf = fopen(path, "r")) == NULL) {
		printf("Unable to open %s. Creating it.\n", path);
		free(path);
		if (!writeconfig()) {
			perror("readconfig: unable to create initial config file");
		}
		return 0;
	}
	free(path);

	buffer[0] = 0;
	while (!feof(conf)) {
		ler = fgetc(conf);
		if (ler == 13); /* Jacobo221 - Make it DOS compatible */
		else if (ler != 10) {
			char tmpbuffer[sizeof(buffer)];
			snprintf(tmpbuffer, sizeof(buffer), "%s%c", buffer, ler);
			memcpy(buffer, tmpbuffer, sizeof(buffer));
		} else {
			/* Jacobo221 - [ToDo] Only first char per line is comment... */
			if (buffer[0] != '#') {
				/* Only two fileds per line */
				sscanf(buffer, "%s %*s", option);

				/* Jacobo221 - [ToDo] So lines can't be swapped... */
				if (strcmp(option, "font") == 0)
					sscanf(buffer, "%*s %s", config->font);
				if (strcmp(option, "font_size") == 0)
					sscanf(buffer, "%*s %f", &config->size);
				if (strcmp(option, "source_image") == 0)
					sscanf(buffer, "%*s %s", config->source);
				if (strcmp(option, "template") == 0)
					sscanf(buffer, "%*s %s", config->template);
					
				for (i = 0; i <= IMG_TEXTLINES; i++)
					if (strcmp(option, lines[i]) == 0)
						sscanf(buffer,
								"%*s %d,%d,%d",
								&config->x[i], &config->y[i],
								&config->enabled[i]);
			}
			buffer[0] = 0;
		}
	}

	fclose(conf);

	return 1;
}
