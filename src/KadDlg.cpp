//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2011 Angel Vidal (Kry) ( kry@amule.org / http://www.amule.org )
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

#include "KadDlg.h"
#include "muuli_wdr.h"
#include "OScopeCtrl.h"
#include "OtherFunctions.h"
#include "HTTPDownload.h"
#include "Logger.h"
#include "amule.h"
#include "Preferences.h"
#include "StatisticsDlg.h"
#include "ColorFrameCtrl.h"
#include "amuleDlg.h"
#include "MuleColour.h"
#include "Statistics.h"

#ifndef CLIENT_GUI
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

#ifndef __WINDOWS__
	//
	// Get label with line breaks out of muuli.wdr, because generated code fails
	// to compile in Windows.
	//
	// In Windows, setting a button label with a newline fails (the newline is ignored).
	// Creating a button with such a label works however. :-/
	// So leave the label from the muuli (without line breaks) here,
	// so it can still be fixed in the translation.
	//
	wxButton* bootstrap = CastChild(ID_KNOWNNODECONNECT, wxButton);
	bootstrap->SetLabel(_("Bootstrap from \nknown clients"));
#endif

	SetUpdatePeriod(thePrefs::GetTrafficOMeterInterval());
	SetGraphColors();
}


void CKadDlg::SetUpdatePeriod(int step)
{
	// this gets called after the value in Preferences/Statistics/Update delay has been changed
	if (step == 0) {
		m_kad_scope->Stop();
	} else {
		m_kad_scope->Reset(step);
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
		ctrl->SetBackgroundBrushColour(CMuleColour(CStatisticsDlg::getColors(12 + i)));
		ctrl->SetFrameBrushColour(*wxBLACK);
	}
}


void CKadDlg::UpdateGraph(const GraphUpdateInfo& update)
{
	std::vector<float *> v(3);
	v[0] = const_cast<float *>(&update.kadnodes[0]);
	v[1] = const_cast<float *>(&update.kadnodes[1]);
	v[2] = const_cast<float *>(&update.kadnodes[2]);
	const std::vector<float *> &apfKad(v);
	unsigned nodeCount = static_cast<unsigned>(update.kadnodes[2]);

	if (!IsShownOnScreen()) {
		m_kad_scope->DelayPoints();
	} else {
		// Check the current node-count to see if we should increase the graph height
		if (m_kad_scope->GetUpperLimit() < update.kadnodes[2]) {
			// Grow the limit by 50 sized increments.
			m_kad_scope->SetRanges(0.0, ((nodeCount + 49) / 50) * 50);
		}

		m_kad_scope->AppendPoints(update.timestamp, apfKad);
	}
}


void CKadDlg::UpdateNodeCount(unsigned nodeCount)
{
	wxStaticText* label = CastChild( wxT("nodesListLabel"), wxStaticText );
	wxCHECK_RET(label, wxT("Failed to find kad-nodes label"));

	label->SetLabel(CFormat(_("Nodes (%u)")) % nodeCount);
	label->GetParent()->Layout();
}


// Enables or disables the node connect button depending on the conents of the text fields
void CKadDlg::OnFieldsChange(wxCommandEvent& WXUNUSED(evt))
{
	// These are the IDs of the search-fields
	int textfields[] = { ID_NODE_IP1, ID_NODE_IP2, ID_NODE_IP3, ID_NODE_IP4, ID_NODE_PORT};

	bool enable = true;
	for ( uint16 i = 0; i < itemsof(textfields); i++ ) {
		enable &= !CastChild(textfields[i], wxTextCtrl)->GetValue().IsEmpty();
	}

	// Enable the node connect button if all fields contain text
	FindWindowById(ID_NODECONNECT)->Enable( enable );
}


void CKadDlg::OnBnClickedBootstrapClient(wxCommandEvent& WXUNUSED(evt))
{
	if (FindWindowById(ID_NODECONNECT)->IsEnabled()) {
		// Ip is reversed since StringIPtoUint32 returns anti-host and kad expects host order
		uint32 ip = StringIPtoUint32(
			dynamic_cast<wxTextCtrl*>(FindWindowById(ID_NODE_IP4))->GetValue() + wxT(".") +
			dynamic_cast<wxTextCtrl*>(FindWindowById(ID_NODE_IP3))->GetValue() + wxT(".") +
			dynamic_cast<wxTextCtrl*>(FindWindowById(ID_NODE_IP2))->GetValue() + wxT(".") +
			dynamic_cast<wxTextCtrl*>(FindWindowById(ID_NODE_IP1))->GetValue() );

		if (ip == 0) {
			wxMessageBox(_("Invalid ip to bootstrap"), _("WARNING"), wxOK | wxICON_EXCLAMATION, this);
		} else {
			unsigned long port;
			if (dynamic_cast<wxTextCtrl*>(FindWindowById(ID_NODE_PORT))->GetValue().ToULong(&port)) {
				theApp->BootstrapKad(ip, port);
			} else {
				wxMessageBox(_("Invalid port to bootstrap"), _("WARNING"), wxOK | wxICON_EXCLAMATION, this);
			}
		}
	} else {
		wxMessageBox(_("Please fill all fields required"), _("Message"), wxOK | wxICON_INFORMATION, this);
	}
}


void CKadDlg::OnBnClickedBootstrapKnown(wxCommandEvent& WXUNUSED(evt))
{
	theApp->StartKad();
}


void CKadDlg::OnBnClickedDisconnectKad(wxCommandEvent& WXUNUSED(evt))
{
	theApp->StopKad();
}


void CKadDlg::OnBnClickedUpdateNodeList(wxCommandEvent& WXUNUSED(evt))
{
	if ( wxMessageBox( wxString(_("Are you sure you want to download a new nodes.dat file?\n")) +
						_("Doing so will remove your current nodes and restart Kademlia connection.")
					, _("Continue?"), wxICON_EXCLAMATION | wxYES_NO, this) == wxYES ) {
		wxString strURL = dynamic_cast<wxTextCtrl*>(FindWindowById(IDC_NODESLISTURL))->GetValue();

		thePrefs::SetKadNodesUrl(strURL);
		theApp->UpdateNotesDat(strURL);
	}
}
// File_checked_for_headers
