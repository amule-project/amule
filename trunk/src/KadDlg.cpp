//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2005 Angel Vidal (Kry) ( kry@amule.org / http://www.amule.org )
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

#include <wx/textctrl.h>
#include "KadDlg.h"
#include "muuli_wdr.h"
#include "MD4Hash.h"
#include "OScopeCtrl.h"
#include "OtherFunctions.h"
#include "HTTPDownload.h"
#include "Logger.h"
#include "amule.h"
#include "Preferences.h"
#include <wx/sizer.h>
#include <wx/intl.h>
#include <wx/msgdlg.h>

#ifndef CLIENT_GUI
#include "NetworkFunctions.h"
#include "kademlia/kademlia/Kademlia.h"
#endif

BEGIN_EVENT_TABLE(CKadDlg, wxPanel)
	
	EVT_TEXT(ID_NODE_IP1, CKadDlg::OnFieldsChange)
	EVT_TEXT(ID_NODE_IP2, CKadDlg::OnFieldsChange)
	EVT_TEXT(ID_NODE_IP3, CKadDlg::OnFieldsChange)	
	EVT_TEXT(ID_NODE_IP4, CKadDlg::OnFieldsChange)
	EVT_TEXT(ID_NODE_PORT, CKadDlg::OnFieldsChange)

	EVT_TEXT_ENTER(IDC_NODESLISTURL ,CKadDlg::OnBnClickedUpdateNodeList)
	
	EVT_BUTTON(ID_NODECONNECT, CKadDlg::OnBnClickedBootstrapClient)
	EVT_BUTTON(ID_KNOWNNODECONNECT, CKadDlg::OnBnClickedBootstrapKnown)
	EVT_BUTTON(ID_KADDISCONNECT, CKadDlg::OnBnClickedDisconnectKad)
	EVT_BUTTON(ID_UPDATEKADLIST, CKadDlg::OnBnClickedUpdateNodeList)
	
END_EVENT_TABLE()



CKadDlg::CKadDlg(wxWindow* pParent) : wxPanel(pParent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxT("kadwnd") ) {
	m_nodecount = 0;
}

void CKadDlg::Init() {
	m_nextshow=0;
	
	pscopeKad = CastChild( wxT("kadScope"), COScopeCtrl );
	wxASSERT(pscopeKad);
	pscopeKad->SetRanges(0.0, (float)(thePrefs::GetStatsMax()));
	pscopeKad->SetYUnits(wxT("Nodes"));		
	
}

void CKadDlg::SetUpdatePeriod()
{
	// this gets called after the value in Preferences/Statistics/Update delay has been changed
	double sStep = thePrefs::GetTrafficOMeterInterval();
	if (sStep == 0.0) {
	 	pscopeKad->Stop();
	} else {
	 	pscopeKad->Reset(sStep);
	}
}

// Enables or disables the node connect button depending on the conents of the text fields
void	CKadDlg::OnFieldsChange(wxCommandEvent& WXUNUSED(evt))
{
	// These are the IDs of the search-fields 
	int textfields[] = { ID_NODE_IP1, ID_NODE_IP2, ID_NODE_IP3, ID_NODE_IP4, ID_NODE_PORT};

	bool enable = false;
	for ( uint16 i = 0; i < itemsof(textfields); i++ ) {
		enable &= !((wxTextCtrl*)FindWindowById( textfields[i] ))->GetValue().IsEmpty();
	}
	
	// Enable the node connect button if all fields contain text
	FindWindowById(ID_NODECONNECT)->Enable( enable );
}

void	CKadDlg::OnBnClickedBootstrapClient(wxCommandEvent& WXUNUSED(evt)) {
	if (FindWindowById(ID_NODECONNECT)->IsEnabled()) {
		#warning TODO EC
		#ifndef CLIENT_GUI
		// Connect to node
		uint32 ip = StringIPtoUint32(
					((wxTextCtrl*)FindWindowById( ID_NODE_IP1 ))->GetValue() +
					wxT(".") + 
					((wxTextCtrl*)FindWindowById( ID_NODE_IP1 ))->GetValue() +
					wxT(".") + 
					((wxTextCtrl*)FindWindowById( ID_NODE_IP1 ))->GetValue() +
					wxT(".") + 
					((wxTextCtrl*)FindWindowById( ID_NODE_IP1 ))->GetValue() );
		if (ip == 0) {
			wxMessageBox(_("Invalid ip to bootstrap"));
		} else {
			unsigned long port;
			if (((wxTextCtrl*)FindWindowById( ID_NODE_PORT ))->GetValue().ToULong(&port)) {
				if ( !Kademlia::CKademlia::isRunning() ) {
					Kademlia::CKademlia::start();
					theApp.ShowConnectionState();
				}
				Kademlia::CKademlia::bootstrap(ip, port);				
			} else {
				wxMessageBox(_("Invalid port to bootstrap"));
			}
		}
		#else
			wxMessageBox(_("You can't bootstrap from remote GUI yet."));
		#endif		
	} else {
		wxMessageBox(_("Please fill all fields required"));
	}
}

void	CKadDlg::OnBnClickedBootstrapKnown(wxCommandEvent& WXUNUSED(evt)) {
	#ifndef CLIENT_GUI
	if ( !Kademlia::CKademlia::isRunning() ) {
		Kademlia::CKademlia::start();
	}	
	#endif
}

void	CKadDlg::OnBnClickedDisconnectKad(wxCommandEvent& WXUNUSED(evt)) {
	#ifndef CLIENT_GUI
	if ( Kademlia::CKademlia::isRunning() ) {
		Kademlia::CKademlia::stop();
	}
	#endif
}

void	CKadDlg::OnBnClickedUpdateNodeList(wxCommandEvent& WXUNUSED(evt)) {
	#warning TODO EC
	#ifndef CLIENT_GUI

	if ( wxMessageBox( wxString(_("Are you sure you want to download a new nodes.dat file?\n")) +
						_("Doing so will remove your current nodes and restart Kademlia connection.")
					, _("Continue?"), wxICON_EXCLAMATION | wxYES_NO) == wxYES ) {
		wxString strURL = ((wxTextCtrl*)FindWindowById( IDC_NODESLISTURL ))->GetValue();
		if (strURL.Find(wxT("://")) == -1) {
			AddLogLineM(true, _("Invalid URL"));
			return;
		}
		wxString strTempFilename(theApp.ConfigDir + wxT("nodes.dat.download"));
		CHTTPDownloadThread *downloader = new CHTTPDownloadThread(strURL,strTempFilename, HTTP_NodesDat);
		downloader->Create();
		downloader->Run();
	}
	#else
	wxMessageBox(_("You can't update server.met from remote GUI yet."));
	#endif		
}

void CKadDlg::ShowNodeCount() {
	uint32 now = ::GetTickCount();

	if (m_nextshow>now)
		return;

	m_nextshow=now+500;
	
	wxStaticText* label = CastChild( wxT("nodesListLabel"), wxStaticText );

	if ( label ) {
		label->SetLabel( wxString::Format( _("Nodes (%i)"), m_nodecount ) );
		label->GetParent()->Layout();
	}
	
}
