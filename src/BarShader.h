// This file is part of the aMule Project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
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

#ifndef BARSHADER_H
#define BARSHADER_H

#include "types.h"		// Needed for uint16 and uint32

#include <list>


class wxRect;
class wxDC;


class CBarShader
{
public:
	CBarShader(uint32 height = 1, uint32 width = 1);
	~CBarShader();

	// Set the width of the bar
	void SetWidth(int width) {
		if( m_Width != width ) {
			m_Width = width;
			m_PixelsPerByte = (double)m_Width / m_FileSize;
			m_BytesPerPixel = (double)m_FileSize / m_Width;
		}
	}

	// Set the height of the bar
	void SetHeight( int height ) {
		if( m_Height != height ) {
			m_Height = height;

			// Reset the modifers
			if ( m_Modifiers ) {
				delete[] m_Modifiers;
				m_Modifiers = NULL;
			}
		}
	}
	
	// Set the (3d) depth of the bar ( range 1-20 )
	void Set3dDepth( int depth ) {
		wxASSERT( ( depth > 1 ) && ( depth < 21 ) );
	
		if ( m_used3dlevel != depth ) {
			m_used3dlevel = depth;
	
			// Reset the modifers
			if ( m_Modifiers ) {
				delete[] m_Modifiers;
				m_Modifiers = NULL;
			}
		}
	}

	// Returns the width of the bar
	int GetWidth() const {
		return m_Width;
	}

	// Returns the height of the bar
	int GetHeight() const {
		return m_used3dlevel;
	}

	// Returns the (3d) depth of the bar
	int Get3dDepth() const {
		return m_Height;
	}
	
	
	// Call this to blank the shader without changing the file size
	void Reset();

	// Sets a new file size and resets the shader
	void SetFileSize(uint32 fileSize);

	// Fills in a range with a certain color, new ranges overwriting old ranges
	void FillRange(uint32 start, uint32 end, const DWORD color);

	// Fills in entire range with a certain color
	void Fill(DWORD color);

	// Draws the bar
	void Draw( wxDC* dc, int iLeft, int iTop, bool bFlat );

	void DebugPrint() {
		SpanList::iterator it = m_spanlist.begin();

		printf("\n\n");
		while ( it != m_spanlist.end() ) {
			printf("%d -- %d-%d\n", it->color, it->start, it->end);
			it++;
		}
	}

protected:
	/* This calculates the modifiers used to create the 3d effect. In essence, 
	   the effect is created by using a sinus curve to calculate the "darkness"
	   of a line. */
	void BuildModifiers();
	// Fills a rectangle with a given color
	void FillRect(wxDC* dc, const wxRect& rectSpan, DWORD color, bool bFlat);

	int    m_Width;
	int    m_Height;
	double m_PixelsPerByte;
	double m_BytesPerPixel;
	uint32 m_FileSize;

private:
	struct BarSpan
	{
		uint32		start;
		uint32		end;
		DWORD		color;

		BarSpan(uint32 s, uint32 e, DWORD cr = 0)
		 : start( s ),
		   end( e ),
		   color( cr )
		{
		}
	};

	float* m_Modifiers;
	uint16 m_used3dlevel;

	typedef std::list<BarSpan> SpanList;
	SpanList m_spanlist;
};

#endif // BARSHADER_H
