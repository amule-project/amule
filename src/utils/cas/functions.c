#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>

/* try (hard) to get correct path for aMule signature
 * !! it's caller's responsibility to free return value
 */
char *get_path(char *file)
{
	char *ret, *home;	/* caller should free return value */

	if ( (home = getenv("HOME")) == NULL) {
		/* if $HOME is not available try user database */
		uid_t uid;
		struct passwd *pwd;

		uid = getuid();
		pwd = getpwuid(uid);
		endpwent();

		if (pwd == NULL || pwd->pw_dir == NULL)
			return NULL;

		home = pwd->pw_dir;
	}

	/* get full path space */
	ret = malloc((strlen(home) + strlen(file) + 2) * sizeof(char));
	if (ret == NULL)
		return NULL;

	strcpy(ret, home);
	strcat(ret, "/");
	strcat(ret, file);

	return ret;
}

/*
 * this function is used to convert bytes to any to other units
 * nicer to the eye.
 *
 * return "Bad format" if could not convert string
 */
char *convbytes(char *input)
{
	char *units[] = { "bytes", "Kb", "Mb", "Gb", "Tb", "Pb" };
	char *endptr;
	static char output[50];
	float bytes;
	int i = 0;

        /* do proper conversion and check for errors */
        errno = 0;
        bytes = (float) strtod(input, &endptr);

        /* check bad string conversion or value out of range */
        if (*endptr != '\0' || errno == ERANGE)
                return "Bad format";

	/* this loop converts bytes and sets i to the appropriate
	 * index in 'units' array
	 * note: sizeof(units) / sizeof(char*) is the number of
	 * elements in 'units' array
	 */
	for (; i < (sizeof(units) / sizeof(char *)) - 1; i++) {
		if (bytes < 1024)
			break;
		bytes /= 1024;
	}

	snprintf(output, 50, "%.2f %s", bytes, units[i]);

	return output;
}
