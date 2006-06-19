//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef SEARCHLISTCTRL_H
#define SEARCHLISTCTRL_H


#include "wx/colour.h"		// Needed for wxColour
#include <wx/regex.h>		// Needed for wxRegExp

#include "MuleListCtrl.h"	// Needed for CMuleListCtrl


class CSearchList;
class CSearchFile;


/**
 * This class is used to display search results.
 * 
 * Results on added to the list will be colored according to 
 * the number of sources and other parameters (see UpdateColor).
 *
 * To display results, first use the ShowResults function, which will display
 * all current results with the specified id and afterwards you can use the 
 * AddResult function to add new results or the UpdateResult function to update
 * already present results. Please note that it is not possible to add results
 * with the AddResult function before calling ShowResults.
 */
class CSearchListCtrl : public CMuleListCtrl
{
public:
	/**
	 * Constructor.
	 * 
	 * @see CMuleListCtrl::CMuleListCtrl for documentation of parameters.
	 */
	 CSearchListCtrl(
	            wxWindow *parent,
                wxWindowID winid = -1,
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize,
                long style = wxLC_ICON,
                const wxValidator& validator = wxDefaultValidator,
                const wxString &name = wxT("mulelistctrl") );
			
	/**
	 * Destructor.
	 */
	virtual ~CSearchListCtrl();


	/**
	 * Adds ths specified file to the list.
	 *
	 * @param The new result to be shown.
	 *
	 * Please note that no duplicates checking is done, so the pointer should 
	 * point to a new file in order to avoid problems. Also note that the result
	 * will be inserted sorted according to current sort-type, so there is no
	 * need to resort the list after adding new items.
	 */
	void	AddResult(CSearchFile* toshow);

	/**
	 * Removes the specified file from the list.
	 */
	void	RemoveResult(CSearchFile* toshow);

	
	/**
	 * Updates the specified source.
	 *
	 * @param The search result to be updated.
	 */
	void	UpdateResult(CSearchFile* toupdate);

	/**
	 * Clears the list and inserts all results with the specified Id instead.
	 *
	 * @param nResult The ID of the results or Zero to simply reset the list.
	 */
	void	ShowResults( long ResultsId );


	/**
	 * Updates the colors of item at the specified index.
	 *
	 * @param index The zero-based index of the item.
	 *
	 * This function returns the color of the item based on the following:
	 *  - Downloading files are marked in red.
	 *  - Known (shared/completed) files are marked in green.
	 *  - New files are marked in blue depending on the number of sources.
	 */
	void	UpdateItemColor( long index );


	/**
	 * Returns the current Search Id. 
	 *
	 * @return The Search Id of the displayed results (set through ShowResults()).
	 */
	long	GetSearchId();
	

	/**
	 * Sets the filter which decides which results should be shown.
	 *
	 * @param regExp A regular expression targeting the filenames.
	 * @param invert If true, invert the results of the filter-test.
	 * @param filterKnown Should files that are queued or known be filtered out.
	 *
	 * An invalid regExp will result in all results being displayed.
	 */
	void	SetFilter(const wxString& regExp, bool invert, bool filterKnown);

	/**
	 * Toggels the use of filtering on and off.
	 */
	void	EnableFiltering(bool enabled);

	
	/**
	 * Returns the number of items hidden due to filtering.
	 */
	size_t	GetHiddenItemCount() const;

	
	/**
	 * Attempts to download all selected items, updating color-scheme as needed.
	 *
	 * @param category The target category, or -1 to use the drop-down selection.
	 */
	void	DownloadSelected(int category = -1);	
	
protected:
	typedef std::list<CSearchFile*> ResultList;

	//! List used to store results that are hidden due to matching the filter.
	ResultList	m_filteredOut;

	//! The current filter reg-exp.
	wxRegEx		m_filter;

	//! The text from which the filter is compiled.
	wxString	m_filterText;

	//! Controls if shared/queued results should be shown.
	bool		m_filterKnown;
	
	//! Controls if the result of filter-hits should be inverted
	bool		m_invert;

	//! Specifies if filtering should be used
	bool		m_filterEnabled;


	/**
	 * Returns true if the filename is filtered.
	 */
	bool	IsFiltered(const CSearchFile* file);
	
	
	/**
	 * Sorter function used by wxListCtrl::SortItems function.
	 *
	 * @see CMuleListCtrl::SetSortFunc
	 * @see wxListCtrl::SortItems
	 */
	static int wxCALLBACK SortProc(long item1, long item2, long sortData);

	/** @see CMuleListCtrl::AltSortAllowed */
	virtual bool AltSortAllowed(unsigned column) const;

	/** @see CMuleListCtrl::GetTTSText */
	virtual wxString GetTTSText(unsigned item) const;
	

	/**
	 * Helper function which syncs two lists.
	 *
	 * @param src The source list.
	 * @param dst The list to be synced with the source list.
	 *
	 * This function syncronises the following settings of two lists:
	 *  - Sort column
	 *  - Sort direction
	 *  - Column widths
	 *
	 * If either sort column or direction is changed, then the dst list will
	 * be resorted. This function is used to ensure that all results list act
	 * as one, while still allowing individual selection.
	 */
	static void SyncLists( CSearchListCtrl* src, CSearchListCtrl* dst );

	/**
	 * Helper function which syncs all other lists against the specified one.
	 *
	 * @param src The list which all other lists should be synced against.
	 *
	 * This function just calls SyncLists() on all lists in s_lists, using
	 * the src argument as the src argument of the SyncLists function.
	 */
	static void SyncOtherLists( CSearchListCtrl* src );

	
	//! This list contains pointers to all current instances of CSearchListCtrl.
	static std::list<CSearchListCtrl*> s_lists;
	
	//! The ID of the search-results which the list is displaying or zero if unset. 
	long	m_nResultsID;


	//! Custom drawing, needed to display children of search-results.
	void OnDrawItem(int item, wxDC* dc, const wxRect& rect, const wxRect& rectHL, bool highlighted);


	/**
	 * Removes or adds child-entries for the given file.
	 */
	void ShowChildren(CSearchFile* file, bool show);

	/**
	 * Event handler for right mouse clicks.
	 */
	void OnRightClick( wxListEvent& event );

	/**
	 * Event handler for double-clicks or enter.
	 */
	void OnItemActivated( wxListEvent& event );
	
	/**
	 * Event handler for left-clicks on the column headers.
	 * 
	 * This eventhandler takes care of sync'ing all the other lists with this one.
	 */
	void OnColumnLClick( wxListEvent& event );

	/**
	 * Event handler for resizing of the columns.
	 *
	 * This eventhandler takes care of sync'ing all the other lists with this one.
	 */
	void OnColumnResize( wxListEvent& event );

	
	/**
	 * Event handler for get-url menu items.
	 */
	void OnPopupGetUrl( wxCommandEvent& event );

	/**
	 * Event handler for Razorback 2 stats menu items.
	 */
	void OnRazorStatsCheck( wxCommandEvent& event );

	/**
	 * Event handler for related search.
	 */
	void OnRelatedSearch( wxCommandEvent& event );

	/**
	 * Event handler for download-file(s) menu item.
	 */
	void OnPopupDownload( wxCommandEvent& event );

	DECLARE_EVENT_TABLE()
};

#endif // SEARCHLISTCTRL_H
// File_checked_for_headers
