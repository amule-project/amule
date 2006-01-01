//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <cmath>
#include <algorithm>		// Needed for std::min
#include <wx/gdicmn.h>
#include <wx/dc.h>
#include "Color.h"		// Needed for RGB

#include "BarShader.h"		// Interface declarations.

const double Pi = 3.14159265358979323846264338328;

#define HALF(X) (((X) + 1) / 2)
#define DEFAULT_DEPTH 10


CBarShader::CBarShader(uint32 height, uint32 width)
: m_Width( width ),
  m_Height( height ),
  m_FileSize( 1 ),
  m_Modifiers( NULL ),
  m_used3dlevel( DEFAULT_DEPTH )
{
	Fill( 0 );
}


CBarShader::~CBarShader()
{
	if ( m_Modifiers ) {
		delete[] m_Modifiers;
	}
}


void CBarShader::Reset()
{
	m_spanlist.clear();
	Fill(0);
}


void CBarShader::SetFileSize(uint32 fileSize)
{
	m_FileSize = fileSize;
	Reset();
}


void CBarShader::SetHeight( int height )
{
	if( m_Height != height ) {
		m_Height = height;

		// Reset the modifers
		if ( m_Modifiers ) {
			delete[] m_Modifiers;
			m_Modifiers = NULL;
		}
	}
}


void CBarShader::Set3dDepth( int depth )
{
	if ( depth < 1 ) {
		depth = 1;
	} else if ( depth > 5 ) {
		depth = 5;
	}

	if ( m_used3dlevel != depth ) {
		m_used3dlevel = depth;

		// Reset the modifers
		if ( m_Modifiers ) {
			delete[] m_Modifiers;
			m_Modifiers = NULL;
		}
	}
}


void CBarShader::BuildModifiers()
{
	if ( m_Modifiers ) {
		delete[] m_Modifiers;
	}

	int depth = (7-m_used3dlevel);
	int count = HALF(m_Height);
	double piOverDepth = Pi/depth;
	double base = piOverDepth * ((depth / 2.0) - 1);
	double increment = piOverDepth / (count - 1);

	m_Modifiers = new double[count];
	for (int i = 0; i < count; i++)
		m_Modifiers[i] = (double)(sin(base + i * increment));
}


void CBarShader::FillRange(uint32 start, uint32 end, const uint32 color)
{
	// Sanity check
	wxASSERT( start <= end );

	if ( start >= m_FileSize ) {
		return;
	}
	
	if ( end >= m_FileSize ) {
		end = m_FileSize - 1;
	}

	m_spanlist.insert( start, end, color );
}


void CBarShader::Fill(uint32 color)
{
	m_spanlist.clear();
	m_spanlist.insert( 0, m_FileSize - 1, color );
}


