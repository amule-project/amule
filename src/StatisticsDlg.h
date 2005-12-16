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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef STATISTICSDLG_H
#define STATISTICSDLG_H

#include <wx/panel.h>		// Needed for wxPanel

#include "Types.h"		// Needed for uint32 and uint64
#include "Color.h"		// Needed for COLORREF

class COScopeCtrl;
class CStatistics;
class CStatTreeItemBase;
class wxTreeCtrl;
class wxTreeItemId;
//struct UpdateInfo;
typedef struct UpdateInfo GraphUpdateInfo;

// CStatisticsDlg panel

class CStatisticsDlg : public wxPanel
{
	friend class PrefsUnifiedDlg;
	friend class CPreferences;

public:
	CStatisticsDlg(wxWindow* pParent, CStatistics* stats);
	~CStatisticsDlg();

	void UpdateStatGraphs(bool bStatsVisible, const uint32 peakconnections, const GraphUpdateInfo& update);
	void SetUpdatePeriod(int step = 0);
	void ResetAveragingTime();
	void ShowStatistics(bool init = false);
	void SetARange(bool SetDownload, int maxValue);
	void FillTree(CStatTreeItemBase* statssubtree, wxTreeItemId& StatsGUITree);
	void Init();
	void InitTree();
	void InitGraphs();
	void ApplyStatsColor(int index);
	static COLORREF getColors(unsigned num);	
	COScopeCtrl* GetDLScope() { return pscopeDL; };
	COScopeCtrl* GetConnScope() { return pscopeConn; };

protected:
	static COLORREF	acrStat[15];
	COScopeCtrl* pscopeDL,*pscopeUL,*pscopeConn;
	wxTreeCtrl* stattree;
	CStatistics* m_stats;
};

#endif // STATISTICSDLG_H
