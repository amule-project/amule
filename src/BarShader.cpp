// This file is part of the aMule project.
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

#include <cmath>
#include <algorithm>		// Needed for std::min
#include <wx/gdicmn.h>
#include <wx/dc.h>
#include "color.h"		// Needed for RGB

#include "BarShader.h"		// Interface declarations.


const float Pi = 3.14159265358979323846264338328;

#define HALF(X) (((X) + 1) / 2)
#define DEFAULT_DEPTH 10


CBarShader::CBarShader(uint32 height, uint32 width)
: m_Width( width ),
  m_Height( height ),
  m_FileSize( 1 ),
  m_Modifiers( NULL ),
  m_used3dlevel( DEFAULT_DEPTH )
{
	m_spanlist.push_front(BarSpan(0, 1));
}


CBarShader::~CBarShader()
{
	if ( m_Modifiers ) {
		delete[] m_Modifiers;
	}
}


void CBarShader::Reset()
{
	Fill(0);
}


void CBarShader::BuildModifiers()
{
	if ( m_Modifiers ) {
		delete[] m_Modifiers;
	}

	int count = HALF(m_Height);
	float increment = ( Pi / m_Height );
	float depth = m_used3dlevel / 10.0;

	m_Modifiers = new float[count];
	for (int i = 0; i < count; i++)
		m_Modifiers[i] = sin(i * increment) * depth;
}


void CBarShader::SetFileSize(uint32 fileSize)
{
	if ( m_FileSize != fileSize ) {
		m_FileSize = fileSize;
		m_PixelsPerByte = (double)m_Width / m_FileSize;
		m_BytesPerPixel = (double)m_FileSize / m_Width;
	}
}


void CBarShader::FillRange(uint32 start, uint32 end, const DWORD color)
{
	// Sanity check
	wxASSERT( start <= end );

	if( end > m_FileSize ) {
		end = m_FileSize;
	}

	// Find the last unaffected span before the new span
	// Items are most often inserted at the back, so we start backwards
	SpanList::iterator it = --m_spanlist.end();
	while ( ( it != m_spanlist.begin() ) && it->end > start )
		--it;

	while ( it != m_spanlist.end() ) {
		// Begins before the current span
		if ( start < it->start ) {
			// Never touches the current span
			if ( end < it->start - 1 ) {
				it++;
				continue;
			}

			// Stops just before the current span
			else if ( end == it->start - 1 ) {
				// If same color: Merge
				if ( color == it->color ) {
					end = it->end;
					it = m_spanlist.erase( it );
				}

				break;
			}

			// Covers part of the current span (maybe entire span)
			else if ( end > it->start - 1 ) {
				// If it only covers part of the span
				if ( end < it->end ) {
					// Same color?
					if ( color == it->color ) {
						end = it->end;
						it = m_spanlist.erase( it );
					} else {
						it->start = end + 1;
					}

					break;
				} else {
					// It covers the entire span
					it = m_spanlist.erase( it );
					continue;
				}
			}

		}
		// It starts at the current span
		else if ( start == it->start ) {
			// Covers only part of the current span
			if ( end < it->end ) {
				// Same color, nothing to do
				if ( color == it->color ) {
					return;
				} else {
					it->start = end + 1;
				}

				break;

			} else {
				// Covers the entire span
				it = m_spanlist.erase( it );
				continue;
			}
		}

		// Starts inside the current span or after the current span
		else if ( start > it->start  && start <= it->end + 1 ) {
			// Covers only a slice of the current span
			if ( end < it->end ) {
				// Same color, nothing to do
				if ( color == it->color ) {
					return;
				} else {
					// Remember the old end-position
					uint32 oldend = it->end;
					// Resize the current span to fit before the new span
					it->end = start - 1;
					// Insert the second part of the old span behind where the new span is supposed to be placed
					it = m_spanlist.insert( ++it, BarSpan( end + 1, oldend, it->color ) );
					break;
				}
			} else {
				// Completly covers a side of the span
				if ( color == it->color ) {
					// Same color, delete old and update start position
					start = it->start;
					it = m_spanlist.erase( it );
					continue;

				} else {
					it->end = start - 1;
				}
			}
		}

		it++;
	}


	m_spanlist.insert( it, BarSpan( start, end, color ) );
}


