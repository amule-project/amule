//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef MULECOLOR_H
#define MULECOLOR_H

#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/pen.h>   // needed for wxPenStyle enum values
#include <wx/brush.h> // needed for wxBrushStyle enum values
#include <wx/font.h>  // needed for wxFontStyle enum values
#include "Types.h"

class wxPen;
class wxBrush;

class CMuleColour
{
public:

	enum ColourComponent { COLOUR_R = 1, COLOUR_G = 2, COLOUR_B = 4 };

	CMuleColour() { Init(); Set(0,0,0); }
	CMuleColour(const wxColour& colour) { Init(); Set(colour.Red(), colour.Green(), colour.Blue()); }
	CMuleColour(uint8_t r, uint8_t g, uint8_t b) { Init(); Set(r,g,b); }
	CMuleColour(unsigned long rgb)
	{
		Init();
		Set((rgb & 0xFF), (rgb >> 8) & 0xFF, (rgb >> 16) & 0xFF);
	}

	CMuleColour(wxSystemColour colour)
	{
		Init();
		const wxColour& wxcolour = wxSystemSettings::GetColour(colour);
		Set(wxcolour.Red(), wxcolour.Green(), wxcolour.Blue());
	}

	void Init() {
		m_cachedpen = NULL;
		m_cachedbrush = NULL;
	}

	~CMuleColour() { }

	void Set(uint8_t red, uint8_t green, uint8_t blue) { m_red = red; m_green = green; m_blue = blue; }

	inline uint8_t Red() const { return m_red; }
	inline uint8_t Green() const { return m_green; }
	inline uint8_t Blue() const { return m_blue; }

	const CMuleColour& Blend(uint8_t percentage, ColourComponent flags = (ColourComponent)(COLOUR_R | COLOUR_G | COLOUR_B) )
	{
		unsigned int red = (unsigned int)(Red() * ((flags & COLOUR_R) ? ((float)percentage/(float)100) : (float)1));
		unsigned int green = (unsigned int)(Green() * ((flags & COLOUR_G) ? ((float)percentage/(float)100) : (float)1));
		unsigned int blue = (unsigned int)(Blue() * ((flags & COLOUR_B) ? ((float)percentage/(float)100) : (float)1));
		Set((red < 255) ? red : 255, (green < 255) ? green : 255, (blue < 255) ? blue : 255);
		return *this;
	}

	const CMuleColour& BlendWith(const CMuleColour& colour, double covered)
	{
		unsigned int red = (unsigned int)(Red() + (colour.Red() * covered) + 0.5);
		unsigned int green = (unsigned int)(Green() + (colour.Green() * covered) + 0.5);
		unsigned int blue = (unsigned int)(Blue() + (colour.Blue() * covered) + 0.5);
		Set((red < 255) ? red : 255, (green < 255) ? green : 255, (blue < 255) ? blue : 255);
		return *this;
	}

	unsigned long GetULong() const { return (Blue() << 16) | (Green() << 8) | Red(); }

	bool IsBlack() const { return !Red() && !Blue() && !Green(); }

	bool IsSameAs(const CMuleColour& colour) const { return (Red() == colour.Red()) && (Green() == colour.Green()) && (Blue() == colour.Blue()); }

	operator wxColour() const {
		return wxColor(m_red, m_green, m_blue);
	}

	const wxPen& GetPen(int width = 1, wxPenStyle style = wxPENSTYLE_SOLID) const;
	const wxBrush& GetBrush(wxBrushStyle style = wxBRUSHSTYLE_SOLID) const;

private:
	uint8_t m_red;
	uint8_t m_green;
	uint8_t m_blue;

	mutable wxPen* m_cachedpen;
	mutable wxBrush* m_cachedbrush;
};

#endif
