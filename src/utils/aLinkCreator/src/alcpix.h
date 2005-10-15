//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         AlcPix Class
///
/// Purpose:      aMule ed2k link creator
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (C) 2004 by ThePolish
///
/// Pixmaps from http://jimmac.musichall.cz/ikony.php3 | http://www.everaldo.com | http://www.icomania.com
///
/// This program is free software; you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
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

#ifndef _ALCPIX_H
#define _ALCPIX_H

// For compilers that support precompilation, includes "wx/wx.h"
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

// Switch between themes (select just one of them)
//#define WANT_KDE_THEME 1

class AlcPix
  {
  private:
    static const char *m_about_xpm[];
#ifndef __WXMSW__
    static const char *m_alc_xpm[];
#endif
    static const char *m_copy_xpm[];
    static const char *m_open_xpm[];
    static const char *m_saveas_xpm[];

  public:
    static wxBitmap getPixmap(const wxString& name);
  };

#endif /* _ALCPIX_H */
