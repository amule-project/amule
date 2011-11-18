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

#ifndef SHAREDFILESWND_H
#define SHAREDFILESWND_H

#include <wx/panel.h>		// Needed for wxPanel


class CKnownFile;
class CSharedFilesCtrl;
class CSharedFilePeersListCtrl;
class wxListEvent;
class wxGauge;
class wxSplitterEvent;
class wxRadioBox;

/**
 * This class represents the window containing the list of shared files.
 */
class CSharedFilesWnd : public wxPanel
{
public:
	/** 
	 * Constructor.
	 */
	CSharedFilesWnd(wxWindow* pParent = NULL);
	
	/**
	 * Destructor.
	 */
	~CSharedFilesWnd();
	

	/**
	 * This function updates the statistics of the selected items.
	 *
	 * Call this function when an item has been selected, or when a
	 * selected item changes. It 
	 */
	void SelectionUpdated();
	

	/**
	 * Deletes all files and updates widget
	 */
	void RemoveAllSharedFiles();

	/**
	 * Call this function before displaying the dialog.
	 *
	 * This functions does a few tasks to ensure that the dialog is looking the right way.
	 */
	void	Prepare();

	//! Pointer to the widget containing the list of shared files.
	CSharedFilesCtrl* sharedfilesctrl;
	
	//! Pointer to the list of clients.
	CSharedFilePeersListCtrl*	peerslistctrl;
	
	//! Contains the current (or last if the clientlist is hidden) position of the splitter.
	int m_splitter;
private:
	/**
	 * Event-handler for reloading the list of shared files.
	 */	
	void OnBtnReloadShared(wxCommandEvent &evt);

	/**
	 * Event-handler for showing details about a shared file(s).
	 */
	void OnItemSelectionChanged(wxListEvent& evt);

	/**
	 * Event-handler for the list-toggle button.
	 */
	void OnToggleClientList( wxCommandEvent& event );

	/**
	 * Event-handler for changes in the sash divider position.
	 */
	void OnSashPositionChanging(wxSplitterEvent& evt);

	/**
	 * Event-handler for changes in the clients mode radio box.
	 */
	void OnSelectClientsMode( wxCommandEvent& WXUNUSED(evt) );

	//! Pointer to the gauge used for showing requests ratio.
	wxGauge* m_bar_requests;
	//! Pointer to the gauge used for showing accepted-requests ratio.
	wxGauge* m_bar_accepted;
	//! Pointer to the gauge used for showing the transferred ratio.
	wxGauge* m_bar_transfer;
	//! Pointer to the radio box selecting the client mode.
	wxRadioBox* m_radioClientMode;
	//! Flag if window has been prepared
	bool	m_prepared;
	//! Minimum position of splitter bar
	static const int s_splitterMin = 90;

	/**
	 * Mode which clients are shown
	 */
	enum EClientShow {
		ClientShowAll = 0,
		ClientShowSelected,
		ClientShowUploading
	};
	EClientShow m_clientShow;

	
	DECLARE_EVENT_TABLE()
};

#endif
// File_checked_for_headers
