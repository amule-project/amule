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

#include <cmath>
#include <algorithm>		// Needed for std::min
#include <wx/gdicmn.h>
#include <wx/dc.h>
#include <wx/dcmemory.h>

#include "BarShader.h"		// Interface declarations.
#include "Preferences.h"	// Needed for CPreferences
#include "CamuleAppBase.h"	// Needed for theApp

#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923
#endif

#ifndef PI
#define PI 3.14159265358979323846264338328
#endif

#define HALF(X) (((X) + 1) / 2)

#define GetRValue(rgb) (((rgb)>>16)&0xff)
#define GetGValue(rgb) (((rgb)>>8)&0xff)
#define GetBValue(rgb) ((rgb)&0xff)

CBarShader::CBarShader(uint32 height, uint32 width) {
	m_iWidth = width;
	m_iHeight = height;
	m_uFileSize = 1;
	m_FirstSpan = new BarSpan(0, 1);
	m_Modifiers = NULL;
}

CBarShader::~CBarShader(void) {
	delete[] m_Modifiers;
	m_FirstSpan->DeleteUpTo(NULL);
	delete m_FirstSpan;
}

void CBarShader::Reset() {
	Fill(0);
}

void CBarShader::BuildModifiers() {
	if(m_Modifiers != NULL)
		delete[] m_Modifiers;

	m_used3dlevel=theApp.glob_prefs->Get3DDepth();
	// Barry - New property page slider to control depth of gradient

	// Depth must be at least 2
	// 2 gives greatest depth, the higher the value, the flatter the appearance
	// m_Modifiers[count-1] will always be 1, m_Modifiers[0] depends on the value of depth
	
	int depth = (7-theApp.glob_prefs->Get3DDepth());
	int count = HALF(m_iHeight);
	double piOverDepth = PI/depth;
	double base = piOverDepth * ((depth / 2.0) - 1);
	double increment = piOverDepth / (count - 1);

	m_Modifiers = new float[count];
	for (int i = 0; i < count; i++)
		m_Modifiers[i] = (float)(sin(base + i * increment));
}

void CBarShader::SetFileSize(uint32 fileSize) {
	if(m_uFileSize != fileSize) {
		m_uFileSize = fileSize;
		m_dPixelsPerByte = (double)m_iWidth / m_uFileSize;
		m_dBytesPerPixel = (double)m_uFileSize / m_iWidth;
	}
}

void CBarShader::FillRange(uint32 start, uint32 end, DWORD color) {
	if(end>m_uFileSize) {
	  end=m_uFileSize;
	}
	if(start >= end)
		return;

	BarSpan *bsPrev, *bsStart, *bsEnd;
	bsPrev = bsStart = m_FirstSpan;
	while(bsStart->next != NULL && bsStart->end < start)
		bsStart = bsStart->next;

	//place new span, unless it's the same color
	BarSpan *bsNew;

	//case 0, same color
	if(bsStart->color == color) {

		//same color, ends before end.
		if(bsStart->end > end)
			return;

		//case 1, same color, ends after end.
		bsNew = bsStart;
		bsNew->end = end;

	}
	
	else if(bsStart->start == start) {

		//case 2, begins at start, ends before end.
		if(bsStart->end > end) {
			//the 'ol switcheroo
			bsNew = new BarSpan(bsStart, end, bsStart->end, bsStart->color);
			bsStart->end = end;
			bsStart->color = color;
			return;
		}

		//case 3, begins at start, ends after end.
		else {
			//hostile takeover
			bsNew = bsStart;
			bsNew->end = end;
			bsNew->color = color;
		}

	}

	else if(bsStart->start < start) {

		//case 4, begins after start, ends before end
		if(bsStart->end > end) {
			bsNew = new BarSpan(bsStart, start, end, color);
			bsNew = new BarSpan(bsNew, end, bsStart->end, bsStart->color);
			bsStart->end = start;
			return;
		}

		//case 5, begins after start, ends after end
		else {
			bsNew = new BarSpan(bsStart, start, end, color);
			bsStart->end = start;
		}
	} else {
		assert(FALSE);//should never get here
	}

	bsEnd = bsNew->next;
	while(bsEnd != NULL && bsEnd->end <= end)
		bsEnd = bsEnd->next;

	if(bsEnd != NULL)
		bsEnd->start = end;

	bsNew->DeleteUpTo(bsEnd);
}

void CBarShader::Fill(DWORD color) {
	m_FirstSpan->DeleteUpTo(NULL);
	m_FirstSpan->start = 0;
	m_FirstSpan->end = m_uFileSize;
	m_FirstSpan->color = color;
}

