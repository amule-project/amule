// This file is part of the aMule project.
//
// Copyright (c) 2003-2004 aMule Project
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


#include <wx/window.h>
#include <wx/stattext.h>

#include "SplitterControl.h"	// Interface declarations

/////////////////////////////////////////////////////////////////////////////
// CSplitterControl

// hCursor1 is for vertiacal one
// and hCursor2 is for horizontal one
#if 0
static HCURSOR SplitterControl_hCursor1 = NULL;
static HCURSOR SplitterControl_hCursor2 = NULL;
#endif

CSplitterControl::CSplitterControl()
{
	// Mouse is pressed down or not ?
	m_bIsPressed = FALSE;	

	// Min and Max range of the splitter.
	m_nMin = m_nMax = -1;
}

CSplitterControl::~CSplitterControl()
{
}


#if 0
BEGIN_MESSAGE_MAP(CSplitterControl, CStatic)
	//{{AFX_MSG_MAP(CSplitterControl)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif

/////////////////////////////////////////////////////////////////////////////
// CSplitterControl message handlers

/****************************************************
*	Create(DWORD dwStyle, const CRect& rect, CWnd* pParent, nID)
*	Use this function instead of the CStatic::Create function
* Parameters: No need to explain (see MSDN (:-) )
*
****************************************************/
#if 0
void CSplitterControl::Create(DWORD dwStyle, const CRect &rect, CWnd *pParent, UINT nID)
{
	CRect rc = rect;
	dwStyle |= SS_NOTIFY;
	
	// Determine default type base on it's size.
	m_nType = (rc.Width() < rc.Height())?
					SPS_VERTICAL:
					SPS_HORIZONTAL;
	
	if (m_nType == SPS_VERTICAL)
		rc.right = rc.left + 5;
	else // SPS_HORIZONTAL
		rc.bottom = rc.top + 5;
	
	CStatic::Create("", dwStyle, rc, pParent, nID);
	
	if (!SplitterControl_hCursor1)
	{
		SplitterControl_hCursor1 = AfxGetApp()->LoadStandardCursor(IDC_SIZEWE);
		SplitterControl_hCursor2 = AfxGetApp()->LoadStandardCursor(IDC_SIZENS);
	}
	
	// force the splitter not to be splited.
	SetRange(0, 0, -1);
}
#endif

// Set style for splitter control
// nStyle = SPS_VERTICAL or SPS_HORIZONTAL
int CSplitterControl::SetStyle(int nStyle)
{
	int m_nOldStyle = m_nType;
	m_nType = nStyle;
	return m_nOldStyle;
}
int CSplitterControl::GetStyle()
{
	return m_nType;
}

#if 0
void CSplitterControl::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	CRect rcClient;
	GetClientRect(rcClient);
	
	CBrush br, *pOB;
	CPen pen, *pOP;

	dc.Draw3dRect(rcClient, GetSysColor(COLOR_BTNHIGHLIGHT), GetSysColor(COLOR_BTNSHADOW));	
	rcClient.DeflateRect(1,1,1,1);
	
	pen.CreatePen(0, 1, RGB(200, 200, 200));
	br.CreateSolidBrush(RGB(200, 220, 220));
	pOB = dc.SelectObject(&br);
	pOP = dc.SelectObject(&pen);
	
	dc.Rectangle(rcClient);

	// Restore pen and brush
	DeleteObject(dc.SelectObject(pOB));
	DeleteObject(dc.SelectObject(pOP));
}
#endif

#if 0
void CSplitterControl::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_bIsPressed)
	{
		CWindowDC dc(NULL);
		DrawLine(&dc, m_nX, m_nY);
		
		CPoint pt = point;
		ClientToScreen(&pt);
		GetParent()->ScreenToClient(&pt);

		if (pt.x < m_nMin)
			pt.x = m_nMin;
		if (pt.y < m_nMin)
			pt.y = m_nMin;

		if (pt.x > m_nMax)
			pt.x = m_nMax;
		if (pt.y > m_nMax)
			pt.y = m_nMax;

		GetParent()->ClientToScreen(&pt);
		m_nX = pt.x;
		m_nY = pt.y;
		DrawLine(&dc, m_nX, m_nY);
	}
	CStatic::OnMouseMove(nFlags, point);
}
#endif

#if 0
bool CSplitterControl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	if (nHitTest == HTCLIENT)
	{
		(m_nType == SPS_VERTICAL)?(::SetCursor(SplitterControl_hCursor1))
			:(::SetCursor(SplitterControl_hCursor2));
		return 0;
	}
	else
		return CStatic::OnSetCursor(pWnd, nHitTest, message);
}
#endif

#if 0
void CSplitterControl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CStatic::OnLButtonDown(nFlags, point);
	
	m_bIsPressed = TRUE;
	SetCapture();
	CRect rcWnd;
	GetWindowRect(rcWnd);
	
	if (m_nType == SPS_VERTICAL)
		m_nX = rcWnd.left + rcWnd.Width() / 2;	
	
	else
		m_nY = rcWnd.top  + rcWnd.Height() / 2;
	
	if (m_nType == SPS_VERTICAL)
		m_nSavePos = m_nX;
	else
		m_nSavePos = m_nY;

	CWindowDC dc(NULL);
	DrawLine(&dc, m_nX, m_nY);
}
#endif

