// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C)2002 Tiku & Patrizio Bassi aka Hetfield <hetfield@email.it>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef SYSTRAY_H
#define SYSTRAY_H

//START - enkeyDEV(kei-kun) -TaskbarNotifier-
#define TBN_NULL				0
#define TBN_CHAT				1
#define TBN_DLOAD				2
#define TBN_LOG					3
#define TBN_IMPORTANTEVENT		        4
#define TBN_NEWVERSION				5
//END - enkeyDEV(kei-kun) -TaskbarNotifier-

#ifndef __SYSTRAY_DISABLED__

#include <cstddef>		// Needed for NULL, must be included BEFORE gtk/gdk headers!
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/wxchar.h>		// Needed for wxChar
#include <wx/string.h>		// Needed for wxString

#include "types.h"		// Needed for DWORD
#include "color.h"		// Needed for COLORREF, GetRValue, GetGValue and GetBValue

class wxWindow;

#ifndef __WXMSW__ // defined in windef.h 
typedef struct _tagSIZE {
  int cx,cy;
} SIZE;
#endif

#if defined(__WXMSW__)
#include <wx/taskbar.h>
class CSysTray : public wxTaskBarIcon {
public:
	CSysTray(wxWindow *parent, int desktopMode = 0, const wxString &title = wxEmptyString);
	void TraySetToolTip(char* data);
	void TraySetIcon(char** data,bool what=false,int* pVals=NULL);
	bool SetColorLevels(int* pLimits,COLORREF* pColors, int nEntries);
private:
	wxIcon c;
};

#elif defined(__WXGTK__)
class CSysTray {
 public:
  CSysTray(wxWindow* parent,int desktopMode,const wxString& title);
  ~CSysTray();

  void Show(const wxChar* caption,int nMsgType,DWORD dwTimeToShow=500,DWORD dwTimeToStay=4000,DWORD dwTimeTOHide=200);

  void TraySetToolTip(char* data);
  void TraySetIcon(char** data,bool what=false,int* pVals=NULL);
  bool SetColorLevels(int* pLimits,COLORREF* pColors, int nEntries);
 
 private:
  void setupProperties();

  void drawMeters(GdkPixmap* pix,GdkBitmap* mask,int* pBarData);
  void DrawIconMeter(GdkPixmap* pix,GdkBitmap* mask,int pBarData,int x);
  COLORREF GetMeterColor(int level);

  int desktopMode;
  wxWindow* parent;
  GtkWidget* status_docklet;
  GtkWidget* status_image;
  GtkTooltips* status_tooltips;
  SIZE m_sDimensions;

  int m_nSpacingWidth;
  int m_nNumBars;
  int m_nMaxVal;
  int *m_pLimits;
  COLORREF *m_pColors;
  int m_nEntries;


};
#else
	// No native tray
	#warning Native systray support missing for your platform.
#endif

#endif // __SYSTRAY_DISABLED__

#endif // SYSTRAY_H
