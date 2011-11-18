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


#include "CaptchaDialog.h"	// Interface declarations
#include "muuli_wdr.h"		// Needed for ID_CLOSEWND
#include "GuiEvents.h"


BEGIN_EVENT_TABLE(CCaptchaDialog,wxDialog)
	EVT_BUTTON(wxID_OK, CCaptchaDialog::OnBnClose)
END_EVENT_TABLE()


CCaptchaDialog::CCaptchaDialog(
	wxWindow *parent,
	const wxImage& captchaImage,
	uint64 id)
:
wxDialog(
	parent,
	-1,
	_("Enter Captcha"),
	wxDefaultPosition,
	wxDefaultSize,
	wxDEFAULT_DIALOG_STYLE)
{
	m_captchaBitmap = new wxBitmap(captchaImage);
	m_id = id;
	wxSizer* content = captchaDlg(this);
	OnInitDialog();
	content->SetSizeHints(this);
	content->Show(this, true);
	m_TextCtrl->SetFocus();
}

CCaptchaDialog::~CCaptchaDialog()
{
	delete m_captchaBitmap;
}

void CCaptchaDialog::OnBnClose(wxCommandEvent& WXUNUSED(evt))
{
	Notify_ChatSendCaptcha(m_TextCtrl->GetLineText(0), m_id);
	Destroy();
}

wxSizer * CCaptchaDialog::captchaDlg( wxWindow *parent )
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxStaticBitmap *item1 = new wxStaticBitmap( parent, -1, *m_captchaBitmap, wxDefaultPosition, wxSize(160,60) );
    item0->Add( item1, 0, wxALIGN_CENTER|wxALL, 5 );

	m_TextCtrl = new wxTextCtrl( parent, -1, wxEmptyString, wxDefaultPosition, wxSize(80,20));
    item0->Add( m_TextCtrl, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item3 = new wxButton( parent, wxID_OK  );
    item3->SetDefault();
    item0->Add( item3, 0, wxALIGN_CENTER|wxALL, 5 );

    parent->SetSizer( item0 );
    item0->SetSizeHints( parent );
    
    return item0;
}

bool CCaptchaDialog::OnInitDialog()
{
	Layout();
	
	return true;
}
// File_checked_for_headers
