/*
 *  Name:         Shared functions
 *
 *  Purpose:      Functions that are used various times in cas
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
#include <string.h>

/* XXX This needs to be replaced so that we use
 * autoconf to detect the target OS -- As of now
 * it should compile fine in other non UNIX-like
 * platforms, at the expense of only being sure
 * that UNIX-specific code will be compiled if
 * using GCC (which defines those __unix__ macros)
 * autoconf defines should be in place to test for
 * functions like getpwuid() in UNIX-like systems
 * instead of doing this...
 */
#if defined(unix) || defined(__unix__) || defined(__unix)
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <fcntl.h>
#define CAS_UNIX
#endif

/* try (hard) to get correct path for aMule signature
 * !! it's caller's responsibility to free return value
 */
char *get_path(char *file)
{
	char *ret, *home;	/* caller should free return value */
	static char *saved_home = NULL;
	static size_t home_len = 0;

	if (saved_home == NULL) {
		/* get home directory */
		if ( (home = getenv("HOME")) == NULL) {
#ifndef CAS_UNIX
			return NULL;
#else
			/* if $HOME is not available try user database */
			uid_t uid;
			struct passwd *pwd;

			uid = getuid();
			pwd = getpwuid(uid);
			endpwent();

			// if (pwd == NULL || pwd->pw_dir == NULL) could brake on left-handed compilers
			/* XXX
			 * Section 6.5.14 of ANSI C specs (grab C99 at http://www.nirvani.net/docs/ansi_c.pdf)
			 * states this:
			 * "Unlike the bitwise | operator, the || operator _guarantees_ left-to-right
			 * evaluation; there is a sequence point after the evaluation of the first
			 * operand. If the first operand compares unequal to 0, the second operand
			 * _is not evaluated_."
			 *
			 * I'm going to revert it and wait until Jacobo or whoever changed it
			 * screams loudly, tries to kill me, and explains why this has to be
			 * this way. Maybe pre-ansi compilers? Doubtful.
			 *
			 * - Unleashed
			 */
			if (pwd == NULL || pwd->pw_dir == NULL)
				return NULL;

			home = pwd->pw_dir;
#endif /* CAS_UNIX */
		}

		/* save the result for future calls */
		home_len = strlen(home);
		if ( (saved_home = strdup(home)) == NULL)
			return NULL;
	}

	/* get full path space */

	/* Unleashed - Guys... you broke this and it was OK
	 * "+ 2" means "plus '/' and '\0'"
	 */
	ret = malloc((home_len + strlen(file) + 2) * sizeof(char));
	if (ret == NULL)
		return NULL;

	strcpy(ret, saved_home);
	strcat(ret, "/");
	strcat(ret, file);
	/* the string is guaranteed to be null-terminated
	 * so no need to do this...
	 * ret[total_len] = '\0';
	 */

	return ret;
}

/*
 * this function is used to convert bytes to any other unit
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
	unsigned int i = 0;

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

char *replace(char *search, char *replace, char *template, int size)
{
	int read;
	unsigned int i=0,a;
	char buffer[255];
	char *output;

	output = malloc(size);

	if (output==NULL)
	{
		printf("couldn't malloc\n");
		exit(44);
	}

	//memset(output, '\0', (size));
	output[0] = 0;

	buffer[0] = 0;

	//while ((read=fgetc(template)) != EOF)
	for (a=0;a<=strlen(template);a++)
	{
		read=template[a];

		snprintf(buffer,255,"%s%c",buffer,read);

		if(read==search[i]) 
		{
			i++;
		}
		else 
		{ 
			i=0;
			snprintf(output,size,"%s%s",output,buffer);
			buffer[0]=0;
		}               

		if(i == strlen(search))
		{
			snprintf(output,size,"%s%s",output,replace);
			i=0;
			buffer[0]=0;
		}
	}

//	printf("%s",output);
//	printf("hy\n");
	return output;
}

char *timeconv(char *input)
{
	int count = atoi(input);
	static char ret[50];

	if (count < 0)
		snprintf (ret,50,"?");
	else if (count < 60)
		snprintf (ret,50,"%02i %s", count, "secs" );
	else if (count < 3600)
		snprintf (ret,50,"%i:%02i %s", count/60, (count % 60), "mins" );
	else if (count < 86400)
		snprintf (ret,50,"%i:%02i %s", count/3600, (count % 3600)/60, "h" );
	else
		snprintf (ret,50,"%i %s %02i %s", count/86400, "D" , (count % 86400) / 3600, "h" );

	return (ret);
}
