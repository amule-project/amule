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

#ifndef _WXCASCANVAS_H
#define _WXCASCANVAS_H

#ifdef __GNUG__
#pragma interface "wxcascanvas.h"
#endif

// Include wxWindows' headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif


/// Splash canvas
class WxCasCanvas:public wxPanel
  {
  private:
    wxBitmap m_bitmap;
    wxWindow *m_model;

  protected:
    void OnPaint (wxPaintEvent & event);

    DECLARE_EVENT_TABLE ();

  public:
    ///Constructor
    WxCasCanvas (wxWindow * parent, wxWindow * model);

    ///Destructor
    ~WxCasCanvas ();

    /// Draw aMule splash
    void DrawImg (wxDC *dc);

    /// Update splash on model resizing
    void Update ();
  };

#endif /* _WXCASCANVAS_H */