void CBarShader::Draw( wxDC* dc, int iLeft, int iTop, bool bFlat )
{
	wxASSERT( dc );

	// Check if there's anything to do ...
	if ( m_spanlist.empty() ) {
		return;
	}

	
	// Do we need to rebuild the modifiers?
	if ( !bFlat && !m_Modifiers ) {
		BuildModifiers();
	}

	wxRect rectSpan;
	rectSpan.x = iLeft;
	rectSpan.y = iTop;
	rectSpan.height = m_Height;
	rectSpan.width = 0;
	
	dc->SetPen(*wxTRANSPARENT_PEN);

	// This modifier is multipled with sizes to allow for better handling of small ranges
	const uint64 MOD = 1000;
	// This is the number of bits each pixel should contain.
	const uint64 bitsPerPixel = ((uint64)m_FileSize * MOD) / (uint64)m_Width;
	
	// The initial values for partial pixel drawing
	uint64 curPixel = 0;
	uint64 curRed = 0;
	uint64 curGreen = 0;
	uint64 curBlue = 0;

	// Initialize to the first range
	SpanList::iterator it = m_spanlist.begin();
	uint64 size = (uint64)( it.keyEnd() - it.keyStart() + 1 ) * MOD;
	uint32 color = *it++;

	// Loop until everything has been drawn	
	while ( size || curPixel ) {
		if ( !size && it != m_spanlist.end() ) {
			// Fetch the next range and increment the iterator
			size = (uint64)( it.keyEnd() - it.keyStart() + 1 ) * MOD;
			color = *it++;
		} else if ( curPixel || size < bitsPerPixel ) {
			// This block is responsible for drawing ranges that are too small
			// to fill a single pixel. To overcome this problem, we gather the
			// sum of ranges until we have enough to draw a single pixel. The
			// color of this pixel will be the sum of the colors of the ranges
			// within the single pixel, each weighted after its relative size.
			
			// See how much we can take from the current range
			uint64 curDiff = std::min( size, bitsPerPixel - curPixel );
		
			// Increment the current size of the partial pixel
			curPixel += curDiff;
			
			// Add the color of the current range times the ammount of the current
			// range that was added to the partial pixel. The result will be divided
			// by the length of the partial pixel to get the average.
			curRed   += curDiff * GetRValue( color );
			curGreen += curDiff * GetGValue( color );
			curBlue  += curDiff * GetBValue( color );

			// If we have a complete pixel, or if we have run out of usable ranges,
			// then draw the partial pixel. Note that size is modified below this
			// check, so that it only triggers when size was 0 to begin with.
			if ( curPixel == bitsPerPixel || !size ) {
				// Draw a single line containing the average of the smaller parts
				uint32 col = RGB( (uint32)(curRed / curPixel),
				                  (uint32)(curGreen / curPixel),
				                  (uint32)(curBlue / curPixel) );

				// Reset the partial-pixel
				curPixel = curRed = curGreen = curBlue = 0;

				// Increment the position on the device-context
				rectSpan.x    += rectSpan.width;
				rectSpan.width = 1;

				// Draw the line
				FillRect(dc, rectSpan, col, bFlat);
			}
			
			// Decrement size
			size     -= curDiff;
		} else {
			// We are dealing with a range that is large enough to draw by itself.
			// We will draw as many complete pixels as we can, and allow the rest
			// to be absorbed by the partial pixel.
			rectSpan.x    += rectSpan.width;
			rectSpan.width = size / bitsPerPixel;
			
			// Unused size will be used by the partial-pixel drawing code.
			size = size % bitsPerPixel; 

			// Draw the range
			FillRect(dc, rectSpan, color, bFlat);
		}
	}
}


void CBarShader::FillRect(wxDC *dc, const wxRect& rectSpan, uint32 color, bool bFlat)
{
	wxASSERT( dc );

	if( bFlat || !color ) {
		wxBrush brush( WxColourFromCr(color), wxSOLID );
		dc->SetBrush( brush );
		dc->DrawRectangle( rectSpan );
	} else {
		int x1 = rectSpan.x;
		int x2 = rectSpan.x + rectSpan.width;
		int y1 = rectSpan.y;
		int y2 = rectSpan.GetBottom();
		
		int Max = HALF(m_Height);
		for (int i = 0; i < Max; i++) {
			int cRed   = std::min( 255, (int)(GetRValue(color) * m_Modifiers[i] + .5f) );
			int cGreen = std::min( 255, (int)(GetGValue(color) * m_Modifiers[i] + .5f) );
			int cBlue  = std::min( 255, (int)(GetBValue(color) * m_Modifiers[i] + .5f) );
				
			wxPen pen( wxColour( cRed, cGreen, cBlue ), 1, wxSOLID );
			dc->SetPen( pen );

			// Draw top row
			dc->DrawLine( x1, y1 + i, x2, y1 + i );

			// Draw bottom row
			dc->DrawLine( x1, y2 - i, x2, y2 - i );
		}
	}

	dc->SetBrush(wxNullBrush);
}

