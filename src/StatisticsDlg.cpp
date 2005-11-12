//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/thread.h>		// Needed for wxMutexLocker

#include <cmath>		// Needed for std::ceil

#include "ColorFrameCtrl.h"	// Needed for CColorFrameCtrl
#include "OtherFunctions.h"	// Needed for CastChild()
#include "OScopeCtrl.h"		// Needed for COScopeCtrl
#include "Preferences.h"	// Needed for thePrefs
#include "muuli_wdr.h"		// Needed for statsDlg()
#include "Statistics.h"		// Needed for theStats
#include "StatisticsDlg.h"	// Interface declarations

// CStatisticsDlg panel

COLORREF CStatisticsDlg::getColors(unsigned num)
{
	wxCHECK(num < 16, RGB(0, 0, 0));
	
	return acrStat[num];
}

CStatisticsDlg::CStatisticsDlg(wxWindow* pParent, CStatistics* stats)
: wxPanel(pParent, -1)
{
	wxSizer* content=statsDlg(this,TRUE);
	content->Show(this,TRUE);

	pscopeDL	= CastChild( wxT("dloadScope"), COScopeCtrl );
	pscopeDL->graph_type = GRAPH_DOWN;
	pscopeUL	= CastChild( wxT("uloadScope"), COScopeCtrl );
	pscopeUL->graph_type = GRAPH_UP;
	pscopeConn	= CastChild( wxT("otherScope"), COScopeCtrl );
	pscopeConn->graph_type = GRAPH_CONN;
	stattree	= CastChild( wxT("statTree"),	wxTreeCtrl  );

	m_stats = stats;
}



CStatisticsDlg::~CStatisticsDlg()
{

}

void CStatisticsDlg::Init()
{
	InitGraphs();
	InitTree();
}


void CStatisticsDlg::InitGraphs()
{
	// called after preferences get initialised
	for (int index=0; index<=10; ++index) {
		ApplyStatsColor(index);
	}

	pscopeDL->SetRanges(0.0, (float)(thePrefs::GetMaxGraphDownloadRate()+4));
	pscopeDL->SetYUnits(_("kB/s"));
	pscopeUL->SetRanges(0.0, (float)(thePrefs::GetMaxGraphUploadRate()+4));
	pscopeUL->SetYUnits(_("kB/s"));
	pscopeConn->SetRanges(0.0, (float)(thePrefs::GetStatsMax()));
	pscopeConn->SetYUnits(wxEmptyString);

	SetUpdatePeriod();
}


// this array is now used to store the current color settings and to define the defaults
COLORREF CStatisticsDlg::acrStat[cntStatColors] =
	{ 
		RGB(0,0,64),		RGB(192,192,255),	RGB(128, 255, 128),	RGB(0, 210, 0),
		RGB(0, 128, 0),		RGB(255, 128, 128),	RGB(200, 0, 0),		RGB(140, 0, 0),
	  	RGB(150, 150, 255),	RGB(192, 0, 192),	RGB(255, 255, 128),	RGB(0, 0, 0), 
	  	RGB(255, 255, 255), RGB(128, 255, 128),	RGB(0, 210, 0),		RGB(0, 128, 0)
	};



void CStatisticsDlg::ApplyStatsColor(int index)
{
	static char aTrend[] = { 0,0,  2,     1,     0,           2,     1,     0,          1,    2,    0 };
	static int aRes[] = { 0,0, IDC_C0,IDC_C0_3,IDC_C0_2,  IDC_C1,IDC_C1_3,IDC_C1_2,  IDC_S0,IDC_S3,IDC_S1 };
	static COScopeCtrl** apscope[] = { NULL, NULL, &pscopeDL,&pscopeDL,&pscopeDL, &pscopeUL,&pscopeUL,&pscopeUL, &pscopeConn,&pscopeConn,&pscopeConn };

	COLORREF cr = acrStat[index];  

	int iRes = aRes[index];
	int iTrend = aTrend[index];
	COScopeCtrl** ppscope = apscope[index];
	CColorFrameCtrl* ctrl;
	switch (index) {
		case 0:	pscopeDL->SetBackgroundColor(cr);
				pscopeUL->SetBackgroundColor(cr);
				pscopeConn->SetBackgroundColor(cr);
				break;
		case 1:	pscopeDL->SetGridColor(cr);
				pscopeUL->SetGridColor(cr);
				pscopeConn->SetGridColor(cr);
				break;
		case 2:  case 3:  case 4:
		case 5:  case 6:  case 7:	
		case 8:  case 9:  case 10:
				(*ppscope)->SetPlotColor(cr, iTrend);
				if ((ctrl= CastChild(iRes, CColorFrameCtrl)) == NULL) {
					printf("CStatisticsDlg::ApplyStatsColor: control missing (%d)\n",iRes);
					exit(1);
				}
				ctrl->SetBackgroundColor(cr);
				ctrl->SetFrameColor((COLORREF)RGB(0,0,0));
				break;
		default:
				break; // ignore unknown index, like SysTray speedbar color
	}
}

