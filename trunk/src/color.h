//
// This file is part of the aMule project.
//
// Copyright (c) 2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2004, Carlo Wood <carlo@alinoe.com>
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

#ifndef COLOR_H
#define COLOR_H

#include <inttypes.h>		// Needed for uint32_t
#include <wx/defs.h>            // Needed before any other wx/*.h, possibly needed for COLORREF
#include <wx/settings.h>        // Needed for wxSystemColour
#include "types.h"


#if !defined(__WXPM__) && !defined(__WXMSW__)  // Otherwise already defined in wx/defs.h.
typedef uint32_t		COLORREF;
#endif


inline int G_BLEND(int a, int percentage)
{
	int result = a * percentage / 100;
	return (result > 255) ? 255 : result;
}


inline wxColour BLEND( wxColour colour, int percentage )
{
	int red   = G_BLEND( colour.Red(),   percentage );
	int green = G_BLEND( colour.Green(), percentage );
	int blue  = G_BLEND( colour.Blue(),  percentage );

	return wxColour( red, green, blue );
}


#ifndef __WXMSW__
inline COLORREF RGB(int a, int b, int c)
{
	COLORREF result;
	result = (a & 0xff) << 16;
	result |= (b & 0xff) << 8;
	result |= (c & 0xff);
	return result;
}


inline int GetRValue(COLORREF rgb)
{
	return (rgb >> 16) & 0xff;
}


inline int GetGValue(COLORREF rgb)
{
	return (rgb >> 8) & 0xff;
}


inline int GetBValue(COLORREF rgb)
{
	return rgb & 0xff;
}

inline COLORREF DarkenColour(COLORREF rgb, int level) 
{	
	return RGB(GetRValue(rgb) / level, GetGValue(rgb) / level, GetBValue(rgb) / level);
}

#endif


inline wxColour WxColourFromCr(COLORREF cr)
{
	return wxColour(GetRValue(cr),GetGValue(cr),GetBValue(cr));
}


inline COLORREF CrFromWxColour(wxColour col)
{
	return RGB(col.Red(), col.Green(), col.Blue());
}


#define SYSCOLOR(x) (wxSystemSettings::GetColour(x))


#endif