void CBarShader::Fill(DWORD color)
{
	m_spanlist.clear();
	m_spanlist.push_front( BarSpan( 0, m_FileSize, color ) );
}


void CBarShader::Draw( wxDC* dc, int iLeft, int iTop, bool bFlat )
{
	wxASSERT( dc );

	// Do we need to rebuild the modifiers?
	if ( !bFlat && !m_Modifiers ) {
		BuildModifiers();
	}

	wxRect rectSpan;
	rectSpan.y = iTop;
	rectSpan.x = iLeft;
	rectSpan.height = m_Height;
	rectSpan.width  = 0;

	dc->SetPen(*wxTRANSPARENT_PEN);

	int iBytesInOnePixel = (int)(m_BytesPerPixel + 0.5f);
	uint32 start = 0;

	SpanList::iterator bsCurrent = m_spanlist.begin();
	while ( bsCurrent != m_spanlist.end() && rectSpan.GetRight() < (iLeft + m_Width)) {
		if ( bsCurrent->end < start ) {
			++bsCurrent;
			continue;
		}
			
		uint32 uSpan = bsCurrent->end - start;
		uint32 iPixels = (int)(uSpan * m_PixelsPerByte + 0.5f);
		
		if (iPixels > 0) {
			rectSpan.x += rectSpan.width;
			rectSpan.width = iPixels;
			
			FillRect(dc, rectSpan, bsCurrent->color, bFlat);

			start += (int)(iPixels * m_BytesPerPixel + 0.5f);
		
			++bsCurrent;
		} else {
			/* If the width of the current span is less than a pixel, we take 
			   as many spans as will fit in one pixel and calculate the 
			   "weight" of their colors PLUS the color weight of the adjacent
			   span. This means that we wont get sharp "streaks" from small
			   gaps, but rather blured lines, which fade in with the next span. */
		
			float fRed = 0;
			float fGreen = 0;
			float fBlue = 0;
			
			uint32 iEnd = start + iBytesInOnePixel;
			uint32 iLast = start;
			do {
				float fWeight = (std::min(bsCurrent->end, iEnd) - iLast) * m_PixelsPerByte;
				fRed   += GetRValue(bsCurrent->color) * fWeight;
				fGreen += GetGValue(bsCurrent->color) * fWeight;
				fBlue  += GetBValue(bsCurrent->color) * fWeight;
				iLast = bsCurrent->end;
				
				if ( bsCurrent->end > iEnd )
					break;
				
				bsCurrent++;
			} while ( bsCurrent != m_spanlist.end() );
			
			start += iBytesInOnePixel;
			
			rectSpan.x += rectSpan.width;
			rectSpan.width = 1;
			
			FillRect( dc, rectSpan, RGB( (int)fRed, (int)fGreen, (int)fBlue ), bFlat);	
		}
	}
}


void CBarShader::FillRect(wxDC *dc, const wxRect& rectSpan, DWORD color, bool bFlat)
{
	wxASSERT( dc );

	if( bFlat || !color ) {
		wxBrush brush( WxColourFromCr(color), wxSOLID );
		dc->SetBrush( brush );
		dc->DrawRectangle( rectSpan );

	} else {
	
		wxRect rect = rectSpan;
		rect.height = 1;

		wxBrush brush( wxColour(0, 0, 0), wxSOLID );
		
		int Max = HALF(m_Height);
		for (int i = 0; i < Max; i++) {
			int cRed   = std::min( 255, (int)(GetRValue(color) * m_Modifiers[i] + .5f) );
			int cGreen = std::min( 255, (int)(GetGValue(color) * m_Modifiers[i] + .5f) );
			int cBlue  = std::min( 255, (int)(GetBValue(color) * m_Modifiers[i] + .5f) );

			brush.SetColour( cRed, cGreen, cBlue );
			dc->SetBrush(brush);

			// Draw top row
			rect.y = rectSpan.y + i;
			dc->DrawRectangle( rect );

			// Draw bottom row
			rect.y = rectSpan.y + rectSpan.height - i - 1;
			dc->DrawRectangle( rect );
		}
	}

	dc->SetBrush(wxNullBrush);
}
