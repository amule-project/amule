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

#ifndef _WXCASCTE_H
#define _WXCASCTE_H

#ifdef __GNUG__
#pragma interface "wxcascte.h"
#endif

// Include wxWindows' headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class WxCasCte
{
public:

  static const wxString AMULESIG_NAME;
  static const wxString AMULESIG_IMG_NAME;

  // Refresh rate limits
  static const wxUint32 MIN_REFRESH_RATE;
  static const wxUint32 MAX_REFRESH_RATE;

  // Key config names
  static const wxString AMULESIG_KEY;
  static const wxString REFRESH_RATE_KEY;
  static const wxString ENABLE_AUTOSTATIMG_KEY;
  static const wxString AUTOSTATIMG_DIR_KEY;
  

  // Default config parameters
  static const wxString AMULESIG_DEFAULT_PATH;
  static const wxUint32 DEFAULT_REFRESH_RATE;
  static const bool ENABLE_AUTOSTATIMG_DEFAULT;
  static const wxString AUTOSTATIMG_DEFAULT_PATH;
};

#endif /* _WXCASCTE_H */
