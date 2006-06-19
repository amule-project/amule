/*
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
#include <stdarg.h>
#include <string.h>

#include "lines.h"

void CreateLine(char *lines[], int line, const char *format, ...)
{
	/* Guess we need no more than 80 bytes. */
	int n, size = 80;
	char *p;
	va_list ap;
	if ((p = malloc(size)) == NULL) {
		lines[line] = NULL;
		return;
	}
	while (1) {
		/* Try to print in the allocated space. */
		va_start(ap, format);
		n = vsnprintf(p, size, format, ap);
		va_end(ap);
		/* If that worked, set the line. */
		if (n > -1 && n < size) {
			lines[line] = p;
			return;
		}
		/* Else try again with more space. */
		if (n > -1)	/* glibc 2.1 */
			size = n+1;	/* precisely what is needed */
		else		/* glibc 2.0 */
			size *= 2;	/* twice the old size */
		if ((p = realloc(p, size)) == NULL) {
			lines[line] = NULL;
			return;
		}
	}
}

void AppendToLine(char *lines[], int line, const char *text)
{
	/* Are we trying to append to an empty line? */
	if (lines[line] == NULL)
		CreateLine(lines, line, text);
	else {
		/* Calculate the new required size... */
		int size = strlen(lines[line]) + strlen(text) + 1;
		if ((lines[line] = realloc(lines[line], size)) == NULL)
			return;
		/* ... and append the new text. */
		strcat(lines[line], text);
	}
}
