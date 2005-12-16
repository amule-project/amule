//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include <wx/dcclient.h>
#include <wx/dc.h>

#include "ColorFrameCtrl.h"	// Interface declarations
#include "Color.h"		// Needed for RGB, GetRValue, GetGValue and GetBValue

/////////////////////////////////////////////////////////////////////////////
// CColorFrameCtrl
CColorFrameCtrl::CColorFrameCtrl( wxWindow* parent,int id, int wid,int hei )
  : wxControl(parent,id,wxDefaultPosition,wxSize(wid,hei)),
    m_brushBack(wxColour(0,0,0),wxSOLID),
    m_brushFrame(wxColour(0,255,255),wxSOLID)
{
	m_crBackColor  = RGB(  0,   0,   0) ;  // see also SetBackgroundColor
	m_crFrameColor  = RGB(  0, 255, 255) ;  // see also SetFrameColor
	SetSizeHints(wid,hei,wid,hei,0,0);
	wxRect rc=GetClientRect();
	m_rectClient.left=rc.x;
	m_rectClient.top=rc.y;
	m_rectClient.right=rc.x + wid;
	m_rectClient.bottom=rc.y + hei;

}  // CColorFrameCtrl

/////////////////////////////////////////////////////////////////////////////
CColorFrameCtrl::~CColorFrameCtrl()
{
} // ~CColorFrameCtrl


BEGIN_EVENT_TABLE(CColorFrameCtrl,wxControl)
  EVT_PAINT(CColorFrameCtrl::OnPaint)
  EVT_SIZE(CColorFrameCtrl::OnSize)
END_EVENT_TABLE()

/////////////////////////////////////////////////////////////////////////////
// CColorFrameCtrl message handlers

/////////////////////////////////////////////////////////////////////////////
void CColorFrameCtrl::SetFrameColor( COLORREF color )
{
	m_crFrameColor = color;
	m_brushFrame.SetColour(wxColour(GetRValue(m_crFrameColor),
					GetGValue(m_crFrameColor),
					GetBValue(m_crFrameColor)));

	Refresh(FALSE);

}  // SetFrameColor


/////////////////////////////////////////////////////////////////////////////
void CColorFrameCtrl::SetBackgroundColor(COLORREF color)
{
	m_crBackColor = color ;

	m_brushBack.SetColour(wxColour(GetRValue(m_crBackColor),
				       GetGValue(m_crBackColor),
				       GetBValue(m_crBackColor)));

	// clear out the existing garbage, re-start with a clean plot
	Refresh(FALSE);

}  // SetBackgroundColor

 
////////////////////////////////////////////////////////////////////////////
void CColorFrameCtrl::OnPaint(wxPaintEvent& WXUNUSED(evt)) 
{
  wxPaintDC dc(this);

  wxRect rc;
  rc.x=m_rectClient.left;
  rc.y=m_rectClient.top;
  rc.width=m_rectClient.right-m_rectClient.left;
  rc.height=m_rectClient.bottom-m_rectClient.top;

  dc.SetPen(*wxTRANSPARENT_PEN);
  dc.SetBrush(m_brushBack);
  dc.DrawRectangle(rc);

  wxPen kyna(wxColour(0,0,0),1,wxSOLID);
  dc.SetPen(kyna);
  dc.DrawLine(rc.x+1,rc.y+1,rc.x+rc.width-2,rc.y+1);
  dc.DrawLine(rc.x+rc.width-2,rc.y+1,rc.x+rc.width-2,rc.y+rc.height-2);
  dc.DrawLine(rc.x+rc.width-2,rc.y+rc.height-2,rc.x+1,rc.y+rc.height-2);
  dc.DrawLine(rc.x+1,rc.y+rc.height-2,rc.x+1,rc.y+1);

  dc.SetPen(*wxWHITE_PEN);
  dc.DrawLine(rc.x+rc.width-1,rc.y,rc.x+rc.width-1,rc.y+rc.height-1);
  dc.DrawLine(rc.x+rc.width-1,rc.y+rc.height-1,rc.x,rc.y+rc.height-1);
  dc.SetPen(*wxGREY_PEN);
  dc.DrawLine(rc.x+rc.width,rc.y,rc.x,rc.y);
  dc.DrawLine(rc.x,rc.y,rc.x,rc.y+rc.height);

} // OnPaint

/////////////////////////////////////////////////////////////////////////////
void CColorFrameCtrl::OnSize(wxSizeEvent& WXUNUSED(evt)) 
{

	// NOTE: OnSize automatically gets called during the setup of the control
	// Kry - Not on Mac.

	wxRect rc=GetClientRect();
	m_rectClient.left=rc.x;
	m_rectClient.top=rc.y;
	m_rectClient.right=rc.x + rc.width;
	m_rectClient.bottom=rc.y + rc.height;

} // OnSize
