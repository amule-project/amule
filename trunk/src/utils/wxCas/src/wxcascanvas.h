/////////////////////////////////////////////////////////////////////////////
// Name:        wxcascanvas.h
// Purpose:     wxCas canvas
// Author:      ThePolish <thepolish@vipmail.ru>
// Created:     2004/04/15
// Modified by:
// Copyright:   (c) ThePolish <thepolish@vipmail.ru>
// Licence:     GPL
// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXCASCANVAS_H
#define _WXCASCANVAS_H

#ifdef __GNUG__
#pragma interface "wxcascanvas.h"
#endif

// Include wxWindows' headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif


// wxCas Frame
class WxCasCanvas:public wxPanel
{
      public:

	//Constructor
	WxCasCanvas (wxWindow * parent, wxBitmap * bitmap);

	//Destructor
	~WxCasCanvas ();

      protected:

	void OnPaint (wxPaintEvent & event);


	  DECLARE_EVENT_TABLE ();

      private:

	  wxBitmap * m_bitmap;
};

#endif /* _WXCASCANVAS_H */
