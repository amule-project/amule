//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Tiku & Patrizio Bassi aka Hetfield <hetfield@amule.org>
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef SYSTRAY_H
#define SYSTRAY_H

#include "StringFunctions.h"

#ifndef USE_WX_TRAY

enum TaskbarNotifier
{
	TBN_NULL = 0,
	TBN_CHAT,
	TBN_DLOAD,
	TBN_LOG,
	TBN_IMPORTANTEVENT,
	TBN_NEWVERSION
};

enum DesktopMode
{
	dmUnknown = 0,
	dmGNOME2,
	dmKDE3,
	dmKDE2,
	dmDisabled
};


#ifndef __SYSTRAY_DISABLED__
#ifndef AMULE_DAEMON

#include <cstddef>		// Needed for NULL, must be included BEFORE gtk/gdk headers!
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/string.h>		// Needed for wxString

#include "Types.h"		// Needed for DWORD


#if defined(__UTF8_SYSTRAY_ENABLED__) || wxUSE_UNICODE 
	// On unicode builds on gtk, UTF8 must be used!
	inline const wxCharBuffer char2gtk(const char *x) { return char2UTF8(x); }
	inline const wxCharBuffer unicode2gtk(wxString x) { return unicode2UTF8(x); }
#else
	inline const wxCharBuffer char2gtk(const char *x) { return x; }
	inline const wxCharBuffer unicode2gtk(wxString x) { return unicode2char(x); }
#endif

class wxWindow;

#ifdef __WXMSW__


#include <wx/taskbar.h>

class CSysTray : public wxTaskBarIcon
{
public:
	CSysTray(wxWindow* parent, DesktopMode desktopmode, const wxString& title);
	
	void SetTrayToolTip(const wxString& tip);
	void SetTrayIcon(const wxIcon& Icon);
	
private:
	wxIcon c;
};


#elif defined(__WXGTK__)


const int SysTrayWidth = 16;
const int SysTrayHeight = 16;
const int SysTraySpacing = 1;
const int SysTrayBarCount = 1;
const int SysTrayMaxValue = 100;


class CSysTray
{
public:
	CSysTray(wxWindow* parent, DesktopMode desktopMode, const wxString& title);
	~CSysTray();

	void SetTrayToolTip(const wxString& tip);
	void SetTrayIcon(char** data, int* pVals);

private:
	void setupProperties();

	void drawMeters(GdkPixmap* pix,GdkBitmap* mask,int* pBarData);
	void DrawIconMeter(GdkPixmap* pix,GdkBitmap* mask,int pBarData,int x);

	DesktopMode		m_DesktopMode;
	
	wxWindow*		m_parent;
	GtkWidget*		m_status_docklet;
	GtkWidget*		m_status_image;
	GtkTooltips*		m_status_tooltips;
};


#else

// No native tray
#warning No systray support for your platform!

#endif

#endif // !AMULE_DAEMON
#endif // __SYSTRAY_DISABLED__
#endif // !USE_WX_TRAY
#endif // SYSTRAY_H