#if 0
void CSplitterControl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (m_bIsPressed)
	{
		ClientToScreen(&point);
		CWindowDC dc(NULL);

		DrawLine(&dc, m_nX, m_nY);
		CPoint pt(m_nX, m_nY);
		m_bIsPressed = FALSE;
		CWnd *pOwner = GetOwner();
		if (pOwner && IsWindow(pOwner->m_hWnd))
		{
			CRect rc;
			int delta;
			pOwner->GetClientRect(rc);
			pOwner->ScreenToClient(&pt);
			MoveWindowTo(pt);

			if (m_nType == SPS_VERTICAL)
				delta = m_nX - m_nSavePos;
			else
				delta = m_nY - m_nSavePos;
			
			
			SPC_NMHDR nmsp;
		
			nmsp.hdr.hwndFrom = m_hWnd;
			nmsp.hdr.idFrom   = GetDlgCtrlID();
			nmsp.hdr.code     = SPN_SIZED;
			nmsp.delta = delta;

			pOwner->SendMessage(WM_NOTIFY, nmsp.hdr.idFrom, (LPARAM)&nmsp);
		}
	}

	CStatic::OnLButtonUp(nFlags, point);
	ReleaseCapture();
}
#endif

void CSplitterControl::DrawLine(wxDC* pDC, int x, int y)
{
#if 0
	int nRop = pDC->SetROP2(R2_NOTXORPEN);

	CRect rcWnd;
	int d = 1;
	GetWindowRect(rcWnd);
	CPen  pen;
	pen.CreatePen(0, 1, RGB(200, 200, 200));
	CPen *pOP = pDC->SelectObject(&pen);
	
	if (m_nType == SPS_VERTICAL)
	{
		pDC->MoveTo(m_nX - d, rcWnd.top);
		pDC->LineTo(m_nX - d, rcWnd.bottom);

		pDC->MoveTo(m_nX + d, rcWnd.top);
		pDC->LineTo(m_nX + d, rcWnd.bottom);
	}
	else // m_nType == SPS_HORIZONTAL
	{
		pDC->MoveTo(rcWnd.left, m_nY - d);
		pDC->LineTo(rcWnd.right, m_nY - d);
		
		pDC->MoveTo(rcWnd.left, m_nY + d);
		pDC->LineTo(rcWnd.right, m_nY + d);
	}
	pDC->SetROP2(nRop);
	pDC->SelectObject(pOP);
#endif
}

void CSplitterControl::MoveWindowTo(wxPoint pt)
{
#if 0
	CRect rc;
	GetWindowRect(rc);
	CWnd* pParent;
	pParent = GetParent();
	if (!pParent || !::IsWindow(pParent->m_hWnd))
		return;

	pParent->ScreenToClient(rc);
	if (m_nType == SPS_VERTICAL)
	{	
		int nMidX = (rc.left + rc.right) / 2;
		int dx = pt.x - nMidX;
		rc.OffsetRect(dx, 0);
	}
	else
	{	
		int nMidY = (rc.top + rc.bottom) / 2;
		int dy = pt.y - nMidY;
		rc.OffsetRect(0, dy);
	}
	MoveWindow(rc);
#endif
}

void CSplitterControl::ChangeWidth(wxWindow *pWnd, int dx, DWORD dwFlag)
{
#if 0
	CWnd* pParent = pWnd->GetParent();
	if (pParent && ::IsWindow(pParent->m_hWnd))
	{
		CRect rcWnd;
		pWnd->GetWindowRect(rcWnd);
		pParent->ScreenToClient(rcWnd);
		if (dwFlag == CW_LEFTALIGN)
			rcWnd.right += dx;
		else if (dwFlag == CW_RIGHTALIGN)
			rcWnd.left -= dx;
		pWnd->MoveWindow(rcWnd);
	}
#endif
}

void CSplitterControl::ChangeHeight(wxWindow *pWnd, int dy, DWORD dwFlag)
{
#if 0
	CWnd* pParent = pWnd->GetParent();
	if (pParent && ::IsWindow(pParent->m_hWnd))
	{
		CRect rcWnd;
		pWnd->GetWindowRect(rcWnd);
		pParent->ScreenToClient(rcWnd);
		if (dwFlag == CW_TOPALIGN)
			rcWnd.bottom += dy;
		else if (dwFlag == CW_BOTTOMALIGN)
			rcWnd.top -= dy;
		pWnd->MoveWindow(rcWnd);
	}
#endif
}

void CSplitterControl::ChangePos(wxWindow* pWnd, int dx, int dy)
{
#if 0
	CWnd* pParent = pWnd->GetParent();
	if (pParent && ::IsWindow(pParent->m_hWnd))
	{
		CRect rcWnd;
		pWnd->GetWindowRect(rcWnd);
		pParent->ScreenToClient(rcWnd);
		rcWnd.OffsetRect(-dx, dy);

		pWnd->MoveWindow(rcWnd);
	}	
#endif
}

void CSplitterControl::SetRange(int nMin, int nMax)
{
	m_nMin = nMin;
	m_nMax = nMax;
}

// Set splitter range from (nRoot - nSubtraction) to (nRoot + nAddition)
// If (nRoot < 0)
//		nRoot =  <current position of the splitter>
void CSplitterControl::SetRange(int nSubtraction, int nAddition, int nRoot)
{
#if 0
	if (nRoot < 0)
	{
		CRect rcWnd;
		GetWindowRect(rcWnd);
		if (m_nType == SPS_VERTICAL)
			nRoot = rcWnd.left + rcWnd.Width() / 2;
		else // if m_nType == SPS_HORIZONTAL
			nRoot = rcWnd.top + rcWnd.Height() / 2;
	}
	m_nMin = nRoot - nSubtraction;
	m_nMax = nRoot + nAddition;
#endif
}
