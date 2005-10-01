//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         wxCasCte Structure
///
/// Purpose:      Store constants used in wxCas application
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

#ifndef _WXCASCTE_H
#define _WXCASCTE_H

#ifdef __GNUG__
#pragma interface "wxcascte.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h"
#include "wx/wxprec.h"

#ifdef __BORLANDC__
 #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
 #include "wx/wx.h"
#endif

/// Constants used in wxCas
struct WxCasCte
{
	/// Name of amulesig.dat file
	static const wxString AMULESIG_FILENAME;

	/// Name of the generated statistics image
	static const wxString AMULESIG_IMG_NAME;

	/// Refresh rate minimum limit
	static const wxUint32 MIN_REFRESH_RATE;

	/// Refresh rate maximum limit
	static const wxUint32 MAX_REFRESH_RATE;

	/// FTP update rate minimum limit
	static const wxUint32 MIN_FTP_RATE;

	/// FTP update rate maximum limit
	static const wxUint32 MAX_FTP_RATE;

	// Key config names

	/// Configuration key for amulesig.dat file's directory
	static const wxString AMULESIG_PATH_KEY;

	/// Configuration key for refresh rate
	static const wxString REFRESH_RATE_KEY;

	/// Configuration key for enabling autogenation of statistics image
	static const wxString ENABLE_AUTOSTATIMG_KEY;

	/// Configuration key for auto saving statistics image directory
	static const wxString AUTOSTATIMG_DIR_KEY;

	/// Configuration key for auto saving statistics image type
	static const wxString AUTOSTATIMG_TYPE_KEY;

	/// Configuration key for enabling FTP auto update
	static const wxString ENABLE_FTP_UPDATE_KEY;

	/// Configuration key for FTP update rate
	static const wxString FTP_UPDATE_RATE_KEY;

	/// Configuration key for FTP URL
	static const wxString FTP_URL_KEY;

	/// Configuration key for FTP path
	static const wxString FTP_PATH_KEY;

	/// Configuration key for FTP login username
	static const wxString FTP_USER_KEY;

	/// Configuration key for FTP login password
	static const wxString FTP_PASSWD_KEY;

	/// Configuration key storing maximum DL rate during previous wxCas runs
	static const wxString ABSOLUTE_MAX_DL_KEY;

	/// Configuration key storing maximum DL rate date during previous wxCas runs
	static const wxString ABSOLUTE_MAX_DL_DATE_KEY;

	// Default config parameters

	/// Configuration default amulesig.dat file's directory
	static const wxString DEFAULT_AMULESIG_PATH;

	/// Configuration default for refresh rate
	static const wxUint32 DEFAULT_REFRESH_RATE;

	/// Configuration default for enabling autogenation of statistics image
	static const bool DEFAULT_AUTOSTATIMG_ISENABLED;

	/// Configuration default for auto saving statistics image directory
	static const wxString DEFAULT_AUTOSTATIMG_PATH;

	/// Configuration default for auto saving statistics image type
	static const wxString DEFAULT_AUTOSTATIMG_TYPE;

	/// Configuration default for enabling FTP auto update
	static const bool DEFAULT_FTP_UPDATE_ISENABLED;

	/// Configuration default for FTP update rate
	static const wxUint32 DEFAULT_FTP_UPDATE_RATE;

	/// Configuration default for FTP URL
	static const wxString DEFAULT_FTP_URL;

	/// Configuration default for FTP path
	static const wxString DEFAULT_FTP_PATH;

	/// Configuration default for FTP login username
	static const wxString DEFAULT_FTP_USER;

	/// Configuration default for FTP login password
	static const wxString DEFAULT_FTP_PASSWD;
};

wxString GetDefaultAmulesigPath();

#endif /* _WXCASCTE_H */
