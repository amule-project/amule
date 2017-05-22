/*
 *  Name:         Config file functions
 *
 *  Purpose:      Read info from casrc ou create one if it doesnt exist
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

#include "configfile.h"
#include "functions.h"

#define STRINGIFY(x)	#x
#define STRINGIFY_EXPAND(x)	STRINGIFY(x)

#define MAX_CONF_ARG_LEN_STR	STRINGIFY_EXPAND(MAX_CONF_ARG_LEN)

#define MAX_CONF_KEY_LEN	12
#define MAX_CONF_KEY_LEN_STR	STRINGIFY_EXPAND(MAX_CONF_KEY_LEN)

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
		"# source_image - image where the text will be written\n",
		"# *_line - x,y,[1/0] enabled or disabled\n\n",
		"font /usr/share/fonts/corefonts/times.ttf\n",
		"font_size 10.5\n",
		"source_image /usr/share/cas/stat.png\n",
		"first_line 23,17,1\n",
		"second_line 23,34,1\n",
		"third_line 23,51,1\n",
		"fourth_line 23,68,1\n",
		"fifth_line 23,85,1\n",
		"sixth_line 23,102,1\n",
		"seventh_line 23,119,1\n",
		"template /usr/share/cas/tmp.html\n"
		"img_type 0\n"
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
	int i = 0;
	char lines[IMG_TEXTLINES][13] = {
		"first_line",
		"second_line",
		"third_line",
		"fourth_line",
		"fifth_line",
		"sixth_line",
		"seventh_line"
	};

	path = get_path("casrc");
	if (path == NULL) {
		return 0;
	}

	if ((conf = fopen(path, "r")) == NULL) {
		printf("Unable to open %s. Creating it.\n", path);
		free(path);
		if (!writeconfig()) {
			perror("readconfig: unable to create initial config file\n");
		}
		return 0;
	}
	free(path);

	buffer[0] = 0;
	while (!feof(conf)) {
		// Jacobo221 - [ToDo] Only first char per line is comment...
		if (fgets (buffer,120,conf)) {
			if (buffer[0] != '#') {
				/* Only two fileds per line */
				sscanf(buffer, "%" MAX_CONF_KEY_LEN_STR "s %*" MAX_CONF_ARG_LEN_STR "s", option);
				fflush (stdout);
		// Jacobo221 - [ToDo] So lines can't be swapped...
				if (strcmp(option, "font") == 0) {
					sscanf(buffer, "%*" MAX_CONF_KEY_LEN_STR "s %" MAX_CONF_ARG_LEN_STR "s", config->font);
				}
				if (strcmp(option, "font_size") == 0) {
					sscanf(buffer, "%*" MAX_CONF_KEY_LEN_STR "s %10f", &config->size);
				}
				if (strcmp(option, "source_image") == 0) {
					sscanf(buffer, "%*" MAX_CONF_KEY_LEN_STR "s %" MAX_CONF_ARG_LEN_STR "s", config->source);
				}
				if (strcmp(option, "template") == 0) {
					sscanf(buffer, "%*" MAX_CONF_KEY_LEN_STR "s %" MAX_CONF_ARG_LEN_STR "s", config->template);
				}
				if (strcmp(option, "img_type") == 0) {
					sscanf(buffer, "%*" MAX_CONF_KEY_LEN_STR "s %1d", &config->img_type);
				}

				for (i = 0; i < IMG_TEXTLINES; i++) {
					if (strcmp(option, lines[i]) == 0) {
						sscanf(buffer,
							"%*" MAX_CONF_KEY_LEN_STR "s %4d,%4d,%4d",
							&config->x[i], &config->y[i],
							&config->enabled[i]);
					}
				}
			}
		}
	}

	fclose(conf);

	return 1;
}

