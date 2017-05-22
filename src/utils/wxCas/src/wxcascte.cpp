//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         wxCasCte Structure
///
/// Purpose:      Store constants used in wxCas application
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (c) 2004-2011 ThePolish ( thepolish@vipmail.ru )
///
/// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
///
/// Pixmaps from aMule http://www.amule.org
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


#include <wx/filename.h>

#ifdef __WXMAC__
	#include <CoreServices/CoreServices.h> // Do_not_auto_remove
	#if wxCHECK_VERSION(2, 9, 0)
		#include <wx/osx/core/cfstring.h>  // Do_not_auto_remove
	#else
		#include <wx/mac/corefoundation/cfstring.h>  // Do_not_auto_remove
	#endif
	#include <wx/intl.h> // Do_not_auto_remove
#elif defined(__WINDOWS__)
	#include <winerror.h> // Do_not_auto_remove
	#include <shlobj.h> // Do_not_auto_remove
	#include <wx/msw/winundef.h>
#endif

#include "wxcascte.h"

const wxString
WxCasCte::AMULESIG_FILENAME ( wxT( "amulesig.dat" ) );
const wxString
WxCasCte::AMULESIG_IMG_NAME ( wxT( "aMule-online-sign" ) );

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
WxCasCte::AMULESIG_PATH_KEY ( wxT( "OSDirectory" ) );
const wxString
WxCasCte::REFRESH_RATE_KEY ( wxT( "RefreshRate" ) );

const wxString
WxCasCte::ENABLE_AUTOSTATIMG_KEY ( wxT( "EnableAutoStatImg" ) );
const wxString
WxCasCte::AUTOSTATIMG_DIR_KEY ( wxT( "StatImgDirectory" ) );
const wxString
WxCasCte::AUTOSTATIMG_TYPE_KEY ( wxT( "StatImgType" ) );

const wxString
WxCasCte::ENABLE_FTP_UPDATE_KEY( wxT( "EnableFtpUpdate" ) );
const wxString
WxCasCte::FTP_UPDATE_RATE_KEY ( wxT( "FtpUpdateRate" ) );
const wxString
WxCasCte::FTP_URL_KEY ( wxT( "FtpUrl" ) );
const wxString
WxCasCte::FTP_PATH_KEY ( wxT( "FtpPath" ) );
const wxString
WxCasCte::FTP_USER_KEY ( wxT( "FtpUser" ) );
const wxString
WxCasCte::FTP_PASSWD_KEY ( wxT( "FtpPasswd" ) );

const wxString
WxCasCte::ABSOLUTE_MAX_DL_KEY ( wxT( "AbsoluteMaxDL" ) );
const wxString
WxCasCte::ABSOLUTE_MAX_DL_DATE_KEY ( wxT( "AbsoluteMaxDlDate" ) );

// Default config parameters
const wxString
WxCasCte::DEFAULT_AMULESIG_PATH ( GetDefaultAmulesigPath() );
const wxUint32
WxCasCte::DEFAULT_REFRESH_RATE = 5;

const bool
WxCasCte::DEFAULT_AUTOSTATIMG_ISENABLED = FALSE;
const wxString
WxCasCte::DEFAULT_AUTOSTATIMG_PATH ( wxFileName::GetHomeDir () );
const wxString
WxCasCte::DEFAULT_AUTOSTATIMG_TYPE ( wxT( "PNG" ) );

const bool
WxCasCte::DEFAULT_FTP_UPDATE_ISENABLED = FALSE;
const wxUint32
WxCasCte::DEFAULT_FTP_UPDATE_RATE = 10;
const wxString
WxCasCte::DEFAULT_FTP_URL( wxT( "ftp.myftp.cx" ) );
const wxString
WxCasCte::DEFAULT_FTP_PATH( wxT( "/pub/myamuledir" ) );
const wxString
WxCasCte::DEFAULT_FTP_USER( wxT( "anonymous" ) );
const wxString
WxCasCte::DEFAULT_FTP_PASSWD( wxT( "whiterabit@here" ) );

wxString GetDefaultAmulesigPath()
{
	wxString strDir;

#ifdef __WXMAC__

	FSRef fsRef;
	if (FSFindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &fsRef) == noErr)
	{
		CFURLRef	urlRef		= CFURLCreateFromFSRef(NULL, &fsRef);
		CFStringRef	cfString	= CFURLCopyFileSystemPath(urlRef, kCFURLPOSIXPathStyle);
		CFRelease(urlRef) ;
		#if wxCHECK_VERSION(2, 9, 0)
			strDir = wxCFStringRef(cfString).AsString(wxLocale::GetSystemEncoding())
			+ wxFileName::GetPathSeparator() + wxT("aMule");
		#else
			strDir = wxMacCFStringHolder(cfString).AsString(wxLocale::GetSystemEncoding())
			+ wxFileName::GetPathSeparator() + wxT("aMule");
		#endif
	}

#elif defined(__WINDOWS__)

	LPITEMIDLIST pidl;

	HRESULT hr = SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);

	if (SUCCEEDED(hr)) {
		if (!SHGetPathFromIDList(pidl, wxStringBuffer(strDir, MAX_PATH))) {
			strDir = wxEmptyString;
		} else {
			strDir = strDir + wxFileName::GetPathSeparator() + wxT("aMule");
		}
	}

	if (pidl) {
		LPMALLOC pMalloc;
		SHGetMalloc(&pMalloc);
		if (pMalloc) {
			pMalloc->Free(pidl);
			pMalloc->Release();
		}
	}

#else

	strDir = wxFileName::GetHomeDir() + wxFileName::GetPathSeparator() + wxT(".aMule");

#endif

	return strDir;
}
// File_checked_for_headers
