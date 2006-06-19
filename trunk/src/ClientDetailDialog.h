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

#ifndef CLIENTDETAILDIALOG_H
#define CLIENTDETAILDIALOG_H

#ifdef __WXMSW__
	#include <wx/msw/winundef.h> // Do_not_auto_remove
#endif
#include <wx/dialog.h>		// Needed for wxDialog

class CUpDownClient;

/**
 * The ClientDetailDialog class is responsible for showing the info about a client.
 *
 * It shows all releavant data about the client: ip, port, hash, name, client
 * type and version, uploading/downloading data, credits, server... etc
 *
 * It's  wxDialog, modal, with return value always '0'.
 *
 */

class CClientDetailDialog : public wxDialog
{
public:
	/**
	 * Constructor. 
	 *
	 * @param parent The window that created the dialog.
	 * @param client The client whose details we're showing.
	 */
	CClientDetailDialog(wxWindow*parent,CUpDownClient* client);   
	
	/**
	 * Destructor. 
	 *
	 * Does nothing currently.
	 */
	virtual ~CClientDetailDialog();

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
	
	//! The CUpDownClient whose data is drawn
	CUpDownClient* m_client;
};
#endif // CLIENTDETAILDIALOG_H
// File_checked_for_headers
