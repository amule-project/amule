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
#include "resource.h"		// Needed for IDD_STATISTICS
#include "OScopeCtrl.h"		// Needed for graphs
#include "CTypedPtrList.h"	// Needed for CList
#include "amule.h"			// Needed for theApp
#include "Preferences.h"	// Needed for cntStatColors

class COScopeCtrl;

// CStatisticsDlg dialog

using namespace std;
#define HTREEITEM wxTreeItemId


class CStatisticsDlg : public wxPanel// CResizableDialog
{
	friend class PrefsUnifiedDlg;

public:
	CStatisticsDlg(wxWindow* pParent = NULL);   // standard constructor
	~CStatisticsDlg();
	enum { IDD = IDD_STATISTICS };

	void Localize();
	void UpdateStatGraphs(bool bStatsVisible);
	void SetUpdatePeriod();
	void ResetAveragingTime();
	void ShowStatistics();
	void SetARange(bool SetDownload,int maxValue) {(SetDownload ? pscopeDL : pscopeUL)->SetRanges(0, maxValue+4);}
	float GetKBpsUpCurrent()		{return kBpsUpCur;}
	float GetKBpsUpRunningAvg()		{return kBpsUpAvg;}
	float GetKBpsUpSession()		{return kBpsUpSession;}
	float GetKBpsDownCurrent()		{return kBpsDownCur;}
	float GetKBpsDownRunningAvg()	{return kBpsDownAvg;}
	float GetKBpsDownSession()		{return kBpsDownSession;}
	void RecordHistory();
	unsigned GetHistory(unsigned cntPoints, double sStep, double sFinal, float **ppf, COScopeCtrl* pscope);
	void VerifyHistory(bool bMsgIfOk = false);
	float GetMaxConperFiveModifier();
	void Init();
	void InitTree();
	void InitGraphs();
	void ApplyStatsColor(int index);
	COLORREF GetTrayBarColor()		{return acrStat[11];}

	// export can't be done directly in the webserver's thread
	// so it is splitted in two parts	
	wxString ExportHTML();
	wxString GetHTML();
protected:
	static COLORREF	acrStat[cntStatColors];
	
 private:
	void ExportHTMLEvent(wxCommandEvent& evt);
	wxString exportString;
	wxMutex exportMutex;
	wxCondition* exportCondition;
	wxCriticalSection exportCrit;
 
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

	// tree-folders
	HTREEITEM h_shared,h_transfer,h_connection,h_clients,h_servers,h_upload,h_download,h_uptime;
	HTREEITEM down1,down2,down3,down4,down5,down6,down7;
	HTREEITEM up1,up2,up3,up4,up5,up6,up7,up8,up9,up10;
	HTREEITEM tran0;
	HTREEITEM con1,con2,con3,con4,con5,con6,con7,con8,con9,con10,con11,con12,con13;
	HTREEITEM shar1,shar2,shar3;
	HTREEITEM cli1,cli2,cli3,cli4,cli5,cli6,cli7,cli8,cli9,cli10, cli11, cli12;
	HTREEITEM cli_versions[12];
	HTREEITEM srv1,srv2,srv3,srv4,srv5,srv6,srv7,srv8,srv9;

	void SetStatsRanges(int min, int max);
 public:
	void	UpdateConnectionsStatus();

    uint32 peakconnections;
	uint32 totalconnectionchecks;
	float averageconnections;
	uint32 activeconnections;
private:
	void ComputeAverages(HR **pphr, POSITION pos, unsigned cntFilled, double sStep, float **ppf, COScopeCtrl	*pscope);

	// ComputeSessionAvg and ComputeRunningAvg are used to assure consistent computations across
	// RecordHistory and ComputeAverages; see note in RecordHistory on the use of double and float 
	void ComputeSessionAvg(float& kBpsSession, float& kBpsCur, double& kBytesTrans, double& sCur, double& sTrans)
	{
		if (theApp.sTransferDelay == 0.0  ||  sCur <= theApp.sTransferDelay) {
			sTrans = 0.0;
			kBpsSession = 0.0;
		} else {
			sTrans = sCur - theApp.sTransferDelay;
			kBpsSession = kBytesTrans / sTrans;
			if (sTrans < 10.0  &&  kBpsSession > kBpsCur) 
				kBpsSession = kBpsCur;	// avoid spiking of the first few values due to small sTrans
		}
	}
	
	
	void ComputeRunningAvg(float& kBpsRunning, float& kBpsSession, double& kBytesTrans, 
							double& kBytesTransPrev, double& sTrans, double& sPrev, float& sAvg)
	{
		float sPeriod;
		if ((float)sTrans < sAvg) {		// startup: just track session average
			kBpsRunning = kBpsSession;
			kBytesTransPrev = kBytesTrans;
		} else if ((sPeriod=(float)(sTrans-sPrev)) > 0.0) { // then use a first-order low-pass filter
			float lambda = std::exp(-sPeriod/sAvg);		
			kBpsRunning = kBpsRunning*lambda + (1.0-lambda)*(float)(kBytesTrans-kBytesTransPrev)/sPeriod;
			kBytesTransPrev = kBytesTrans;
		}							// if sPeriod is zero then leave the average unchanged
	}
	
protected:
	DECLARE_EVENT_TABLE()

};

#endif // STATISTICSDLG_H
