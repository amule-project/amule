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
#pragma implementation "wxcascte.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include <wx/filename.h>

#include "wxcascte.h"

const wxString
WxCasCte::AMULESIG_FILENAME (wxT("amulesig.dat"));
const wxString
WxCasCte::AMULESIG_IMG_NAME (wxT("aMule-online-sign"));

// Refresh rate limits
const wxUint32
WxCasCte::MIN_REFRESH_RATE = 1;
const wxUint32
WxCasCte::MAX_REFRESH_RATE = 3600;

// FTP update limits
const wxUint32
WxCasCte::MIN_FTP_RATE = 1;
const wxUint32
WxCasCte::MAX_FTP_RATE = 1440;


// Key config names
const wxString
WxCasCte::AMULESIG_DIR_KEY (wxT("OSDirectory"));
const wxString
WxCasCte::REFRESH_RATE_KEY (wxT("RefreshRate"));

const wxString
WxCasCte::ENABLE_AUTOSTATIMG_KEY (wxT("EnableAutoStatImg"));
const wxString
WxCasCte::AUTOSTATIMG_DIR_KEY (wxT("StatImgDirectory"));
const wxString
WxCasCte::AUTOSTATIMG_TYPE_KEY (wxT("StatImgType"));

const wxString
WxCasCte::ENABLE_FTP_UPDATE_KEY(wxT("EnableFtpUpdate"));
const wxString
WxCasCte::FTP_UPDATE_RATE_KEY (wxT("FtpUpdateRate"));
const wxString
WxCasCte::FTP_URL_KEY (wxT("FtpUrl"));
const wxString
WxCasCte::FTP_PATH_KEY (wxT("FtpPath"));
const wxString
WxCasCte::FTP_USER_KEY (wxT("FtpUser"));
const wxString
WxCasCte::FTP_PASSWD_KEY (wxT("FtpPasswd"));


// Default config parameters
const wxString
WxCasCte::DEFAULT_AMULESIG_PATH (wxFileName::GetHomeDir () +
                                 wxFileName::GetPathSeparator () + wxT(".aMule"));
const wxUint32
WxCasCte::DEFAULT_REFRESH_RATE = 5;

const bool
WxCasCte::DEFAULT_AUTOSTATIMG_ISENABLED = FALSE;
const wxString
WxCasCte::DEFAULT_AUTOSTATIMG_PATH (wxFileName::GetHomeDir ());
const wxString
WxCasCte::DEFAULT_AUTOSTATIMG_TYPE (wxT("PNG"));

const bool
WxCasCte::DEFAULT_FTP_UPDATE_ISENABLED = FALSE;
const wxUint32
WxCasCte::DEFAULT_FTP_UPDATE_RATE = 10;
const wxString
WxCasCte::DEFAULT_FTP_URL(wxT("ftp.myftp.cx"));
const wxString
WxCasCte::DEFAULT_FTP_PATH(wxT("/pub/myamuledir"));
const wxString
WxCasCte::DEFAULT_FTP_USER(wxT("anonymous"));
const wxString
WxCasCte::DEFAULT_FTP_PASSWD(wxT("whiterabit@here"));
