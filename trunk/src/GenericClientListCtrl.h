//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef GENERICCLIENTLISTCTRL_H
#define GENERICCLIENTLISTCTRL_H

#include <map>				// Needed for std::multimap
#include <vector>			// Needed for std::vector
#include <wx/brush.h>

#include "Types.h"			// Needed for uint8
#include "Constants.h"		// Needed for DownloadItemType
#include "MuleListCtrl.h"	// Needed for CMuleListCtrl
#include "amuleDlg.h"		// Needed for CamuleDlg::DialogType

class CPartFile;
class CClientRef;
class wxBitmap;
class wxRect;
class wxDC;

struct ClientCtrlItem_Struct;

enum GenericColumnEnum {
	ColumnUserName = 0,
	ColumnUserDownloaded,
	ColumnUserUploaded,
	ColumnUserSpeedDown,
	ColumnUserSpeedUp,
	ColumnUserProgress,
	ColumnUserAvailable,
	ColumnUserVersion,
	ColumnUserQueueRankLocal,
	ColumnUserQueueRankRemote,
	ColumnUserOrigin,
	ColumnUserFileNameDownload,
	ColumnUserFileNameUpload,
	ColumnUserFileNameDownloadRemote,
	ColumnInvalid
};

struct CGenericClientListCtrlColumn {
	GenericColumnEnum cid;
	wxString title;
	int width;
};

struct GenericColumnInfo {
	GenericColumnInfo(int n, CGenericClientListCtrlColumn* col) : n_columns(n), columns(col) { };
	int n_columns;
	CGenericClientListCtrlColumn* columns;
};

typedef std::vector<CKnownFile*> CKnownFileVector;

/**
 * This class is responsible for representing clients in a generic way.
 */

class CGenericClientListCtrl : public CMuleListCtrl
{
public:
	/**
	 * Constructor.
	 * 
	 * @see CMuleListCtrl::CMuleListCtrl for documentation of parameters.
	 */
	 CGenericClientListCtrl(
				const wxString& tablename,
	            wxWindow *parent,
                wxWindowID winid,
                const wxPoint &pos,
                const wxSize &size,
                long style,
                const wxValidator& validator,
                const wxString &name);
				
	/**
	 * Destructor.
	 */	 
	virtual	~CGenericClientListCtrl();	

	/**
	 * Initializes the control. We need a 2-stage initialization so the derived class members can be called.
	 */
	 void InitColumnData();

	/**
	 * Adds a source belonging to the specified file.
	 *
	 * @param owner The owner of this specific source-entry, must be a valid pointer.
	 * @param source The client object to be added, must be a valid pointer.
	 * @param type If the source is a current source, or a A4AF source.
	 *
	 * Please note that the specified client will only be added to the list if it's
	 * owner is shown, otherwise the source will simply be ignored.
	 * Duplicates wont be added.
	 */
	void AddSource( CKnownFile* owner, const CClientRef& source, SourceItemType type );

	/**
	 * Removes a source from the list.
	 *
	 * @param source ID of the source to be removed.
	 * @param owner Either a specific file, or NULL to remove the source from all files.
	 */
	void RemoveSource( uint32 source, const CKnownFile* owner );
	
	/**
	 * Shows the clients of specific files.
	 *
	 * @param file A valid, sorted vector of files whose clients will be shown.
	 *
	 * WARNING: The received vector *MUST* be odered with std::sort.
	 *
	 */
	void ShowSources( const CKnownFileVector& files );

	/**
	 * Updates the state of the specified item, possibly causing a redrawing.
	 *
	 * @param toupdate ID of the client to be updated.
	 * @param type If the source is a current source, or a A4AF source.	 
	 *
	 */
	void UpdateItem(uint32 toupdate, SourceItemType type);

