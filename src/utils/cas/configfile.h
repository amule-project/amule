/*
 *  Name:         Config file functions
 *
 *  Purpose:      Read info from casrc ou create one if it doesnt exist
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


#ifndef CAS_CONFIGFILE_H
#define CAS_CONFIGFILE_H

#define IMG_TEXTLINES 6

typedef struct {
        char font[120];
        char source[120];
	char template[120];
        int x[6];
        int y[6];
        int enabled[6];
        float size;
} CONF;

int writeconfig(void);
int readconfig(CONF *config);

#endif
/* // File_checked_for_headers */
