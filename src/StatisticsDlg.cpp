//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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


#include <cmath>		// Needed for std::ceil

#include "ColorFrameCtrl.h"	// Needed for CColorFrameCtrl
#include "OtherFunctions.h"	// Needed for CastChild()
#include "OScopeCtrl.h"		// Needed for COScopeCtrl
#include "Preferences.h"	// Needed for thePrefs
#include "muuli_wdr.h"		// Needed for statsDlg()
#include "StatisticsDlg.h"	// Interface declarations
#include "Statistics.h"


class CTreeItemData : public wxTreeItemData
{
      public:
	explicit CTreeItemData(uint32_t uniqueid)
		: m_uniqueid(uniqueid)
	{}

	uint32_t GetUniqueId() const throw() { return m_uniqueid; }
	void SetUniqueId(uint32_t val) throw() { m_uniqueid = val; }

      private:
	uint32_t m_uniqueid;
};


// CStatisticsDlg panel

const wxColour& CStatisticsDlg::getColors(unsigned num)
{
	wxCHECK(num < 15, *wxBLACK);
	
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

	SetUpdatePeriod(thePrefs::GetTrafficOMeterInterval());
}


// this array is now used to store the current color settings and to define the defaults
wxColour CStatisticsDlg::acrStat[cntStatColors] =
	{ 
		wxColour(0,0,64), wxColour(192,192,255), wxColour(128, 255, 128), wxColour(0, 210, 0),
		wxColour(0, 128, 0), wxColour(255, 128, 128), wxColour(200, 0, 0), wxColour(140, 0, 0),
	  	wxColour(150, 150, 255), wxColour(192, 0, 192), wxColour(255, 255, 128), wxColour(0, 0, 0), 
	  	wxColour(128, 255, 128), wxColour(0, 210, 0), wxColour(0, 128, 0)
	};

void CStatisticsDlg::ApplyStatsColor(int index)
{
	static char aTrend[] = { 0,0,  2,     1,     0,           2,     1,     0,          1,    2,    0 };
	static int aRes[] = { 0,0, IDC_C0,IDC_C0_3,IDC_C0_2,  IDC_C1,IDC_C1_3,IDC_C1_2,  IDC_S0,IDC_S3,IDC_S1 };
	static COScopeCtrl** apscope[] = { NULL, NULL, &pscopeDL,&pscopeDL,&pscopeDL, &pscopeUL,&pscopeUL,&pscopeUL, &pscopeConn,&pscopeConn,&pscopeConn };

	const wxColour& cr = acrStat[index];  

	int iRes = aRes[index];
	int iTrend = aTrend[index];
	COScopeCtrl** ppscope = apscope[index];
	CColorFrameCtrl* ctrl;
	switch (index) {
		case 0:	
				pscopeDL->SetBackgroundColor(cr);
				pscopeUL->SetBackgroundColor(cr);
				pscopeConn->SetBackgroundColor(cr);
				break;
		case 1:	
				pscopeDL->SetGridColor(cr);
				pscopeUL->SetGridColor(cr);
				pscopeConn->SetGridColor(cr);
				break;
		case 2:  case 3:  case 4:
		case 5:  case 6:  case 7:	
		case 8:  case 9:  case 10:
				(*ppscope)->SetPlotColor(cr, iTrend);
				if ((ctrl = CastChild(iRes, CColorFrameCtrl)) == NULL) {
					throw wxString(CFormat(wxT("CStatisticsDlg::ApplyStatsColor: control missing (%d)\n")) % iRes);
				}
				ctrl->SetBackgroundBrushColour(cr);
				ctrl->SetFrameBrushColour(*wxBLACK);
				break;
		default:
				break; // ignore unknown index, like SysTray speedbar color
	}
}

