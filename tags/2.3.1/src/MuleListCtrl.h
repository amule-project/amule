//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef MULELISTCTRL_H
#define MULELISTCTRL_H

#ifdef _WIN32
#include <wx/msw/winundef.h>
#endif

#include <wx/defs.h> // Do_not_auto_remove (Mac, Win32, and just good practice)
#include "extern/wxWidgets/listctrl.h"

#include <vector>
#include <list>

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
	 * through the SetSortFunc function, otherwise it will just return the 
	 * position after the last item.
	 */
	long GetInsertPos( wxUIntPtr data );

	/**
	 * Sorts the list.
	 *
	 * Before you can use this function, you will need to specify a sorter
	 * function using SetSortFunc. wxListCtrl needs such a function to
	 * perform the sort.
	 */
	virtual void SortList();

	//! The type of the list of item specific data
	typedef std::vector<wxUIntPtr> ItemDataList;

	/**
	 * Returns a list the user-data of all selected items.
	 *
	 * @return A list of data associated with the selected items.
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
	void SetSortFunc(MuleListCtrlCompare func)	{ m_sort_func = func; }

	/**
	 * Deselects all selected items, but does not change focus.
	 */
	void ClearSelection();

	/**
	 * Insert a new column.
	 *
	 * @param[in] col	Column will be inserted at this position.
	 * @param[in] heading	Heading text for the column.
	 * @param[in] format	Format (alignment) of the column.
	 * @param[in] width	The column width.
	 * @param[in] name	Internal name of the column.
	 *
	 * We override this method to allow a name to be specified for each
	 * column. The name is used for saving/loading column settings
	 * independently of their index. It is necessary to set a unique name
	 * for each column, to let column settings be saved/loaded. If you
	 * don't set a name for a column, settings for that column won't be
	 * saved.
	 *
	 * Requirements about column names:
	 * - they must not contain the ':' (colon) and ',' (comma) characters,
	 * - they should be at least one character long.
	 *
	 * @note Changing the internal name of one column results in width of
	 * that column being reset to default, and if sorting was done by that
	 * column, sorting being forgotten. If you change the name of a column,
	 * don't forget to update the list GetOldColumnOrder() returns, if
	 * necessary.
	 *
	 * For more information refer to the wxWidgets documentation of
	 * wxListCtrl::InsertColumn()
	 */
	long InsertColumn(
			  long col,
			  const wxString& heading,
			  int format = wxLIST_FORMAT_LEFT,
			  int width = -1,
			  const wxString& name = wxEmptyString
			  );

	/**
	 * Clears all items and all columns.
	 *
	 * Intercepted to clear column name list.
	 *
	 * For more information see the wxWidgets documentation of
	 * wxListCtrl::ClearAll()
	 */
	void ClearAll()
	{
		m_column_names.clear();
		MuleExtern::wxGenericListCtrl::ClearAll();
	}

	/**
	 * Delete one column from the list.
	 *
	 * Not implemented yet, intercepted here just as a sanity check.
	 */
	bool DeleteColumn(int WXUNUSED(col))	{ wxFAIL; return false; }

	/**
	 * Indicates if we're in the process of sorting.
	 */
	bool IsSorting() const { return m_isSorting; }
	
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
	void SetTableName(const wxString& name)		{ m_name = name; }

	/**
	 * Return old column order.
	 *
	 * @return The pre-2.2.2 column order.
	 *
	 * This function should be overridden in descendant classes to return a
	 * comma-separated list of the old column order, when column data was
	 * saved/loaded by index. The default implementation returns an empty
	 * string, meaning that old settings (if any) should be discarded.
	 *
	 * @note When you add or remove columns from the list control, DO NOT
	 * change this list. This list may only be updated if you changed a
	 * column name that is already in this list, to reflect the name
	 * change. List order also must be preserved.
	 */
	virtual wxString GetOldColumnOrder() const;

	/**
	 * Returns the column which is currently used to sort the list.
	 */
	unsigned GetSortColumn() const	{ return m_sort_orders.front().first; }

	/**
	 * Returns the current sorting order, a combination of the DES and ALT flags.
	 */
	unsigned GetSortOrder() const	{ return m_sort_orders.front().second; }

	/**
	 * Set the sort column
	 *
	 * @param column The column with which the list should be sorted.
	 * @param order The order in which to sort the column.
	 *
	 * Note that attempting to sort a column in an unsupported order
	 * is an illegal operation.
	 */
	virtual void SetSorting(unsigned column, unsigned order);

	/**
	 * Returns true if the item is sorted compared to its neighbours.
	 */
	bool IsItemSorted(long item);

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
	 * Event handler for item selection/deletion, needed by TTS.
	 */
	void OnItemSelected(wxListEvent& evt);
	void OnItemDeleted(wxListEvent& evt);
	void OnAllItemsDeleted(wxListEvent& evt);

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
	wxString		m_name;

	//! The sorter function needed by wxListCtrl.
	MuleListCtrlCompare	m_sort_func;

	//! Contains the current search string.
	wxString		m_tts_text;

	//! Timestamp for the last TTS event.
	unsigned		m_tts_time;

	//! The index of the last item selected via TTS.
	int			m_tts_item;

	/**
	 * Wrapper around the user-provided sorter function.
	 * 
	 * This function ensures that items are sorted in the order
	 * specified by clicking on column-headers, and also enforces
	 * that different entries are never considered equal. This is
	 * required for lists that make use of child-items, since
	 * otherwise, parents may not end up properly located in
	 * relation to child-items.
	 */	
	static int wxCALLBACK SortProc(wxUIntPtr item1, wxUIntPtr item2, long sortData);

	/** Compares two items in the list, using the current sort sequence. */
	int CompareItems(wxUIntPtr item1, wxUIntPtr item2);

	//! This pair contains a column number and its sorting order.
	typedef std::pair<unsigned, unsigned> CColPair;
	typedef std::list<CColPair> CSortingList;

	class MuleSortData {
	public:
		MuleSortData(CSortingList sort_orders, MuleListCtrlCompare sort_func) : m_sort_orders(sort_orders), m_sort_func(sort_func) { };
		
		CSortingList m_sort_orders;
		MuleListCtrlCompare m_sort_func;
	};	
	
	//! This list contains in order the columns sequence to sort by.
	CSortingList m_sort_orders;

	/**
	 * Get the column name by index.
	 *
	 * @param[in] column Index of the column whose name we're looking for.
	 *
	 * @return The column name or an empty string if index is invalid
	 * (out of range), or the column name hasn't been set.
	 */
	const wxString& GetColumnName(int column) const;

	/**
	 * Get the column default width by index.
	 *
	 * @param[in] column Index of the column whose name we're looking for.
	 *
	 * @return The column default width or wx default width if index is invalid
	 * (out of range), or the column name hasn't been set.
	 */
	int GetColumnDefaultWidth(int column) const;

	/**
	 * Get column index by name.
	 *
	 * @param[in] name Internal name of the colunm whose index is needed.
	 *
	 * @return The column index, or -1 in case the name was invalid.
	 */
	int GetColumnIndex(const wxString& name) const;

	/**
	 * Find out the new index of the column by the old index.
	 *
	 * @param[in] oldindex Old column index which we want to turn into a
	 * new index.
	 *
	 * @return The new index of the column, or -1 if an error occured.
	 */
	int GetNewColumnIndex(int oldindex) const;

	/**
	 * Parses old config entries.
	 *
	 * @param[in] sortOrders	Old sort orders line.
	 * @param[in] columnWidths	Old column widths line.
	 */
	void ParseOldConfigEntries(const wxString& sortOrders, const wxString& columnWidths);

	/// This class contains a column index, its default width and its name.
	class ColNameEntry {
	public:
		int index;
		int	defaultWidth;
		wxString name;
		ColNameEntry(int _index, int _defaultWidth, const wxString& _name) 
			:	index(_index), defaultWidth(_defaultWidth), name(_name) {}
	};

	/// This list contains the columns' names.
	typedef std::list<ColNameEntry>		ColNameList;

	/// Container for column names, sorted by column index.
	ColNameList	m_column_names;

	/// This vector contains a cache of the columns' sizes.
	typedef std::vector<int>		ColSizeVector;
	
	/// Container for column sizes cache.
	ColSizeVector	m_column_sizes;

	// True while sorting.
	bool m_isSorting;
	
	DECLARE_EVENT_TABLE()
};

#endif // MULELISTCTRL_H
// File_checked_for_headers
