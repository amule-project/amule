//
// This file is part of the aMule Project.
//
// Copyright (c) 2003 aMule Team ( http://www.amule-project.net )
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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "Statistics.h"
#endif

#include "Statistics.h"
#include "ClientList.h"
#include "Preferences.h"
#include "amule.h"
#include "UploadQueue.h"
#include "DownloadQueue.h"
#include "OtherFunctions.h"
#include "ListenSocket.h"
#include "ServerConnect.h"
#include "SharedFileList.h"
#include "ServerList.h"

#include <cmath>

#include <wx/debug.h>

#ifdef __BSD__
	// glibc -> bsd libc
	#define round rint
#else
	#define round(x) floor(x+0.5)
#endif /* __BSD__ */

CStatistics::CStatistics() {

	Start_time = GetTickCount64();
	
	// Init graphs
	
	aposRecycle = NULL;
	maxDown = 0.0;
	maxDownavg = 0;
	kBpsUpCur = 0.0;
	kBpsUpAvg = 0.0;
	kBpsUpSession = 0.0;
	kBpsDownCur = 0.0;
	kBpsDownAvg = 0.0;
	kBpsDownSession = 0.0;
	HR hr = {0.0, 0.0, 0.0, 0.0, 0.0, 0, 0, 0};
	hrInit = hr;
	nHistRanges = 7;	// =ceil(log(max_update_delay)/log(2))
	nPointsPerRange = GetPointsPerRange(); 
	bitsHistClockMask = (1 << (nHistRanges-1)) - 1;
	aposRecycle = new POSITION[nHistRanges];
	POSITION *ppos = aposRecycle+nHistRanges-1;
	for (int i=nHistRanges; i>0; --i) {  // permanently allocated history list
		*ppos-- = listHR.AddTail(hr);
		for (int j=nPointsPerRange; j>1; --j)
			listHR.AddTail(hr);
	}	
	
	// Init Tree
	
	stat_sessionReceivedBytes = 0;
	stat_sessionSentBytes = 0;
	stat_reconnects = 0;
	stat_transferStarttime = 0;
	stat_serverConnectTime = 0;
	stat_filteredclients = 0;
	sTransferDelay = 0.0;
	m_ilastMaxConnReached = 0;
	
	InitStatsTree();	


	// Download-values
	m_nDownDataRateMSOverhead = 0;
	m_nDownDatarateTotal = 0;
	m_nDownDatarateOverhead = 0;
	m_nDownDataOverheadSourceExchange = 0;
	m_nDownDataOverheadFileRequest = 0;
	m_nDownDataOverheadOther = 0;
	m_nDownDataOverheadServer = 0;
	m_nDownDataOverheadSourceExchangePackets = 0;
	m_nDownDataOverheadFileRequestPackets = 0;
	m_nDownDataOverheadOtherPackets = 0;
	m_nDownDataOverheadServerPackets = 0;

	downloaded_aMule = 0;
	downloaded_eMule = 0;
	downloaded_eDonkey = 0;
	downloaded_eDonkeyHybrid = 0;
	downloaded_Shareaza = 0;
	downloaded_MLDonkey = 0;
	downloaded_lxMule = 0;
	downloaded_Other = 0;

	// Upload-values
	m_nUpDatarateTotal = 0;
	m_nUpDataRateMSOverhead = 0;
	m_nUpDatarateOverhead = 0;
	m_nUpDataOverheadSourceExchange = 0;
	m_nUpDataOverheadFileRequest = 0;
	m_nUpDataOverheadOther = 0;
	m_nUpDataOverheadServer = 0;
	m_nUpDataOverheadSourceExchangePackets = 0;
	m_nUpDataOverheadFileRequestPackets = 0;
	m_nUpDataOverheadOtherPackets = 0;
	m_nUpDataOverheadServerPackets = 0;
}


CStatistics::~CStatistics()
{
	
	// the destructor for listHR frees the memory occupied by the nodes
	delete[] aposRecycle;	
	
}


/* ------------------------------ OVERHEAD ---------------------------- */

void CStatistics::CompDownDatarateOverhead()
{
	// Adding the new overhead
	m_nDownDatarateTotal += m_nDownDataRateMSOverhead * 10;
	m_AverageDDRO_list.push_back( m_nDownDataRateMSOverhead * 10 );
	
	// Reset the overhead count
	m_nDownDataRateMSOverhead = 0;
	
	// We want at least 11 elements before we will start doing averages
	if ( m_AverageDDRO_list.size() > 10 ) {
		
		// We want 50 elements at most (~5s)
		if ( m_AverageDDRO_list.size() > 50 ) {
			m_nDownDatarateTotal -= m_AverageDDRO_list.front();
		
			m_AverageDDRO_list.pop_front();
		
			m_nDownDatarateOverhead = m_nDownDatarateTotal / 50.0f;
		} else {
			m_nDownDatarateOverhead = m_nDownDatarateTotal / (double)m_AverageDDRO_list.size();
		}
	} else {
		m_nDownDatarateOverhead = 0;
	}
}


