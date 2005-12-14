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

#include "KadDlg.h"
#include "muuli_wdr.h"
#include "MD4Hash.h"
#include "OScopeCtrl.h"
#include "OtherFunctions.h"
#include "HTTPDownload.h"
#include "Logger.h"
#include "amule.h"
#include "Preferences.h"
#include "Statistics.h"
#include "StatisticsDlg.h"
#include "ColorFrameCtrl.h"

#include <wx/msgdlg.h>

#ifndef CLIENT_GUI
#include "NetworkFunctions.h"
#include "kademlia/kademlia/Kademlia.h"
#endif

#include <utility>

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



CKadDlg::CKadDlg(wxWindow* pParent) 
	: wxPanel(pParent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, wxT("kadwnd") )
{
	m_kad_scope = NULL;
}


void CKadDlg::Init()
{
	m_kad_scope = CastChild( wxT("kadScope"), COScopeCtrl );
	m_kad_scope->SetRanges(0.0, thePrefs::GetStatsMax());
	m_kad_scope->SetYUnits(wxT("Nodes"));

	SetUpdatePeriod();
	SetGraphColors();
}


void CKadDlg::SetUpdatePeriod(int step)
{
	// this gets called after the value in Preferences/Statistics/Update delay has been changed
	double sStep = step ? step : thePrefs::GetTrafficOMeterInterval();
		if (sStep == 0.0) {
	 	m_kad_scope->Stop();
	} else {
	 	m_kad_scope->Reset(sStep);
	}
}


void CKadDlg::SetGraphColors()
{
	static const char aTrend[] = { 2,      1,        0        };
	static const int aRes[]    = { IDC_C0, IDC_C0_3, IDC_C0_2 };
	
	m_kad_scope->SetBackgroundColor(CStatisticsDlg::getColors(0));
	m_kad_scope->SetGridColor(CStatisticsDlg::getColors(1));
	
	for (size_t i = 0; i < 3; ++i) {	
		m_kad_scope->SetPlotColor(CStatisticsDlg::getColors(12 + i), aTrend[i]);
		
		CColorFrameCtrl* ctrl = CastChild(aRes[i], CColorFrameCtrl);
		ctrl->SetBackgroundColor(CStatisticsDlg::getColors(12 + i));
		ctrl->SetFrameColor((COLORREF)RGB(0,0,0));	
	}
}


void CKadDlg::UpdateGraph(bool bStatsVisible, const GraphUpdateInfo& update)
{	
	const float* apfKad[] = { &update.kadnodes[0], &update.kadnodes[1], &update.kadnodes[2] };
	unsigned nodeCount = static_cast<unsigned>(update.kadnodes[2]);
	
	if (!bStatsVisible) {
		m_kad_scope->DelayPoints();
	} else {
		// Check the current node-count to see if we should increase the graph height
		if (m_kad_scope->GetUpperLimit() < update.kadnodes[2]) {
			// Grow the limit by 50 sized increments.
			m_kad_scope->SetRanges(0.0, ((nodeCount + 49) / 50) * 50);
		}

		m_kad_scope->AppendPoints(update.timestamp, apfKad);
	}

	wxStaticText* label = CastChild( wxT("nodesListLabel"), wxStaticText );
	wxCHECK_RET(label, wxT("Failed to find kad-nodes label"));

	label->SetLabel(wxString::Format(_("Nodes (%u)"), nodeCount));
	label->GetParent()->Layout();
}


// Enables or disables the node connect button depending on the conents of the text fields
void CKadDlg::OnFieldsChange(wxCommandEvent& WXUNUSED(evt))
{
	// These are the IDs of the search-fields 
	int textfields[] = { ID_NODE_IP1, ID_NODE_IP2, ID_NODE_IP3, ID_NODE_IP4, ID_NODE_PORT};

	bool enable = false;
	for ( uint16 i = 0; i < itemsof(textfields); i++ ) {
		enable &= !CastChild(textfields[i], wxTextCtrl)->GetValue().IsEmpty();
	}
	
	// Enable the node connect button if all fields contain text
	FindWindowById(ID_NODECONNECT)->Enable( enable );
}


void CKadDlg::OnBnClickedBootstrapClient(wxCommandEvent& WXUNUSED(evt))
{
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
			wxMessageBox(_("Invalid ip to bootstrap"), _("Warning"), wxOK | wxICON_EXCLAMATION, this);
		} else {
			unsigned long port;
			if (((wxTextCtrl*)FindWindowById( ID_NODE_PORT ))->GetValue().ToULong(&port)) {
				if ( !Kademlia::CKademlia::isRunning() ) {
					Kademlia::CKademlia::start();
					theApp.ShowConnectionState();
				}
				Kademlia::CKademlia::bootstrap(ip, port);				
			} else {
				wxMessageBox(_("Invalid port to bootstrap"), _("Warning"), wxOK | wxICON_EXCLAMATION, this);
			}
		}
		#else
			wxMessageBox(_("You can't bootstrap an specific ip from remote GUI yet."), _("Message"), wxOK | wxICON_INFORMATION, this);
		#endif		
	} else {
		wxMessageBox(_("Please fill all fields required"), _("Message"), wxOK | wxICON_INFORMATION, this);
	}
}


void CKadDlg::OnBnClickedBootstrapKnown(wxCommandEvent& WXUNUSED(evt))
{
	theApp.StartKad();
}


void CKadDlg::OnBnClickedDisconnectKad(wxCommandEvent& WXUNUSED(evt))
{
	theApp.StopKad();
}


void CKadDlg::OnBnClickedUpdateNodeList(wxCommandEvent& WXUNUSED(evt))
{
	#warning TODO EC
	#ifndef CLIENT_GUI
	if ( wxMessageBox( wxString(_("Are you sure you want to download a new nodes.dat file?\n")) +
						_("Doing so will remove your current nodes and restart Kademlia connection.")
					, _("Continue?"), wxICON_EXCLAMATION | wxYES_NO, this) == wxYES ) {
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
	wxMessageBox(_("You can't update server.met from remote GUI yet."), _("Message"), wxOK | wxICON_INFORMATION, this);
	#endif		
}
