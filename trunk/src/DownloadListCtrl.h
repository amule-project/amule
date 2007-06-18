//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef DOWNLOADLISTCTRL_H
#define DOWNLOADLISTCTRL_H

#include <map>				// Needed for std::multimap

#include "Types.h"			// Needed for uint8
#include "Constants.h"		// Needed for DownloadItemType
#include "MuleListCtrl.h"	// Needed for CMuleListCtrl


class CPartFile;
class CUpDownClient;
class wxBitmap;
class wxRect;
class wxDC;

struct CtrlItem_Struct;



/**
 * This class is responsible for representing the download queue.
 *
 * The CDownlodListCtrl class is responsible for drawing not only files being
 * downloaded, but also sources assosiated with these. It is in many ways
 * primary widget within the application, since it is here that users can
 * inspect and manipulate their current downloads.
 *
 * Due to the fact that sources are one of the major sources of update-class,
 * this class has been designed such that it does not need to recieve these 
 * unless sources in question are actually being displayed. Upon the initial
 * showing of sources (when a file is expanded), a current list of that file's
 * sources are requested. If the sources are hidden again, the class will
 * discard all knowledge of their existance.
 */
class CDownloadListCtrl : public CMuleListCtrl
{
public:
	/**
	 * Constructor.
	 * 
	 * @see CMuleListCtrl::CMuleListCtrl for documentation of parameters.
	 */
	 CDownloadListCtrl(
	            wxWindow *parent,
                wxWindowID winid = -1,
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize,
                long style = wxLC_ICON,
                const wxValidator& validator = wxDefaultValidator,
                const wxString &name = wxT("downloadlistctrl") );
				
	/**
	 * Destructor.
	 */	 
	virtual	~CDownloadListCtrl();	


	/**
	 * Adds a file to the list, but it wont show unless it matches the current category.
	 *
	 * @param A valid pointer to a new partfile.
	 *
	 * Please note that duplicates wont be added.
	 */
	void AddFile( CPartFile* file );

	/**
	 * Adds a source belonging to the specified partfile.
	 *
	 * @param owner The owner of this specific source-entry, must be a valid pointer.
	 * @param source The client object to be added, must be a valid pointer.
	 * @param available If the source is a current source, or a A4AF source.
	 *
	 * Please note that the specified client will only be added to the list if it's
	 * owner is shown and has ShowSources set to true, otherwise the source will 
	 * simply be ignored. Duplicates wont be added.
	 */
	void AddSource( CPartFile* owner, CUpDownClient* source, DownloadItemType type );

	
	/**
	 * Removes a source from the list.
	 *
	 * @param source A pointer to the source to be removed.
	 * @param owner Either a specific file, or NULL to remove the source from all files.
	 */
	void RemoveSource( const CUpDownClient* source, const CPartFile* owner );
	
	/**
	 * Removes the specified file from the list.
	 *
	 * @param file A valid pointer of the file to be removed.
	 *
	 * This function also removes any sources assosiated with the file. 
	 */
	void RemoveFile( CPartFile* file );

	
	/**
	 * Toggles showing of a file's sources on or off.
	 *
	 * @param file The file whoose sources should be shown/hidden.
	 * @param show Whenever or not to show sources.
	 */
	void ShowSources( CPartFile* file, bool show );
	
	/**
	 * Shows or hides the sources of a specific file.
	 *
	 * @param file A valid pointer to the file to be shown/hidden.
	 * @param show Whenever or not to show the file.
	 *
	 * If the file is hidden, then its sources will also be hidden.
	 */
	void ShowFile( CPartFile* file, bool show );


	/**
	 * Updates the state of the specified item, possibly causing a redrawing.
	 *
	 * @param toupdate The source or file to be updated.
	 *
	 * Calling this function with a file as the argument will ensure that the 
	 * file is hidden/shown depending on its state and the currently selected
	 * category.
	 */
	void UpdateItem(const void* toupdate);


	/**
	 * Returns the current category.
	 */
	uint8 GetCategory() const;
	
	/**
	 * Changes the displayed category and updates the list of shown files.
	 * 
	 * @param newCategory The new category to display.
	 */
	void ChangeCategory( int newCategory );

	
	/**
	 * Clears all completed files from the list.
	 */
	void ClearCompleted();

private:
	/**
	 * Updates the displayed number representing the ammount of files currently shown.
	 */
	void ShowFilesCount( int diff );

	
	/**
	 * @see CMuleListCtrl::GetTTSText
	 */
	virtual wxString GetTTSText(unsigned item) const;	
	
	
	/**
	 * Overloaded function needed for custom drawing of items.
	 */
	virtual void OnDrawItem( int item, wxDC* dc, const wxRect& rect, const wxRect& rectHL, bool highlighted );

