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
					0.0, config->x[i], config->y[i], lines[i]);
	}
	
	if (config->img_type==0) {
		path = get_amule_path("aMule-online-sign.png", 0, path_for_picture);
	} else {
		path = get_amule_path("aMule-online-sign.jpg", 0, path_for_picture);
	}
		
	if (path == NULL && config->img_type==0) {
		perror("could not get PNG path\n");
		return 0;
	} else if (path == NULL) {
		perror("could not get JPG path\n");
		return 0;
	}
	out = fopen(path, "w");
	free(path);
	
	if (config->img_type==0) {
		gdImagePng(im, out);
	} else {
		gdImageJpeg(im, out, -1);
	}
	
	fclose(out);
	printf("Online Signature picture created.\n");
	gdImageDestroy(im);

	return 1;
}

#endif

