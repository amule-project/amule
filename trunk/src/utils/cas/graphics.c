/*
 *  Name:         Graphics functions
 *
 *  Purpose:      All the functions that are used to create the Online Signature Image
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

#ifdef __GD__

#include <stdlib.h>

#include <gd.h>

#include "functions.h"
#include "configfile.h"
#include "graphics.h"

#ifdef __APPLE__
	#define CAS_DIR_SEPARATOR	"/"
#elif defined(__WIN32__)
	#define CAS_DIR_SEPARATOR	"\\"
#else
	#define CAS_DIR_SEPARATOR	"/"
#endif

/*
 * this is the funcion that writes the text to the image.
 * almost everything is taken from libgd examples
 */
int createimage(CONF *config, char *lines[IMG_TEXTLINES], char *path_for_picture)
{
	FILE *in, *out;
	char *path;
	gdImagePtr im;
	int white, i;
	int brect[8];

	if ( (in = fopen(config->font, "r")) == NULL) {
		perror("font not found\ncheck casrc\n");
		return 0;
	}
	fclose(in);

	if ( (in = fopen(config->source, "rb")) == NULL) {
		perror("source_image not found\ncheck casrc\n");
		return 0;
	}

	im = gdImageCreateFromPng(in);
	white = gdImageColorResolve(im, 255, 255, 255);

	for (i = 0; i <= (IMG_TEXTLINES - 1); i++) {
		if (config->enabled[i] == 1)
			gdImageStringFT(im, &brect[0], white, config->font, config->size,
					0., config->x[i], config->y[i], lines[i]);
	}
	
//	printf (path_for_picture);
		if (path_for_picture == NULL) {
		path = get_path("aMule-online-sign.png");
	} else {
		if (path_for_picture[strlen(path_for_picture)-1] != CAS_DIR_SEPARATOR[0]) {
			strcat(path_for_picture, CAS_DIR_SEPARATOR);
		}
		strcat(path_for_picture, "aMule-online-sign.png");
		path = path_for_picture;
	}

	if (path == NULL) {
		perror("could not get PNG path\n");
		return 0;
	}
	out = fopen(path, "w");
	free(path);

	gdImagePng(im, out);
	fclose(out);
	printf("Online Signature picture created.\n");
	gdImageDestroy(im);

	return 1;
}

#endif
