//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2009 aMule Team ( admin@amule.org / http://www.amule.org )
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
  m_used3dlevel( DEFAULT_DEPTH ),
  m_Content(width, 0)
{
}


CBarShader::~CBarShader()
{
	if ( m_Modifiers ) {
		delete[] m_Modifiers;
	}
}


void CBarShader::SetFileSize(uint64 fileSize)
{
	m_FileSize = fileSize;
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


void CBarShader::SetWidth(int width)
{
       if (width > 0) {
               m_Width = width;
               m_Content.clear();
               m_Content.resize(m_Width, 0);
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


void CBarShader::FillRange(uint64 start, uint64 end, uint32 color)
{
	if (start >= end || start >= m_FileSize) {
		return;
	}
	
	// precision for small files: end must be increased by one 
	// think of each byte as a visible block, then start points to
	// the beginning of its block, but end points to the END of its block
	end++;
	
	if (end > m_FileSize) {
		end = m_FileSize;
	}

	uint32 firstPixel = start * m_Width / m_FileSize;
	uint32 lastPixel  = end   * m_Width / m_FileSize;
	if (lastPixel == m_Width) {
		lastPixel--;
	}
	double f_Width = m_Width;
	// calculate how much of this pixels is to be covered with the fill
	double firstCovered = firstPixel + 1 - start * f_Width / m_FileSize;
	double lastCovered  = end * f_Width / m_FileSize - lastPixel;
	// all inside one pixel ?
	if (firstPixel == lastPixel) {
		BlendPixel(firstPixel, color, firstCovered + lastCovered - 1.0);
	} else {
		BlendPixel(firstPixel, color, firstCovered);
		BlendPixel(lastPixel, color, lastCovered);
		// fill pixels between (if any)
		for (uint32 i = firstPixel + 1; i < lastPixel; i++) {
			m_Content[i] = color;
		}
	}
}


// This function is responsible for drawing ranges that are too small
// to fill a single pixel. To overcome this problem, we gather the
// sum of ranges until we have enough to draw a single pixel. The
// color of this pixel will be the sum of the colors of the ranges
// within the single pixel, each weighted after its relative size.
void CBarShader::BlendPixel(uint32 index, uint32 color, double covered)
{
	uint32 oldcolor = m_Content[index];
	// Colors are added up, so the bar must be initialized black (zero) for blending to work.
	// So after blending in 10 * the same color with covered == 0.1, the pixel will
	// have the color.
	// Blending in black will thus have no effect.
	// This works as long each part of the virtual bar is overwritten just once (or left black).
	int Red   = (int) (GetRValue(oldcolor) + GetRValue(color) * covered + 0.5);
	int Green = (int) (GetGValue(oldcolor) + GetGValue(color) * covered + 0.5);
	int Blue  = (int) (GetBValue(oldcolor) + GetBValue(color) * covered + 0.5);
	Red   = Red   > 255 ? 255 : Red  ;
	Green = Green > 255 ? 255 : Green;
	Blue  = Blue  > 255 ? 255 : Blue ;
	m_Content[index] = RGB(Red, Green, Blue);
}


void CBarShader::Fill(uint32 color)
{
	m_Content.clear();
	m_Content.resize(m_Width, color);
}


void CBarShader::Draw( wxDC* dc, int iLeft, int iTop, bool bFlat )
{
	wxASSERT( dc );

	// Do we need to rebuild the modifiers?
	if ( !bFlat && !m_Modifiers ) {
		BuildModifiers();
	}

	wxRect rectSpan;
	rectSpan.x = iLeft;
	rectSpan.y = iTop;
	rectSpan.height = m_Height;
	rectSpan.width = 0;
	uint32 lastcolor = 0xffffffff; // invalid value
	
	dc->SetPen(*wxTRANSPARENT_PEN);

	// draw each pixel, draw same colored pixels together
	for (int x = 0; x < m_Width; x++) {
		uint32 color = m_Content[x];
		if (color == lastcolor) {
			rectSpan.width++;
		} else {
			if (rectSpan.width) {
				FillRect(dc, rectSpan, lastcolor, bFlat);
				rectSpan.x += rectSpan.width;
			}
			rectSpan.width = 1;
			lastcolor = color;
		}
	}
	FillRect(dc, rectSpan, lastcolor, bFlat);
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
// File_checked_for_headers
