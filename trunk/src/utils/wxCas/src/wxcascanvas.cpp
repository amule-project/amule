/////////////////////////////////////////////////////////////////////////////
// Name:        wxcascanvas.cpp
// Purpose:     wxCas canvas
// Author:      ThePolish <thepolish@vipmail.ru>
// Created:     2004/04/15
// Modified by:
// Copyright:   (c) ThePolish <thepolish@vipmail.ru>
// Licence:     GPL
// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
// Pixmats from aMule http://www.amule.org
/////////////////////////////////////////////////////////////////////////////

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
	delete m_bitmap;
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
