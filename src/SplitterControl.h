// This file is part of the aMule project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net ) 
// Copyright (c) 2003 Nguyen Huy Hung
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

/**************CSplitterControl interface***********
*	Class name :CSplitterControl
*	Purpose: Implement splitter control for dialog
*			or any other windows.
*	Author: Nguyen Huy Hung, Vietnamese student.
*	Date:	May 29th 2002.
*	Note: You can use it for any purposes. Feel free
*			to change, modify, but please do not 
*			remove this.
*	No warranty of any risks.
*	(:-)
*/

#ifndef SPLITTERCONTROL_H
#define SPLITTERCONTROL_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/stattext.h>	// Needed for wxStaticText

#include "types.h"		// Needed for DWORD

// SplitterControl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSplitterControl window
#ifndef WM_USER
	#define WM_USER 0x8000
#endif

#define SPN_SIZED WM_USER + 1
#define CW_LEFTALIGN 1
#define CW_RIGHTALIGN 2
#define CW_TOPALIGN 3
#define CW_BOTTOMALIGN 4
#define SPS_VERTICAL 1
#define SPS_HORIZONTAL 2

typedef struct SPC_NMHDR
{
  //NMHDR hdr;
	int delta;
} SPC_NMHDR;

class CSplitterControl : public wxStaticText
{
// Construction
public:
	CSplitterControl();

// Attributes
public:
protected:
	bool		m_bIsPressed;
	int			m_nType;
	int			m_nX, m_nY;
	int			m_nMin, m_nMax;
	int			m_nSavePos;		// Save point on the lbutton down 
								// message
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSplitterControl)
	//}}AFX_VIRTUAL

// Implementation
public:
	static void ChangePos(wxWindow* pWnd, int dx, int dy);
	static void ChangeWidth(wxWindow* pWnd, int dx, DWORD dwFlag = CW_LEFTALIGN);
	static void ChangeHeight(wxWindow* pWnd, int dy, DWORD dwFlag = CW_TOPALIGN);
public:
	void		SetRange(int nMin, int nMax);
	void		SetRange(int nSubtraction, int nAddition, int nRoot);

	int			GetStyle();
	int			SetStyle(int nStyle = SPS_VERTICAL);
	void		Create(DWORD dwStyle, const wxRect& rect, wxWindow* pParent, unsigned int nID);
	virtual		~CSplitterControl();

	// Generated message map functions
protected:
	virtual void	DrawLine(wxDC* pDC, int x, int y);
	void			MoveWindowTo(wxPoint pt);
#if 0
	//{{AFX_MSG(CSplitterControl)
	afx_msg void	OnPaint();
	afx_msg void	OnMouseMove(unsigned int nFlags, CPoint point);
	afx_msg bool	OnSetCursor(CWnd* pWnd, unsigned int nHitTest, unsigned int message);
	afx_msg void	OnLButtonDown(unsigned int nFlags, CPoint point);
	afx_msg void	OnLButtonUp(unsigned int nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
#endif
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // SPLITTERCONTROL_H
