//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
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
#include <wx/menu.h>		// Needed for wxMenu
#include <wx/button.h>		// Needed for wxButton
#include <wx/statusbr.h>	// Needed for wxStatusBar
#include <wx/frame.h>		// Needed for wxFrame
#include <wx/imaglist.h>	// Needed for wxImageList
#include <wx/socket.h>		// Needed for wxSocketEvent
#include <wx/timer.h>		// Needed for wxTimerEvent
#include <wx/textctrl.h>	// Needed for wxTextCtrl

#include "types.h"		// Needed for uint32
#include "resource.h"		// Needed for IDD_EMULE_DIALOG
#include "PrefsUnifiedDlg.h"

class CTransferWnd;
class CServerWnd;
class CPreferencesDlg;
class CSharedFilesWnd;
class CSearchDlg;
class CChatWnd;
class CStatisticsDlg;

#ifndef __SYSTRAY_DISABLED__
class CSysTray;
#endif

#define MP_RESTORE		4001
#define MP_CONNECT		4002
#define MP_DISCONNECT		4003
#define MP_EXIT			4004

enum APPState {
  APP_STATE_RUNNING = 0,
  APP_STATE_SHUTINGDOWN,
  APP_STATE_DONE,
  APP_STATE_STARTING
};

// CamuleDlg Dialogfeld
class CamuleDlg :public wxFrame {
public:
	CamuleDlg(wxWindow* pParent=NULL, wxString title=wxT(""));
	~CamuleDlg();
	enum { IDD = IDD_EMULE_DIALOG };

	void AddLogLine(bool addtostatusbar,const wxChar* line,...);
	void AddDebugLogLine(bool addtostatusbar,const wxChar* line,...);
	void AddServerMessageLine(char* line,...);
	void ShowConnectionState(bool connected);
	void ShowConnectionState(bool connected,wxString server,bool iconOnly=false);
	void ShowNotifier(wxString Text, int MsgType, bool ForceSoundOFF = false);
	void ShowUserCount(uint32 toshow,uint32 filetoshow);
	void ShowMessageState(uint8 iconnr);
	void SetActiveDialog(wxWindow* dlg);
	void ShowTransferRate(bool forceAll=true);
	void ShowStatistics();
	bool StatisticsWindowActive()	{return (activewnd == (wxWindow*)statisticswnd);}
	void ResetLog();
	void ResetDebugLog();
	void StopTimer();
	// Barry - To find out if app is running or shutting/shut down
	bool IsRunning();
	void DoVersioncheck(bool manual);
	void Hide_aMule(bool iconize = true);
	void Show_aMule(bool uniconize = true);
	// has to be done in own method
	void CreateSystray(const wxString& title);
	void RemoveSystray();
	void changeDesktopMode();

	// Madcat - Toggles Fast ED2K Links Handler on/off.
	void ToggleFastED2KLinksHandler();

	// Refresh timing
	void Thaw_AllTransfering();
	void Freeze_AllTransfering();
	void UpdateLists(DWORD msCur);

	/* Public function to check which tab is active. Needed to check what to redraw. */
	int GetActiveDialog()	{return m_nActiveDialog;}

	void StartFast(wxTextCtrl *ctl);
	void OnClose(wxCloseEvent& evt);
	void OnBnConnect(wxEvent& evt);
	void InitDialog();

	CTransferWnd*		transferwnd;
	CServerWnd*		serverwnd;
	CPreferencesDlg*	preferenceswnd;
	PrefsUnifiedDlg*	prefsunifiedwnd;
	CSharedFilesWnd*	sharedfileswnd;
	CSearchDlg*		searchwnd;
	CChatWnd*		chatwnd;
	wxStatusBar  		statusbar;
	wxWindow		*activewnd;
	CStatisticsDlg*  statisticswnd;
#ifndef __SYSTRAY_DISABLED__
	CSysTray *m_wndTaskbarNotifier;
#endif // __SYSTRAY_DISABLED__
	volatile APPState	m_app_state;	// added volatile to get some more security when accessing this as a shared object...
	uint8			status;
	uint16                  lastbutton;

	DWORD			m_lastRefreshedQDisplay;
	bool			transfers_frozen;

	bool list_no_refresh;
	int split_pos;
	int srv_split_pos;

protected:
	void socketHandler(wxSocketEvent& event);
	void OnUQTimer(wxTimerEvent& evt);
	void OnUDPTimer(wxTimerEvent& evt);
	void OnSocketTimer(wxTimerEvent& evt);
	void OnQLTimer(wxTimerEvent& evt);

	void btnServers(wxEvent& ev);
	void btnSearch(wxEvent& ev);
	void btnTransfer(wxEvent& ev);
	void btnPreferences(wxEvent& ev);
	void OnBnNewPreferences(wxEvent& ev);
	void OnBnClickedPrefOk(wxCommandEvent &event);
	void OnBnClickedCancel(wxCommandEvent &event);
	void OnBnShared(wxEvent& ev);
	void OnBnStats(wxEvent& ev);
	void OnBnMessages(wxEvent& ev);
	void OnFinishedHashing(wxCommandEvent& evt);
	void OnDnsDone(wxCommandEvent& evt);
	void OnSourcesDnsDone(wxCommandEvent& evt);
	void OnMinimize(wxEvent& evt);
	void OnHashingShutdown(wxCommandEvent&);
	void OnBnClickedFast(wxEvent& evt);
private:
	wxString	logtext;
	bool		ready;
	wxBitmap 	transicons[4];
	uint32		lastuprate;
	uint32		lastdownrate;
	wxImageList	imagelist;
	wxMenu		trayPopup;

	wxButton	m_btnConnect;
	wxButton	m_btnDownloads;
	wxButton	m_btnServers;
	wxButton	m_btnSearch;
	wxButton	m_btnFiles;
	wxButton	m_btnPreferences;
	wxButton	m_btnMessages;
	wxButton	m_btnStatistics;
	wxButton	m_btnIrc;
	wxToolBar	*m_wndToolbar;
	wxPanel 	*p_cnt;
	void		StartConnection();
	void		CloseConnection();
	void		RestoreWindow();
	void		UpdateTrayIcon(int procent);
	void		CreateMuleToolBar();
	bool		LoadRazorPrefs();

	int		m_nActiveDialog;
	DWORD 		old_list_refresh;
	bool		old_update_queue_list;
	bool		switch_thaw_hide_mutex;
	DECLARE_EVENT_TABLE()
};

#endif // AMULEDLG_H
