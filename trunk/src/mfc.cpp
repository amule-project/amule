// This file is part of the aMule project.
//
// Copyright (c) 2003,
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//


#include <wx/defs.h>		// Needed before any other wx/*.h
#include "CString.h"		// Interface declarations.

#include <wx/listimpl.cpp>	// Needed for WX_DEFINE_LIST
_DEFINE_LIST(char*, VoidList);	// WX_DEFINE_LIST(VoidList), but avoiding warning.
WX_DEFINE_LIST(stringList);

int stricmp( const char* s1, const char* s2 ) {
	return(wxString::wxString(s1).CmpNoCase(s2));
}

 /*
 ** The following two functions together make up an itoa()
 ** implementation. Function i2a() is a 'private' function
 ** called by the public itoa() function.
 **
 ** itoa() takes three arguments:
 ** 1) the integer to be converted,
 ** 2) a pointer to a character conversion buffer,
 ** 3) the radix for the conversion
 ** which can range between 2 and 36 inclusive
 ** range errors on the radix default it to base10
 */

static char *i2a(unsigned i, char *a, unsigned r)
{
	if (i/r > 0) a = i2a(i/r,a,r);
	*a = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i%r];
	return a+1;
}

char *itoa(int i, char *a, int r)
{
	if ((r < 2) || (r > 36)) r = 10;
	if (i<0) {
		*a = '-';
		*i2a(-(unsigned)i,a+1,r) = 0;
	} else *i2a(i,a,r) = 0;
	return a;
}
