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
// Jacobo221 - [ToDo] port to DOS
char *get_path(char *file)
{
	char *ret, *home;	/* caller should free return value */
	static char *saved_home = NULL;
	static size_t home_len = 0;
	static size_t total_len = 0
	
	if (saved_home == NULL) {
		/* get home directory */
		if ( (home = getenv("HOME")) == NULL) {
			/* if $HOME is not available try user database */
			uid_t uid;
			struct passwd *pwd;

			uid = getuid();
			pwd = getpwuid(uid);
			endpwent();

			// if (pwd == NULL || pwd->pw_dir == NULL) could brake on left-handed compilers
			if (pwd == NULL)
				return NULL;
			if (pwd->pw_dir == NULL)
				return NULL;

			home = pwd->pw_dir;
		}

		/* save the result for future calls */
		home_len = strlen(home);
		if ( (saved_home = strdup(home)) == NULL)
			return NULL;
	}

	/* get full path space */
	// Kry - Guys... do 'man malloc'
	total_len = (home_len + strlen("/") + strlen(file)) * sizeof(char) + 1 /* for '\0' */ ;
	ret = malloc(total_len);
	if (ret == NULL)
		return NULL;

	strcpy(ret, saved_home);
	strcat(ret, "/");
	strcat(ret, file);
	ret[total_len] = '\0';

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
