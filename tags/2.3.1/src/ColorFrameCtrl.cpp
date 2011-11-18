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

#include <wx/dcclient.h>

#include "ColorFrameCtrl.h"	// Interface declarations
#include "MuleColour.h"

/////////////////////////////////////////////////////////////////////////////
// CColorFrameCtrl
CColorFrameCtrl::CColorFrameCtrl( wxWindow* parent,int id, int wid,int hei )
  : wxControl(parent,id,wxDefaultPosition,wxSize(wid,hei)),
    m_brushBack(*wxBLACK_BRUSH),
    m_brushFrame(CMuleColour(0,255,255).GetBrush())
{
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
void CColorFrameCtrl::SetFrameBrushColour(const wxColour& colour)
{
	m_brushFrame = *(wxTheBrushList->FindOrCreateBrush(colour, wxSOLID));

	Refresh(FALSE);
}  // SetFrameColor


/////////////////////////////////////////////////////////////////////////////
void CColorFrameCtrl::SetBackgroundBrushColour(const wxColour& colour)
{
	m_brushBack = *(wxTheBrushList->FindOrCreateBrush(colour, wxSOLID));

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

  dc.SetPen(*wxBLACK_PEN);
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
// File_checked_for_headers