void CStatisticsDlg::UpdateStatGraphs(const uint32 peakconnections, const GraphUpdateInfo& update)
{
	
	std::vector<float *> v1(3);
	v1[0] = const_cast<float *>(&update.downloads[0]);
	v1[1] = const_cast<float *>(&update.downloads[1]);
	v1[2] = const_cast<float *>(&update.downloads[2]);
	const std::vector<float *> &apfDown(v1);
	std::vector<float *> v2(3);
	v2[0] = const_cast<float *>(&update.uploads[0]);
	v2[1] = const_cast<float *>(&update.uploads[1]);
	v2[2] = const_cast<float *>(&update.uploads[2]);
	const std::vector<float *> &apfUp(v2);
	std::vector<float *> v3(3);
	v3[0] = const_cast<float *>(&update.connections[0]);
	v3[1] = const_cast<float *>(&update.connections[1]);
	v3[2] = const_cast<float *>(&update.connections[2]);
	const std::vector<float *> &apfConn(v3);
	
	if (!IsShownOnScreen()) {
		pscopeDL->DelayPoints();
		pscopeUL->DelayPoints();
		pscopeConn->DelayPoints();
	}

	static unsigned nScalePrev=1;
	unsigned nScale = (unsigned)std::ceil((float)peakconnections / pscopeConn->GetUpperLimit());
	if (nScale != nScalePrev) {
		nScalePrev = nScale;
		wxStaticText* label = CastChild( ID_ACTIVEC, wxStaticText );
		
		label->SetLabel(CFormat(_("Active connections (1:%u)")) % nScale);
		label->GetParent()->Layout();
		
		pscopeConn->SetRange(0.0, (float)nScale*pscopeConn->GetUpperLimit(), 1);
	}
	
	if (!IsShownOnScreen()) {
		return;
	}
	
	pscopeDL->AppendPoints(update.timestamp, apfDown);
	pscopeUL->AppendPoints(update.timestamp, apfUp);
	pscopeConn->AppendPoints(update.timestamp, apfConn);
}


void CStatisticsDlg::SetUpdatePeriod(int step)
{
	// this gets called after the value in Preferences/Statistics/Update delay has been changed
	if (step == 0) {
	 	pscopeDL->Stop();
 		pscopeUL->Stop();
	 	pscopeConn->Stop();
	} else {
	 	pscopeDL->Reset(step);
 		pscopeUL->Reset(step);
	 	pscopeConn->Reset(step);
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

void CStatisticsDlg::GetExpandedNodes(NodeIdSet& nodeset, const wxTreeItemId& root)
{
	wxTreeItemIdValue cookie;
	wxTreeItemId temp_it = stattree->GetFirstChild(root,cookie);
	
	while (temp_it.IsOk()) {
		if (stattree->IsExpanded(temp_it)) {
			nodeset.insert(dynamic_cast<CTreeItemData*>(stattree->GetItemData(temp_it))->GetUniqueId());
		}
		if (stattree->ItemHasChildren(temp_it)) {
			GetExpandedNodes(nodeset, temp_it);
		}
		temp_it = stattree->GetNextSibling(temp_it);
	}
}

void CStatisticsDlg::ShowStatistics(bool init)
{
	NodeIdSet ExpandedNodes;

	// If it's not the first initialization of the tree, i.e. application startup
	if (!init) {
		GetExpandedNodes(ExpandedNodes, stattree->GetRootItem());
		// Update sorting / get tree via EC
		m_stats->UpdateStatsTree();
	}
	
	CStatTreeItemBase* treeRoot = theStats::GetTreeRoot();
	wxTreeItemId root = stattree->GetRootItem();
	FillTree(treeRoot, root, ExpandedNodes);
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


#ifdef CLIENT_GUI
void CStatisticsDlg::RebuildStatTreeRemote(const CECTag * tag)
{
	m_stats->RebuildStatTreeRemote(tag);
}
#endif


void CStatisticsDlg::FillTree(CStatTreeItemBase* statssubtree, wxTreeItemId& StatsGUITree, const NodeIdSet& expandednodes)
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
			uint32_t uid = (*temp_it)->GetUniqueId();
			dynamic_cast<CTreeItemData*>(stattree->GetItemData(temp_GUI_it))->SetUniqueId(uid);
			if (expandednodes.find(uid) != expandednodes.end()) {
				stattree->Expand(temp_GUI_it);
			} else {
				stattree->Collapse(temp_GUI_it);
			}
			temp_GUI_it = stattree->GetNextSibling(temp_GUI_it);
		} else {
			// No more child on GUI, add them.
			temp_item = stattree->AppendItem(StatsGUITree,(*temp_it)->GetDisplayString());
			stattree->SetItemData(temp_item, new CTreeItemData((*temp_it)->GetUniqueId()));
		}
		// Has childs?
		if ((*temp_it)->HasVisibleChildren()) {
			FillTree((*temp_it), temp_item, expandednodes);
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
// File_checked_for_headers