void CStatistics::CompUpDatarateOverhead()
{
	// Adding the new overhead
	m_nUpDatarateTotal += m_nUpDataRateMSOverhead * 10;
	m_AverageUDRO_list.push_back( m_nUpDataRateMSOverhead * 10 );
	
	// Reset the overhead count
	m_nUpDataRateMSOverhead = 0;
	
	// We want at least 11 elements before we will start doing averages
	if ( m_AverageUDRO_list.size() > 10 ) {
		
		// We want 50 elements at most (5s)
		if ( m_AverageUDRO_list.size() > 50 ) {
			m_nUpDatarateTotal -= m_AverageUDRO_list.front();
		
			m_AverageUDRO_list.pop_front();
		
			m_nUpDatarateOverhead = m_nUpDatarateTotal / 50.0f;
		} else {
			m_nUpDatarateOverhead = m_nUpDatarateTotal / (double)m_AverageUDRO_list.size();
		}
	} else {
		m_nUpDatarateOverhead = 0;
	}
}



#ifndef CLIENT_GUI

/* ------------------------------- GRAPHS ---------------------------- */

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

void CStatistics::RecordHistory()
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

	double		sCur = GetUptimeMsecs()/1000.0;
	double		sTrans;
	static double	sPrev;
	float		sAvg = thePrefs::GetStatsAverageMinutes()*60.0;
	double		kBytesRec = stat_sessionReceivedBytes/1024.0;
	double		kBytesSent = stat_sessionSentBytes/1024.0;
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
	phr->cntConnections = theApp.listensocket->GetOpenSockets();
	phr->sTimestamp = sCur;
	uint32 iStats[6];
	theApp.downloadqueue->GetDownloadStats(iStats);
	phr->cntDownloads = iStats[1];
#ifdef __DEBUG__
//	if (bits > bitsHistClockMask)  // every 64 seconds - 
//		VerifyHistory();  // must do this AFTER phr->sTimestamp has been set
#endif
}

#endif

void CStatistics::ComputeSessionAvg(float& kBpsSession, float& kBpsCur, double& kBytesTrans, double& sCur, double& sTrans)
{
	if (sTransferDelay == 0.0  ||  sCur <= sTransferDelay) {
		sTrans = 0.0;
		kBpsSession = 0.0;
	} else {
		sTrans = sCur - sTransferDelay;
		kBpsSession = kBytesTrans / sTrans;
		if (sTrans < 10.0  &&  kBpsSession > kBpsCur) 
			kBpsSession = kBpsCur;	// avoid spiking of the first few values due to small sTrans
	}
}


void CStatistics::ComputeRunningAvg(float& kBpsRunning, float& kBpsSession, double& kBytesTrans, 
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



unsigned CStatistics::GetHistory(  // Assemble arrays of sample points for a graph
	unsigned cntPoints,		// number of sample points to assemble
	double sStep,			// time difference between sample points
	double sFinal,			// latest allowed timestamp
	float** ppf,			// an array of pointers to arrays of floats for the result
	StatsGraphType which_graph)	// the graph which will receive the points
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
	bool	bRateGraph = (which_graph != GRAPH_CONN);	// rate graph or connections graph?
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
		ComputeAverages(pphr, pos, cntFilled, sStep, ppf, which_graph);
	delete[] ahr;
	return cntFilled;
}


