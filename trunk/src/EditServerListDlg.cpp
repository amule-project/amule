//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Drager
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

#include <wx/dialog.h>		// Needed for wxDialog	// Do_not_auto_remove
#include <wx/sizer.h>		// Needed for wxBoxSizer
#include <wx/file.h>		// Needed for wxFile
#include <wx/log.h>		// Needed for wxLogSysError
#include <wx/textctrl.h>

#include "EditServerListDlg.h"	// Interface declarations

BEGIN_EVENT_TABLE(EditServerListDlg, wxDialog)
    EVT_BUTTON(wxID_OK, EditServerListDlg::OnOK)
END_EVENT_TABLE()

IMPLEMENT_CLASS(EditServerListDlg, wxDialog)

EditServerListDlg::EditServerListDlg(wxWindow *parent,
                                     const wxString& caption,
                                     const wxString& message,
				     const wxString& filename) : wxDialog(parent, -1, caption, 
					     			      wxDefaultPosition, wxSize(400,200),
								      wxDEFAULT_DIALOG_STYLE | wxDIALOG_MODAL |wxRESIZE_BORDER)
{
  m_file = filename;

  wxBeginBusyCursor();

  wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );

  topsizer->Add( CreateTextSizer( message ), 0, wxALL, 10 );

  m_textctrl = new wxTextCtrl(this, -1, wxEmptyString,
			      wxDefaultPosition, 
			      wxDefaultSize,
			      wxTE_MULTILINE);
  topsizer->Add( m_textctrl, 1, wxEXPAND | wxLEFT|wxRIGHT, 15 );

  topsizer->Add( CreateButtonSizer( wxOK | wxCANCEL ), 0, wxCENTRE | wxALL, 10 );

  SetAutoLayout( TRUE );
  SetSizer( topsizer );

  Centre( wxBOTH );

  if (wxFile::Exists(filename))
  	m_textctrl->LoadFile(filename);
  
  m_textctrl->SetFocus();

  wxEndBusyCursor();
}

EditServerListDlg::~EditServerListDlg()
{
}

void EditServerListDlg::OnOK(wxCommandEvent& WXUNUSED(event) )
{
	if (m_textctrl->SaveFile(m_file))
		EndModal(1);
	else
		wxLogSysError(wxT("Can't write to file '") +  m_file + wxT("'"));
}
// File_checked_for_headers
