/*
 * This file is part of the aMule Project.
 *
 * Copyright (c) 2011 aMule Team ( admin@amule.org / http://www.amule.org )
 *
 * Any parts of this program derived from the xMule, lMule or eMule project,
 * or contributed by third-party developers are copyrighted by their
 * respective authors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#ifndef _XOPEN_SOURCE
#	define _XOPEN_SOURCE	600
#endif

#include <string.h>			// Needed for strerror_r() and size_t

#ifdef HAVE_ERRNO_H
#	include <errno.h>			// Needed for errno
#endif

#ifndef HAVE_STRERROR_R
#	ifdef HAVE_STRERROR

/* Replacement strerror_r() function for systems that don't have any.
   Note that this replacement function is NOT thread-safe! */
int mule_strerror_r(int errnum, char *buf, size_t buflen)
{
	char *tmp;
	if ((buf == NULL) || (buflen == 0)) {
		errno = ERANGE;
		return -1;
	}
	tmp = strerror(errnum);
	if (tmp == NULL) {
		errno = EINVAL;
		return -1;
	} else {
		strncpy(buf, tmp, buflen - 1);
		buf[buflen - 1] = '\0';
		if (strlen(tmp) >= buflen) {
			errno = ERANGE;
			return -1;
		}
	}
	return 0;
}

#	else

/* No way to get error description */
int mule_strerror_r()
{
#ifdef HAVE_ERRNO_H
	errno = ENOSYS;	/* not implemented */
#endif
	return -1;
}

#	endif
#else
#	ifdef STRERROR_R_CHAR_P

/* Replacement strerror_r() function for systems that return a char*. */
int mule_strerror_r(int errnum, char *buf, size_t buflen)
{
	char *tmp = strerror_r(errnum, buf, buflen);
	if (tmp == NULL) {
		errno = EINVAL;
		return -1;
	} else if (tmp != buf) {
		if ((buf == NULL) || (buflen == 0)) {
			errno = ERANGE;
			return -1;
		} else {
			strncpy(buf, tmp, buflen - 1);
			buf[buflen - 1] = '\0';
			if (strlen(tmp) >= buflen) {
				errno = ERANGE;
				return -1;
			}
		}
	}
	return 0;
}

#	else

/* Nothing to do, library strerror_r() is XSI-compliant. */
int mule_strerror_r(int errnum, char *buf, size_t buflen)
{
	return strerror_r(errnum, buf, buflen);
}
#	endif
#endif
