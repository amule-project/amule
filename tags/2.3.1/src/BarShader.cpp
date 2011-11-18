//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include <cstring>		// Needed for std::memcpy

const double Pi = 3.14159265358979323846264338328;

#define HALF(X) (((X) + 1) / 2)
#define DEFAULT_DEPTH 10

CBarShader::CBarShader(unsigned height, unsigned width)
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


void CBarShader::SetHeight(unsigned height)
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


void CBarShader::Set3dDepth(unsigned depth)
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
	wxASSERT(m_used3dlevel < 7);

	if ( m_Modifiers ) {
		delete[] m_Modifiers;
	}

	unsigned depth = (7 - m_used3dlevel);
	unsigned count = HALF(m_Height);
	double piOverDepth = Pi/depth;
	double base = piOverDepth * ((depth / 2.0) - 1);
	double increment = piOverDepth / (count - 1);

	m_Modifiers = new double[count];
	for (unsigned i = 0; i < count; i++)
		m_Modifiers[i] = (double)(sin(base + i * increment));
}


void CBarShader::FillRange(uint64 start, uint64 end, const CMuleColour& colour)
{
	wxASSERT(m_FileSize > 0);

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

	unsigned firstPixel = start * m_Width / m_FileSize;
	unsigned lastPixel  = end   * m_Width / m_FileSize;
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
		for (unsigned i = firstPixel + 1; i < lastPixel; i++) {
			m_Content[i] = colour;
		}
	}
}


void CBarShader::Draw( wxDC* dc, int iLeft, int iTop, bool bFlat )
{
	wxASSERT( dc );

	// Do we need to rebuild the modifiers?
	if ( !bFlat && !m_Modifiers ) {
		BuildModifiers();
	}

	// Render the bar into a raw buffer
	unsigned char * buf = (unsigned char *) malloc(m_Width * m_Height * 3);

	if (bFlat) {
		// draw flat bar
		unsigned idx = 0;
		for (unsigned x = 0; x < m_Width; x++) {
			unsigned cRed   = m_Content[x].Red();
			unsigned cGreen = m_Content[x].Green();
			unsigned cBlue  = m_Content[x].Blue();
			buf[idx++] = cRed;
			buf[idx++] = cGreen;
			buf[idx++] = cBlue;
		}
		unsigned linelength = idx;
		unsigned y = 1;
		for (; y < m_Height >> 1; y <<= 1, idx <<= 1) {
			std::memcpy(buf + idx, buf, idx);
		}
		if (y < m_Height) {
			std::memcpy(buf + idx, buf, (m_Height - y) * linelength);
		}
	} else {
		// draw rounded bar
		unsigned Max = HALF(m_Height);
		unsigned idx = 0;
		for (unsigned y = 0; y < Max; y++) {
			for (unsigned x = 0; x < m_Width; x++) {
				unsigned cRed   = (unsigned)(m_Content[x].Red()   * m_Modifiers[y] + .5f);
				unsigned cGreen = (unsigned)(m_Content[x].Green() * m_Modifiers[y] + .5f);
				unsigned cBlue  = (unsigned)(m_Content[x].Blue()  * m_Modifiers[y] + .5f);
				cRed   = std::min(255u, cRed);
				cGreen = std::min(255u, cGreen);
				cBlue  = std::min(255u, cBlue);
				buf[idx++] = cRed;
				buf[idx++] = cGreen;
				buf[idx++] = cBlue;
			}
		}
		unsigned linelength = m_Width * 3;
		for (unsigned y = std::max(Max, m_Height - Max); y < m_Height; y++) {
			std::memcpy(buf + y * linelength, buf + (m_Height - 1 - y) * linelength, linelength);
		}
	}
	wxImage image(m_Width, m_Height);
	image.SetData(buf);
	wxBitmap bitmap(image);
	dc->DrawBitmap(bitmap, iLeft, iTop);
}
// File_checked_for_headers
