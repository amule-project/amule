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

#ifndef __SYSTRAY_DISABLED__
	#include "MuleTrayIcon.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h.
#include <wx/frame.h>		// Needed for wxFrame
#include <wx/timer.h>
#include <wx/imaglist.h>

#include "types.h"			// Needed for uint32

class CTransferWnd;
class CServerWnd;
class CSharedFilesWnd;
class CSearchDlg;
class CChatWnd;
class CStatisticsDlg;
class CKadDlg;
class PrefsUnifiedDlg;	

class wxTimerEvent;
class wxTextCtrl;

#ifndef __SYSTRAY_DISABLED__
class CSysTray;
#endif

#define MP_RESTORE	4001
#define MP_CONNECT	4002
#define MP_DISCONNECT	4003
#define MP_EXIT		4004

#define DEFAULT_SIZE_X  800
#define DEFAULT_SIZE_Y  600

// CamuleDlg Dialogfeld
class CamuleDlg : public wxFrame 
{
public:
	CamuleDlg(wxWindow* pParent = NULL, const wxString &title = wxEmptyString,
		wxPoint where = wxDefaultPosition,
		wxSize dlg_size = wxSize(DEFAULT_SIZE_X,DEFAULT_SIZE_Y));
	~CamuleDlg();

	void AddLogLine(bool addtostatusbar, const wxString& line);
	void AddDebugLogLine(bool addtostatusbar, const wxString& line);
	void AddServerMessageLine(wxString& message);
	void ResetLog(uint8 whichone = 1);
	
	void ShowConnectionState(bool connected, const wxString &server = wxEmptyString);
	void ShowUserCount(uint32 toshow, uint32 filetoshow);
	void ShowTransferRate();
	
	bool StatisticsWindowActive()	{return (activewnd == (wxWindow*)statisticswnd);}
	
	/* Returns the active dialog. Needed to check what to redraw. */
	enum DialogType { TransferWnd, ServerWnd, SearchWnd, SharedWnd, ChatWnd, StatsWnd, KadWnd };
	DialogType GetActiveDialog()	{return m_nActiveDialog;}
	void SetActiveDialog(DialogType type, wxWindow* dlg);

	/**
	 * Helper function for deciding if a certian dlg is visible.
	 *
	 * @return True if the dialog is visible to the user, false otherwise.
	 */
	bool IsDialogVisible( DialogType dlg )
	{
		return ( m_nActiveDialog == dlg ) && ( is_safe_state ) /* && ( !IsIconized() ) */; 
	}

	void ShowED2KLinksHandler( bool show );

	void OnClose(wxCloseEvent& evt);
	void OnBnConnect(wxCommandEvent& evt);

	void ShowNotifier(wxString Text, int MsgType, bool ForceSoundOFF = false); // Should be removed or implemented!
	void Hide_aMule(bool iconize = true);
	void Show_aMule(bool uniconize = true);
	// has to be done in own method
	void changeDesktopMode();
	
	bool SafeState() { return is_safe_state; }

	void LaunchUrl(const wxString &url);
	
	//! These are the currently known web-search providers
	enum WebSearch { wsFileHash, wsJugle };
	// websearch function
	wxString GenWebSearchUrl( const wxString &filename, WebSearch provider );


#ifndef __SYSTRAY_DISABLED__
	void CreateSystray(const wxString& title);
#endif

	CTransferWnd*		transferwnd;
	CServerWnd*		serverwnd;
	CSharedFilesWnd*	sharedfileswnd;
	CSearchDlg*		searchwnd;
	CChatWnd*		chatwnd;
	wxWindow*		activewnd;
	CStatisticsDlg*		statisticswnd;
	CKadDlg*		kadwnd;

	int			srv_split_pos;
	
	wxImageList imagelist;
	
	void StartGuiTimer() { gui_timer->Start(100); }
	
	PrefsUnifiedDlg* prefs_dialog;

	/**
	 * This function ensures that _all_ list widgets are properly sorted.
	 */
	void InitSort();

protected:
	
	void OnToolBarButton(wxCommandEvent& ev);
	void OnAboutButton(wxCommandEvent& ev);
	void OnPrefButton(wxCommandEvent& ev);

	void OnMinimize(wxIconizeEvent& evt);

	void OnBnClickedFast(wxCommandEvent& evt);
	void OnBnStatusText(wxCommandEvent& evt);

	void OnGUITimer(wxTimerEvent& evt);

	void OnMainGUISizeChange(wxSizeEvent& evt);

private:

	wxToolBar*	m_wndToolbar;
	bool		LoadGUIPrefs(bool override_pos, bool override_size); 
	bool		SaveGUIPrefs();

	wxTimer* gui_timer;

// Systray functions
#ifndef __SYSTRAY_DISABLED__
	void RemoveSystray();
	void UpdateTrayIcon(int percent);
	#if USE_WX_TRAY
		CMuleTrayIcon* m_wndTaskbarNotifier;
	#else
		CSysTray *m_wndTaskbarNotifier;
	#endif
#endif

	DialogType m_nActiveDialog;

	bool is_safe_state;

	//bool is_hidden;

	uint32 last_iconizing;

	void Apply_Clients_Skin(wxString file);
	
	void Create_Toolbar(wxString skinfile);

	DECLARE_EVENT_TABLE()
};

#endif
