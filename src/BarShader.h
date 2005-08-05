//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef BARSHADER_H
#define BARSHADER_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "BarShader.h"
#endif

#include "Types.h"	// Needed for uint16 and uint32
#include "RangeMap.h"	// Needed for CRangeMap


class wxRect;
class wxDC;


/**
 * The barshader class is responsible for drawing the chunk-based progress bars used in aMule.
 *
 * CBarShader represents the chunks of a file through the use of spans, which 
 * cover a range in the file with a certain color. New spans can be added on 
 * the fly and old spans are automatically removed, resized or merged when
 * neseccary.
 *
 * CBarShader will try to minimize the number of spans when possible.
 */
class CBarShader
{
public:
	/**
	 * Constructor.
	 * 
	 * @param height The height of the area upon which the span is drawn.
	 * @param width  The width of the area upon which the span is drawn.
	 */
	CBarShader(uint32 height = 1, uint32 width = 1);

	/**
	 * Destructor.
	 */
	~CBarShader();

	/**
	 * Sets the width of the drawn bar.
	 *
	 * @param width The new width. 
	 *
	 * Setting this sets the width the bar which is used when it
	 * is drawn. The BarShader automatically fits the intire span-
	 * structure inside this area.
	 */
	void SetWidth(int width) {
		m_Width = width;
	}

	/**
	 * Sets the height of the drawn bar.
	 *
	 * @param height The new height. 
	 *
	 * Changes the height of the bar, used when it is drawn.
	 */
	void SetHeight( int height );


	/**
	 * Sets the 3D-depth of the bar
	 *
	 * @param depth A value in the range from 1 to 5.
	 */
	void Set3dDepth( int depth );

	
	/**
	 * Returns the current width of the bar.
	 *
	 * @return The width of the bar.
	 */
	int GetWidth() const {
		return m_Width;
	}

	/**
	 * Returns the current height of the bar.
	 *
	 * @return The height of the bar.
	 */
	int GetHeight() const {
		return m_Height;
	}

	/**
	 * Returns the current 3D-depth of the bar.
	 *
	 * @return The 3D-depth of the bar.
	 */
	int Get3dDepth() const {
		return m_used3dlevel;
	}
	
	
	/**
	 * Removes all spans from the bar.
	 *
	 * Calling this function deletes all current spans and fills the
	 * bar with a black span from 0 to filesize.
	 */
	void Reset();

	/**
	 * Sets a new filesize.
	 *
	 * @param fileSize The new filesize.
	 *
	 * Calling this function sets a new filesize, which is the virtual 
	 * length of the bar. This function does not change any spans already
	 * present and therefore, they might end up pointing past current
	 * filesize if the size if smaller than before.
	 */
	void SetFileSize(uint32 fileSize);

	/**
	 * Fills in a range with a certain color.
	 *
	 * @param start The starting position of the new span.
	 * @param end The ending position of the new span. Must be larger than start.
	 * @param color The color of the new span.
	 *
	 * Calling this function fill the specified range with the specified color.
	 * Any spans completly or partially covered by the new span are either
	 * removed or resized. If the value of end is larger than the current
	 * filesize, the filesize is increased to the value of end.
	 */
	void FillRange(uint32 start, uint32 end, const uint32 color);

	/**
	 * Fill the entire bar with a span of the specified color.
	 *
	 * @param color The color of the new span.
	 */
	void Fill(uint32 color);

	/**
	 * Draws the bar on the specifed wxDC.
	 *
	 * @param dc The wxDC upon which the bar should be drawn.
	 * @param iLeft The left position from where to start drawing.
	 * @param iTop The top position from where to start drawing.
	 * @param bFlat 3D effect is not applied if this is true.
	 *
	 * This functions draws the bar with the height and width specified
	 * through either the contructor or with SetWidth() and SetHeight().
	 */
	void Draw( wxDC* dc, int iLeft, int iTop, bool bFlat );

private:
	/**
	 * Calculates the modifiers used to create 3d effect.
	 */
	void BuildModifiers();
	
	/**
	 * Fills a rectangle with a given color.
	 *
	 * @param dc The DC upon which to draw the bar.
	 * @param rectSpan The area within the specifed DC upon which to draw.
	 * @param color The color of the rectangle.
	 * @param bFlat If this is true, a simple rectangle will be drawn, otherwise the modifers will be applyed to achive a 3D effect.
	 */
	void FillRect(wxDC* dc, const wxRect& rectSpan, uint32 color, bool bFlat);

	//! The width of the drawn bar
	int    m_Width;
	//! The height of the drawn bar
	int    m_Height;
	//! The virtual filesize assosiated with the bar
	uint32 m_FileSize;
	//! Pointer to array of modifers used to create 3D effect. Size is (m_Height+1)/2 when set.
	double* m_Modifiers;
	//! The current 3d level 
	uint16 m_used3dlevel;

	
	//! SpanList is defined as a CRangeMap with uint32s as range-data
	typedef CRangeMap<uint32> SpanList;
	//! The list of spans. This list is kept sorted.
	SpanList m_spanlist;
};

#endif
