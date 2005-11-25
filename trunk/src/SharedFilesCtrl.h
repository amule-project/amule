//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef SHAREDFILESCTRL_H
#define SHAREDFILESCTRL_H

#include "MuleListCtrl.h"	// Needed for CMuleListCtrl


class CSharedFileList;
class CKnownFile;
class wxMenu;


/**
 * This class represents the widget used to list shared files.
 */
class CSharedFilesCtrl : public CMuleListCtrl
{
public:
	/**
	 * Constructor.
	 *
	 * @see CMuleListCtrl::CMuleListCtrl
	 */
	CSharedFilesCtrl(wxWindow* parent, int id, const wxPoint& pos, wxSize size, int flags);

	/**
	 * Destructor.
	 */
	~CSharedFilesCtrl();


#ifndef CLIENT_GUI
	/**
	 * Replaces the current contents of the list with that on the given list.
	 *
	 * @param The list with which to update the displayed contents.
	 *
	 * This function removes the current list of files and replaces it with the 
	 * files contained on the list given as an argument.
	 */
	void	ShowFileList(CSharedFileList* list);
#endif

	/**
	 * Adds the specified file to the list.
	 *
	 * @param file The new file to be shown.
	 */
	void	ShowFile(CKnownFile* file);

	/**
	 * Removes a file from the list.
	 *
	 * @param toremove The file to be removed.
	 */
	void	RemoveFile(CKnownFile* toremove);

	/**
	 * Updates a file on the list.
	 *
	 * @param toupdate The file to be updated.
	 */
	void	UpdateItem(CKnownFile* toupdate);

	/**
	 * Updates the number of shared files displayed above the list.
	 */
	void	ShowFilesCount();
	
private:
	/**
	 * Updates the item at the specified position using the specified file.
	 *
	 * @param file The file to use as reference.
	 * @param pos The item which to update.
	 *
	 * This function updates the fields of the specified item, using the data
	 * taken from the pointed to file.
	 */
	void	UpdateFile(CKnownFile* file, long pos);
	
	/**
	 * Draws the graph of file-part availability.
	 *
	 * @param file The file to make a graph over.
	 * @param dc The wcDC to draw on.
	 * @param rect The drawing area.
	 *
	 * This function draws a barspan showing the availability of the parts of 
	 * a file, for both Part-files and Known-files. Availability for Part-files
	 * is determined using the currently known sources, while availability for 
	 * Known-files is determined using the sources requesting that file.
	 */
	void	DrawAvailabilityBar( CKnownFile* file, wxDC* dc, const wxRect& rect ) const;
	
	/**
	 * Overloaded function needed to do custom drawing of the items.
	 */
	virtual void OnDrawItem(int item, wxDC* dc, const wxRect& rect, const wxRect& rectHL, bool highlighted);

	
	/**
	 * @see CMuleListCtrl::GetTTSText
	 */
	virtual wxString GetTTSText(unsigned item) const;
	
	
	/**
	 * Sorter-function.
	 *
	 * @see wxListCtrl::SortItems
	 */
	static int wxCALLBACK SortProc(long item1, long item2, long sortData);

	/**
	 * Function that specifies which columns have alternate sorting.
	 *
	 * @see CMuleListCtrl::AltSortAllowed
	 */
	virtual bool AltSortAllowed(unsigned column) const;


	/**
	 * Event-handler for right-clicks on the list-items.
	 */
	void	OnRightClick(wxListEvent& event);
	
	/**
	 * Event-handler for the Set Priority menu items.
	 */
	void	OnSetPriority( wxCommandEvent& event );
	
	/**
	 * Event-handler for the Auto-Priority menu item.
	 */
	void	OnSetPriorityAuto( wxCommandEvent& event );
	
	/**
	 * Event-handler for the Create ED2K URI items.
	 */
	void	OnCreateURI( wxCommandEvent& event );
	
	/**
	 * Event handler for get-comments menu item.
	 */
	void OnGetComment( wxCommandEvent& evt );
	 
	/**
	 * Event handler for get-razorback stats menu item
	 */	 
	void OnGetRazorStats( wxCommandEvent& evt );
	 
	/**
	 * Event-handler for the Edit Comment menu item.
	 */
	void	OnEditComment( wxCommandEvent& event );

	/**
	 * Event-handler for the Rename menu item.
	 */
	void	OnRename( wxCommandEvent& event );


	//! Pointer used to ensure that the menu isn't displayed twice.
	wxMenu* m_menu;


	DECLARE_EVENT_TABLE()
};

#endif
