#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "configfile.h"
#include "functions.h"

int writeconfig(void)
{
	FILE *config;
	char *path;
	int i;
	char *def[] = { "# cas config file\n",
		"#\n",
		"# font - full path to a ttf font\n",
		"# font_size - size the font\n",
		"# source_image - image where the text will be writen\n",
		"# *_line - x,y,[1/0] enabled or disabled\n\n",
		"font /usr/share/fonts/corefonts/times.ttf\n",
		"font_size 10.5\n",
		"source_image stat.png\n",
		"first_line 23,19,1\n",
		"second_line 23,36,1\n",
		"third_line 23,54,1\n",
		"fourth_line 23,72,1\n",
		"fifth_line 23,89,1\n",
		"sixth_line 23,106,1\n"
	};

	path = get_path(".aMule/casrc");
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

int readconfig(CONF *config)
{
	char buffer[120], option[15], *path;
	FILE *conf;
	int i = 0, ler;
	char lines[6][12] = { "first_line",
		"second_line",
		"third_line",
		"fourth_line",
		"fifth_line",
		"sixth_line"
	};

	path = get_path(".aMule/casrc");
	if (path == NULL)
		return 0;

	if ((conf = fopen(path, "r")) == NULL) {
		printf("Unable to open %s\nCreating it. ", path);
		free(path);
		if (!writeconfig()) {
			printf("readconfig: unable to create initial config file");
		}
		return 0;
	}
	free(path);

	buffer[0] = 0;
	while (!feof(conf)) {
		ler = fgetc(conf);
		if (ler != 10) {
			sprintf(buffer, "%s%c", buffer, ler);
		} else {
			if (buffer[0] != '#') {
				sscanf(buffer, "%s %*s", option);

				if (strcmp(option, "font") == 0)
					sscanf(buffer, "%*s %s", config->font);
				if (strcmp(option, "font_size") == 0)
					sscanf(buffer, "%*s %f", &config->size);
				if (strcmp(option, "source_image") == 0)
					sscanf(buffer, "%*s %s", config->source);
				for (i = 0; i <= 6; i++)
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
