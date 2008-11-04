//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2008 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include <wx/image.h>
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
		Fill(CMuleColour(0,0,0));
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


void CBarShader::FillRange(uint64 start, uint64 end, const CMuleColour& colour)
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
		m_Content[firstPixel].BlendWith(colour, firstCovered + lastCovered - 1.0);
	} else {
		m_Content[firstPixel].BlendWith(colour, firstCovered);
		m_Content[lastPixel].BlendWith(colour, lastCovered);
		// fill pixels between (if any)
		for (uint32 i = firstPixel + 1; i < lastPixel; i++) {
			m_Content[i] = colour;
		}
	}
}


void CBarShader::Fill(const CMuleColour& colour)
{
	m_Content.clear();
	m_Content.resize(m_Width, colour);
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
	
	// Render the bar into a raw buffer
	unsigned char * buf = (unsigned char *) malloc(m_Width * m_Height * 3);

	// draw flat bar
	if (bFlat) {
		for (int x = 0; x < m_Width; x++) {
			int cRed   = m_Content[x].Red();
			int cGreen = m_Content[x].Green();
			int cBlue  = m_Content[x].Blue();
			for (int y = 0; y < m_Height; y++) {
				int idx = (y * m_Width + x) * 3;
				buf[idx++] = cRed;
				buf[idx++] = cGreen;
				buf[idx]   = cBlue;
			}
		}
	} else {
	// draw rounded bar
		int Max = HALF(m_Height);
		for (int x = 0; x < m_Width; x++) {
			for (int i = 0; i < Max; i++) {
				int cRed   = (int)(m_Content[x].Red()   * m_Modifiers[i] + .5f);
				int cGreen = (int)(m_Content[x].Green() * m_Modifiers[i] + .5f);
				int cBlue  = (int)(m_Content[x].Blue()  * m_Modifiers[i] + .5f);
				cRed   = std::min(255, cRed);
				cGreen = std::min(255, cGreen);
				cBlue  = std::min(255, cBlue);
				// Draw top row
				int y = i;
				int idx = (y * m_Width + x) * 3;
				buf[idx++] = cRed;
				buf[idx++] = cGreen;
				buf[idx]   = cBlue;
				// Draw bottom row
				y = m_Height - 1 - i;
				idx = (y * m_Width + x) * 3;
				buf[idx++] = cRed;
				buf[idx++] = cGreen;
				buf[idx]   = cBlue;
			}
		}
	}
	wxImage image(m_Width, m_Height);
	image.SetData(buf);
	wxBitmap bitmap(image);
	dc->DrawBitmap(bitmap, iLeft, iTop);
}

// File_checked_for_headers
