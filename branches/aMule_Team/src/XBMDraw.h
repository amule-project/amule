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

#ifndef XBMDRAW_H
#define XBMDRAW_H

#include "types.h"		// Needed for BYTE
#include "CString.h"		// Needed for CString


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class XBMDraw  
{
public:
	bool Line(int x1, int y1, int x2, int y2, bool bXOR = false);
	CString GetImageTag();
	bool Plot(int x, int y, bool bXOR = false);
	bool GetImage(CString &sImage);
	bool CreateImage(CString sName, int nWidth, int nHeight, BYTE bBackground = 0x00);
	XBMDraw();
	virtual ~XBMDraw();
private:
	CString	m_sName;
	int		m_nWidth;
	int		m_nHeight;
	BYTE*	m_pImage;
};

#endif // XBMDRAW_H
