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

#ifndef CAPTCHADIALOG_H
#define CAPTCHADIALOG_H

#include <wx/dialog.h>		// Needed for wxDialog
#include "Types.h"

/**
 * The ClientDetailDialog class is responsible for showing the info about a client.
 *
 * It shows all releavant data about the client: ip, port, hash, name, client
 * type and version, uploading/downloading data, credits, server... etc
 *
 * It's  wxDialog, modal, with return value always '0'.
 *
 */

class CCaptchaDialog : public wxDialog
{
public:
	/**
	 * Constructor. 
	 *
	 * @param parent The window that created the dialog.
	 * @param client The client whose details we're showing.
	 */
	CCaptchaDialog(wxWindow*parent, const wxImage& captchaImage, uint64 id);   
	
	/**
	 * Destructor. 
	 */
	virtual ~CCaptchaDialog();

protected:

	/**
	 * Creates all the data objects in the dialog, filling them accordingly.
	 * 
	 * Called when the dialog object is created.
	 */
	virtual bool OnInitDialog();	
	
	/**
	 * Ends the dialog, calling EndModal with return value 0
	 * 
	 * @param evt The close event, unused right now
	 */
	void OnBnClose(wxCommandEvent& evt);

	DECLARE_EVENT_TABLE()

private:

	wxSizer * captchaDlg( wxWindow *parent );

	class wxBitmap * m_captchaBitmap;
	class wxTextCtrl * m_TextCtrl;
	uint64 m_id;
};
#endif // CAPTCHADIALOG_H
// File_checked_for_headers
