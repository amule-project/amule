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

#ifndef MULELISTCTRL_H
#define MULELISTCTRL_H


#include "listctrl.h"

#include <vector>





/**
 * Enhanced wxListCtrl provided custom-drawing among other things.
 *
 * This class provides these features which the original wxListCtrl lacks:
 *  - Automatic sort arrows upon clicks on the column headers
 *  - Custom drawing of items.
 *  - Hiding of columns through auto-generated popup-menu.
 *  - Helper function for inserting items pre-sorted.
 *  - Loading and saving of column properties.
 *  - Selection of items by typing an initial part of the text (TTS).
 */
class CMuleListCtrl : public MuleExtern::wxGenericListCtrl
{
public:
	/**
	 * The various ways in which a column can be sorted.
	 *
	 * If SORT_DES is not set, sorting is taken to be 
	 * ascending. If SORT_ALT is not set, sorting is 
	 * taken to be normal.
	 */
	enum MLOrder
	{
		//! If set, sorting is to be in descending order.
		SORT_DES	= 0x1000,
		
		//! If sorting should use alternate method.
		//! Is specified in with or without DEC.
		SORT_ALT	= 0x2000
	};

	//! Mask which covers the column part of the sort-data.
	static const unsigned COLUMN_MASK = 0xfff;

	//! Mask which covers the sorting part of the sort-data.
	static const unsigned SORTING_MASK = 0x3000;

	/**
	 * Constructor.
	 * 
	 * @see wxGenericListCtrl::wxGenericListCtrl for documentation of parameters.
	 */
	 CMuleListCtrl(
	            wxWindow *parent,
                wxWindowID winid = -1,
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize,
                long style = wxLC_ICON,
                const wxValidator& validator = wxDefaultValidator,
                const wxString &name = wxT("mulelistctrl") );
	
	/**
	 * Destructor.
	 *
	 * If a name for the table has been specified with SetTableName, then 
	 * column settings will be saved automatically.
	 */ 
	virtual ~CMuleListCtrl();


	/**
	 * Saves column settings.
	 *
	 * Currently saves the width of all columns, hidden columns, the column
	 * to sort by and in which direction to sort.
	 */
	virtual void SaveSettings();

	/**
	 * Loads column settings.
	 *
	 * Currently loads the width of all columns, hidden columns, the column
	 * to sort by and in which direction to sort. This function also ensures
	 * that the items are sorted after the settings have been read.
	 */
	virtual void LoadSettings();


	/**
	 * This function tries to locate the best place to insert an item.
	 *
	 * @param The userdata of the new item.
	 * 
	 * This function does a binary type search to locate the best place to
	 * insert the new item with the specified userdata. It then returns the 
	 * item after this position. To do this, the sorter-function must be set
	 * though the SetSortFunc function, otherwise it will just return the 
	 * position after the last item.
	 */
	long GetInsertPos( long data );


	/**
	 * Sorts the list.
	 *
	 * Before you can use this function, you will need to specify a sorter
	 * function using SetSortFunc. wxListCtrl needs such a function to
	 * perform the sort.
	 */
	virtual void SortList();


	//! The type of the list of item specific data
	typedef std::vector<long> ItemDataList;

	/**
	 * Returns a list the user-data of all selected items.
	 *
	 * @return A list of data assosiated with the selected items.
	 *
	 * This function will return the user-data for each selected item in a
	 * vector, which can then be manipulated with regards to changes made
	 * in the current order of the listctrl items.
	 */
	ItemDataList GetSelectedItems() const;	

	
	/**
	 * Sets the sorter function.
	 *
	 * @param func
	 *
	 * See the documentation on wxListCtrl::SortItems for more information
	 * about the expected function type.
	 */
	void SetSortFunc(wxListCtrlCompare func);


	/**
	 * Deselects all selected items, but does not change focus.
	 */
	void ClearSelection();
	
protected:

	/**
	 * Must be overwritten to enable alternate sorting.
	 *
	 * @param The column being sorted.
	 *
	 * Subclasses of CMuleListCtrl can allow alternative sorting
	 * of columns. This is done by overriding this function and
	 * returning true for the columns where alternative sorting
	 * is desired. 
	 */
	virtual bool AltSortAllowed(unsigned column) const;

	/**
	 * Returns the string used when selecting rows via Type-To-Select.
	 *
	 * @param item The index of the item being examined.
	 *
	 * By default, this function simply returns the text in the first
	 * column for the given item. However, when owner-drawing is
	 * enabled, this function _must_ be overriden.
	 */
	virtual wxString GetTTSText(unsigned item) const;
	

	/**
	 * Sets the internally used table-name.
	 *
	 * @param name The new name or an empty string to disable.
	 *
	 * You need to call this function with a unique name before you can 
	 * make use of the LoadSettings/SaveSettings functions. CMuleListCtrl
	 * uses the name specified in this command to create unique keynames.
	 */
	void SetTableName(const wxString& name);

	/**
	 * Returns the column which is currently used to sort the list.
	 */
	unsigned GetSortColumn() const;

	/**
	 * Returns the current sorting order, a combination of the DES and ALT flags.
	 */
	unsigned GetSortOrder() const;

	/**
	 * Set the sort column
	 *
	 * @param column The column with which the list should be sorted.
	 * @param order The order in which to sort the column.
	 *
	 * Note that attempting to sort a column in an unsupported order
	 * is an illegal operation.
	 */
	void SetSorting(unsigned column, unsigned order);

	
	/**
	 * Check and fix selection state.
	 * 
	 * @param event The event which triggered the selection.
	 * @return The index of the item selected or -1 if none.
	 *
	 * This function checks if the clicked item is selected.
	 * If not, then the item is selected and all other items
	 * are deselected.
	 */
	//@{
	long CheckSelection(wxListEvent& event);
	long CheckSelection(wxMouseEvent& event);
	//@}
	

	/**
	 * Event handler for right-clicks on the column headers.
	 */
	void OnColumnRClick(wxListEvent& evt);
	/**
	 * Event handler for left-clicks on the column headers.
	 */
	void OnColumnLClick(wxListEvent& evt);
	/**
	 * Event handler for the hide/show menu items.
	 */
	void OnMenuSelected(wxCommandEvent& evt);
	/**
	 * Event handler for the mouse wheel.
	 */
	void OnMouseWheel(wxMouseEvent &event);
	/**
	 * Event handler for key-presses, needed by TTS.
	 */
	void OnChar(wxKeyEvent& evt);
	/**
	 * Event handler for item selection, needed by TTS.
	 */
	void OnItemSelected(wxListEvent& evt);

	
private:
	/**
	 * Resets the current TTS session.
	 */
	void ResetTTS();
	
	/**
	 * Sets the image of a specific column.
	 *
	 * @param col The column to change.
	 * @param order The sorting order to represent. Zero unsets the image.
	 */
	void SetColumnImage(unsigned col, int image);

	
	//! The name of the table. Used to load/save settings.
	wxString			m_name;
	//! The sort order. Ascending is the default.
	unsigned			m_sort_order;
	//! The column to sort by.
	unsigned			m_sort_column;
	//! The sorter function needed by wxListCtrl.
	wxListCtrlCompare	m_sort_func;

	//! Contains the current search string.
	wxString			m_tts_text;
	//! Timestamp for the last TTS event.
	unsigned			m_tts_time;
	//! The index of the last item selected via TTS.
	int					m_tts_item;

	
	DECLARE_EVENT_TABLE()
};


#endif // MULELISTCTRL_H
