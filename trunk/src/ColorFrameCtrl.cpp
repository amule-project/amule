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

#include <wx/dcclient.h>
#include <wx/dc.h>

#include "ColorFrameCtrl.h"	// Interface declarations
#include "color.h"		// Needed for RGB, GetRValue, GetGValue and GetBValue

/////////////////////////////////////////////////////////////////////////////
// CColorFrameCtrl
CColorFrameCtrl::CColorFrameCtrl( wxWindow* parent,int id, int wid,int hei )
  : wxControl(parent,id,wxDefaultPosition,wxSize(wid,hei)),
    m_brushBack(wxColour(0,0,0),wxSOLID),
    m_brushFrame(wxColour(0,255,255),wxSOLID)
{
	m_crBackColor  = RGB(  0,   0,   0) ;  // see also SetBackgroundColor
	m_crFrameColor  = RGB(  0, 255, 255) ;  // see also SetFrameColor

	//m_brushBack.CreateSolidBrush( m_crBackColor ) ;
	//m_brushFrame.CreateSolidBrush( m_crFrameColor );

}  // CColorFrameCtrl

/////////////////////////////////////////////////////////////////////////////
CColorFrameCtrl::~CColorFrameCtrl()
{
  //m_brushFrame.DeleteObject() ;
  //m_brushBack.DeleteObject();
} // ~CColorFrameCtrl


#if 0
BEGIN_MESSAGE_MAP(CColorFrameCtrl, CWnd)
	//{{AFX_MSG_MAP(CColorFrameCtrl)
	ON_WM_PAINT()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif

BEGIN_EVENT_TABLE(CColorFrameCtrl,wxControl)
  EVT_PAINT(CColorFrameCtrl::OnPaint)
  EVT_SIZE(CColorFrameCtrl::OnSize)
END_EVENT_TABLE()

/////////////////////////////////////////////////////////////////////////////
// CColorFrameCtrl message handlers

#if 0
/////////////////////////////////////////////////////////////////////////////
bool CColorFrameCtrl::Create(DWORD dwStyle, const RECT& rect, 
		                     CWnd* pParentWnd, UINT nID) 
{
	bool result ;
	static CString className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW) ;

	result = CWnd::CreateEx( WS_EX_STATICEDGE, 
		                      className, NULL, dwStyle, 
		                      rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,
		                      pParentWnd->GetSafeHwnd(), (HMENU)nID) ;
	if (result != 0)
		Invalidate() ;

	return result ;

} // Create
#endif

/////////////////////////////////////////////////////////////////////////////
void CColorFrameCtrl::SetFrameColor( COLORREF color )
{
	m_crFrameColor = color;
	//m_brushFrame.DeleteObject() ;
	//m_brushFrame.CreateSolidBrush(m_crFrameColor) ;
	m_brushFrame.SetColour(wxColour(GetRValue(m_crFrameColor),
					GetGValue(m_crFrameColor),
					GetBValue(m_crFrameColor)));

	// clear out the existing garbage, re-start with a clean plot
	//Invalidate() ;
	Refresh(FALSE);

}  // SetFrameColor


/////////////////////////////////////////////////////////////////////////////
void CColorFrameCtrl::SetBackgroundColor(COLORREF color)
{
	m_crBackColor = color ;

	//m_brushBack.DeleteObject() ;
	//m_brushBack.CreateSolidBrush(m_crBackColor) ;
	m_brushBack.SetColour(wxColour(GetRValue(m_crBackColor),
				       GetGValue(m_crBackColor),
				       GetBValue(m_crBackColor)));

	// clear out the existing garbage, re-start with a clean plot
	//Invalidate() ;
	Refresh(FALSE);

}  // SetBackgroundColor

 
////////////////////////////////////////////////////////////////////////////
void CColorFrameCtrl::OnPaint(wxPaintEvent& evt) 
{
  //CPaintDC dc(this) ;  // device context for painting
  wxPaintDC dc(this);

  wxRect rc;
  rc.x=m_rectClient.left;
  rc.y=m_rectClient.top;
  rc.width=m_rectClient.right-m_rectClient.left;
  rc.height=m_rectClient.bottom-m_rectClient.top;

  //rc.Deflate(1,1);
  //rc.Offset(1,1);
  //rc.Inflate(-1,-1);

  dc.SetPen(*wxTRANSPARENT_PEN);
  dc.SetBrush(m_brushBack);
  dc.DrawRectangle(rc);

  wxPen kyna(wxColour(0,0,0),1,wxSOLID);
  dc.SetPen(kyna);
  dc.DrawLine(rc.x+1,rc.y+1,rc.x+rc.width-2,rc.y+1);
  dc.DrawLine(rc.x+rc.width-2,rc.y+1,rc.x+rc.width-2,rc.y+rc.height-2);
  dc.DrawLine(rc.x+rc.width-2,rc.y+rc.height-2,rc.x+1,rc.y+rc.height-2);
  dc.DrawLine(rc.x+1,rc.y+rc.height-2,rc.x+1,rc.y+1);

  // 3d-luuk
  //rc.Offset(-1,-1);
  //rc.Inflate(1,1);

  dc.SetPen(*wxWHITE_PEN);
  dc.DrawLine(rc.x+rc.width-1,rc.y,rc.x+rc.width-1,rc.y+rc.height-1);
  dc.DrawLine(rc.x+rc.width-1,rc.y+rc.height-1,rc.x,rc.y+rc.height-1);
  dc.SetPen(*wxGREY_PEN);
  dc.DrawLine(rc.x+rc.width,rc.y,rc.x,rc.y);
  dc.DrawLine(rc.x,rc.y,rc.x,rc.y+rc.height);

  //dc.FillRect( m_rectClient, &m_brushBack ) ;
  //dc.FrameRect( m_rectClient, &m_brushFrame );

} // OnPaint


/////////////////////////////////////////////////////////////////////////////
void CColorFrameCtrl::OnSize(wxSizeEvent& evt) 
{
  //CWnd::OnSize(nType, cx, cy) ;

  // NOTE: OnSize automatically gets called during the setup of the control
	
  //GetClientRect(m_rectClient) ;
  wxRect rc=GetClientRect();
  m_rectClient.left=rc.x;
  m_rectClient.top=rc.y;
  m_rectClient.right=rc.x+rc.width;
  m_rectClient.bottom=rc.y+rc.height;

} // OnSize
