//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

// StatisticsDlg.cpp : implementation file
//

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "StatisticsDlg.h"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cmath>		// Needed for std::exp
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/sizer.h>

#include "muuli_wdr.h"		// Needed for statsDlg
#include "StatisticsDlg.h"	// Interface declarations
#include "GetTickCount.h"	// Needed for GetTickCount
#include "ServerList.h"		// Needed for CServerList
#include "ClientList.h"		// Needed for CClientList
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "otherfunctions.h"	// Needed for GetTickCount
#include "sockets.h"		// Needed for CServerConnect
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "ListenSocket.h"	// Needed for CListenSocket
#include "ColorFrameCtrl.h"	// Needed for CColorFrameCtrl
#include "Preferences.h"	// Needed for CPreferences
#include "OScopeCtrl.h"		// Needed for COScopeCtrl
#include "amuleDlg.h"		// Needed for ShowStatistics

#ifndef __WXMSW__
	#include "netinet/in.h"	// Needed for htonl()
#else
	#include "winsock.h"
#endif

#define ID_EXPORT_HTML 20001
#ifdef __BSD__
	// glibc -> bsd libc
	#define round rint
#else
	#define round(x) floor(x+0.5)
#endif /* __BSD__ */
// CStatisticsDlg dialog

BEGIN_EVENT_TABLE(CStatisticsDlg,wxPanel)
END_EVENT_TABLE()

using namespace otherfunctions;

CStatisticsDlg::CStatisticsDlg(wxWindow* pParent)
: wxPanel(pParent, -1)
{
	wxSizer* content=statsDlg(this,TRUE);
	content->Show(this,TRUE);

	// Resolution of the history list: in each sampling range we want at least one sample
	// per pixel, so we need the max plot width; take the window width in maximized state,
	// divide by 2 (2 graphs side-by-side) and subtract a constant for framing.  Note that
	// this is simply a "reasonable" calculation - if the user drags the aMule window
	// partly off-screen and then resizes it to fill the screen, he will get graph sizes
	// bigger than we assume here.  The graphs will still work then, but he won't get max
	// resolution if the graphs are rebuilt based on the history list.
	nPointsPerRange = wxGetTopLevelParent(this)->GetMaxSize().GetWidth()/2 - 80;

	pscopeDL	= CastChild( wxT("dloadScope"), COScopeCtrl );
	pscopeUL	= CastChild( wxT("uloadScope"), COScopeCtrl );
	pscopeConn	= CastChild( wxT("otherScope"), COScopeCtrl );
	stattree	= CastChild( wxT("statTree"),	wxTreeCtrl  );

	aposRecycle = NULL;
	m_ilastMaxConnReached = 0;
	peakconnections = 0;
	activeconnections = 0;
	maxDown = 0.0;
	maxDownavg = 0;
	kBpsUpCur = 0.0;
	kBpsUpAvg = 0.0;
	kBpsUpSession = 0.0;
	kBpsDownCur = 0.0;
	kBpsDownAvg = 0.0;
	kBpsDownSession = 0.0;
}



CStatisticsDlg::~CStatisticsDlg()
{
	// the destructor for listHR frees the memory occupied by the nodes
	delete[] aposRecycle;
}

void CStatisticsDlg::Init()
{
	InitGraphs();
	InitTree();
}


void CStatisticsDlg::InitGraphs()
{
	HR hr = {0.0, 0.0, 0.0, 0.0, 0.0, 0, 0, 0};
	hrInit = hr;
	nHistRanges = 7;	// =ceil(log(max_update_delay)/log(2))
	bitsHistClockMask = (1 << (nHistRanges-1)) - 1;
	aposRecycle = new POSITION[nHistRanges];
	POSITION *ppos = aposRecycle+nHistRanges-1;
	for (int i=nHistRanges; i>0; --i) {  // permanently allocated history list
		*ppos-- = listHR.AddTail(hr);
		for (int j=nPointsPerRange; j>1; --j)
			listHR.AddTail(hr);
	}

	// called after preferences get initialised
	for (int index=0; index<=10; ++index)
		ApplyStatsColor(index);
	pscopeDL->SetRanges(0.0, (float)(thePrefs::GetMaxGraphDownloadRate()+4));
	pscopeDL->SetYUnits(_("kB/s"));
	pscopeUL->SetRanges(0.0, (float)(thePrefs::GetMaxGraphUploadRate()+4));
	pscopeUL->SetYUnits(_("kB/s"));
	pscopeConn->SetRanges(0.0, (float)(thePrefs::GetStatsMax()));
	pscopeConn->SetYUnits(wxEmptyString);
}


