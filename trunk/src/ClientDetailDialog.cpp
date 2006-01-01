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

#include "ClientDetailDialog.h"	// Interface declarations
#include "OtherFunctions.h"	// Needed for CastItoIShort
#include "NetworkFunctions.h" // Needed for Uint32toStringIP
#include "PartFile.h"		// Needed for CPartFile
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "ServerList.h"		// Needed for CServerList
#include "amule.h"			// Needed for theApp
#include "Server.h"		// Needed for CServer
#include "updownclient.h"	// Needed for CUpDownClient
#include "muuli_wdr.h"		// Needed for ID_CLOSEWND
#include <common/Format.h>		// Needed for CFormat
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <sys/types.h>

// CClientDetailDialog dialog

BEGIN_EVENT_TABLE(CClientDetailDialog,wxDialog)
	EVT_BUTTON(ID_CLOSEWND,CClientDetailDialog::OnBnClose)
END_EVENT_TABLE()


CClientDetailDialog::CClientDetailDialog(wxWindow* parent,CUpDownClient* client)
: wxDialog(parent,9997,_("Client Details"),wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE)
{
	m_client = client;
	wxSizer* content=clientDetails(this,TRUE);
	OnInitDialog();
	content->SetSizeHints(this);
	content->Show(this,TRUE);
}

CClientDetailDialog::~CClientDetailDialog()
{
}

void CClientDetailDialog::OnBnClose(wxCommandEvent& WXUNUSED(evt))
{
	EndModal(0);
}

bool CClientDetailDialog::OnInitDialog() {
	
	if (!m_client->GetUserName().IsEmpty()) {
		CastChild(ID_DNAME,wxStaticText)->SetLabel(m_client->GetUserName());

		wxASSERT(!m_client->GetUserHash().IsEmpty()); // if we have client name we have userhash
		CastChild(ID_DHASH,wxStaticText)->SetLabel(m_client->GetUserHash().Encode());

		CastChild(ID_DRATING,wxStaticText)->SetLabel(wxString::Format(wxT("%.1f"),(float)m_client->GetScore(false,m_client->IsDownloading(),true)));
	} else {
		CastChild(ID_DNAME,wxStaticText)->SetLabel(_("Unknown"));
		CastChild(ID_DHASH,wxStaticText)->SetLabel(_("Unknown"));
		CastChild(ID_DRATING,wxStaticText)->SetLabel(_("Unknown"));;
	}	


	wxString OSInfo = m_client->GetClientOSInfo();
	if (!OSInfo.IsEmpty())
	    CastChild(ID_DSOFT,wxStaticText)->SetLabel(m_client->GetSoftStr()+wxT(" (")+OSInfo+wxT(")"));
	else
	    CastChild(ID_DSOFT,wxStaticText)->SetLabel(m_client->GetSoftStr());
	CastChild(ID_DVERSION,wxStaticText)->SetLabel(m_client->GetSoftVerStr());

	CastChild(ID_DID,wxStaticText)->SetLabel(wxString::Format(wxT("%u (%s)"),ENDIAN_NTOHL(m_client->GetIP()),(m_client->HasLowID() ? _("LowID"):_("HighID"))));
	
	CastChild(ID_DIP,wxStaticText)->SetLabel(m_client->GetFullIP() + wxString::Format(wxT(":%i"),m_client->GetUserPort()));

	if (m_client->GetServerIP()) {
		
		wxString srvaddr = Uint32toStringIP(m_client->GetServerIP());
		CastChild(ID_DSIP,wxStaticText)->SetLabel(srvaddr);
		
		CServer* cserver = theApp.serverlist->GetServerByAddress(srvaddr, m_client->GetServerPort()); 
		if (cserver) {
			CastChild(ID_DSNAME,wxStaticText)->SetLabel(cserver->GetListName());
		} else {
			CastChild(ID_DSNAME,wxStaticText)->SetLabel(_("Unknown"));
		}
	} else {
		CastChild(ID_DSIP,wxStaticText)->SetLabel(_("Unknown"));
		CastChild(ID_DSNAME,wxStaticText)->SetLabel(_("Unknown"));
	}

	const CKnownFile* file = m_client->GetUploadFile();
	
	if ( file ) {
		wxString filename = MakeStringEscaped( TruncateFilename( file->GetFileName(), 60 ) );
	
		CastChild(ID_DDOWNLOADING,wxStaticText)->SetLabel( filename );
	} else {
		CastChild(ID_DDOWNLOADING,wxStaticText)->SetLabel(wxT("-"));
	}

	CastChild(ID_DDUP,wxStaticText)->SetLabel(CastItoXBytes(m_client->GetTransferedDown()));
	CastChild(ID_DDOWN,wxStaticText)->SetLabel(CastItoXBytes(m_client->GetTransferredUp()));
	CastChild(ID_DAVUR,wxStaticText)->SetLabel(wxString::Format(_("%.1f kB/s"),m_client->GetKBpsDown()));
	CastChild(ID_DAVDR,wxStaticText)->SetLabel(wxString::Format(_("%.1f kB/s"),m_client->GetUploadDatarate() / 1024.0f));

	CastChild(ID_DUPTOTAL,wxStaticText)->SetLabel(CastItoXBytes(m_client->GetDownloadedTotal()));		
	CastChild(ID_DDOWNTOTAL,wxStaticText)->SetLabel(CastItoXBytes(m_client->GetUploadedTotal()));
	CastChild(ID_DRATIO,wxStaticText)->SetLabel(wxString::Format(wxT("%.1f"),(float)m_client->GetScoreRatio()));
		
	if (theApp.CryptoAvailable()){
			
		if (m_client->SUINotSupported()) {
			CastChild(IDC_CDIDENT,wxStaticText)->SetLabel(_("Not Supported"));
		} else if (m_client->SUIFailed()) {
			CastChild(IDC_CDIDENT,wxStaticText)->SetLabel(_("Failed"));
		} else if (m_client->SUINeeded()) {
			CastChild(IDC_CDIDENT,wxStaticText)->SetLabel(_("Not complete"));
		} else if (m_client->IsBadGuy()) {
			CastChild(IDC_CDIDENT,wxStaticText)->SetLabel(_("Bad Guy"));
		} else if (m_client->IsIdentified()) {
			CastChild(IDC_CDIDENT,wxStaticText)->SetLabel(_("Verified - OK"));
		}
			
	} else {
		CastChild(IDC_CDIDENT,wxStaticText)->SetLabel(_("Not Available"));
	}

	if (m_client->GetUploadState() != US_NONE) {
		#ifdef CLIENT_GUI
		#warning EC needed for queue position
		CastChild(ID_DSCORE,wxStaticText)->SetLabel(wxString::Format(wxT("%u (QR: %u)"),m_client->GetScore(m_client->IsDownloading(),false), 0));
		#else
		CastChild(ID_DSCORE,wxStaticText)->SetLabel(wxString::Format(wxT("%u (QR: %u)"),m_client->GetScore(m_client->IsDownloading(),false),theApp.uploadqueue->GetWaitingPosition(m_client)));		
		#endif
	} else {
		CastChild(ID_DSCORE,wxStaticText)->SetLabel(wxT("-"));
	}
	Layout();
	
	
	return true;
}