void CStatisticsDlg::UpdateStatGraphs(bool bStatsVisible, const uint32 peakconnections, const GraphUpdateInfo& update)
{
	
	const float *apfDown[] = { &update.downloads[0], &update.downloads[1], &update.downloads[2] };
	const float *apfUp[] = { &update.uploads[0], &update.uploads[1], &update.uploads[2] };
	const float *apfConn[] = { &update.connections[0], &update.connections[1], &update.connections[2] };
	
	if (!bStatsVisible) {
		pscopeDL->DelayPoints();
		pscopeUL->DelayPoints();
		pscopeConn->DelayPoints();
	}

	static unsigned nScalePrev=1;
	unsigned nScale = (unsigned)std::ceil((float)peakconnections / pscopeConn->GetUpperLimit());
	if (nScale != nScalePrev) {
		nScalePrev = nScale;
		wxStaticText* label = CastChild( ID_ACTIVEC, wxStaticText );
		
		label->SetLabel(wxString::Format(_("Active connections (1:%u)"), nScale));
		label->GetParent()->Layout();
		
		pscopeConn->SetRange(0.0, (float)nScale*pscopeConn->GetUpperLimit(), 1);
	}
	if (!bStatsVisible)
		return;
	
	pscopeDL->AppendPoints(update.timestamp, apfDown);
	pscopeUL->AppendPoints(update.timestamp, apfUp);
	pscopeConn->AppendPoints(update.timestamp, apfConn);
}


void CStatisticsDlg::SetUpdatePeriod()
{
	// this gets called after the value in Preferences/Statistics/Update delay has been changed
	double sStep = thePrefs::GetTrafficOMeterInterval();
	if (sStep == 0.0) {
	 	pscopeDL->Stop();
 		pscopeUL->Stop();
	 	pscopeConn->Stop();
	} else {
	 	pscopeDL->Reset(sStep);
 		pscopeUL->Reset(sStep);
	 	pscopeConn->Reset(sStep);
	}
}


void CStatisticsDlg::ResetAveragingTime()
{
	// this gets called after the value in Preferences/Statistics/time for running avg has been changed
 	pscopeDL->InvalidateGraph();
 	pscopeUL->InvalidateGraph();
}


void CStatisticsDlg::SetARange(bool SetDownload,int maxValue)
{
	if ( SetDownload ) {
		pscopeDL->SetRanges( 0, maxValue + 4 );
	} else {
		pscopeUL->SetRanges( 0, maxValue + 4 );
	}
}


void  CStatisticsDlg::InitTree()
{
	wxTreeItemId root=stattree->AddRoot(theStats::GetTreeRoot()->GetDisplayString());

	ShowStatistics(true);

#ifndef CLIENT_GUI
	// Expand root
	stattree->Expand(root);

	// Expand main items
	wxTreeItemIdValue cookie;
	wxTreeItemId expand_it = stattree->GetFirstChild(root,cookie);

	while(expand_it.IsOk()) {
		stattree->Expand(expand_it);
		// Next on this level
		expand_it = stattree->GetNextSibling(expand_it);
	}
#endif
}


void CStatisticsDlg::ShowStatistics(bool init)
{
	// If it's not the first initialization of the tree, i.e. application startup
	if (!init) {
		// Update sorting / get tree via EC
		m_stats->UpdateStatsTree();
	}
	
	CStatTreeItemBase* treeRoot = theStats::GetTreeRoot();
	wxTreeItemId root = stattree->GetRootItem();
	FillTree(treeRoot, root);
#ifdef CLIENT_GUI
	if (!init) {
		static bool firstUpdate = true;
		if (firstUpdate) {
			// Expand root
			root = stattree->GetRootItem();
			stattree->Expand(root);

			// Expand main items
			wxTreeItemIdValue cookie;
			wxTreeItemId expand_it = stattree->GetFirstChild(root,cookie);

			while(expand_it.IsOk()) {
				stattree->Expand(expand_it);
				// Next on this level
				expand_it = stattree->GetNextSibling(expand_it);
			}
			firstUpdate = false;
		}
	}
#endif
}


void CStatisticsDlg::FillTree(CStatTreeItemBase* statssubtree, wxTreeItemId& StatsGUITree)
{
	wxMutexLocker lock(statssubtree->GetLock());

#ifndef CLIENT_GUI
	StatTreeItemIterator temp_it = statssubtree->GetFirstVisibleChild(thePrefs::GetMaxClientVersions());
#else
	StatTreeItemIterator temp_it = statssubtree->GetFirstVisibleChild();
#endif	

	wxTreeItemIdValue cookie;
	wxTreeItemId temp_GUI_it = stattree->GetFirstChild(StatsGUITree,cookie);
	
	while (!statssubtree->IsAtEndOfList(temp_it)) {
		wxTreeItemId temp_item;
		if (temp_GUI_it.IsOk()) {
			// There's already a child there, update it.
			stattree->SetItemText(temp_GUI_it, (*temp_it)->GetDisplayString());
			temp_item = temp_GUI_it;
			temp_GUI_it = stattree->GetNextSibling(temp_GUI_it);
		} else {
			// No more child on GUI, add them.
			temp_item = stattree->AppendItem(StatsGUITree,(*temp_it)->GetDisplayString());
		}
		// Has childs?
		if ((*temp_it)->HasVisibleChildren()) {
			FillTree((*temp_it), temp_item);
		} else {
			stattree->DeleteChildren(temp_item);
		}
		statssubtree->GetNextVisibleChild(temp_it);
	}	

	// What if GUI has more items than tree?
	// Delete the extra items.
	while (temp_GUI_it.IsOk()) {
		wxTreeItemId backup_node = stattree->GetNextSibling(temp_GUI_it);
		stattree->DeleteChildren(temp_GUI_it);
		stattree->Delete(temp_GUI_it);
		temp_GUI_it = backup_node;
	}
}
