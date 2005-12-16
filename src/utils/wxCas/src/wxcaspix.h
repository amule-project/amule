//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         WxCasPix Class
///
/// Purpose:      Monitor aMule Online Statistics by reading amulesig.dat file
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
/// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _WXCASPIX_H
#define _WXCASPIX_H

// For compilers that support precompilation, includes "wx/wx.h"
#include "wx/wxprec.h"

#ifdef __BORLANDC__
 #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
 #include "wx/wx.h"
#endif

class WxCasPix
{
private:
	static const char *m_about_xpm[];
	static const char *m_prefs_xpm[];
	static const char *m_print_xpm[];
	static const char *m_refresh_xpm[];
	static const char *m_save_xpm[];
	static const char *m_stat_xpm[];
	static const char *m_stop_xpm[];
#ifndef __WXMSW__
	static const char *m_wxcas_xpm[];
#endif

public:
	static wxBitmap getPixmap( const wxString& name );
};

#endif /* _WXCASPIX_H */
