//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         wxCasCanvas Class
///
/// Purpose:      Display aMule splash and resize it accordingly to the statistics panel size
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (C) 2004 by ThePolish
///
/// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
///
/// Pixmats from aMule http://www.amule.org
///
/// This program is free software; you can redistribute it and/or modify
///  it under the terms of the GNU General Public License as published by
/// the Free Software Foundation; either version 2 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the
/// Free Software Foundation, Inc.,
/// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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

#ifndef __WXMSW__
#include "../pixmaps/amule.xpm"
#endif

// Constructor
WxCasCanvas::WxCasCanvas (wxWindow * parent, wxWindow * model):wxPanel (parent,
        -1)
{
  // Add Bitmap Splash
  m_bitmap = wxBITMAP (amule);

  m_model = model;

  // Draw Image
  wxClientDC dc (this);
  DrawImg (&dc);
}

// Destructor
WxCasCanvas::~WxCasCanvas ()
{}

// Event table
BEGIN_EVENT_TABLE (WxCasCanvas, wxPanel)
EVT_PAINT (WxCasCanvas::OnPaint)
END_EVENT_TABLE ()

// OnPaint event
void
WxCasCanvas::OnPaint (wxPaintEvent & event)
{
  wxPaintDC dc (this);
  DrawImg (&dc);
}

// Update if model size has changed
void
WxCasCanvas::Update ()
{
  wxInt32 h, wS, wI;
  m_model->GetClientSize (&wS, &h);
  GetClientSize (&wI, &h);
  if (wI != wS)
    {
      wxClientDC dc (this);
      DrawImg (&dc);
    }
}

// Draw image
void
WxCasCanvas::DrawImg (wxDC *dc)
{
  // rescale image
  wxInt32 ph, pw, h;
  m_model->GetClientSize (&pw, &ph);

  h =
    (wxInt32) ((double) (m_bitmap.GetHeight ()) / m_bitmap.GetWidth () *
               pw);

  wxBitmap bitmap (wxImage (m_bitmap.ConvertToImage ()).
                   Scale (pw, h));

  SetClientSize (pw, h);

  // Draw image
  PrepareDC (*dc);
  dc->BeginDrawing();
  dc->DrawBitmap (bitmap, 0, 0);
  dc->EndDrawing();
}
