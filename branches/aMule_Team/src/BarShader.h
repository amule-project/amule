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

#ifndef BARSHADER_H
#define BARSHADER_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/dc.h>		// Needed for wxDC
#include "types.h"		// Needed for uint16 and uint32
#include "color.h"		// Needed for RGB

class CBarShader {
public:
	CBarShader(uint32 height = 1, uint32 width = 1);
	~CBarShader(void);

	//set the width of the bar
	void SetWidth(int width) {
		if(m_iWidth != width) {
			m_iWidth = width;
			m_dPixelsPerByte = (double)m_iWidth / m_uFileSize;
			m_dBytesPerPixel = (double)m_uFileSize / m_iWidth;
		}
	}

	//set the height of the bar
	void SetHeight(int height) {
		if(m_iHeight != height) {
			m_iHeight = height;

			BuildModifiers();
		}
	}

	//returns the width of the bar
	int GetWidth() {
		return m_iWidth;
	}

	//returns the height of the bar
	int GetHeight() {
		return m_iHeight;
	}

	//call this to blank the shaderwithout changing file size
	void Reset();

	//sets new file size and resets the shader
	void SetFileSize(uint32 fileSize);

	//fills in a range with a certain color, new ranges overwrite old
	void FillRange(uint32 start, uint32 end, DWORD color);

	//fills in entire range with a certain color
	void Fill(DWORD color);

	//draws the bar
	//void Draw(wxMemoryDC* dc, int iLeft, int iTop, bool bFlat);
	void Draw(wxDC* dc, int iLeft, int iTop, bool bFlat);

protected:
	void BuildModifiers();
	//void FillRect(wxMemoryDC *dc, RECT* rectSpan, float fRed, float fGreen, float fBlue, bool bFlat);
	void FillRect(wxDC *dc, RECT* rectSpan, float fRed, float fGreen, float fBlue, bool bFlat);
	//void FillRect(wxMemoryDC* dc, RECT* rectSpan, DWORD color, bool bFlat);
	void FillRect(wxDC* dc, RECT* rectSpan, DWORD color, bool bFlat);

	int    m_iWidth;
	int    m_iHeight;
	double m_dPixelsPerByte;
	double m_dBytesPerPixel;
	uint32 m_uFileSize;

private:
	struct BarSpan {
		uint32		start;
		uint32		end;
		DWORD	color;
		BarSpan		*next;

		BarSpan(uint32 s, uint32 e, DWORD cr = 0) {
			start = s;
			end = e;
			color = cr;
			next = NULL;
		}

		BarSpan(BarSpan *prev, uint32 s, uint32 e, DWORD cr) {
			start = s;
			end = e;
			color = cr;
			next = prev->next;
			prev->next = this;
		}

		void DeleteNext() {
			BarSpan *del = next;
			next = next->next;
			delete del;
		}

		void DeleteUpTo(BarSpan *last) {
			BarSpan *del = next;
			BarSpan *temp;
			while(del != last) {
				temp = del->next;
				delete del;
				del = temp;
			}
			next = last;
		}

		/*void DeleteAll() {
			DeleteUpTo(NULL);
			delete this;
		}*/
	};

	BarSpan* m_FirstSpan;
	float* m_Modifiers;
	uint16 m_used3dlevel;
};

#endif // BARSHADER_H
