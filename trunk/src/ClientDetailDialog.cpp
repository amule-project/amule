// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

// ClientDetailDialog.cpp : implementation file
//


#if defined(__WXMAC__) || defined(__WXCOCOA__)
	#include <wx/wx.h>
#endif
#ifdef __WXMSW__
	#include <winsock.h>
#else
#ifdef __BSD__
       #include <sys/types.h>
#endif /* __BSD__ */

	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
#endif
#include "ClientDetailDialog.h"	// Interface declarations
#include "otherfunctions.h"	// Needed for CastItoIShort
#include "ClientCredits.h"	// Needed for GetDownloadedTotal
#include "PartFile.h"		// Needed for CPartFile
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "ServerList.h"		// Needed for CServerList
#include "amule.h"			// Needed for theApp
#include "server.h"		// Needed for CServer
#include "updownclient.h"	// Needed for CUpDownClient
#include "muuli_wdr.h"		// Needed for ID_CLOSEWND
#include <wx/stattext.h>
#include <wx/sizer.h>

// CClientDetailDialog dialog

BEGIN_EVENT_TABLE(CClientDetailDialog,wxDialog)
	EVT_BUTTON(ID_CLOSEWND,CClientDetailDialog::OnBnClose)
END_EVENT_TABLE()

//IMPLEMENT_DYNAMIC(CClientDetailDialog, CDialog)

CClientDetailDialog::CClientDetailDialog(wxWindow* parent,CUpDownClient* client)
: wxDialog(parent,9997,_("Client Details"),wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE)
{
	m_client = client;
	wxSizer* content=clientDetails(this,TRUE);
	OnInitDialog();
	content->SetSizeHints(this);
	content->Show(this,TRUE);
	Centre();
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
		CastChild(ID_DRATING,wxStaticText)->SetLabel(wxT("Unknown"));;
	}	

	printf("ClientSoftware ->%.2x<- ClientVersion ->v%.2x<- ClientModString ->%s\n",m_client->GetClientSoft(), m_client->GetMuleVersion(), unicode2char(m_client->GetClientModString()));

	CastChild(ID_DSOFT,wxStaticText)->SetLabel(m_client->GetSoftStr());
	CastChild(ID_DVERSION,wxStaticText)->SetLabel(m_client->GetSoftVerStr());

	CastChild(ID_DID,wxStaticText)->SetLabel(wxString::Format(wxT("%u (%s)"),m_client->GetUserID(),(m_client->HasLowID() ? _("Low"):_("High"))));
	
	CastChild(ID_DIP,wxStaticText)->SetLabel(m_client->GetFullIP() + wxString::Format(wxT(":%i"),m_client->GetUserPort()));

	if (m_client->GetServerIP()) {
		in_addr server;
		server.s_addr = m_client->GetServerIP();
		wxString srvaddr = char2unicode(inet_ntoa(server));
		CastChild(ID_DSIP,wxStaticText)->SetLabel(srvaddr);
		
		CServer* cserver = theApp.serverlist->GetServerByAddress(srvaddr, m_client->GetServerPort()); 
		if (cserver) {
			CastChild(ID_DSNAME,wxStaticText)->SetLabel(cserver->GetListName());
		} else {
			CastChild(ID_DSNAME,wxStaticText)->SetLabel(wxT("Unknown"));
		}
	} else {
		CastChild(ID_DSIP,wxStaticText)->SetLabel(wxT("Unknown"));
		CastChild(ID_DSNAME,wxStaticText)->SetLabel(wxT("Unknown"));
	}

	CKnownFile* file = theApp.sharedfiles->GetFileByID(m_client->GetUploadFileID());

	if (file) {
		CastChild(ID_DDOWNLOADING,wxStaticText)->SetLabel(file->GetFileName());
	} else {
		CastChild(ID_DDOWNLOADING,wxStaticText)->SetLabel(wxT("-"));
	}

	CastChild(ID_DDUP,wxStaticText)->SetLabel(CastItoXBytes(m_client->GetTransferedDown()));
	CastChild(ID_DDOWN,wxStaticText)->SetLabel(CastItoXBytes(m_client->GetTransferedUp()));
	CastChild(ID_DAVUR,wxStaticText)->SetLabel(wxString::Format(wxT("%.1f kB/s"),m_client->GetKBpsDown()));
	CastChild(ID_DAVDR,wxStaticText)->SetLabel(wxString::Format(wxT("%.1f kB/s"),m_client->GetKBpsUp()));

	if (m_client->Credits()) {
		CastChild(ID_DUPTOTAL,wxStaticText)->SetLabel(CastItoXBytes(m_client->Credits()->GetDownloadedTotal()));		
		CastChild(ID_DDOWNTOTAL,wxStaticText)->SetLabel(CastItoXBytes(m_client->Credits()->GetUploadedTotal()));
		CastChild(ID_DRATIO,wxStaticText)->SetLabel(wxString::Format(wxT("%.1f"),(float)m_client->Credits()->GetScoreRatio(m_client->GetIP())));
		
		if (theApp.clientcredits->CryptoAvailable()){
			printf("Crypto available\n");
			switch(m_client->Credits()->GetCurrentIdentState(m_client->GetIP())){
				case IS_NOTAVAILABLE:
					printf("CurrentIdentState not available\n");
					CastChild(IDC_CDIDENT,wxStaticText)->SetLabel(wxT("Not Supported"));
					break;
				case IS_IDFAILED:
					printf("CurrentIdentState failed\n");
					CastChild(IDC_CDIDENT,wxStaticText)->SetLabel(wxT("Failed"));
					break;
				case IS_IDNEEDED:
					printf("CurrentIdentState needed\n");
					CastChild(IDC_CDIDENT,wxStaticText)->SetLabel(wxT("Not complete"));
					break;
				case IS_IDBADGUY:
					printf("CurrentIdentState badguy\n");
					CastChild(IDC_CDIDENT,wxStaticText)->SetLabel(wxT("Bad Guy"));
					break;
				case IS_IDENTIFIED:
					printf("CurrentIdentState Ident OK\n");
					CastChild(IDC_CDIDENT,wxStaticText)->SetLabel(wxT("Verified - OK"));
					break;
			}
		} else {
			printf("Crypto not available\n");
			CastChild(IDC_CDIDENT,wxStaticText)->SetLabel(wxT("Not Available"));
		}
		
		
	} else {
		CastChild(ID_DDOWNTOTAL,wxStaticText)->SetLabel(wxT("Unknown"));
		CastChild(ID_DUPTOTAL,wxStaticText)->SetLabel(wxT("Unknown"));
		CastChild(ID_DRATIO,wxStaticText)->SetLabel(wxT("Unknown"));
	}
	
	if (m_client->GetUploadState() != US_NONE) {
		CastChild(ID_DSCORE,wxStaticText)->SetLabel(wxString::Format(wxT("%u"),m_client->GetScore(m_client->IsDownloading(),false)));
	} else {
		CastChild(ID_DSCORE,wxStaticText)->SetLabel(wxT("-"));
	}
	Layout();
	return true;
}
