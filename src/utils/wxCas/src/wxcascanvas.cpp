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

#include <wx/image.h>

#include "wxcascanvas.h"

#if defined(__WXGTK__) || defined(__WXMOTIF__) || wxUSE_XPM_IN_MSW
#include "../pixmaps/amule.xpm"
#endif

// Constructor
WxCasCanvas::WxCasCanvas (wxWindow * parent, wxWindow * model):wxPanel (parent,
	 -1)
{
  // Add Bitmap Splash
#if USE_XPM_BITMAPS
  m_bitmap = new wxBitmap (amule_xpm);
#else
  m_bitmap = new wxBITMAP (amule);
#endif

  m_model = model;

  // Draw Image
  DrawImg ();
}

// Destructor
WxCasCanvas::~WxCasCanvas ()
{
}

// Event table
BEGIN_EVENT_TABLE (WxCasCanvas, wxPanel)
EVT_PAINT (WxCasCanvas::OnPaint) 
END_EVENT_TABLE ()

// OnPaint event
void
     WxCasCanvas::OnPaint (wxPaintEvent & event)
{
  DrawImg ();
}

// Draw image
void
WxCasCanvas::DrawImg ()
{
// rescale image
  wxInt32 ph, pw, h;
  m_model->GetClientSize (&pw, &ph);
  h =
    (wxInt32) ((double) (m_bitmap->GetHeight ()) / m_bitmap->GetWidth () *
	       pw);

  wxBitmap *bitmap = new wxBitmap (wxImage (m_bitmap->ConvertToImage ()).
				   Scale (pw, h));

  this->SetClientSize (pw, h);

  // Draw image
  wxPaintDC dc (this);
  PrepareDC (dc);
  dc.DrawBitmap (*bitmap, 0, 0);
  delete bitmap;

  // Frame layout
  GetParent ()->Layout ();
  GetParent ()->Fit ();
}