	/**
	 * Draws a file item.
	 */
	void	DrawFileItem( wxDC* dc, int nColumn, const wxRect& rect, CtrlItem_Struct* item ) const;

	/**
	 * Draws a source item.
	 */
	void	DrawSourceItem( wxDC* dc, int nColumn, const wxRect& rect, CtrlItem_Struct* item ) const;

	/**
	 * Draws the status (chunk) bar for a file.
	 */
	void	DrawFileStatusBar( const CPartFile* file, wxDC* dc, const wxRect& rect, bool bFlat ) const;

	/**
	 * Draws the status (chunk) bar for a source.
	 */
	void	DrawSourceStatusBar( const CUpDownClient* source, wxDC* dc, const wxRect& rect, bool  bFlat) const;


	static int wxCALLBACK SortProc(long item1, long item2, long sortData);
	static int Compare( const CPartFile* file1, const CPartFile* file2, long lParamSort );
	static int Compare( const CUpDownClient* client1, const CUpDownClient* client2, long lParamSort);
	

	// Event-handlers for files
	void	OnCleanUpSources( wxCommandEvent& event ); 
	void	OnCancelFile( wxCommandEvent& event );
	void	OnSetPriority( wxCommandEvent& event );
	void	OnSwapSources( wxCommandEvent& event );
	void	OnSetCategory( wxCommandEvent& event );	
	void	OnSetStatus( wxCommandEvent& event );
	void	OnClearCompleted( wxCommandEvent& event );
	void	OnGetLink( wxCommandEvent& event );
	void	OnGetFeedback( wxCommandEvent& event );
	void	OnGetRazorStats( wxCommandEvent& event );
	void	OnViewFileInfo( wxCommandEvent& event );
	void	OnViewFileComments( wxCommandEvent& event );
	void	OnPreviewFile( wxCommandEvent& event );
	
	// Event-handlers for sources
	void	OnSwapSource( wxCommandEvent& event );
	void	OnViewFiles( wxCommandEvent& event );
	void	OnAddFriend( wxCommandEvent& event );
	void	OnSendMessage( wxCommandEvent& event );
	void	OnViewClientInfo( wxCommandEvent& event );

	// Misc event-handlers
	void	OnItemActivated( wxListEvent& event );
	void 	OnMouseRightClick( wxListEvent& event );
	void 	OnMouseMiddleClick( wxListEvent& event );
	void	OnKeyPressed( wxKeyEvent& event );


	/**
	 * Returns true if the given file should be shown in the specified category.
	 *
	 * @param file The file to be examined.
	 * @param newel The new category selection.
	 * @return True if the file should be shown, false otherwise.
	 */
	bool ShowItemInCurrentCat( const CPartFile* file, int newsel ) const;

	/**
	 * Executes the user-selected preview command on the specified file.
	 *
	 * @file The file to be previewed.
	 */
	void PreviewFile(CPartFile* file);


	//! The type of list used to store items on the listctrl.
	typedef std::multimap<const void*,CtrlItem_Struct*> ListItems;
	//! Shortcut to the pair-type used on the list.
	typedef ListItems::value_type ListItemsPair;
	//! This pair is used when searching for equal-ranges.
	typedef std::pair< ListItems::iterator, ListItems::iterator > ListIteratorPair;

	//! This list contains everything shown on the list. Sources are only to
	//! be found on this list if they are being displayed, whereas files can
	//! always be found on this list, even if they are currently hidden.
	ListItems	m_ListItems;

	
	//! Pointer to the current menu object, used to avoid multiple menus.
	wxMenu*		m_menu;
	//! Pointer to a cached brush object.
	wxBrush*	m_hilightBrush;
	//! Pointer to a cached brush object.
	wxBrush*	m_hilightUnfocusBrush;
	
	
	//! The currently displayed category
	uint8 m_category;

	//! True if there are any completed files being displayed.
	bool  m_completedFiles;
	
	//! The number of displayed files
	int m_filecount;

	DECLARE_EVENT_TABLE()
};

#endif
// File_checked_for_headers
