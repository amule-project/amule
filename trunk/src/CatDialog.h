//
// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2002 quekky
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef CATDIALOG_H
#define CATDIALOG_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "CatDialog.h"
#endif

#include <wx/dialog.h>		// Needed for wxDialog
#include "types.h"		// Needed for DWORD


class Category_Struct;
class wxStaticBitmap;
class wxBitmap;
class wxColour;


/**
 * This dialog takes of displaying either existing or new categories, so that
 * the user can add or change them.
 *
 * It is a self-contained entity, and does not rely on the categories staying
 * the same while the dialog is visble, though it will overwrite any changes 
 * made to the selected category in the mean time. Also, if the selected category 
 * has been deleted then it will simply be readded.
 *
 * It does however rely on the Transferwnd keeping its own list of categories up-
 * to-date.
 */
class CCatDialog : public wxDialog
{
public:
	/**
	 * Constructor.
	 *
	 * @param parent The parent of the new dialog.
	 * @param catindex The category to be edited.
	 *
	 * The parameter catindex can be a valid index, in which case that category
	 * will be selected, or it can be less than zero, in which case a new 
	 * category will be created.
	 */
	CCatDialog(wxWindow* parent, int catindex = -1 );
	
	/**
	 * Destructor.
	 */
	~CCatDialog();
	
private:
	/**
	 * Helper function for making the color-preview.
	 *
	 * @param colour The color with which to fill the bitmap.
	 *
	 * This function creates a single-color 16x16 image, using the
	 * specified colour.
	 */
	wxBitmap MakeBitmap( wxColour colour );


	//! Variable used to store the user-selected color.
	DWORD				m_color;

	//! Pointer to category to be edited or NULL if we are adding a new category.
	Category_Struct*	m_category;


	/**
	 * Event-handler for selecting incomming dir.
	 */
	void OnBnClickedBrowse(wxCommandEvent& evt);

	/**
	 * Event-handler for saving the changes.
	 */
	void OnBnClickedOk(wxCommandEvent& evt);

	/**
	 * Event-handler for selecting category color.
	 */
	void OnBnClickColor(wxCommandEvent& evt);
	
	DECLARE_EVENT_TABLE()
};

#endif