unsigned CStatistics::GetHistoryForWeb(  // Assemble arrays of sample points for the webserver
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
					(*graphData)[3 * i    ] = ENDIAN_HTONL((uint32)(phr->kBpsDownCur * 1024.0));
					(*graphData)[3 * i + 1] = ENDIAN_HTONL((uint32)(phr->kBpsUpCur * 1024.0));
					(*graphData)[3 * i + 2] = ENDIAN_HTONL((uint32)phr->cntConnections);
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


void CStatistics::ComputeAverages(
	HR			**pphr,			// pointer to (end of) array of assembled history records
	POSITION	pos,			// position in history list from which to backtrack
	unsigned	cntFilled,		// number of points in the sample data
	double		sStep,			// time difference between two samples
	float		**ppf,			// an array of pointers to arrays of floats with sample data
	 StatsGraphType which_graph)		// the graph which will receive the points
{	
	double		sCur = 0.0, sPrev, sTarget, sTrans, kBytesPrev, kBytesRun;
	float		kBpsAvg;
	float 		sAvg = (float)thePrefs::GetStatsAverageMinutes()*60.0;
	POSITION	posPrev = listHR.PrevAt(pos);
	HR			*phr = &listHR.GetAt(pos);
		
	kBpsAvg = 0.0;
	sCur = phr->sTimestamp;
	sPrev = std::max(0.0, sCur-sStep);
	kBytesPrev = 0.0;
	// running average: backtrack in history far enough so that the residual error 
	// of the running avg computation will be <1/2 pixel for the first sample point
	if (posPrev != NULL  &&  sCur > 0.0) {
		// how long would the low-pass averaging filter need to decay the max to half a pixel
		//sTarget = max(0.0, sCur - sAvg*log((float)(/*pscope->GetPlotHeightPixels()*/*2)));
		sTarget = std::max(0.0, sCur - sAvg*std::log((float)((thePrefs::GetMaxGraphDownloadRate()+4)*2)));
		
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
		kBytesPrev = (which_graph==GRAPH_DOWN ? phr->kBytesReceived : phr->kBytesSent);
		do  {
			float kBpsSession;
			pos = listHR.NextAt(pos);
			phr = &listHR.GetAt(pos);
			kBytesRun = (which_graph==GRAPH_DOWN ? phr->kBytesReceived : phr->kBytesSent);
			ComputeSessionAvg(kBpsSession, (which_graph==GRAPH_DOWN ? phr->kBpsDownCur : phr->kBpsUpCur), 
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
		if (which_graph==GRAPH_DOWN) {
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
	(which_graph==GRAPH_DOWN ? kBpsDownAvg : kBpsUpAvg) = kBpsAvg;
}


GraphUpdateInfo CStatistics::GetPointsForUpdate() {

	GraphUpdateInfo update;
	HR* phr = &listHR.GetTail();	
	update.timestamp = (double) phr->sTimestamp;
	
	update.downloads[0] = kBpsDownSession;
	update.downloads[1] = kBpsDownAvg;
	update.downloads[2] = kBpsDownCur;

	update.uploads[0] = kBpsUpSession;
	update.uploads[1] = kBpsUpAvg;
	update.uploads[2] = kBpsUpCur;

	update.connections[0] = (float)phr->cntUploads; 
	update.connections[1] = (float)phr->cntConnections;
	update.connections[2] = (float)phr->cntDownloads;	

	return update;
}

void CStatistics::VerifyHistory(bool bMsgIfOk)
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
	double sStart = (double)(GetUptimeSecs()) + 0.01;
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

/* ------------------------------- TREE ---------------------------- */

void CStatistics::InitStatsTree() {
	
	StatsTreeNode root = statstree.append_child(statstree.begin(),_("Statistics"));

	h_uptime = statstree.append_child(root,_("Waiting..."));
	h_transfer = statstree.append_child(root,_("Transfer"));
	tran0= statstree.append_child(h_transfer,_("Waiting..."));

	h_upload = statstree.append_child(h_transfer,_("Uploads"));
	up1= statstree.append_child(h_upload,_("Waiting..."));
	up2= statstree.append_child(h_upload,_("Waiting..."));
	up3= statstree.append_child(h_upload,_("Waiting..."));
	up4= statstree.append_child(h_upload,_("Waiting..."));
	up5= statstree.append_child(h_upload,_("Waiting..."));
	up6= statstree.append_child(h_upload,_("Waiting..."));
	up7= statstree.append_child(h_upload,_("Waiting..."));
	up8= statstree.append_child(h_upload,_("Waiting..."));
	up9= statstree.append_child(h_upload,_("Waiting..."));
	up10= statstree.append_child(h_upload,_("Waiting..."));

	h_download = statstree.append_child(h_transfer,_("Downloads"));
	down1= statstree.append_child(h_download,_("Waiting..."));
	down1_1 = statstree.append_child(down1,_("Waiting..."));
	down1_2 = statstree.append_child(down1,_("Waiting..."));
	down1_3 = statstree.append_child(down1,_("Waiting..."));
	down1_4 = statstree.append_child(down1,_("Waiting..."));
	down1_5 = statstree.append_child(down1,_("Waiting..."));
	down1_6 = statstree.append_child(down1,_("Waiting..."));
	down1_7 = statstree.append_child(down1,_("Waiting..."));
	down1_8 = statstree.append_child(down1,_("Waiting..."));
	down2= statstree.append_child(h_download,_("Waiting..."));
	down3= statstree.append_child(h_download,_("Waiting..."));
	down4= statstree.append_child(h_download,_("Waiting..."));
	down5= statstree.append_child(h_download,_("Waiting..."));
	down6= statstree.append_child(h_download,_("Waiting..."));
	down7= statstree.append_child(h_download,_("Waiting..."));

	h_connection = statstree.append_child(root,_("Connection"));
	con1= statstree.append_child(h_connection,_("Waiting..."));
	con2= statstree.append_child(h_connection,_("Waiting..."));
	con12=statstree.append_child(h_connection,_("Waiting..."));
	con13=statstree.append_child(h_connection,_("Waiting..."));
	con3= statstree.append_child(h_connection,_("Waiting..."));
	con4= statstree.append_child(h_connection,_("Waiting..."));
	con5= statstree.append_child(h_connection,_("Waiting..."));
	con6= statstree.append_child(h_connection,_("Waiting..."));
	con7= statstree.append_child(h_connection,_("Waiting..."));
	con8= statstree.append_child(h_connection,_("Waiting..."));
	con9= statstree.append_child(h_connection,_("Waiting..."));

	h_clients = statstree.append_child(root,_("Clients"));
	cli15= statstree.append_child(h_clients,_("Waiting..."));
	cli1= statstree.append_child(h_clients,_("Waiting...")); // eMule
	cli_versions[0].active = false;
	cli_versions[1].active = false;
	cli_versions[2].active = false;
	cli_versions[3].active = false;
	cli10= statstree.append_child(h_clients,_("Waiting...")); // aMule
	cli10_1= statstree.append_child(cli10,_("Version")); // aMule
	cli10_2= statstree.append_child(cli10,_("Operative System")); // aMule
	cli_versions[12].active = false;
	cli_versions[13].active = false;
	cli_versions[14].active = false;
	cli_versions[15].active = false;	
	cli8= statstree.append_child(h_clients,_("Waiting..."));  // (l/x)mule
	cli2= statstree.append_child(h_clients,_("Waiting..."));  // eDonkey Hybrid
	cli_versions[4].active = false;
	cli_versions[5].active = false;
	cli_versions[6].active = false;
	cli_versions[7].active = false;	
	cli3= statstree.append_child(h_clients,_("Waiting..."));  // eDonkey
	cli_versions[8].active = false;
	cli_versions[9].active = false;
	cli_versions[10].active = false;
	cli_versions[11].active = false;	
	cli4= statstree.append_child(h_clients,_("Waiting..."));  // cDonkey
	cli5= statstree.append_child(h_clients,_("Waiting..."));  // Old MLDonkey
	cli9= statstree.append_child(h_clients,_("Waiting..."));   // New MLDonkey
	cli16= statstree.append_child(h_clients,_("Waiting..."));  // Shareaza
	cli12= statstree.append_child(h_clients,_("Waiting...")); // lphant
	cli11= statstree.append_child(h_clients,_("Waiting...")); // Compatible
	cli6= statstree.append_child(h_clients,_("Waiting..."));  // Unknown
	cli13= statstree.append_child(h_clients,_("Waiting...")); // lowid
	cli14= statstree.append_child(h_clients,_("Waiting...")); // Sec Ident
#ifdef  __DEBUG__
	cli17= statstree.append_child(h_clients,_("Waiting...")); // HasSocket
#endif
	cli7= statstree.append_child(h_clients,_("Waiting..."));


	h_servers = statstree.append_child(root,_("Servers"));
	srv1= statstree.append_child(h_servers,_("Waiting..."));
	srv2= statstree.append_child(h_servers,_("Waiting..."));
	srv3= statstree.append_child(h_servers,_("Waiting..."));
	srv4= statstree.append_child(h_servers,_("Waiting..."));
	srv5= statstree.append_child(h_servers,_("Waiting..."));
	srv6= statstree.append_child(h_servers,_("Waiting..."));
	srv7= statstree.append_child(h_servers,_("Waiting..."));
	srv8= statstree.append_child(h_servers,_("Waiting..."));
	srv9= statstree.append_child(h_servers,_("Waiting..."));

	h_shared = statstree.append_child(root, _("Shared Files"));
	shar1= statstree.append_child(h_shared,_("Waiting..."));
	shar2= statstree.append_child(h_shared,_("Waiting..."));
	shar3= statstree.append_child(h_shared,_("Waiting..."));
}

using namespace otherfunctions;


#ifndef CLIENT_GUI

void CStatistics::UpdateStatsTree() {
	wxString cbuffer;
	wxString cbuffer2;
	bool resize;
	uint32 myStats[19];

	resize=false;

	if (Start_time>0) {
		(*h_uptime) = _("Uptime: ") + CastSecondsToHM(GetUptimeSecs());
	}

	theApp.downloadqueue->GetDownloadStats(myStats);

	#define a_brackets_b(a,b) a +wxT(" (") + b + wxT(")")
	
	uint64 DownOHTotal = GetDownDataOverheadFileRequest() 
								+ GetDownDataOverheadSourceExchange() 
								+ GetDownDataOverheadServer() 
								+ GetDownDataOverheadOther();
	uint64 DownOHTotalPackets = GetDownDataOverheadFileRequestPackets() 
										+ GetDownDataOverheadSourceExchangePackets() 
										+ GetDownDataOverheadServerPackets() 
										+ GetDownDataOverheadOtherPackets();

	(*down1) = _("Downloaded Data (Session (Total)): ") +
										a_brackets_b(
											CastItoXBytes( stat_sessionReceivedBytes),
											CastItoXBytes( stat_sessionReceivedBytes+thePrefs::GetTotalDownloaded()));

	(*down1_1) = wxT("eMule: ") + CastItoXBytes(downloaded_eMule);
	(*down1_2) = wxT("aMule: ") + CastItoXBytes(downloaded_aMule);
	(*down1_3) = wxT("eDonkey: ") + CastItoXBytes(downloaded_eDonkey);
	(*down1_4) = wxT("eDonkeyHybrid: ") + CastItoXBytes(downloaded_eDonkeyHybrid);
	(*down1_5) = wxT("Shareaza: ") + CastItoXBytes(downloaded_Shareaza);
	(*down1_6) = wxT("MLDonkey: ") + CastItoXBytes(downloaded_MLDonkey);
	(*down1_7) = wxT("(l/x)Mule: ") + CastItoXBytes(downloaded_lxMule);
	(*down1_8) = wxT("Other: ") + CastItoXBytes(downloaded_Other);
	
	
	(*down2) = _("Total Overhead (Packets): ") +
										a_brackets_b(
											CastItoXBytes(DownOHTotal), 
											CastItoIShort(DownOHTotalPackets));

	(*down3) = _("File Request Overhead (Packets): ") +
										a_brackets_b(
											CastItoXBytes(GetDownDataOverheadFileRequest()),
											CastItoIShort(GetDownDataOverheadFileRequestPackets()));
											
	(*down4) = _("Source Exchange Overhead (Packets): ") +
										a_brackets_b(
											CastItoXBytes(GetDownDataOverheadSourceExchange()),
											CastItoIShort(GetDownDataOverheadSourceExchangePackets()));	

	(*down5) = _("Server Overhead (Packets): ") +
										a_brackets_b(
											CastItoXBytes(GetDownDataOverheadServer()),
											CastItoIShort(GetDownDataOverheadServerPackets()));
											
	(*down6) = wxString::Format(_("Found Sources: %i"),myStats[0]);
	
	(*down7) = wxString::Format(_("Active Downloads (chunks): %i"),myStats[1]);
	
	(*up1) = _("Uploaded Data (Session (Total)): ") + 
										a_brackets_b(
											CastItoXBytes( stat_sessionSentBytes),
											CastItoXBytes( stat_sessionSentBytes+thePrefs::GetTotalUploaded()));


	uint64 UpOHTotal = GetUpDataOverheadFileRequest() 
							+ GetUpDataOverheadSourceExchange() 
							+ GetUpDataOverheadServer() 
							+ GetUpDataOverheadOther();
	uint64 UpOHTotalPackets = GetUpDataOverheadFileRequestPackets() 
									+ GetUpDataOverheadSourceExchangePackets() 
									+ GetUpDataOverheadServerPackets() 
									+ GetUpDataOverheadOtherPackets();		

	(*up2) = _("Total Overhead (Packets): ") + 
										a_brackets_b(
											CastItoXBytes( UpOHTotal), 
											CastItoIShort(UpOHTotalPackets));

	(*up3) = _("File Request Overhead (Packets): ") + 
										a_brackets_b(
											CastItoXBytes(GetUpDataOverheadFileRequest()),
											CastItoIShort(GetUpDataOverheadFileRequestPackets()));
											
	(*up4) = _("Source Exchange Overhead (Packets): ") +
										a_brackets_b(
											CastItoXBytes(GetUpDataOverheadSourceExchange()),
											CastItoIShort(GetUpDataOverheadSourceExchangePackets()));
											
	(*up5) = _("Server Overhead (Packets): ") +
										a_brackets_b(
											CastItoXBytes(GetUpDataOverheadServer()),
											CastItoIShort(GetUpDataOverheadServerPackets()));
											
	(*up6) = wxString::Format(_("Active Uploads: %i"),theApp.uploadqueue->GetUploadQueueLength());
	(*up7) = wxString::Format(_("Waiting Uploads: %i"),theApp.uploadqueue->GetWaitingUserCount());
	(*up8) = wxString::Format(_("Total successful upload sessions: %i"),theApp.uploadqueue->GetSuccessfullUpCount());
	(*up9) = wxString::Format(_("Total failed upload sessions: %i"),theApp.uploadqueue->GetFailedUpCount());
	(*up10) = wxString::Format(_("Average upload time: ") + CastSecondsToHM(theApp.uploadqueue->GetAverageUpTime()));

	if (stat_transferStarttime>0) {
		(*con1) = wxString::Format(_("Average Downloadrate (Session): %.2f kB/s"),kBpsDownSession);
		(*con2) = wxString::Format(_("Average Uploadrate (Session): %.2f kB/s"),kBpsUpSession);
		(*con12) = wxString::Format(_("Max Downloadrate Average (Session): %.2f kB/s"),maxDownavg);	
		(*con13) = wxString::Format(_("Max Downloadrate (Session): %.2f kB/s"),maxDown);		
	}

	(*con3) = wxString::Format(_("Reconnects: %i"),
										(stat_reconnects>0) ? stat_reconnects-1 : 0);	

	if (stat_transferStarttime==0) {
		(*con4) = _("waiting for transfer...");
	} else {
		(*con4) =  _("Time Since First Transfer: ") + CastSecondsToHM(GetTransferSecs());
	}

	if (stat_serverConnectTime==0) {
		(*con5) = _("Waiting for connection...");
	}	else {
		(*con5) = _("Connected To Server Since: ") + CastSecondsToHM(GetServerSecs());
	}

	(*con6) = wxString::Format(wxT("%s: %i"),_("Active Connections (estimate)"), theApp.listensocket->GetActiveConnections());	
	uint32 m_itemp = theApp.listensocket->GetMaxConnectionReached();
	if( m_itemp != m_ilastMaxConnReached ) {
		char osDate[60];

		time_t mytime=time(NULL);
		struct tm* now=localtime(&mytime);
		strftime(osDate,sizeof(osDate)-1,"%d.%m.%Y %H:%M:%S",now);

		(*con7) = wxString::Format(wxT("%s: %i : %s"),_("Max Connection Limit Reached"),m_itemp,osDate);
		m_ilastMaxConnReached = m_itemp;
	} else if( m_itemp == 0 ) {
		(*con7) = wxString::Format(wxT("%s: %i"),_("Max Connection Limit Reached"),m_itemp);
	}

	if(theApp.serverconnect->IsConnected()) {
		(*con8) = wxString::Format(wxT("%s: %f"),_("Average Connections (estimate)"),(theApp.listensocket->GetAverageConnections()));
	} else {
		(*con8) = _("waiting for connection...");
	}
	(*con9) = wxString::Format(wxT("%s: %i"),_("Peak Connections (estimate)"),theApp.listensocket->GetPeakConnections());

		
	if (stat_sessionReceivedBytes>0 && stat_sessionSentBytes>0 ) {
		if (stat_sessionReceivedBytes<stat_sessionSentBytes) {
			(*tran0) = _("Session UL:DL Ratio (Total): ") 
						+  wxString::Format(wxT("%.2f : 1"),(float)stat_sessionSentBytes/stat_sessionReceivedBytes);
		} else {
			(*tran0) = _("Session UL:DL Ratio (Total): ")
						+ wxString::Format(wxT("1 : %.2f"),(float)stat_sessionReceivedBytes/stat_sessionSentBytes);
		}
	} else {
			(*tran0) = wxString(_("Session UL:DL Ratio (Total): ")) + _("Not available");
	}


	// shared files stats
	uint32 file_count = theApp.sharedfiles->GetCount();
	(*shar1) = wxString::Format(_("Number of Shared Files: %i"),file_count);

	uint64 allsize=theApp.sharedfiles->GetDatasize();
	(*shar2) = wxString::Format(_("Total size of Shared Files: ") + CastItoXBytes(allsize));
	
	if(file_count != 0) {
		(*shar3) = _("Average filesize: ") +  CastItoXBytes((uint64)allsize/file_count);
	} else {
		(*shar3) = wxString(_("Average filesize: ")) + wxT("-");
	}

	// get clientversion-counts

	// statsclientstatus : original idea and code by xrmb
	
	CClientList::ClientMap clientVersionEDonkey;
	CClientList::ClientMap clientVersionEDonkeyHybrid;
	CClientList::ClientMap clientVersionEMule;
	CClientList::ClientMap clientVersionAMule;
	uint32 totalclient;
	aMuleOSInfoMap OSInfo;
	theApp.clientlist->GetStatistics(totalclient, myStats, &clientVersionEDonkey, &clientVersionEDonkeyHybrid, &clientVersionEMule, &clientVersionAMule, &OSInfo);
	totalclient -= myStats[0];
	if( !totalclient ) {
		totalclient = 1;
	}
	
	(*cli15) = wxString::Format(_("Total: %i Known: %i"),totalclient + myStats[0],totalclient);
	(*cli1) = wxString::Format(wxT("eMule: %i (%1.1f%%)"),myStats[2],(double)100*myStats[2]/totalclient);
	(*cli10) = wxString::Format(wxT("aMule: %i (%1.1f%%)"),myStats[8],(double)100*myStats[8]/totalclient);
	(*cli8) = wxString::Format(wxT("lMule/xMule: %i (%1.1f%%)"),myStats[6],(double)100*myStats[6]/totalclient);
	(*cli2) = wxString::Format(wxT("eDonkeyHybrid: %i (%1.1f%%)"),myStats[4],(double)100*myStats[4]/totalclient);
	(*cli3) = wxString::Format(wxT("eDonkey: %i (%1.1f%%)"),myStats[1],(double)100*myStats[1]/totalclient);
	(*cli4) = wxString::Format(wxT("cDonkey: %i (%1.1f%%)"),myStats[5],(double)100*myStats[5]/totalclient);
	(*cli5) = wxString::Format(_("Old MLDonkey: %i (%1.1f%%)"),myStats[3],(double)100*myStats[3]/totalclient);
	(*cli9) = wxString::Format(_("New MLDonkey: %i (%1.1f%%)"),myStats[7],(double)100*myStats[7]/totalclient);
	(*cli12) = wxString::Format(wxT("lphant: %i (%1.1f%%)"),myStats[10],(double)100*myStats[10]/totalclient);
	(*cli16) = wxString::Format(wxT("Shareaza: %i (%1.1f%%)"),myStats[16],(double)100*myStats[16]/totalclient);
	(*cli11) = wxString::Format(_("Compatible: %i (%1.1f%%)"),myStats[9],(double)100*myStats[9]/totalclient);
	(*cli6) = wxString::Format(_("Unknown: %i"),myStats[0]);
	(*cli7) = wxString::Format(_("Filtered: %i"),stat_filteredclients);
	(*cli13) = wxString::Format(_("LowID: %u (%.2f%% Total %.2f%% Known)"),myStats[11] , ((totalclient + myStats[0])>0)?((double)100*myStats[11] /(totalclient + myStats[0])):0, (double)100*myStats[18]/totalclient);
	(*cli14) = wxString::Format(_("SecIdent On/Off: %u (%.2f%%) : %u (%.2f%%)"), myStats[12] , ((myStats[2]+myStats[8])>0)?((double)100*myStats[12] / (myStats[2]+myStats[8])):0, myStats[13] , ((myStats[2]+myStats[8])>0)?((double)100*myStats[13] /(myStats[2]+myStats[8]) ):0);
#ifdef __DEBUG__
	(*cli17) = wxString::Format(_("HasSocket: %i (%1.1f%%)"),myStats[17],(double)100*myStats[17]/(totalclient + myStats[0]));
#endif
	
	if(myStats[1] > 0) {

		uint32 totcnt = myStats[1];

		//--- find top 4 eDonkey client versions ---
		uint32	currtop = 0;
		uint32	lasttop = 0xFFFFFFFF;
		for(uint32 i=0; i<4; ++i) {
			CClientList::ClientMap::iterator it = clientVersionEDonkey.begin();
			uint32 topver=0;
			uint32 topcnt=0;
			double topper=0;
			for ( ; it != clientVersionEDonkey.end(); ++it ) {
				uint32	ver = it->first;
				uint32	cnt = it->second;
				
				if(currtop<ver && ver<lasttop && ver != 0x91) {
					double percent = (double)cnt/totcnt;
					if( lasttop == 0xFFFFFFFF && ((totcnt > 75 && ((cnt <= 2) || percent < 0.01)) || (totcnt > 50 && cnt <= 1))) {
						continue;
					}
					topper=percent;
					topver=ver;
					topcnt=cnt;
					currtop=ver;
				}
			}
			lasttop=currtop;
			currtop=0;
			if(topcnt) {
				UINT verMaj = topver/(100*10*100);
				UINT verMin = (topver - (verMaj*100*10*100))/(100*10);
				UINT verUp = (topver - (verMaj*100*10*100) - (verMin*100*10))/(100);
				cbuffer = wxString::Format(wxT("v%u.%u.%u: %i (%1.1f%%)"), verMaj, verMin, verUp, topcnt, topper*100);							
			} else {
				cbuffer=wxEmptyString;
			}
			if (cbuffer.IsEmpty()) {
				if (cli_versions[i+8].active) {
					statstree.erase(cli_versions[i+8].TreeItem);
					cli_versions[i+8].active = false;
				}
			} else {
				if (cli_versions[i+8].active) {
					(*(cli_versions[i+8].TreeItem)) = cbuffer;
				} else {
					cli_versions[i+8].TreeItem = statstree.append_child(cli3,cbuffer);
					cli_versions[i+8].active = true;
				}
			}
		}
	}

	if(myStats[4] >0) {

		uint32 totcnt = myStats[4];

		//--- find top 4 eDonkey Hybrid client versions ---
		uint32	currtop = 0;
		uint32	lasttop = 0xFFFFFFFF;
		for(uint32 i=0; i<4; ++i) {
			CClientList::ClientMap::iterator it = clientVersionEDonkeyHybrid.begin();
			uint32 topver=0;
			uint32 topcnt=0;
			double topper=0;
			for ( ; it != clientVersionEDonkeyHybrid.end(); ++it ) {
				uint32	ver = it->first;
				uint32	cnt = it->second;
				if(currtop<ver && ver<lasttop && ver != 0x91) {
					double percent = (double)cnt/totcnt;
					if( lasttop == 0xFFFFFFFF && ((totcnt > 75 && ((cnt <= 2) || percent < 0.01)) || (totcnt > 50 && cnt <= 1))) {
						continue;
					}
					topper=percent;
					topver=ver;
					topcnt=cnt;
					currtop=ver;
				}
			}
			lasttop=currtop;
			currtop=0;
			if(topcnt) {
				UINT verMaj = topver/(100*10*100);
				UINT verMin = (topver - (verMaj*100*10*100))/(100*10);
				UINT verUp = (topver - (verMaj*100*10*100) - (verMin*100*10))/(100);
				cbuffer = wxString::Format(wxT("v%u.%u.%u: %i (%1.1f%%)"), verMaj, verMin, verUp, topcnt, topper*100);
			} else {
				cbuffer= wxEmptyString;
			}
			if (cbuffer.IsEmpty()) {
				if (cli_versions[i+4].active) {
					statstree.erase(cli_versions[i+4].TreeItem);
					cli_versions[i+4].active = false;
				}
			} else {
				if (cli_versions[i+4].active) {
					(*(cli_versions[i+4].TreeItem)) = cbuffer;
				} else {
					cli_versions[i+4].TreeItem = statstree.append_child(cli2,cbuffer);
					cli_versions[i+4].active = true;
				}
			}
		}
	}

	if(myStats[2] > 0) {
		uint32 totcnt = myStats[2];

		//--- find top 4 eMule client versions ---
		uint32	currtop = 0;
		uint32	lasttop = 0xFFFFFFFF;
		for(uint32 i=0; i<4; ++i) {
			CClientList::ClientMap::iterator it = clientVersionEMule.begin();
			uint32 topver=0;
			uint32 topcnt=0;
			double topper=0;
			for ( ; it != clientVersionEMule.end(); ++it ) {
				uint32	ver = it->first;;
				uint32	cnt = it->second;
				if(currtop<ver && ver<lasttop )	{
					double percent = (double)cnt/totcnt;
					if( lasttop == 0xFFFFFFFF && ((totcnt > 75 && ((cnt <= 2) || percent < 0.01)) || (totcnt > 50 && cnt <= 1))) {
						continue;
					}
					topper=percent;
					topver=ver;
					topcnt=cnt;
					currtop=ver;
				}
			}
			lasttop=currtop;
			currtop=0;
			if(topcnt) {
				UINT verMaj = topver/(100*10*100);
				UINT verMin = (topver - (verMaj*100*10*100))/(100*10);
				UINT verUp = (topver - (verMaj*100*10*100) - (verMin*100*10))/(100);
				cbuffer = wxString::Format(wxT(" v%u.%u%c: %i (%1.1f%%)"),verMaj, verMin, 'a' + verUp, topcnt, topper*100);
			} else {
				cbuffer=wxEmptyString;
			}
			if (cbuffer.IsEmpty()) {
				if (cli_versions[i].active) {
					statstree.erase(cli_versions[i].TreeItem);
					cli_versions[i].active = false;
				}
			} else {
				if (cli_versions[i].active) {
					(*(cli_versions[i].TreeItem)) = cbuffer;
				} else {
					cli_versions[i].TreeItem = statstree.append_child(cli1,cbuffer);
					cli_versions[i].active = true;
				}
			}
		}
	}

	if(myStats[8] > 0) {
		uint32 totcnt = myStats[8];

		//--- find top 4 aMule client versions ---
		uint32	currtop = 0;
		uint32	lasttop = 0xFFFFFFFF;
		for(uint32 i=0; i<4; ++i) {
			CClientList::ClientMap::iterator it = clientVersionAMule.begin();
			uint32 topver=0;
			uint32 topcnt=0;
			double topper=0;
			for ( ; it != clientVersionAMule.end(); ++it ) {
				uint32	ver = it->first;
				uint32	cnt = it->second;
				if(currtop<ver && ver<lasttop )	{
					double percent = (double)cnt/totcnt;
					if( lasttop == 0xFFFFFFFF && ((totcnt > 75 && ((cnt <= 2) || percent < 0.01)) || (totcnt > 50 && cnt <= 1))) {
						continue;
					}
					topper=percent;
					topver=ver;
					topcnt=cnt;
					currtop=ver;
				}
			}
			lasttop=currtop;
			currtop=0;
			if(topcnt) {
				uint8 verMaj = topver/(100*10*100);
				uint8 verMin = (topver - (verMaj*100*10*100))/(100*10);
				uint8 verUp = (topver - (verMaj*100*10*100) - (verMin*100*10))/(100);
				if ((verMaj == 0) && (verUp == 0)) {
					cbuffer = wxString::Format(wxT(" v1.x: %i (%1.1f%%)"), topcnt, topper*100);
				} else {
					cbuffer = wxString::Format(wxT(" v%u.%u.%u: %i (%1.1f%%)"),verMaj, verMin, verUp, topcnt, topper*100);
				}
			} else {
				cbuffer=wxEmptyString;
			}
			if (cbuffer.IsEmpty()) {
				if (cli_versions[i+12].active) {
					statstree.erase(cli_versions[i+12].TreeItem);
					cli_versions[i+12].active = false;
				}
			} else {
				if (cli_versions[i+12].active) {
					(*(cli_versions[i+12].TreeItem)) = cbuffer;
				} else {
					cli_versions[i+12].TreeItem = statstree.append_child(cli10_1,cbuffer);
					cli_versions[i+12].active = true;
				}
			}
		}
	}
	
	if(myStats[8]) {

		statstree.erase_children(cli10_2);
		
		uint32 total = 0;
		
		for (aMuleOSInfoMap::iterator it = OSInfo.begin(); it != OSInfo.end(); ++it ) {
			total += it->second;
			statstree.append_child(cli10_2, it->first + wxString::Format(wxT(": %u (%1.1f%%)"),it->second, ((double)it->second / myStats[8]) * 100 ));
		}
		
		wxASSERT(((int)myStats[8] - (int)total)>=0);
		
		uint32 not_rec = (myStats[8] - total);
		
		if (not_rec > 0 ) {
			statstree.append_child(cli10_2, _("Not Received") + wxString::Format(wxT(": %u (%1.1f%%)"),not_rec, ((double)not_rec / myStats[8]) * 100 ));
		}
		
	}
	
	// get serverstats
	uint32 servtotal;
	uint32 servfail;
	uint32 servuser;
	uint32 servfile;
	uint32 servtuser;
	uint32 servtfile;
	float servocc;
	theApp.serverlist->GetStatus( servtotal, servfail, servuser, servfile, servtuser, servtfile,servocc);
	(*srv1) = wxString::Format(wxT("%s: %i"),_("Working Servers"),servtotal-servfail);
	(*srv2) = wxString::Format(wxT("%s: %i"),_("Failed Servers"),servfail);
	(*srv3) = wxString::Format(wxT("%s: %i"),_("Total"),servtotal);
	(*srv4) = wxString::Format(wxT("%s: %i"),_("Deleted Servers"),theApp.serverlist->GetDeletedServerCount());
	(*srv5) = wxString::Format(wxT("%s: %i"),_("Users on Working Servers"),servuser);
	(*srv6) = wxString::Format(wxT("%s: %i"),_("Files on Working Servers"),servfile);
	(*srv7) = wxString::Format(wxT("%s: %i"),_("Total Users"),servtuser);
	(*srv8) = wxString::Format(wxT("%s: %i"),_("Total Files"),servtfile);
	(*srv9) = wxString::Format(_("Server Occupation: %.2f%%"),servocc);
	
}

void CStatistics::AddDownloadFromSoft(uint8 SoftType, uint32 bytes) {
	switch (SoftType) {
		case SO_OLDEMULE:
		case SO_EMULE:
		case SO_EMULEPLUS:
		case SO_COMPAT_UNK:
			downloaded_eMule += bytes;
			break;
		case SO_AMULE:
			downloaded_aMule += bytes;
			break;
		case SO_EDONKEY:
			downloaded_eDonkey += bytes;
			break;
		case SO_EDONKEYHYBRID:
			downloaded_eDonkeyHybrid += bytes;
			break;
		case SO_SHAREAZA:
		case SO_NEW_SHAREAZA:
			downloaded_Shareaza += bytes;
			break;
		case SO_MLDONKEY:
		case SO_NEW_MLDONKEY:
		case SO_NEW2_MLDONKEY:			
			downloaded_MLDonkey += bytes;
			break;
		case SO_LXMULE:
			downloaded_lxMule += bytes;
			break;
		default:
			downloaded_Other += bytes;
			break;
	}
}

#endif // CLIENT_GUI

/* Left here so amulecmd can copy it.
void CStatistics::ShowStatsTree() {
	// Just clients
	
	if(h_clients!=statstree.end()) {
	StatsTreeNode Node = statstree.begin(h_clients);
	StatsTreeNode end = statstree.end(h_clients);
	while(Node!=end) {
		for(int i=0; i<statstree.depth(Node)-2; ++i) {
			printf(" ");
		}
		printf("%s\n",unicode2char(*Node));
		++Node;
		}
	}
	printf("\n\n");
	
}
*/