//void CBarShader::Draw(wxMemoryDC* dc, int iLeft, int iTop, bool bFlat) {
void CBarShader::Draw(wxDC* dc, int iLeft, int iTop, bool bFlat) {
	BarSpan *bsCurrent = m_FirstSpan;
	RECT rectSpan;
	rectSpan.top = iTop;
	rectSpan.bottom = iTop + m_iHeight;
	rectSpan.right = iLeft;

	dc->SetPen(*wxTRANSPARENT_PEN);

	int iBytesInOnePixel = (int)(m_dBytesPerPixel + 0.5f);
	uint32 start = 0;//bsCurrent->start;
	uint32 drawnItems=0;
	while(bsCurrent != NULL && (int)rectSpan.right < (iLeft + m_iWidth)) {
		uint32 uSpan = bsCurrent->end - start;
		int iPixels = (int)(uSpan * m_dPixelsPerByte + 0.5f);
		if(iPixels > 0) {
			rectSpan.left = rectSpan.right;
			rectSpan.right += iPixels;
			FillRect(dc, &rectSpan, bsCurrent->color, bFlat);
			drawnItems++;

			start += (int)(iPixels * m_dBytesPerPixel + 0.5f);
		} else {
			float fRed = 0;
			float fGreen = 0;
			float fBlue = 0;
			uint32 iEnd = start + iBytesInOnePixel;
			int iLast = start;
			do {
				float fWeight = (std::min(bsCurrent->end, iEnd) - iLast) * m_dPixelsPerByte;
				fRed   += GetRValue(bsCurrent->color) * fWeight;
				fGreen += GetGValue(bsCurrent->color) * fWeight;
				fBlue  += GetBValue(bsCurrent->color) * fWeight;
				if(bsCurrent->end > iEnd)
					break;
				iLast = bsCurrent->end;
				bsCurrent = bsCurrent->next;
			} while(bsCurrent != NULL);
			rectSpan.left = rectSpan.right;
			rectSpan.right++;
			FillRect(dc, &rectSpan, fRed, fGreen, fBlue, bFlat);
			drawnItems++;
			start += iBytesInOnePixel;
		}
		while(bsCurrent != NULL && bsCurrent->end < start)
			bsCurrent = bsCurrent->next;
	}
	// for debugging:
	///printf(" piirettiin %d palaa. Kesti %d ms. Leveys %d\n",drawnItems,GetTickCount()-now,m_iWidth);
}

//void CBarShader::FillRect(wxMemoryDC *dc, RECT* rectSpan, DWORD color, bool bFlat) {
void CBarShader::FillRect(wxDC *dc, RECT* rectSpan, DWORD color, bool bFlat) {
  if(rectSpan->right-rectSpan->left<1)
    return;

  if(!color || bFlat) {
    //dc->FillRect(rectSpan, &CBrush(color));
    //dc->SetBrush(*(wxTheBrushList->FindOrCreateBrush(wxColour(GetRValue(color),GetGValue(color),GetBValue(color)),wxSOLID)));
    wxBrush suti(wxColour(GetRValue(color),GetGValue(color),GetBValue(color)),wxSOLID);
    dc->SetBrush(suti);
    dc->DrawRectangle(rectSpan->left,rectSpan->top,rectSpan->right,rectSpan->bottom);
    dc->SetBrush(wxNullBrush);
  } else {
    FillRect(dc, rectSpan, GetRValue(color), GetGValue(color), GetBValue(color), false);
  }
}

//void CBarShader::FillRect(wxMemoryDC *dc, RECT* rectSpan, float fRed, float fGreen,
void CBarShader::FillRect(wxDC *dc, RECT* rectSpan, float fRed, float fGreen,
						  float fBlue, bool bFlat) {
  if(rectSpan->right-rectSpan->left<1)
    return;

  if(bFlat) {
    //dc->SetBrush(*(wxTheBrushList->FindOrCreateBrush(wxColour((int)(fRed+.5f),(int)(fGreen+.5f),(int)(fBlue+.5f)),wxSOLID)));
    wxBrush suti(wxColour((int)(fRed+.5f),(int)(fGreen+.5f),(int)(fBlue+.5f)),wxSOLID);
    dc->SetBrush(suti);
    dc->DrawRectangle(rectSpan->left,rectSpan->top,rectSpan->right,rectSpan->bottom);
    dc->SetBrush(wxNullBrush);
    //dc->FillRect(rectSpan, &CBrush(color));
    
  } else {
    if(m_Modifiers==NULL || m_used3dlevel!=theApp.glob_prefs->Get3DDepth())
      BuildModifiers();
    
    RECT rect;
    memcpy(&rect, rectSpan, sizeof(RECT));
    
    int iTop = rect.top;
    int iBot = rect.bottom;
    int iMax = HALF(m_iHeight);
    for(int i = 0; i < iMax; i++) {
      //CBrush cbNew(RGB((int)(fRed * m_Modifiers[i] + .5f), (int)(fGreen * m_Modifiers[i] + .5f), (int)(fBlue * m_Modifiers[i] + .5f)));
      //wxBrush* cbNew=wxTheBrushList->FindOrCreateBrush(wxColour((int)(fRed * m_Modifiers[i] + .5f), (int)(fGreen * m_Modifiers[i] + .5f), (int)(fBlue * m_Modifiers[i] + .5f)),wxSOLID);
      wxBrush suti(wxColour((int)(fRed * m_Modifiers[i] + .5f), (int)(fGreen * m_Modifiers[i] + .5f), (int)(fBlue * m_Modifiers[i] + .5f)),wxSOLID);

      //printf("(väri=%d;%d,%d\n",(int)(fRed * m_Modifiers[i] + .5f), (int)(fGreen * m_Modifiers[i] + .5f), (int)(fBlue * m_Modifiers[i] + .5f));
      
      rect.top = iTop + i;
      rect.bottom = iTop + i + 1;
      //dc->SetBrush(*cbNew);
      dc->SetBrush(suti);

      //dc->FillRect(&rect, &cbNew);
      //printf("RC: %d,%d-%d,%d\n",rect.left,rect.top,rect.right,rect.bottom);
      int x,y,w,h;
      x=rect.left; y=rect.top;
      w=rect.right-rect.left; h=rect.bottom-rect.top;
      dc->DrawRectangle(x,y,w,h);
      
      rect.top = iBot - i - 1;
      rect.bottom = iBot - i;
      //printf("RC2:%d,%d-%d,%d\n",rect.left,rect.top,rect.right,rect.bottom);
      //dc->FillRect(&rect, &cbNew);
      x=rect.left; y=rect.top;
      w=rect.right-rect.left; h=rect.bottom-rect.top;
      dc->DrawRectangle(x,y,w,h);

      dc->SetBrush(wxNullBrush);
    }
  }
  //printf(" FillRect took %d ms\n",end-start);
}
