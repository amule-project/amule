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
 *  51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef CAS_FUNCTIONS_H
#define CAS_FUNCTIONS_H

char *get_path(char *file);
char *convbytes(char *input);
char *timeconv(char *input);
void replace(char *tmpl, const char *search, const char *replace);

#endif
// File_checked_for_headers
