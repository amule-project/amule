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

#ifndef STATISTICSDLG_H
#define STATISTICSDLG_H

#include <cmath>			// Needed for std::exp
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel
#include <wx/treebase.h>	// Needed for wxTreeItemId (HTREEITEM) and wxTreeCtrl

#include "types.h"			// Needed for uint32 and uint64
#include "CTypedPtrList.h"	// Needed for CList
#include "Statistics.h"

class COScopeCtrl;

// CStatisticsDlg dialog

using namespace std;

class CStatisticsDlg : public wxPanel// CResizableDialog
{
	friend class PrefsUnifiedDlg;
	friend class CPreferences;
public:
	CStatisticsDlg(wxWindow* pParent = NULL);   // standard constructor
	~CStatisticsDlg();

	void UpdateStatGraphs(bool bStatsVisible, const uint32 peakconnections, const GraphUpdateInfo& update);
	void SetUpdatePeriod();
	void ResetAveragingTime();
	void ShowStatistics();
	void SetARange(bool SetDownload,int maxValue);
	void FillTree(StatsTreeSiblingIterator& statssubtree, wxTreeItemId& StatsGUITree);
	void Init();
	void InitTree();
	void InitGraphs();
	void ApplyStatsColor(int index);
	
	COScopeCtrl* GetDLScope()	{ return pscopeDL; };
	
protected:
	static COLORREF	acrStat[13];
	
 private:

    COScopeCtrl* pscopeDL,*pscopeUL,*pscopeConn;
    wxTreeCtrl* stattree;

	void SetStatsRanges(int min, int max);
				
protected:
	DECLARE_EVENT_TABLE()

};

#endif // STATISTICSDLG_H
