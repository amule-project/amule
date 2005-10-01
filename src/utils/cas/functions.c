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
#include <errno.h>


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

#ifdef __APPLE__
	#include <CoreServices/CoreServices.h>
	#define CAS_DIR_SEPARATOR	"/"
#elif defined(__WIN32__)
	#include <winerror.h>
	#include <shlobj.h>
	#define CAS_DIR_SEPARATOR	"\\"
#else
	#define CAS_DIR_SEPARATOR	"/"
	#if defined(unix) || defined(__unix__) || defined(__unix)
		#include <unistd.h>
		#include <pwd.h>
		#include <fcntl.h>
		#define CAS_UNIX
	#endif
#endif

/* try (hard) to get correct path for aMule signature
 * !! it's caller's responsibility to free return value
 */
char *get_path(char *file)
{
	char *ret;	/* caller should free return value */
	static char *saved_home = NULL;
	static size_t home_len = 0;

	if (saved_home == NULL) {
#ifdef __APPLE__

		char home[PATH_MAX];
		home[0] = '\0';

		FSRef fsRef;
		if (FSFindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &fsRef) == noErr) {
			CFURLRef urlRef = CFURLCreateFromFSRef(NULL, &fsRef);
			if (urlRef != NULL) {
				if (CFURLGetFileSystemRepresentation(urlRef, true, home, sizeof(home))) {
					strcat(home, CAS_DIR_SEPARATOR "aMule");
				}
				CFRelease(urlRef) ;
			}
		}

#elif defined(__WIN32__)

		LPITEMIDLIST pidl;
		char home[MAX_PATH];
		home[0] = '\0';

		HRESULT hr = SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);

		if (SUCCEEDED(hr)) {
			if (SHGetPathFromIDList(pidl, home)) {
				strcat(home, CAS_DIR_SEPARATOR "aMule");
			}
		}

		if (pidl) {
			LPMALLOC pMalloc;
			SHGetMalloc(&pMalloc);
			if (pMalloc) {
				pMalloc->Free(pidl);
				pMalloc->Release();
			}
		}


#else
		char *home;

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
		strcat(home, CAS_DIR_SEPARATOR ".aMule");

#endif /* !__APPLE__ && !__WIN32__ */

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
	ret[home_len] = CAS_DIR_SEPARATOR[0];
	ret[home_len+1] = '\0';
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

void replace(char *tmpl, const char *search, const char *replace)
{
	char *dest   = NULL;
	char *retStr = NULL;
	int	befLen,srchLen,repLen,totLen;

	/* returning the 'tmpl' if 'search' is NULL */
  if (NULL == tmpl || NULL == search) /* || NULL == replace) */
  {
		return;
	}

	while (1)
	{
		/* if 'search' is found in 'tmpl' */
		retStr = strstr(tmpl, search);
		if (NULL == retStr)
		{
			return;
		}

		totLen	= strlen(tmpl);
		befLen	= (int)(retStr - tmpl);
		srchLen = strlen(search);
		repLen	= strlen(replace);

		/* dynamic buffer creation... */
		dest = (char*)malloc(totLen + 1 + repLen - srchLen);
		if (NULL == dest)
			return;

		/* copy the before buffer */
		strncpy(dest, tmpl, befLen);
		/* copy the replace string */
		memcpy((dest+befLen), replace, repLen); /* strcat(dest, replace); */
		/* copy the after buffer */
		memcpy((dest+befLen+repLen), &tmpl[befLen + srchLen], strlen(&tmpl[befLen + srchLen])); /*strcat(dest, &tmpl[befLen + repLen]); */

		/* now replace the template string with the resulting and search it again */
		strcpy(tmpl, dest);
		/* we need this, because we're modifying the 'tmpl' instead of creating a new one (so we need to update the position of the null char) */
		tmpl[totLen - srchLen + repLen] = '\0';
		/* clean up... */
		free(dest);
	}
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
