//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Name:        wxCas
// Purpose:    Display aMule Online Statistics
// Author:       ThePolish <thepolish@vipmail.ru>
// Copyright (C) 2004 by ThePolish
//
// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
// Pixmats from aMule http://www.amule.org
//
// This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the
// Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "wxcascanvas.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "wxcascanvas.h"

WxCasCanvas::WxCasCanvas (wxWindow * parent, wxBitmap * bitmap):wxPanel (parent, -1, wxDefaultPosition,
	 wxDefaultSize, wxTAB_TRAVERSAL,
	 "WxCasPanel")
{
	m_bitmap = bitmap;
	
	this->SetClientSize(m_bitmap->GetWidth(),m_bitmap->GetHeight());

	wxClientDC
	dc (this);
	dc.DrawBitmap (*m_bitmap, 0, 0);
}

WxCasCanvas::~WxCasCanvas ()
{
}

BEGIN_EVENT_TABLE (WxCasCanvas, wxPanel)
EVT_PAINT (WxCasCanvas::OnPaint) END_EVENT_TABLE ()
     void
     WxCasCanvas::OnPaint (wxPaintEvent & event)
{
	wxPaintDC dc (this);
	PrepareDC (dc);

	dc.DrawBitmap (*m_bitmap, 0, 0);
}