// this array is now used to store the current color settings and to define the defaults
COLORREF CStatisticsDlg::acrStat[cntStatColors] =
	{ 
		RGB(0,0,64),		RGB(192,192,255),	RGB(128, 255, 128),	RGB(0, 210, 0),
		RGB(0, 128, 0),		RGB(255, 128, 128),	RGB(200, 0, 0),		RGB(140, 0, 0),
	  	RGB(150, 150, 255),	RGB(192, 0, 192),	RGB(255, 255, 128),	RGB(0, 0, 0), 
	  	RGB(255, 255, 255)
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

//
// FIXME:
// lfroen: all this code should not be here but in core.
//
#ifndef CLIENT_GUI

/*
History List

  The basic idea here is that we want to keep as much history as we can without paying 
a high price in terms of memory space.  Because we keep the history for display purposes, 
we can take advantage of the fact that when the period shown in the graphs is long 
then each pixel represents a long period.  So as the most recent history we keep one 
window full of points at a resolution of 1 second, the next window full at 2 seconds, 
the next at 4 seconds and so on, up to the maximum desired.  This way there is always 
at least one sample point per pixel for any update delay set by the user, and the 
memory required grows with the *log* of the total time period covered.
  The history is kept in a doubly-linked list, with the most recent snapshot at the tail.  
The number of nodes in the list is fixed, and there are no calls to RemoveHead() and 
AddTail() which would add overhead and contribute to memory fragmentation.  Instead, 
every second when a new point gets recorded, one of the existing nodes is recycled; 
it is disjoined from its present place, put at the tail of the list, and then gets 
filled with new data.   [Emilio Sandoz]
*/

void CStatisticsDlg::RecordHistory()
{	// First we query and compute some values, then we store them in the history list
	
	// A few comments about the use of double and float in computations:
	// Even on a hi-res screen our graphs will have 10 bits of resolution at most,
	// so the 24 bits resolution of a float on 32 bit Intel processors is more than
	// enough for all displayed values.  Rate computations however, and especially 
	// running average computations, use differences (delta bytes/ delta time), and 
	// for long uptimes the difference between two timestamps can lose too much 
	// accuracy because the large mantissa causes less significant bits to be dropped
	// (same for the difference between two cumulative byte counts).  [We don't store 
	// these values as integers because they will be used in floating point calculations,
	// and we want to perform the conversion only once).  Therefore timestamps and 
	// Kbyte counts are stored in the history as doubles, while computed values use
	// float (to save space and execution time).

	double		sCur = theApp.GetUptimeMsecs()/1000.0;
	double		sTrans;
	static double	sPrev;
	float		sAvg = thePrefs::GetStatsAverageMinutes()*60.0;
	double		kBytesRec = theApp.stat_sessionReceivedBytes/1024.0;
	double		kBytesSent = theApp.stat_sessionSentBytes/1024.0;
	static double	kBytesRecPrev, kBytesSentPrev;

	kBpsUpCur = theApp.uploadqueue->GetKBps();
	kBpsDownCur = theApp.downloadqueue->GetKBps();
    if (maxDown < kBpsDownCur)
		maxDown = kBpsDownCur;
	ComputeSessionAvg(kBpsUpSession, kBpsUpCur, kBytesSent, sCur, sTrans);
	ComputeRunningAvg(kBpsUpAvg, kBpsUpSession, kBytesSent, kBytesSentPrev, sTrans, sPrev, sAvg);
	ComputeSessionAvg(kBpsDownSession, kBpsDownCur, kBytesRec, sCur, sTrans);
	ComputeRunningAvg(kBpsDownAvg, kBpsDownSession, kBytesRec, kBytesRecPrev, sTrans, sPrev, sAvg);
	sPrev = sTrans;
	if (maxDownavg < kBpsDownSession)
		maxDownavg = kBpsDownSession;
	activeconnections = theApp.listensocket->GetOpenSockets();
	if(peakconnections < activeconnections)
		peakconnections = activeconnections;
/*
	Store values; first determine the node to be recycled (using the bits in iClock)
	
    oldest records ----------------- listHR ------------------ youngest records
	
	O-[Range 2^n sec]-O- ... -O-[Range 4 sec]-O-[Range 2 sec]-O-[Range 1 sec]-O
	|                 |       |               |               > every 2 secs -^
	|                 |  ...  |                >--------------- every 4 secs -^
	|                 |       >------------------------ recycle every 8 secs -^
	|                 |                                                 ...
	|                 >-the node at this position is recycled every 2^n secs -^	
	>-------------------(ditto for the oldest node at the head of the list) --^	
	^                                                         ^
    aposRecycle[nHistRanges-1]               ...              aposRecycle[0]  Tail
*/
	POSITION	*ppos;
	static int	iClock;
	int		iClockPrev = iClock++;
	int		bits = (iClockPrev^iClock) & iClock;  // identify the highest changed bit
	if (bits <= bitsHistClockMask) {
		ppos = aposRecycle;
		while ((bits /= 2) != 0)  // count to the highest bit that was just toggled to 1
			++ppos;	
		// recycle one node and jump over the next to move it to the next higher range
		*ppos = listHR.NextAt(listHR.RecycleNodeAsTail(*ppos));
	} else {
		ppos = aposRecycle+nHistRanges-1;
		// recycle the node at the head; there is no higher range to move nodes into
		*ppos = listHR.RecycleNodeAsTail(*ppos);
	}
	
	// now save the latest data point in this node
 	HR* phr = &listHR.GetTail();
	phr->kBytesSent = kBytesSent;
	phr->kBytesReceived = kBytesRec;
	phr->kBpsUpCur = kBpsUpCur;
	phr->kBpsDownCur = kBpsDownCur;
	phr->cntUploads = theApp.uploadqueue->GetUploadQueueLength();
	phr->cntConnections = activeconnections;
	phr->sTimestamp = sCur;
	uint32 iStats[6];
	theApp.downloadqueue->GetDownloadStats(iStats);
	phr->cntDownloads = iStats[1];
#ifdef __DEBUG__
//	if (bits > bitsHistClockMask)  // every 64 seconds - 
//		VerifyHistory();  // must do this AFTER phr->sTimestamp has been set
#endif
}

#endif // CLIENT_GUI

unsigned CStatisticsDlg::GetHistory(  // Assemble arrays of sample points for a graph
	unsigned cntPoints,		// number of sample points to assemble
	double sStep,			// time difference between sample points
	double sFinal,			// latest allowed timestamp
	float** ppf,			// an array of pointers to arrays of floats for the result
	COScopeCtrl* pscope)	// the graph which will receive the points
{	
	if (sStep==0.0 || cntPoints==0)
		return(0);
	float		*pf1 = *ppf;
	float		*pf2 = *(ppf+1);
	float		*pf3 = *(ppf+2);
	unsigned	cntFilled = 0;
	POSITION	pos = listHR.GetTailPosition(), posPrev;
	HR			*phr = &listHR.GetAt(pos);  	// pointer to history record

	// start of list should be an integer multiple of the sampling period for samples 
	// to be consistent when the graphs are resized horizontally
	double	sTarget;
	if (sFinal >= 0.0)
		sTarget = sFinal;
	else
		sTarget = (sStep==1.0 ? phr->sTimestamp : std::floor(phr->sTimestamp/sStep) * sStep); 

	HR		**ahr = NULL, **pphr = NULL;
	bool	bRateGraph = (pscope!=pscopeConn);	// rate graph or connections graph?
	if (bRateGraph) {
		ahr = new HR* [cntPoints];
		pphr = ahr;
	}
	
	do {
		while ((posPrev=listHR.PrevAt(pos)) != NULL  // find next history record
			&& ((phr=&listHR.GetAt(posPrev))->sTimestamp > sTarget))
			pos = posPrev;
		if (bRateGraph) {	// assemble an array of pointers for ComputeAverages
			*pphr++ = phr;
		} else {			// or build the arrays if possible
			*pf1++ = (float)phr->cntUploads;
			*pf2++ = (float)phr->cntConnections;
			*pf3++ = (float)phr->cntDownloads;
		}
		if (++cntFilled  == cntPoints)		// enough points 
			break;
		if (phr->sTimestamp == 0.0)			// reached beginning of uptime
			break;
		if ((sTarget -= sStep) <= 0.0) {	// don't overshoot the beginning
			if (bRateGraph)
				*pphr++ = &hrInit;
			else
				*pf1++ = *pf2++ = *pf3++ = 0.0;
			++cntFilled;
			break;
		}
	} while (posPrev != NULL);

	if (!bRateGraph)
		return cntFilled;
	else if  (cntFilled > 0)
		ComputeAverages(pphr, pos, cntFilled, sStep, ppf, pscope);
	delete[] ahr;
	return cntFilled;
}


unsigned CStatisticsDlg::GetHistoryForWeb(  // Assemble arrays of sample points for the webserver
	unsigned cntPoints,		// maximum number of sample points to assemble
	double sStep,			// time difference between sample points
	double *sStart,			// earliest allowed timestamp
	uint32 **graphData)		// a pointer to a pointer that will point to the graph data array
{	
	if (*sStart < 0.0) {
		*sStart = 0.0;
	}
	if (sStep==0.0 || cntPoints==0)
		return(0);
	unsigned	cntFilled = 0;
	POSITION	pos = listHR.GetTailPosition(), posPrev;
	HR		*phr = &listHR.GetAt(pos);  	// pointer to history record
	double		LastTimeStamp = phr->sTimestamp;
	double		sTarget = LastTimeStamp;
	
	HR	**pphr = new HR *[cntPoints];

	do {
		while ((posPrev=listHR.PrevAt(pos)) != NULL	// find next history record
			&& ((phr=&listHR.GetAt(posPrev))->sTimestamp > sTarget))
			pos = posPrev;
		pphr[cntFilled] = phr;
		if (++cntFilled  == cntPoints)		// enough points 
			break;
		if (phr->sTimestamp <= *sStart)			// reached beginning of requested time
			break;
		if ((sTarget -= sStep) <= 0.0) {	// don't overshoot the beginning
			pphr[cntFilled++] = NULL;
			break;
		}
	} while (posPrev != NULL);

	if (cntFilled) {
		*graphData = new uint32 [3 * cntFilled];
		if (*graphData) {
			for (unsigned int i = 0; i < cntFilled; i++) {
				phr = pphr[cntFilled - i - 1];
				if (phr) {
					(*graphData)[3 * i    ] = htonl((uint32)(phr->kBpsDownCur * 1024.0));
					(*graphData)[3 * i + 1] = htonl((uint32)(phr->kBpsUpCur * 1024.0));
					(*graphData)[3 * i + 2] = htonl((uint32)phr->cntConnections);
				} else {
					(*graphData)[3 * i] = (*graphData)[3 * i + 1] = (*graphData)[3 * i + 2] = 0;
				}
			}
		}
	} else {
		*graphData = NULL;
	}

	delete [] pphr;

	*sStart = LastTimeStamp;

	return cntFilled;
}


void CStatisticsDlg::ComputeAverages(
	HR			**pphr,			// pointer to (end of) array of assembled history records
	POSITION	pos,			// position in history list from which to backtrack
	unsigned	cntFilled,		// number of points in the sample data
	double		sStep,			// time difference between two samples
	float		**ppf,			// an array of pointers to arrays of floats with sample data
	COScopeCtrl	*pscope)		// the graph which will receive the points
{	
	double		sCur = 0.0, sPrev, sTarget, sTrans, kBytesPrev, kBytesRun;
	float		kBpsAvg;
	float 		sAvg = (float)thePrefs::GetStatsAverageMinutes()*60.0;
	POSITION	posPrev = listHR.PrevAt(pos);
	HR			*phr = &listHR.GetAt(pos);
		
	kBpsAvg = 0.0;
	sCur = phr->sTimestamp;
	sPrev = max(0.0, sCur-sStep);
	kBytesPrev = 0.0;
	// running average: backtrack in history far enough so that the residual error 
	// of the running avg computation will be <1/2 pixel for the first sample point
	if (posPrev != NULL  &&  sCur > 0.0) {
		// how long would the low-pass averaging filter need to decay the max to half a pixel
		sTarget = max(0.0, sCur - sAvg*std::log((float)(pscope->GetPlotHeightPixels()*2)));
		for (POSITION *ppos=aposRecycle; ppos<aposRecycle+nHistRanges; ++ppos) {
			// accelerate search by using our intermediate pointers into the history list
			if (listHR.GetAt(*ppos).sTimestamp >= sTarget)
				pos = *ppos;
			else
				break;
		}
		while ((posPrev=listHR.PrevAt(pos)) != NULL) {
			if (listHR.GetAt(pos=posPrev).sTimestamp <= sTarget)
				break;
		}
		// backtracked enough, now compute running average up to first sample point
		phr = &listHR.GetAt(pos);
		sPrev = phr->sTimestamp;
		kBytesPrev = (pscope==pscopeDL ? phr->kBytesReceived : phr->kBytesSent);
		do  {
			float kBpsSession;
			pos = listHR.NextAt(pos);
			phr = &listHR.GetAt(pos);
			kBytesRun = (pscope==pscopeDL ? phr->kBytesReceived : phr->kBytesSent);
			ComputeSessionAvg(kBpsSession, (pscope==pscopeDL ? phr->kBpsDownCur : phr->kBpsUpCur), 
								kBytesRun, phr->sTimestamp, sTrans);
			ComputeRunningAvg(kBpsAvg, kBpsSession, kBytesRun, kBytesPrev, sTrans, sPrev, sAvg);
			sPrev = sTrans;
		} while (phr->sTimestamp < sCur);
	}

	// now compute averages in returned arrays, starting with the earliest values
	float	*pf1 = *ppf++ + cntFilled -1;  // holds session avg
	float	*pf2 = *ppf++ + cntFilled -1;  // holds running avg
	float	*pf3 = *ppf + cntFilled -1;    // holds current rate

	for (int cnt=cntFilled; cnt>0; cnt--, pf1--, pf2--, pf3--) {
		phr = *(--pphr);
		if (pscope == pscopeDL) {
			kBytesRun = phr->kBytesReceived;
			*pf3 = phr->kBpsDownCur;
		} else {
			kBytesRun = phr->kBytesSent;
			*pf3 = phr->kBpsUpCur;
		}
		ComputeSessionAvg(*pf1, *pf3, kBytesRun, phr->sTimestamp, sTrans);
		ComputeRunningAvg(kBpsAvg, *pf1, kBytesRun, kBytesPrev, sTrans, sPrev, sAvg);
		*pf2 = kBpsAvg;
		sPrev = sTrans;
	}
	// make sure the "running average" line will continue smoothly the next time a point
	// is appended, especially if the averaging time constant has been changed
	(pscope==pscopeDL ? kBpsDownAvg : kBpsUpAvg) = kBpsAvg;
}


void CStatisticsDlg::VerifyHistory(bool bMsgIfOk)
// Debugging only: This performs a basic sanity check on the history list: 
// link integrity, correct number of nodes, sequentiality of timestamps, 
// approximate gap length between sample points.
// See graph in RecordHistory() for the mechanics of the list.
// Un-comment the call at the end of RecordHistory() to activate these checks.
// Note: if the system is very busy we sometimes get a long gap followed by a short one,
// and they will be flagged as they move through the list.
{
#ifdef __DEBUG__
	int	cnt, cntRanges=1, cntInRange=0, cntExpected=nHistRanges*nPointsPerRange;
	double sStart = (double)(theApp.GetUptimeSecs()) + 0.01;
	double sStep=1.0, sPrev=sStart, sCur = 0.0;
	POSITION *ppos = aposRecycle;
	POSITION posPrev = NULL;
	POSITION posCur = listHR.GetTailPosition();
	
	for (cnt=1; cnt<=cntExpected; ++cnt) {
		++cntInRange;
		if (posCur==NULL) {
			printf("History list too short: %i elements (%i expected), ends at t=%.2f\n", cnt-1,cntExpected, sPrev);
			return;
		}
		sCur = listHR.GetAt(posCur).sTimestamp;
		if (!(sCur >= 0.0)) {
			printf("History list has INVALID timestamp at position %i (t1=%.2f)\n", cnt, sCur);
			return;
		}
		if (sCur > sPrev) {
			printf("History list is non-sequential at position %i (t1=%.2f t0=%.2f dT=%.2f)\n", cnt, sCur, sPrev, sCur-sPrev);
			return;
		}
		if (sCur==sPrev) {
			if (sCur>0.0) {
				printf("History list has duplicate timestamp at position %i (t1=%.2f)\n", cnt, sCur);
				return;
			}
		} else if (sStep>1.0 && cntInRange>1) {
			float sDelta = std::abs(sPrev-sCur-sStep);
			if (sCur != 0.0  &&  sDelta > sStep*0.5)
				printf("T=%i History list: gap of %.2f (vs. %i) #%i is %i in range  [%i]t=%.2f [%i]t=%.2f [%i]t=%.2f\n", 
							(int)round(sStart), sPrev-sCur, (int)round(sStep), cnt, cntInRange,
									 cnt+1, listHR.GetAt(listHR.PrevAt(posCur)).sTimestamp, cnt, sCur, cnt-1, sPrev);
		}
		if (listHR.NextAt(posCur) != posPrev) {
			printf("History list has bad backlink at position %i (t=%.2f seconds)\n", cnt, sCur);
			return;
		}
		if (cntInRange > nPointsPerRange+1) {
			printf("History list: range %i is too long (>=%i points, expected %i +/-1)\n", cntRanges, cntInRange, nPointsPerRange);
		}
		if (posCur == *ppos) {
			++ppos;
			++cntRanges;
			if (cntInRange < nPointsPerRange-1) {
				printf("History list: range %i is too short (<>=%i points, expected %i +/-1)\n", cntRanges, cntInRange, nPointsPerRange);
			}
			cntInRange = 0;
			sStep *= 2.0;
		}
		posPrev = posCur;
		posCur = listHR.PrevAt(posCur);
		sPrev = sCur;
	}
	
	if (posCur != NULL) {
		printf("History list: head->prev!=NULL (t=%.2f)\n", sCur);
		return;
	}
	if (bMsgIfOk)
		printf("History list is OK (t=%f)\n", sStart);
#endif
}


void CStatisticsDlg::UpdateStatGraphs(bool bStatsVisible)
{
	if (!bStatsVisible) {
		pscopeDL->DelayPoints();
		pscopeUL->DelayPoints();
		pscopeConn->DelayPoints();
	}

	static unsigned nScalePrev=1;
	unsigned nScale = (unsigned)std::ceil((float)peakconnections / pscopeConn->GetUpperLimit());
	if (nScale != nScalePrev) {
		nScalePrev = nScale;
		wxString txt = wxString::Format(_("Active connections (1:%u)"), nScale);
		CastChild( ID_ACTIVEC, wxStaticText )->SetLabel(txt);
		pscopeConn->SetRange(0.0, (float)nScale*pscopeConn->GetUpperLimit(), 1);
	}
	if (!bStatsVisible)
		return;
	
	HR* phr = &listHR.GetTail();
	float cUp, cDown, cConn;
	cUp   = (float)phr->cntUploads;
	cDown = (float)phr->cntConnections;
	cConn = (float)phr->cntDownloads;
	float *apfDown[] = { &kBpsDownSession, &kBpsDownAvg, &kBpsDownCur };
	float *apfUp[] = { &kBpsUpSession, &kBpsUpAvg, &kBpsUpCur };
	float *apfConn[] = { &cUp, &cDown, &cConn };
	pscopeDL->AppendPoints(phr->sTimestamp, apfDown);
	pscopeUL->AppendPoints(phr->sTimestamp, apfUp);
	pscopeConn->AppendPoints(phr->sTimestamp, apfConn);
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

void  CStatisticsDlg::InitTree()
{
	
	wxTreeItemId root=stattree->AddRoot(_("Statistics"));

	wxASSERT((++theApp.statstree.begin()).begin() != (++theApp.statstree.begin()).end());
	
	ShowStatistics();
	
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
	
}

void CStatisticsDlg::ShowStatistics()
{

	StatsTreeSiblingIterator sib = theApp.statstree.begin().begin();
	wxTreeItemId root = stattree->GetRootItem();
	
	FillTree(sib,root);
	
}


wxString CStatisticsDlg::IterateChilds(wxTreeItemId hChild, int level) {
	
	wxString strBuffer;
	
	wxTreeItemId hChild2;
	wxTreeItemIdValue cookie;
	hChild2 = stattree->GetFirstChild(hChild,cookie);
	
	while(hChild2.IsOk()) {
		
		// Add this item
		strBuffer+= wxT("<br>");
		for (int ix=0; ix < level; ++ix) {
			strBuffer += wxT("&nbsp;&nbsp;&nbsp;");
		}		
		strBuffer += stattree->GetItemText(hChild2);
		
		// Add subchilds
		
		strBuffer+= IterateChilds(hChild2,level+1);
		
		// Next on this level
		hChild2 = stattree->GetNextSibling(hChild2);
	}	
	
	return strBuffer;
}

// FIXME: 
// lfroen: must be reworked without taking data from gui controls
#ifndef CLIENT_GUI

// This is the primary function for generating HTML output of the statistics tree.
wxString CStatisticsDlg::GetHTML() {
	
	wxString strBuffer;
	wxTreeItemId item;

	strBuffer.Printf(wxT("<font face=\"Verdana,Courier New,Helvetica\" size=\"2\">\r\n<b>aMule v%s %s [%s]</b>\r\n<br><br>\r\n"), wxT(PACKAGE_VERSION), _("Statistics"), thePrefs::GetUserNick().c_str());
	// update it
	ShowStatistics();
	stattree->GetFirstVisibleItem();
	item=stattree->GetRootItem();
	strBuffer+=stattree->GetItemText(item);
	
	strBuffer += IterateChilds(item, 0);
			
 	strBuffer+=wxT("</font>");

	// return the string
	
	int filecount = theApp.downloadqueue->GetFileCount();
	uint32 stats[2]; // get the source count
	theApp.downloadqueue->GetDownloadStats(stats);
	strBuffer += 
		wxString::Format(wxT(
		"\tStatistics: \n"
		"\t\tDownloading files: %d\n"
		"\t\tFound sources: %d\n"
		"\t\tActive downloads: %d\n"
		"\t\tActive Uploads: %d\n"
		"\t\tUsers on upload queue: %d"),
		filecount, stats[0], stats[1], 
		theApp.uploadqueue->GetUploadQueueLength(), 
		theApp.uploadqueue->GetWaitingUserCount());
	
	return(strBuffer);
}

#endif // CLIENT_GUI

void CStatisticsDlg::ComputeSessionAvg(float& kBpsSession, float& kBpsCur, double& kBytesTrans, double& sCur, double& sTrans)
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

void CStatisticsDlg::ComputeRunningAvg(float& kBpsRunning, float& kBpsSession, double& kBytesTrans, 
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


void CStatisticsDlg::SetARange(bool SetDownload,int maxValue)
{
	if ( SetDownload ) {
		pscopeDL->SetRanges( 0, maxValue + 4 );
	} else {
		pscopeUL->SetRanges( 0, maxValue + 4 );
	}
}

void CStatisticsDlg::FillTree(StatsTreeSiblingIterator& statssubtree, wxTreeItemId& StatsGUITree) {
	
	StatsTreeSiblingIterator temp_it = statssubtree.begin();
	
	wxTreeItemIdValue cookie;
	wxTreeItemId temp_GUI_it = stattree->GetFirstChild(StatsGUITree,cookie);
	
	while (temp_it != statssubtree.end()) {
		wxTreeItemId temp_item;
		if (temp_GUI_it.IsOk()) {
			// There's already a child there, update it.
			stattree->SetItemText(temp_GUI_it, (*temp_it));
			temp_item = temp_GUI_it;
			temp_GUI_it = stattree->GetNextSibling(temp_GUI_it);
		} else {
			// No more child on GUI, add them.
			temp_item = stattree->AppendItem(StatsGUITree,(*temp_it));
		}
		// Has childs?
		if (temp_it.begin() != temp_it.end()) {
			FillTree(temp_it, temp_item);
		}
		++temp_it;
	}	
	
	// What if GUI has more items than tree?
	// This can only happen on the dynamic trees, i.e. client versions. 
	// Delete the extra items.
	// On a second thought, it CAN'T happen - dynamic trees only add items.
	// I'll this around just because we might need it later.
	while (temp_GUI_it.IsOk()) {
		wxTreeItemId backup_node = stattree->GetNextSibling(temp_GUI_it);
		stattree->Delete(temp_GUI_it);
		temp_GUI_it = backup_node;
	}
}
