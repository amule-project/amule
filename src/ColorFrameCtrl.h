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

#ifndef COLORFRAMECTRL_H
#define COLORFRAMECTRL_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/control.h>		// Needed for wxControl

#include "types.h"		// Needed for RECT
#include "color.h"		// Needed for COLORREF

/////////////////////////////////////////////////////////////////////////////
// CColorFrameCtrl window

class CColorFrameCtrl : public wxControl {
// Construction
public:
	CColorFrameCtrl( wxWindow* parent, int id,int wid,int hei );

// Attributes
public:
	void SetFrameColor(COLORREF color);
	void SetBackgroundColor(COLORREF color);

	// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorFrameCtrl)
	public:
	//virtual bool Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, unsigned int nID=0);
	//}}AFX_VIRTUAL

// Implementation
public:
	COLORREF m_crBackColor;        // background color
	COLORREF m_crFrameColor;       // frame color

	virtual ~CColorFrameCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorFrameCtrl)
	//afx_msg void OnPaint();
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);
	//ax_msg void OnSize(unsigned int nType, int cx, int cy); 
	//}}AFX_MSG
	//DECLARE_MESSAGE_MAP()
	DECLARE_EVENT_TABLE()

	  //CRect  m_rectClient;
	  //CBrush m_brushBack;
	  //CBrush m_brushFrame;
	  RECT m_rectClient;
	wxBrush m_brushBack,m_brushFrame;
};

/////////////////////////////////////////////////////////////////////////////

#endif // COLORFRAMECTRL_H
