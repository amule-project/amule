//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Original author: Emilio Sandoz
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

#ifndef __PrefsUnifiedDlg_H__
#define __PrefsUnifiedDlg_H__

#include <wx/dialog.h>		// Needed for wxDialog


class Cfg_Base;
class CDirectoryTreeCtrl;

class wxWindow;
class wxChoice;
class wxButton;
class wxPanel;
class wxListCtrl;

class wxCommandEvent;
class wxListEvent;
class wxSpinEvent;
class wxScrollEvent;
class wxInitDialogEvent;


/**
 * This class represents a dialog used to display preferences.
 */
class PrefsUnifiedDlg : public wxDialog
{
public:
	/**
	 * Constructor.
	 *
	 * @param parent The parent window.
	 *
	 * This constructor is a much more simple version of the wxDialog one,
	 * which only needs to know the parent of the dialog. Please note that
	 * it is private so that we can ensure that only one dialog has been
	 * created at one time.
	 */
	PrefsUnifiedDlg(wxWindow* parent);
#ifdef ENABLE_IP2COUNTRY
	~PrefsUnifiedDlg();

	// Public module hook: CamuleDlg::IP2CountryDownloadFinished calls
	// this so an open Preferences dialog refreshes its status block as
	// soon as the new database is loaded. No-op if no dialog is open.
	static void RefreshIP2CountryStatusIfOpen();

	// Public module hook: CIP2Country calls this on a *manual* update
	// failure (the "Update now" button) so the user sees a modal popup,
	// not just a buried log line. No-op if the prefs dialog isn't
	// open — caller is expected to have already logged the same
	// message via AddLogLineC so the failure is still recorded.
	static void NotifyIP2CountryUpdateFailedIfOpen(const wxString& msg);
#endif

	/**
	 * Updates the widgets with the values of the preference-variables.
	 */
	bool TransferFromWindow();
	/**
	 * Updates the prefernce-variables with the values of the widgets.
	 */
	bool TransferToWindow();


protected:
	/**
	 * Helper functions which checks if a Cfg has has changed.
	 */
	bool			CfgChanged(int id);

	/**
	 * Helper functions which returns the Cfg associated with the specified id.
	 */
	Cfg_Base*		GetCfg(int id);


	//! Pointer to the shared-files list
	CDirectoryTreeCtrl*	m_ShareSelector;

	//! Pointer to the color-selector
	wxChoice*		m_choiceColor;

	//! Pointer to the color-selection button
	wxButton*		m_buttonColor;

	//! Pointer to the currently shown preference-page
	wxPanel*		m_CurrentPanel;

	//! hide/show server tab
	int				m_IndexServerTab;
	bool			m_ServerTabVisible;
	wxPanel*		m_ServerWidget;
	wxListCtrl*		m_PrefsIcons;
	void EnableServerTab(bool enable);

	void OnOk(wxCommandEvent &event);
	void OnCancel(wxCommandEvent &event);
	void OnClose(wxCloseEvent &event);

	void OnButtonBrowseApplication(wxCommandEvent &event);
	void OnButtonDir(wxCommandEvent& event);
	void OnButtonEditAddr(wxCommandEvent& event);
	void OnButtonColorChange(wxCommandEvent &event);
	void OnButtonIPFilterReload(wxCommandEvent &event);
	void OnButtonIPFilterUpdate(wxCommandEvent &event);
#ifdef ENABLE_IP2COUNTRY
	void OnGeoIPSourceChange(wxCommandEvent &event);
	void OnGeoIPUpdateNow(wxCommandEvent &event);
	void OnGeoIPMasterToggle(wxCommandEvent &event);
	// Show/hide the three source-specific sub-panels based on the
	// dropdown index; called from OnGeoIPSourceChange and from
	// TransferToWindow on dialog open.
	void UpdateGeoIPSourcePanel();
	// Re-render the status line from the current CIP2Country state +
	// selected source. Called whenever the dropdown changes, after a
	// successful Update Now, and from RefreshIP2CountryStatusIfOpen.
	void UpdateGeoIPStatus();
	// Grey out the source selector + everything downstream when the
	// master "Show country flags for clients" checkbox is unchecked.
	// Called from OnGeoIPMasterToggle and TransferToWindow.
	void UpdateGeoIPControlsEnabled();

private:
	// Set in the ctor / cleared in dtor so the IP2Country download
	// callback can find an open dialog without a global pointer chain.
	static PrefsUnifiedDlg *s_activeInstance;

	// Snapshots taken at TransferToWindow so OnOk can detect "the user
	// switched GeoIP source / pasted a new license / changed the URL
	// during this dialog session" and kick off a download — otherwise
	// the user has to remember to click Update now after each change.
	// The Cfg system only tracks credential fields bound through
	// NewCfgItem; the source dropdown is committed live, so we have
	// to compare it manually.
	int		m_GeoIPSourceAtOpen;
	wxString	m_GeoIPMaxMindLicenseAtOpen;
	wxString	m_GeoIPCustomUrlAtOpen;
public:
#endif
	void OnColorCategorySelected(wxCommandEvent &event);
	void OnCheckBoxChange(wxCommandEvent &event);
	void OnAutostartToggle(wxCommandEvent &event);
	void OnPrefsPageChange(wxListEvent& event);
	void OnToolTipDelayChange(wxSpinEvent& event);
	void OnScrollBarChange( wxScrollEvent& event );
	void OnRateLimitChanged( wxSpinEvent& event );
	void OnTCPClientPortChange(wxSpinEvent& event);
	void OnUserEventSelected(wxListEvent& event);
	void OnLanguageChoice(wxCommandEvent &event);
	void CreateEventPanels(const int idx, const wxString& vars, wxWindow* parent);

	void OnInitDialog( wxInitDialogEvent& evt );

	// Tri-state outcome of an attempt to commit the pending share
	// selection. Used by OnOk to decide between three flows:
	//   * Committed       → continue to Save() + Reload + Show(false)
	//   * NothingToCommit → continue to Save() + Show(false), skip Reload
	//   * CancelledByUser → return early from OnOk: keep the prefs
	//                       dialog open so the user can adjust their
	//                       selection without losing the rest of
	//                       their pending pref changes
	enum class SharedDirsCommitResult {
		NothingToCommit,
		Committed,
		CancelledByUser,
	};

	// Commits the pending share selection from the directory tree
	// into theApp->glob_prefs->shareddir_list. Confirms before
	// committing recursive-share roots that look like sensitive
	// system locations (e.g. home, /etc), then runs the recursive
	// directory enumeration on a worker thread with a cancellable
	// progress dialog so the UI never freezes on large roots.
	SharedDirsCommitResult CommitSharedDirsWithProgress();

	wxDECLARE_EVENT_TABLE();

private:
	bool	m_verticalToolbar;
	bool	m_toolbarOrientationChanged;
};

#endif
// File_checked_for_headers
