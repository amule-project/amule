/////////////////////////////////////////////////////////////////////////////
// Name:        wxcas.h
// Purpose:     wxCas App
// Author:      ThePolish <thepolish@vipmail.ru>
// Created:     2003/04/10
// Modified by:
// Copyright:   (c) ThePolish <thepolish@vipmail.ru>
// Licence:     GPL
// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
/////////////////////////////////////////////////////////////////////////////


#ifndef _WXCAS_H
#define _WXCAS_H

#ifdef __GNUG__
#pragma interface "wxcas.h"
#endif

// Include wxWindows' headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

// Application
class WxCas:public wxApp
{
public:
  virtual bool OnInit ();
};

DECLARE_APP (WxCas)
#endif /* _WXCAS_H */
