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


#if defined(__WXMAC__)
	#include <wx/wx.h>
#endif
#ifdef __WXMSW__
	#include <winsock.h>
#else
#ifdef __OPENBSD__
       #include <sys/types.h>
#endif /* __OPENBSD__ */

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

// CClientDetailDialog dialog

BEGIN_EVENT_TABLE(CClientDetailDialog,wxDialog)
	EVT_BUTTON(ID_CLOSEWND,CClientDetailDialog::OnBnClose)
END_EVENT_TABLE()

//IMPLEMENT_DYNAMIC(CClientDetailDialog, CDialog)

CClientDetailDialog::CClientDetailDialog(wxWindow* parent,CUpDownClient* client)
: wxDialog(parent,9997,CString(_("Client Details")),wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE)
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

void CClientDetailDialog::OnBnClose(wxEvent& evt)
{
	EndModal(0);
}

#define GetDlgItem(a,b) wxStaticCast(FindWindowById((a)),b)

bool CClientDetailDialog::OnInitDialog() {
	char buffer[100];
	
	if (m_client->GetUserName()) {
		GetDlgItem(ID_DNAME,wxStaticText)->SetLabel(m_client->GetUserName());
	} else {
		GetDlgItem(ID_DNAME,wxStaticText)->SetLabel(CString(_("Unknown")));
	}	

	if (m_client->GetUserName()) {
		buffer[0] = 0;
		for (uint16 i = 0;i != 16;i++) {
			sprintf(buffer,"%s%02X",buffer,m_client->GetUserHash()[i]);
		}
		GetDlgItem(ID_DHASH,wxStaticText)->SetLabel(buffer);
	} else {
		GetDlgItem(ID_DHASH,wxStaticText)->SetLabel(CString(_("Unknown")));
	}
	
	wxString software=_("ClientSoftware ->")+(CastItoIShort(m_client->GetClientSoft()))+_("<- ClientVersion ->v")+(CastItoIShort(m_client->GetMuleVersion()))+_("<- ClientModString ->")+(m_client->GetClientModString().c_str())+"<-";
	printf("%s\n", software.c_str());

	switch(m_client->GetClientSoft()) {
		case SO_UNKNOWN:
			GetDlgItem(ID_DSOFT,wxStaticText)->SetLabel(_("Unknown"));
			GetDlgItem(ID_DVERSION,wxStaticText)->SetLabel(_("Unknown"));
			break;
		case SO_EMULE:
		case SO_OLDEMULE:
			GetDlgItem(ID_DSOFT,wxStaticText)->SetLabel("eMule");
			if (m_client->GetMuleVersion()) {
				sprintf(buffer,"v%X",m_client->GetMuleVersion());
				if(m_client->GetClientModString().IsEmpty() == false) {
					sprintf(buffer, "[ %s ]", m_client->GetClientModString().c_str());
				}
				GetDlgItem(ID_DVERSION,wxStaticText)->SetLabel(buffer);
			} else {
				GetDlgItem(ID_DVERSION,wxStaticText)->SetLabel(_("Unknown"));
			}
			break;
		case SO_LXMULE:
			GetDlgItem(ID_DSOFT,wxStaticText)->SetLabel("lMule/xMule");
			sprintf(buffer,"v0.%02X",m_client->GetMuleVersion());
			GetDlgItem(ID_DVERSION,wxStaticText)->SetLabel(buffer);
			break;
		case SO_AMULE:
			GetDlgItem(ID_DSOFT,wxStaticText)->SetLabel("aMule");
			if(m_client->GetClientModString().IsEmpty() == false) {
				sprintf(buffer, "[ %s ]", m_client->GetClientModString().c_str());
			} else {
				sprintf(buffer,"v0.%02X",m_client->GetMuleVersion());
			}
			GetDlgItem(ID_DVERSION,wxStaticText)->SetLabel(buffer);
			break;
		case SO_EDONKEY:
			GetDlgItem(ID_DSOFT,wxStaticText)->SetLabel("eDonkey");
			sprintf(buffer,"v%i",m_client->GetVersion());
			GetDlgItem(ID_DVERSION,wxStaticText)->SetLabel(buffer);
			break;
		case SO_MLDONKEY:
			GetDlgItem(ID_DSOFT,wxStaticText)->SetLabel("Old MlDonkey");
			sprintf(buffer,"v%i",m_client->GetVersion());
			GetDlgItem(ID_DVERSION,wxStaticText)->SetLabel(buffer);
			break;
		case SO_NEW_MLDONKEY:
			GetDlgItem(ID_DSOFT,wxStaticText)->SetLabel("New MlDonkey");
			sprintf(buffer,"v%i",m_client->GetVersion());
			GetDlgItem(ID_DVERSION,wxStaticText)->SetLabel(buffer);
			break;
		case SO_EDONKEYHYBRID:
			GetDlgItem(ID_DSOFT,wxStaticText)->SetLabel("Hybrid");
			sprintf(buffer,"v%i",m_client->GetVersion());
			GetDlgItem(ID_DVERSION,wxStaticText)->SetLabel(buffer);
			break;
		default:
			GetDlgItem(ID_DVERSION,wxStaticText)->SetLabel(_("Unknown"));
	}

	sprintf(buffer,"%u (%s)",m_client->GetUserID(),(m_client->HasLowID() ? CString(_("Low")).GetData():CString(_("High")).GetData()));
	GetDlgItem(ID_DID,wxStaticText)->SetLabel(buffer);
	
	sprintf(buffer,"%s:%i",m_client->GetFullIP(),m_client->GetUserPort());
	GetDlgItem(ID_DIP,wxStaticText)->SetLabel(buffer);

	if (m_client->GetServerIP()) {
		in_addr server;
		server.s_addr = m_client->GetServerIP();
		GetDlgItem(ID_DSIP,wxStaticText)->SetLabel(inet_ntoa(server));
		
		CServer* cserver = theApp.serverlist->GetServerByAddress(inet_ntoa(server), m_client->GetServerPort()); 
		if (cserver) {
			GetDlgItem(ID_DSNAME,wxStaticText)->SetLabel(cserver->GetListName());
		} else {
			GetDlgItem(ID_DSNAME,wxStaticText)->SetLabel(_("Unknown"));
		}
	} else {
		GetDlgItem(ID_DSIP,wxStaticText)->SetLabel(_("Unknown"));
		GetDlgItem(ID_DSNAME,wxStaticText)->SetLabel(_("Unknown"));
	}

	CKnownFile* file = theApp.sharedfiles->GetFileByID(m_client->GetUploadFileID());

	if (file) {
		GetDlgItem(ID_DDOWNLOADING,wxStaticText)->SetLabel(file->GetFileName());
	} else {
		GetDlgItem(ID_DDOWNLOADING,wxStaticText)->SetLabel("-");
	}

	GetDlgItem(ID_DDUP,wxStaticText)->SetLabel(CastItoXBytes(m_client->GetTransferedDown()));
	GetDlgItem(ID_DDOWN,wxStaticText)->SetLabel(CastItoXBytes(m_client->GetTransferedUp()));
	sprintf(buffer,"%.1f %s",m_client->GetKBpsDown(),CString(_("kB/s")).GetData());
	GetDlgItem(ID_DAVUR,wxStaticText)->SetLabel(buffer);
	sprintf(buffer,"%.1f %s",m_client->GetKBpsUp(),CString(_("kB/s")).GetData());
	GetDlgItem(ID_DAVDR,wxStaticText)->SetLabel(buffer);

	if (m_client->Credits()) {
		GetDlgItem(ID_DUPTOTAL,wxStaticText)->SetLabel(CastItoXBytes(m_client->Credits()->GetDownloadedTotal()));		
		GetDlgItem(ID_DDOWNTOTAL,wxStaticText)->SetLabel(CastItoXBytes(m_client->Credits()->GetUploadedTotal()));
		sprintf(buffer,"%.1f",(float)m_client->Credits()->GetScoreRatio(m_client->GetIP()));
		GetDlgItem(ID_DRATIO,wxStaticText)->SetLabel(buffer);
		
		if (theApp.clientcredits->CryptoAvailable()){
			printf("Crypto available\n");
			switch(m_client->Credits()->GetCurrentIdentState(m_client->GetIP())){
				case IS_NOTAVAILABLE:
					printf("CurrentIdentState not available\n");
					GetDlgItem(IDC_CDIDENT,wxStaticText)->SetLabel(CString(_("Not Supported")));
					break;
				case IS_IDFAILED:
					printf("CurrentIdentState failed\n");
					GetDlgItem(IDC_CDIDENT,wxStaticText)->SetLabel(CString(_("Failed")));
					break;
				case IS_IDNEEDED:
					printf("CurrentIdentState needed\n");
					GetDlgItem(IDC_CDIDENT,wxStaticText)->SetLabel(CString(_("Not complete")));
					break;
				case IS_IDBADGUY:
					printf("CurrentIdentState badguy\n");
					GetDlgItem(IDC_CDIDENT,wxStaticText)->SetLabel(CString(_("Bad Guy")));
					break;
				case IS_IDENTIFIED:
					printf("CurrentIdentState Ident OK\n");
					GetDlgItem(IDC_CDIDENT,wxStaticText)->SetLabel(CString(_("Verified - OK")));
					break;
			}
		} else {
			printf("Crypto not available\n");
			GetDlgItem(IDC_CDIDENT,wxStaticText)->SetLabel(CString(_("Not Available")));
		}
		
		
	} else {
		GetDlgItem(ID_DDOWNTOTAL,wxStaticText)->SetLabel(_("Unknown"));
		GetDlgItem(ID_DUPTOTAL,wxStaticText)->SetLabel(_("Unknown"));
		GetDlgItem(ID_DRATIO,wxStaticText)->SetLabel(_("Unknown"));
	}
	
	if (m_client->GetUserName()) {
		sprintf(buffer,"%.1f",(float)m_client->GetScore(false,m_client->IsDownloading(),true));
		GetDlgItem(ID_DRATING,wxStaticText)->SetLabel(buffer);
	} else {
		GetDlgItem(ID_DRATING,wxStaticText)->SetLabel(_("Unknown"));;
	}
	if (m_client->GetUploadState() != US_NONE) {
		sprintf(buffer,"%u",m_client->GetScore(m_client->IsDownloading(),false));
		GetDlgItem(ID_DSCORE,wxStaticText)->SetLabel(buffer);
	} else {
		GetDlgItem(ID_DSCORE,wxStaticText)->SetLabel("-");
	}
	Layout();
	return true;
}
