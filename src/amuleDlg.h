//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef AMULEDLG_H
#define AMULEDLG_H


#include <wx/archive.h>
#include <wx/filename.h>
#include <wx/frame.h>			// Needed for wxFrame
#include <wx/imaglist.h>
#include <wx/timer.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include "Types.h"			// Needed for uint32
#include "StatisticsDlg.h"

class wxTimerEvent;
class wxTextCtrl;

class CIP2Country;
class CTransferWnd;
class CServerWnd;
class CSharedFilesWnd;
class CSearchDlg;
class CChatWnd;
class CKadDlg;
class PrefsUnifiedDlg;


class CMuleTrayIcon;

struct PageType {
	wxWindow* page;
	wxString name;
};

#define MP_RESTORE	4001
#define MP_CONNECT	4002
#define MP_DISCONNECT	4003
#define MP_EXIT		4004


#define DEFAULT_SIZE_X  800
#define DEFAULT_SIZE_Y  600


enum ClientSkinEnum {
	Client_Green_Smiley = 0,
	Client_Red_Smiley,
	Client_Yellow_Smiley,
	Client_Grey_Smiley,
	Client_White_Smiley,
	Client_ExtendedProtocol_Smiley,
	Client_SecIdent_Smiley,
	Client_BadGuy_Smiley,
	Client_CreditsGrey_Smiley,
	Client_CreditsYellow_Smiley,
	Client_Upload_Smiley,
	Client_Friend_Smiley,
	Client_eMule_Smiley,
	Client_mlDonkey_Smiley,
	Client_eDonkeyHybrid_Smiley,
	Client_aMule_Smiley,
	Client_lphant_Smiley,
	Client_Shareaza_Smiley,
	Client_xMule_Smiley,
	Client_Unknown,
	Client_InvalidRating_Smiley,
	Client_PoorRating_Smiley,
	Client_FairRating_Smiley,
	Client_GoodRating_Smiley,
	Client_ExcellentRating_Smiley,
	Client_CommentOnly_Smiley,
	Client_Encryption_Smiley,
	// Add items here.
	CLIENT_SKIN_SIZE
};


// CamuleDlg Dialogfeld
class CamuleDlg : public wxFrame
{
public:
	CamuleDlg(
		wxWindow *pParent = NULL,
		const wxString &title = wxEmptyString,
		wxPoint where = wxDefaultPosition,
		wxSize dlg_size = wxSize(DEFAULT_SIZE_X,DEFAULT_SIZE_Y));
	~CamuleDlg();

	void AddLogLine(const wxString& line);
	void AddServerMessageLine(wxString& message);
	void ResetLog(int id);

	void ShowUserCount(const wxString& info = wxEmptyString);
	void ShowConnectionState(bool skinChanged = false);
	void ShowTransferRate();

	bool StatisticsWindowActive()
		{ return (m_activewnd == static_cast<wxWindow*>(m_statisticswnd)); }

	/* Returns the active dialog. Needed to check what to redraw. */
	enum DialogType {
		DT_TRANSFER_WND,
		DT_NETWORKS_WND,
		DT_SEARCH_WND,
		DT_SHARED_WND,
		DT_CHAT_WND,
		DT_STATS_WND,
		DT_KAD_WND	// this one is still unused
	};
	DialogType GetActiveDialog()
		{ return m_nActiveDialog; }
	void SetActiveDialog(DialogType type, wxWindow* dlg);

	/**
	 * Helper function for deciding if a certian dlg is visible.
	 *
	 * @return True if the dialog is visible to the user, false otherwise.
	 */
	bool IsDialogVisible( DialogType dlg )
	{
		return m_nActiveDialog == dlg && m_is_safe_state /* && !IsIconized() */;
	}

	void ShowED2KLinksHandler( bool show );

	void DlgShutDown();
	void OnClose(wxCloseEvent& evt);
	void OnBnConnect(wxCommandEvent& evt);

	void DoIconize(bool iconize);

	bool SafeState()	{ return m_is_safe_state; }

	void LaunchUrl(const wxString &url);

	//! These are the currently known web-search providers
	enum WebSearch {
		WS_FILEHASH
	};
	// websearch function
	wxString GenWebSearchUrl( const wxString &filename, WebSearch provider );

	void CreateSystray();
	void RemoveSystray();

	void StartGuiTimer()	{ gui_timer->Start(100); }
	void StopGuiTimer()	{ gui_timer->Stop(); }

	/**
	 * This function ensures that _all_ list widgets are properly sorted.
	 */
	void InitSort();

	void SetMessageBlink(bool state) { m_BlinkMessages = state; }
	void Create_Toolbar(bool orientation);

	void DoNetworkRearrange();

	CIP2Country*		m_IP2Country;
	void IP2CountryDownloadFinished(uint32 result);
	void EnableIP2Country();

	wxWindow*		m_activewnd;
	CTransferWnd*		m_transferwnd;
	CServerWnd*		m_serverwnd;
	CSharedFilesWnd*	m_sharedfileswnd;
	CSearchDlg*		m_searchwnd;
	CChatWnd*		m_chatwnd;
	CStatisticsDlg*		m_statisticswnd;
	CKadDlg*		m_kademliawnd;
	//! Pointer to the current preference dialog, if any.
	PrefsUnifiedDlg*	m_prefsDialog;

	int			m_srv_split_pos;

	wxImageList m_imagelist;
	wxImageList m_tblist;

protected:
	void OnToolBarButton(wxCommandEvent& ev);
	void OnAboutButton(wxCommandEvent& ev);
	void OnPrefButton(wxCommandEvent& ev);
	void OnImportButton(wxCommandEvent& ev);
	void OnMinimize(wxIconizeEvent& evt);
	void OnBnClickedFast(wxCommandEvent& evt);
	void OnGUITimer(wxTimerEvent& evt);
	void OnMainGUISizeChange(wxSizeEvent& evt);
	void OnExit(wxCommandEvent& evt);

private:
	//! Specifies if the prefs-dialog was shown before minimizing.
	bool m_prefsVisible;
	wxToolBar *m_wndToolbar;
	wxTimer *gui_timer;
	CMuleTrayIcon *m_wndTaskbarNotifier;
	DialogType m_nActiveDialog;
	bool m_is_safe_state;
	bool m_BlinkMessages;
	int m_CurrentBlinkBitmap;
	uint32 m_last_iconizing;
	wxFileName m_skinFileName;
	std::vector<wxString> m_clientSkinNames;
	bool m_GeoIPavailable;

	WX_DECLARE_STRING_HASH_MAP(wxZipEntry*, ZipCatalog);
	ZipCatalog cat;

	PageType m_logpages[4];
	PageType m_networkpages[2];

	bool LoadGUIPrefs(bool override_pos, bool override_size);
	bool SaveGUIPrefs();

	void UpdateTrayIcon(int percent);

	void Apply_Clients_Skin();
	void Apply_Toolbar_Skin(wxToolBar *wndToolbar);
	bool Check_and_Init_Skin();
	void Add_Skin_Icon(const wxString &iconName, const wxBitmap &stdIcon, bool useSkins);
	void ToogleED2KLinksHandler();
	void SetMessagesTool();
	void OnKeyPressed(wxKeyEvent& evt);

	DECLARE_EVENT_TABLE()
};

#endif

// File_checked_for_headers
