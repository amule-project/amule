// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

#ifndef AMULEDLG_H
#define AMULEDLG_H

#ifdef __WXMSW__
	#include <wx/msw/winundef.h> // Needed to be able to include wx headers
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h.
#include <wx/statusbr.h>	// Needed for wxStatusBar
#include <wx/frame.h>		// Needed for wxFrame
#include <wx/socket.h>		// Needed for wxSocketEvent
#include <wx/timer.h>		// Needed for wxTimerEvent
#include <wx/textctrl.h>	// Needed for wxTextCtrl
#include <wx/imaglist.h>


#include "types.h"			// Needed for uint32
#include "resource.h"		// Needed for IDD_EMULE_DIALOG

class CTransferWnd;
class CServerWnd;
class CSharedFilesWnd;
class CSearchDlg;
class CChatWnd;
class CStatisticsDlg;
class PrefsUnifiedDlg;

#ifndef __SYSTRAY_DISABLED__
class CSysTray;
#endif

#define MP_RESTORE		4001
#define MP_CONNECT		4002
#define MP_DISCONNECT	4003
#define MP_EXIT			4004

#define DEFAULT_SIZE_X  800
#define DEFAULT_SIZE_Y  600

// CamuleDlg Dialogfeld
class CamuleDlg : public wxFrame 
{
public:
	enum { IDD = IDD_EMULE_DIALOG };
	
	CamuleDlg(wxWindow* pParent=NULL, wxString title=wxT(""),wxPoint where = wxDefaultPosition, wxSize dlg_size = wxSize(DEFAULT_SIZE_X,DEFAULT_SIZE_Y));
	~CamuleDlg();

	void AddLogLine(bool addtostatusbar, const wxChar* line, ...);
	void AddDebugLogLine(bool addtostatusbar, const wxChar* line, ...);
	void AddServerMessageLine(char* line, ...);
	void ResetLog(uint8 whichone = 1);
	void ResetDebugLog();
	
	void ShowConnectionState(bool connected, wxString server = wxT(""), bool iconOnly = false);
	void ShowUserCount(uint32 toshow, uint32 filetoshow);
	void ShowTransferRate();
	
	bool StatisticsWindowActive()	{return (activewnd == (wxWindow*)statisticswnd);}
	
	/* Returns the ID of the active dialog. Needed to check what to redraw. */
	int GetActiveDialog()	{return m_nActiveDialog;}
	void SetActiveDialog(wxWindow* dlg);
	
	// Madcat - Toggles Fast ED2K Links Handler on/off.
	void ToggleFastED2KLinksHandler();
	void StartFast(wxTextCtrl *ctl);

	void OnClose(wxCloseEvent& evt);
	void OnBnConnect(wxCommandEvent& evt);

	void ShowNotifier(wxString Text, int MsgType, bool ForceSoundOFF = false); // Should be removed or implemented!
	void Hide_aMule(bool iconize = true);
	void Show_aMule(bool uniconize = true);
	// has to be done in own method
	void changeDesktopMode();
	
	bool SafeState() { return is_safe_state; }

#ifndef __SYSTRAY_DISABLED__
	void CreateSystray(const wxString& title);
#endif

	CTransferWnd*		transferwnd;
	CServerWnd*			serverwnd;
	PrefsUnifiedDlg*	prefsunifiedwnd;
	CSharedFilesWnd*	sharedfileswnd;
	CSearchDlg*			searchwnd;
	CChatWnd*			chatwnd;
	wxWindow*			activewnd;
	CStatisticsDlg*		statisticswnd;

	int					split_pos;
	int					srv_split_pos;
	
	wxImageList imagelist;

protected:
	
	void OnToolBarButton(wxCommandEvent& ev);
	void OnPrefButton(wxCommandEvent& ev);
	void OnBnClickedPrefOk(wxCommandEvent &event);

	void OnMinimize(wxIconizeEvent& evt);

	void OnBnClickedFast(wxCommandEvent& evt);

	void OnGUITimer(wxTimerEvent& evt);

private:

	wxToolBar*	m_wndToolbar;
	bool		LoadGUIPrefs(bool override_pos, bool override_size); 
	bool		SaveGUIPrefs();

	wxTimer* gui_timer;

// Systray functions
#ifndef __SYSTRAY_DISABLED__
	void RemoveSystray();
	void UpdateTrayIcon(int procent);
	CSysTray*			m_wndTaskbarNotifier;
#endif

	int			m_nActiveDialog;

	bool is_safe_state;

	bool is_hidden;

	DECLARE_EVENT_TABLE()
};

#endif