	void SetShowing( bool status ) { m_showing = status; }
	bool GetShowing() const { return m_showing; }

protected:
	// The columns with their attributes; MUST be defined by the derived class.
	GenericColumnInfo m_columndata;
	static int wxCALLBACK SortProc(wxUIntPtr item1, wxUIntPtr item2, long sortData);

private:
	/**
     *
	 * Must be overriden by the derived class and return the dialog where this list is.
     * @see CamuleDlg::DialogType
	 *
     */
	virtual CamuleDlg::DialogType GetParentDialog() = 0;

	/**
	 * Updates the displayed number representing the amount of clients currently shown.
	 */
	void ShowSourcesCount( int diff );

	/**
	 * Overloaded function needed for custom drawing of items.
	 */
	virtual void OnDrawItem( int item, wxDC* dc, const wxRect& rect, const wxRect& rectHL, bool highlighted );

	/**
	 * Draws a client item.
	 */
	void	DrawClientItem( wxDC* dc, int nColumn, const wxRect& rect, ClientCtrlItem_Struct* item, int iTextOffset, int iBitmapOffset, int iBitmapXSize ) const;

	/**
	 * Draws the download status (chunk) bar for a client.
	 */
	void	DrawSourceStatusBar( const CClientRef& source, wxDC* dc, const wxRect& rect, bool  bFlat) const;

	/**
	  * Draaws the file parts bar for a client.
	  */
	void	DrawStatusBar( const CClientRef& client, wxDC* dc, const wxRect& rect1 ) const;

	/**
	 * @see CMuleListCtrl::GetTTSText
	 * Just a dummy
	 */
	virtual wxString GetTTSText(unsigned) const { return wxEmptyString; }

	/**
	 * Set "show sources" or "show peers" flag in Known File
	 */
	virtual void SetShowSources(CKnownFile *, bool) const = 0;

	/**
	 * Translate the CID to a unique string for saving column sizes
	 * @see CMuleListCtrl::InsertColumn
	 */
	wxString TranslateCIDToName(GenericColumnEnum cid);

	static int Compare( const CClientRef& client1, const CClientRef& client2, long lParamColumnSort);
	
	// Event-handlers for clients.
	void	OnSwapSource( wxCommandEvent& event );
	void	OnViewFiles( wxCommandEvent& event );
	void	OnAddFriend( wxCommandEvent& event );
	void	OnSetFriendslot( wxCommandEvent& event );
	void	OnSendMessage( wxCommandEvent& event );
	void	OnViewClientInfo( wxCommandEvent& event );

	// Misc event-handlers
	void	OnItemActivated( wxListEvent& event );
	void 	OnMouseRightClick( wxListEvent& event );
	void 	OnMouseMiddleClick( wxListEvent& event );
	void	OnKeyPressed( wxKeyEvent& event );

	//! The type of list used to store items on the listctrl. We use the unique ECID as key.
	typedef std::multimap<uint32, ClientCtrlItem_Struct*> ListItems;
	//! Shortcut to the pair-type used on the list.
	typedef ListItems::value_type ListItemsPair;
	//! This pair is used when searching for equal-ranges.
	typedef std::pair< ListItems::iterator, ListItems::iterator > ListIteratorPair;

	//! This list contains everything shown on the list. Sources are only to
	//! be found on this list if they are being displayed
	ListItems	m_ListItems;
	
	//! Pointer to the current menu object, used to avoid multiple menus.
	wxMenu*		m_menu;
	//! Cached brush object.
	wxBrush	m_hilightBrush;
	//! Cached brush object.
	wxBrush	m_hilightUnfocusBrush;
	
	//! The number of displayed sources
	int m_clientcount;

	//! The files being shown, if any.
	CKnownFileVector m_knownfiles;
	
	DECLARE_EVENT_TABLE()

	bool m_showing;
	
	void RawAddSource(CKnownFile* owner, CClientRef source, SourceItemType type);
	void RawRemoveSource( ListItems::iterator& it );

	virtual bool IsShowingDownloadSources() const = 0;
};

#endif
// File_checked_for_headers
