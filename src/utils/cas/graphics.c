#ifdef __GD__

#include <stdlib.h>

#include <gd.h>

#include "functions.h"
#include "configfile.h"

/*
 * this is the funcion that writes the text to the image.
 * almost everything is taken from libgd examples
 */
int createimage(CONF *config, char lines[6][80])
{
	FILE *in, *out;
	char *path;
	gdImagePtr im;
	int white, i;
	int brect[8];

	if ( (in = fopen(config->font, "r")) == NULL) {
		printf("font not found\ncheck casrc\n");
		return 0;
	}
	fclose(in);

	if ( (in = fopen(config->source, "rb")) == NULL) {
		printf("source_image not found\ncheck casrc\n");
		return 0;
	}

	im = gdImageCreateFromPng(in);
	white = gdImageColorResolve(im, 255, 255, 255);

	for (i = 0; i <= 5; i++) {
		if (config->enabled[i] == 1)
			gdImageStringFT(im, &brect[0], white, config->font, config->size,
					0., config->x[i], config->y[i], lines[i]);
	}

	path = get_path(".aMule/aMule-online-sign.png");
	if (path == NULL) {
		printf("could not get PNG path\n");
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
