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
#include "amule.h"			// Needed for StatsTreeNode
#include "CTypedPtrList.h"	// Needed for CList

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

	void Localize();
	void UpdateStatGraphs(bool bStatsVisible);
	void SetUpdatePeriod();
	void ResetAveragingTime();
	void ShowStatistics();
	void SetARange(bool SetDownload,int maxValue);
	float GetKBpsUpCurrent()		{return kBpsUpCur;}
	float GetKBpsUpRunningAvg()		{return kBpsUpAvg;}
	float GetKBpsUpSession()		{return kBpsUpSession;}
	float GetKBpsDownCurrent()		{return kBpsDownCur;}
	float GetKBpsDownRunningAvg()	{return kBpsDownAvg;}
	float GetKBpsDownSession()		{return kBpsDownSession;}
	void RecordHistory();
	unsigned GetHistoryForWeb(unsigned cntPoints, double sStep, double *sStart, uint32 **graphData);
	unsigned GetHistory(unsigned cntPoints, double sStep, double sFinal, float **ppf, COScopeCtrl* pscope);
	void VerifyHistory(bool bMsgIfOk = false);
	void FillTree(StatsTreeSiblingIterator& statssubtree, wxTreeItemId& StatsGUITree);
	void Init();
	void InitTree();
	void InitGraphs();
	void ApplyStatsColor(int index);
	COLORREF GetTrayBarColor()		{return acrStat[11];}

	wxString IterateChilds(wxTreeItemId hChild, int level);
	
	COScopeCtrl* GetDLScope()	{ return pscopeDL; };
	
protected:
	static COLORREF	acrStat[13];
	
 private:
 
 	typedef struct HistoryRecord {
		double		kBytesSent;
		double		kBytesReceived;
		float		kBpsUpCur;
		float		kBpsDownCur;
		double		sTimestamp;
		uint16		cntDownloads;
		uint16		cntUploads;
		uint16		cntConnections;
	} HR;
	HR hrInit;

	int				nHistRanges;
	int				bitsHistClockMask;
	int				nPointsPerRange;
	POSITION*		aposRecycle;
	CList<HR,HR>	listHR;
    COScopeCtrl* pscopeDL,*pscopeUL,*pscopeConn;
    wxTreeCtrl* stattree;

	int dl_users,dl_active;
	int stat_max;
	float maxDownavg;
	float maxDown;
	float kBpsUpCur;
	float kBpsUpAvg;
	float kBpsUpSession;
	float kBpsDownCur;
	float kBpsDownAvg;
	float kBpsDownSession;
	uint32 m_ilastMaxConnReached;

	void SetStatsRanges(int min, int max);
 public:

 	uint32 peakconnections;
	uint32 activeconnections;
 
private:
	void ComputeAverages(HR **pphr, POSITION pos, unsigned cntFilled, double sStep, float **ppf, COScopeCtrl	*pscope);

	// ComputeSessionAvg and ComputeRunningAvg are used to assure consistent computations across
	// RecordHistory and ComputeAverages; see note in RecordHistory on the use of double and float 
	void ComputeSessionAvg(float& kBpsSession, float& kBpsCur, double& kBytesTrans, double& sCur, double& sTrans);

	void ComputeRunningAvg(float& kBpsRunning, float& kBpsSession, double& kBytesTrans, 
							double& kBytesTransPrev, double& sTrans, double& sPrev, float& sAvg);
							
protected:
	DECLARE_EVENT_TABLE()

};

#endif // STATISTICSDLG_H
