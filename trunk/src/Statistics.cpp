//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
// Copyright (c) 2005 Dévai Tamás ( gonosztopi@amule.org )
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

#include "Statistics.h"		// Interface declarations
#include "ECcodes.h"		// Needed for EC tagnames
#include "ECPacket.h"		// Needed for CECTag

#ifndef EC_REMOTE
	#include "GetTickCount.h"	// Needed for GetTickCount64()
	#ifndef AMULE_DAEMON
		#include "Format.h"		// Needed for CFormat
		#include "OtherFunctions.h"	// Needed for CastItoSpeed()
	#endif
	#include "DataToText.h"		// Needed for GetSoftName()
	#include "Preferences.h"	// Needed for thePrefs
	#include "amule.h"		// Needed for theApp
	#include "ListenSocket.h"	// (tree, GetAverageConnections)
	#include "ServerList.h"		// Needed for CServerList (tree)
	#include <cmath>		// Needed for std::floor
#else
	#include "amule-remote-gui.h"	// Needed for CRemoteConnect
#endif


#ifdef __BSD__
	// glibc -> bsd libc
	#define round rint
#else
	#define round(x) floor(x+0.5)
#endif /* __BSD__ */


#ifndef EC_REMOTE

/*----- CPreciseRateCounter -----*/

void CPreciseRateCounter::CalculateRate(uint64_t now)
{
	wxMutexLocker lock(m_mutex);

	m_total += m_tmp_sum;
	m_byte_history.push_back(m_tmp_sum);
	m_tick_history.push_back(now);
	m_tmp_sum = 0;

	uint64_t timespan = now - m_tick_history.front();

	// Checking maximal timespan, but make sure not to remove
	// the extra node in m_tick_history.
	while (timespan > m_timespan && m_byte_history.size() > 0) {
		m_total -= m_byte_history.front();
		m_byte_history.pop_front();
		m_tick_history.pop_front();
		timespan = now - m_tick_history.front();
	}

	// Count rate/average
	if (m_count_average) {
		if (m_byte_history.size() > 0) {
			m_rate = m_total / (double)m_byte_history.size();
		}
	} else {
		if (timespan > 0) {
			m_rate = m_total / (timespan / 1000.0);
		}
	}

	if (m_rate > m_max_rate) {
		m_max_rate = m_rate;
	}
}


/*----- CStatTreeItemRateCounter -----*/

#ifndef AMULE_DAEMON
wxString CStatTreeItemRateCounter::GetDisplayString() const
{
	return CFormat(wxGetTranslation(m_label)) % otherfunctions::CastItoSpeed(m_show_maxrate ? (uint32)m_max_rate : (uint32)m_rate);
}
#endif

void CStatTreeItemRateCounter::AddECValues(CECTag* tag) const
{
	CECTag value(EC_TAG_STAT_NODE_VALUE, m_show_maxrate ? (uint32)m_max_rate : (uint32)m_rate);
	value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_SPEED));
	tag->AddTag(value);
}


/*----- CStatTreeItemPeakConnections -----*/

#ifndef AMULE_DAEMON
wxString CStatTreeItemPeakConnections::GetDisplayString() const
{
	return wxString::Format(wxGetTranslation(m_label), theStats::GetPeakConnections());
}
#endif

void CStatTreeItemPeakConnections::AddECValues(CECTag* tag) const
{
	tag->AddTag(CECTag(EC_TAG_STAT_NODE_VALUE, (uint64)theStats::GetPeakConnections()));
}


/*----- CStatistics -----*/

// Static variables

// Rate counters
CPreciseRateCounter*		CStatistics::s_upOverheadRate;
CPreciseRateCounter*		CStatistics::s_downOverheadRate;
CStatTreeItemRateCounter*	CStatistics::s_uploadrate;
CStatTreeItemRateCounter*	CStatistics::s_downloadrate;
CStatTreeItemRateCounter*	CStatistics::s_runningAverageUp;
CStatTreeItemRateCounter*	CStatistics::s_runningAverageDown;

#else /* EC_REMOTE */

uint64	CStatistics::s_start_time;
uint64	CStatistics::s_statData[sdTotalItems];

#endif /* !EC_REMOTE / EC_REMOTE */

// Tree root
CStatTreeItemBase*		CStatistics::s_statTree;

#ifndef EC_REMOTE
// Uptime
CStatTreeItemTimer*		CStatistics::s_uptime;

// Upload
CStatTreeItemUlDlCounter*	CStatistics::s_sessionUpload;
CStatTreeItemPacketTotals*	CStatistics::s_totalUpOverhead;
CStatTreeItemPackets*		CStatistics::s_fileReqUpOverhead;
CStatTreeItemPackets*		CStatistics::s_sourceXchgUpOverhead;
CStatTreeItemPackets*		CStatistics::s_serverUpOverhead;
CStatTreeItemPackets*		CStatistics::s_kadUpOverhead;
CStatTreeItemNativeCounter*	CStatistics::s_activeUploads;
CStatTreeItemNativeCounter*	CStatistics::s_waitingUploads;
CStatTreeItemCounter*		CStatistics::s_totalSuccUploads;
CStatTreeItemCounter*		CStatistics::s_totalFailedUploads;
CStatTreeItemCounter*		CStatistics::s_totalUploadTime;

// Download
CStatTreeItemUlDlCounter*	CStatistics::s_sessionDownload;
CStatTreeItemPacketTotals*	CStatistics::s_totalDownOverhead;
CStatTreeItemPackets*		CStatistics::s_fileReqDownOverhead;
CStatTreeItemPackets*		CStatistics::s_sourceXchgDownOverhead;
CStatTreeItemPackets*		CStatistics::s_serverDownOverhead;
CStatTreeItemPackets*		CStatistics::s_kadDownOverhead;
CStatTreeItemNativeCounter*	CStatistics::s_foundSources;
CStatTreeItemNativeCounter*	CStatistics::s_activeDownloads;

// Connection
CStatTreeItemReconnects*	CStatistics::s_reconnects;
CStatTreeItemTimer*		CStatistics::s_sinceFirstTransfer;
CStatTreeItemTimer*		CStatistics::s_sinceConnected;
CStatTreeItemCounterMax*	CStatistics::s_activeConnections;
CStatTreeItemMaxConnLimitReached* CStatistics::s_limitReached;
CStatTreeItemSimple*		CStatistics::s_avgConnections;

// Clients
CStatTreeItemHiddenCounter*	CStatistics::s_clients;
CStatTreeItemCounter*		CStatistics::s_unknown;
//CStatTreeItem			CStatistics::s_lowID;
//CStatTreeItem			CStatistics::s_secIdentOnOff;
#ifdef __DEBUG__
CStatTreeItemNativeCounter*	CStatistics::s_hasSocket;
#endif
CStatTreeItemNativeCounter*	CStatistics::s_filtered;
CStatTreeItemNativeCounter*	CStatistics::s_banned;

// Servers
CStatTreeItemSimple*		CStatistics::s_workingServers;
CStatTreeItemSimple*		CStatistics::s_failedServers;
CStatTreeItemNativeCounter*	CStatistics::s_totalServers;
CStatTreeItemNativeCounter*	CStatistics::s_deletedServers;
CStatTreeItemNativeCounter*	CStatistics::s_filteredServers;
CStatTreeItemSimple*		CStatistics::s_usersOnWorking;
CStatTreeItemSimple*		CStatistics::s_filesOnWorking;
CStatTreeItemSimple*		CStatistics::s_totalUsers;
CStatTreeItemSimple*		CStatistics::s_totalFiles;
CStatTreeItemSimple*		CStatistics::s_serverOccupation;

// Shared files
CStatTreeItemCounter*		CStatistics::s_numberOfShared;
CStatTreeItemCounter*		CStatistics::s_sizeOfShare;


CStatistics::CStatistics()
	: m_graphRunningAvgDown(thePrefs::GetStatsAverageMinutes() * 60 * 1000, true),
	  m_graphRunningAvgUp(thePrefs::GetStatsAverageMinutes() * 60 * 1000, true)
{
	uint64 start_time = GetTickCount64();

	// Init graphs

	HR hr = {0.0, 0.0, 0.0, 0.0, 0.0, 0, 0, 0};
	hrInit = hr;
	nHistRanges = 7;	// =ceil(log(max_update_delay)/log(2))
	nPointsPerRange = GetPointsPerRange(); 
	bitsHistClockMask = (1 << (nHistRanges-1)) - 1;
	aposRecycle = new listPOS[nHistRanges];
	listPOS *ppos = aposRecycle+nHistRanges-1;
	for (int i=nHistRanges; i>0; --i) {  // permanently allocated history list
		listHR.push_back(hr);
		*ppos-- = --listHR.end();
		for (int j=nPointsPerRange; j>1; --j)
			listHR.push_back(hr);
	}	

	// Init rate counters outside the tree

	s_upOverheadRate = new CPreciseRateCounter(5000);
	s_downOverheadRate = new CPreciseRateCounter(5000);

	// Init Tree

	InitStatsTree();
	s_uptime->SetStartTime(start_time);
}


CStatistics::~CStatistics()
{
	// clearing listHR frees the memory occupied by the nodes
	listHR.clear();

	delete s_statTree;

	// delete items not in the tree
	delete s_totalUploadTime;

	// delete rate counters outside the tree
	delete s_upOverheadRate;
	delete s_downOverheadRate;
}


void CStatistics::CalculateRates()
{
	uint64_t now = GetTickCount64();
	s_downOverheadRate->CalculateRate(now);
	s_upOverheadRate->CalculateRate(now);
	s_downloadrate->CalculateRate(now);
	s_uploadrate->CalculateRate(now);
	(*s_runningAverageDown) += (uint32)s_downloadrate->GetRate();
	(*s_runningAverageUp) += (uint32)s_uploadrate->GetRate();
	s_runningAverageDown->CalculateRate(now);
	s_runningAverageUp->CalculateRate(now);
}


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
  This unfortunately does not work with stl classes, as none of them supports moving
a node to another place, so we have to erase and re-add nodes.
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
	listPOS		*ppos;
	static int	iClock;
	int		iClockPrev = iClock++;
	int		bits = (iClockPrev^iClock) & iClock;  // identify the highest changed bit
	if (bits <= bitsHistClockMask) {
		ppos = aposRecycle;
		while ((bits /= 2) != 0)  // count to the highest bit that was just toggled to 1
			++ppos;	
		// recycle one node and jump over the next to move it to the next higher range
		listHR.push_back(**ppos);
		*ppos = ++listHR.erase(*ppos);
	} else {
		ppos = aposRecycle+nHistRanges-1;
		// recycle the node at the head; there is no higher range to move nodes into
		listHR.push_back(**ppos);
		*ppos = listHR.erase(*ppos);
	}
	
	// now save the latest data point in this node
 	listPOS phr = --listHR.end();
	phr->kBytesSent = GetSessionSentBytes() / 1024.0;
	phr->kBytesReceived = GetSessionReceivedBytes() / 1024.0;
	phr->kBpsUpCur = GetUploadRate() / 1024.0;
	phr->kBpsDownCur = GetDownloadRate() / 1024.0;
	phr->cntUploads = GetActiveUploadsCount();
	phr->cntConnections = GetActiveConnections();
	phr->cntDownloads = GetDownloadingSources();
	phr->sTimestamp = GetUptimeMillis() / 1000.0;
#ifdef __DEBUG__
//	if (bits > bitsHistClockMask)  // every 64 seconds - 
//		VerifyHistory();  // must do this AFTER phr->sTimestamp has been set
#endif
}


unsigned CStatistics::GetHistory(	// Assemble arrays of sample points for a graph
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
	listRPOS	pos = listHR.rbegin();

	// start of list should be an integer multiple of the sampling period for samples 
	// to be consistent when the graphs are resized horizontally
	double	sTarget;
	if (sFinal >= 0.0)
		sTarget = sFinal;
	else
		sTarget = (sStep==1.0 ? pos->sTimestamp : std::floor(pos->sTimestamp/sStep) * sStep); 

	HR	**ahr = NULL, **pphr = NULL;
	bool	bRateGraph = (which_graph != GRAPH_CONN);	// rate graph or connections graph?
	if (bRateGraph) {
		ahr = new HR* [cntPoints];
		pphr = ahr;
	}
	
	do {
		while (pos != listHR.rend() && pos->sTimestamp > sTarget) ++pos;	// find next history record
		if (bRateGraph) {		// assemble an array of pointers for ComputeAverages
			*pphr++ = &(*pos);
		} else {			// or build the arrays if possible
			*pf1++ = (float)pos->cntUploads;
			*pf2++ = (float)pos->cntConnections;
			*pf3++ = (float)pos->cntDownloads;
		}
		if (++cntFilled  == cntPoints)		// enough points 
			break;
		if (pos->sTimestamp == 0.0)		// reached beginning of uptime
			break;
		if ((sTarget -= sStep) <= 0.0) {	// don't overshoot the beginning
			if (bRateGraph)
				*pphr++ = &hrInit;
			else
				*pf1++ = *pf2++ = *pf3++ = 0.0;
			++cntFilled;
			break;
		}
	} while (pos != listHR.rend());

	if (bRateGraph) {
		if  (cntFilled > 0)
			ComputeAverages(pphr, pos, cntFilled, sStep, ppf, which_graph);
		delete[] ahr;
	}

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
	listRPOS	pos = listHR.rbegin();
	double		LastTimeStamp = pos->sTimestamp;
	double		sTarget = LastTimeStamp;
	
	HR	**pphr = new HR *[cntPoints];

	do {
		while (pos != listHR.rend() && pos->sTimestamp > sTarget) ++pos;	// find next history record
		pphr[cntFilled] = &(*pos);
		if (++cntFilled  == cntPoints)		// enough points 
			break;
		if (pos->sTimestamp <= *sStart)		// reached beginning of requested time
			break;
		if ((sTarget -= sStep) <= 0.0) {	// don't overshoot the beginning
			pphr[cntFilled++] = NULL;
			break;
		}
	} while (pos != listHR.rend());

	if (cntFilled) {
		*graphData = new uint32 [3 * cntFilled];
		if (*graphData) {
			for (unsigned int i = 0; i < cntFilled; i++) {
				HR *phr = pphr[cntFilled - i - 1];
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
	HR		**pphr,			// pointer to (end of) array of assembled history records
	listRPOS	pos,			// position in history list from which to backtrack
	unsigned	cntFilled,		// number of points in the sample data
	double		sStep,			// time difference between two samples
	float		**ppf,			// an array of pointers to arrays of floats with sample data
	StatsGraphType	which_graph)		// the graph which will receive the points
{	
	double		sTarget, kBytesRun;
	uint64 		avgTime = thePrefs::GetStatsAverageMinutes() * 60;
	unsigned	nBtPoints = (unsigned)(avgTime / sStep);
	CPreciseRateCounter *runningAvg = (which_graph == GRAPH_DOWN) ? &m_graphRunningAvgDown : &m_graphRunningAvgUp;

	runningAvg->m_timespan = avgTime * 1000;
	runningAvg->m_tick_history.clear();
	runningAvg->m_byte_history.clear();
	runningAvg->m_total = 0;
	runningAvg->m_tmp_sum = 0;

	sTarget = std::max(0.0, pos->sTimestamp - sStep);
	
	while (nBtPoints--) {
		while (pos != listHR.rend() && pos->sTimestamp > sTarget) ++pos;	// find next history record
		if (pos != listHR.rend()) {
			runningAvg->m_tick_history.push_front((uint64)(pos->sTimestamp * 1000.0));
			uint32 bytes = (uint32)((which_graph == GRAPH_DOWN ? pos->kBpsDownCur : pos->kBpsUpCur) * 1024.0);
			runningAvg->m_byte_history.push_front(bytes);
			runningAvg->m_total += bytes;
		} else {
			break;
		}
		if ((sTarget -= sStep) < 0.0) {
			break;
		}
	};

	// now compute averages in returned arrays, starting with the earliest values
	float	*pf1 = *ppf++ + cntFilled - 1;	// holds session avg
	float	*pf2 = *ppf++ + cntFilled - 1;	// holds running avg
	float	*pf3 = *ppf + cntFilled - 1;	// holds current rate

	for (int cnt=cntFilled; cnt>0; cnt--, pf1--, pf2--, pf3--) {
		HR *phr = *(--pphr);
		if (which_graph==GRAPH_DOWN) {
			kBytesRun = phr->kBytesReceived;
			*pf3 = phr->kBpsDownCur;
		} else {
			kBytesRun = phr->kBytesSent;
			*pf3 = phr->kBpsUpCur;
		}
		*pf1 = kBytesRun / phr->sTimestamp;
		(*runningAvg) += (uint32)(*pf3 * 1024.0);
		runningAvg->CalculateRate((uint64)(phr->sTimestamp * 1000.0));
		*pf2 = (float)(runningAvg->GetRate() / 1024.0);
	}
}


GraphUpdateInfo CStatistics::GetPointsForUpdate()
{
	GraphUpdateInfo update;
	listPOS phr = --listHR.end();
	update.timestamp = (double) phr->sTimestamp;

	m_graphRunningAvgDown += (uint32)(phr->kBpsDownCur * 1024.0);
	m_graphRunningAvgUp += (uint32)(phr->kBpsUpCur * 1024.0);
	m_graphRunningAvgDown.CalculateRate((uint64)(phr->sTimestamp * 1000.0));
	m_graphRunningAvgUp.CalculateRate((uint64)(phr->sTimestamp * 1000.0));

	update.downloads[0] = phr->kBytesReceived / phr->sTimestamp;
	update.downloads[1] = (float)(m_graphRunningAvgDown.GetRate() / 1024.0);
	update.downloads[2] = phr->kBpsDownCur;

	update.uploads[0] = phr->kBytesSent / phr->sTimestamp;
	update.uploads[1] = (float)(m_graphRunningAvgUp.GetRate() / 1024.0);
	update.uploads[2] = phr->kBpsUpCur;

	update.connections[0] = (float)phr->cntUploads;
	update.connections[1] = (float)phr->cntConnections;
	update.connections[2] = (float)phr->cntDownloads;

	return update;
}


#if 0 //def __DEBUG__ // Needs to be ported to stl
void CStatistics::VerifyHistory(bool bMsgIfOk)
// Debugging only: This performs a basic sanity check on the history list: 
// link integrity, correct number of nodes, sequentiality of timestamps, 
// approximate gap length between sample points.
// See graph in RecordHistory() for the mechanics of the list.
// Un-comment the call at the end of RecordHistory() to activate these checks.
// Note: if the system is very busy we sometimes get a long gap followed by a short one,
// and they will be flagged as they move through the list.
{
	int	cnt, cntRanges=1, cntInRange=0, cntExpected=nHistRanges*nPointsPerRange;
	double sStart = (double)GetUptimeMillis() + 0.01;
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
}
#endif	/* __DEBUG__ */


/* ------------------------------- TREE ---------------------------- */

void CStatistics::InitStatsTree()
{
	s_statTree = new CStatTreeItemBase(wxTRANSLATE("Statistics"));

	CStatTreeItemBase* tmpRoot1;
	CStatTreeItemBase* tmpRoot2;

	s_uptime = (CStatTreeItemTimer*)s_statTree->AddChild(new CStatTreeItemTimer(wxTRANSLATE("Uptime: %s")));

	tmpRoot1 = s_statTree->AddChild(new CStatTreeItemBase(wxTRANSLATE("Transfer"), stSortChildren));

	tmpRoot2 = tmpRoot1->AddChild(new CStatTreeItemBase(wxTRANSLATE("Uploads")), 2);
	s_sessionUpload = (CStatTreeItemUlDlCounter*)tmpRoot2->AddChild(new CStatTreeItemUlDlCounter(wxTRANSLATE("Uploaded Data (Session (Total)): %s"), thePrefs::GetTotalUploaded, stSortChildren | stSortByValue));
	// Children will be added on-the-fly
	s_totalUpOverhead = (CStatTreeItemPacketTotals*)tmpRoot2->AddChild(new CStatTreeItemPacketTotals(wxTRANSLATE("Total Overhead (Packets): %s")));
	s_fileReqUpOverhead = (CStatTreeItemPackets*)tmpRoot2->AddChild(new CStatTreeItemPackets(wxTRANSLATE("File Request Overhead (Packets): %s")));
	s_totalUpOverhead->AddPacketCounter(s_fileReqUpOverhead);
	s_sourceXchgUpOverhead = (CStatTreeItemPackets*)tmpRoot2->AddChild(new CStatTreeItemPackets(wxTRANSLATE("Source Exchange Overhead (Packets): %s")));
	s_totalUpOverhead->AddPacketCounter(s_sourceXchgUpOverhead);
	s_serverUpOverhead = (CStatTreeItemPackets*)tmpRoot2->AddChild(new CStatTreeItemPackets(wxTRANSLATE("Server Overhead (Packets): %s")));
	s_totalUpOverhead->AddPacketCounter(s_serverUpOverhead);
	s_kadUpOverhead = (CStatTreeItemPackets*)tmpRoot2->AddChild(new CStatTreeItemPackets(wxTRANSLATE("Kad Overhead (Packets): %s")));
	s_totalUpOverhead->AddPacketCounter(s_kadUpOverhead);
	s_activeUploads = (CStatTreeItemNativeCounter*)tmpRoot2->AddChild(new CStatTreeItemNativeCounter(wxTRANSLATE("Active Uploads: %s")));
	s_waitingUploads = (CStatTreeItemNativeCounter*)tmpRoot2->AddChild(new CStatTreeItemNativeCounter(wxTRANSLATE("Waiting Uploads: %s")));
	s_totalSuccUploads = (CStatTreeItemCounter*)tmpRoot2->AddChild(new CStatTreeItemCounter(wxTRANSLATE("Total successful upload sessions: %s")));
	s_totalFailedUploads = (CStatTreeItemCounter*)tmpRoot2->AddChild(new CStatTreeItemCounter(wxTRANSLATE("Total failed upload sessions: %s")));
	s_totalUploadTime = new CStatTreeItemCounter(wxEmptyString);
	tmpRoot2->AddChild(new CStatTreeItemAverage(wxTRANSLATE("Average upload time: %s"), s_totalUploadTime, s_totalSuccUploads, dmTime));

	tmpRoot2 = tmpRoot1->AddChild(new CStatTreeItemBase(wxTRANSLATE("Downloads")), 1);
	s_sessionDownload = (CStatTreeItemUlDlCounter*)tmpRoot2->AddChild(new CStatTreeItemUlDlCounter(wxTRANSLATE("Downloaded Data (Session (Total)): %s"), thePrefs::GetTotalDownloaded, stSortChildren | stSortByValue));
	// Children will be added on-the-fly
	s_totalDownOverhead = (CStatTreeItemPacketTotals*)tmpRoot2->AddChild(new CStatTreeItemPacketTotals(wxTRANSLATE("Total Overhead (Packets): %s")));
	s_fileReqDownOverhead = (CStatTreeItemPackets*)tmpRoot2->AddChild(new CStatTreeItemPackets(wxTRANSLATE("File Request Overhead (Packets): %s")));
	s_totalDownOverhead->AddPacketCounter(s_fileReqDownOverhead);
	s_sourceXchgDownOverhead = (CStatTreeItemPackets*)tmpRoot2->AddChild(new CStatTreeItemPackets(wxTRANSLATE("Source Exchange Overhead (Packets): %s")));
	s_totalDownOverhead->AddPacketCounter(s_sourceXchgDownOverhead);
	s_serverDownOverhead = (CStatTreeItemPackets*)tmpRoot2->AddChild(new CStatTreeItemPackets(wxTRANSLATE("Server Overhead (Packets): %s")));
	s_totalDownOverhead->AddPacketCounter(s_serverDownOverhead);
	s_kadDownOverhead = (CStatTreeItemPackets*)tmpRoot2->AddChild(new CStatTreeItemPackets(wxTRANSLATE("Kad Overhead (Packets): %s")));
	s_totalDownOverhead->AddPacketCounter(s_kadDownOverhead);
	s_foundSources = (CStatTreeItemNativeCounter*)tmpRoot2->AddChild(new CStatTreeItemNativeCounter(wxTRANSLATE("Found Sources: %s"), stSortChildren | stSortByValue));
	s_activeDownloads = (CStatTreeItemNativeCounter*)tmpRoot2->AddChild(new CStatTreeItemNativeCounter(wxTRANSLATE("Active Downloads (chunks): %s")));

	tmpRoot1->AddChild(new CStatTreeItemRatio(wxTRANSLATE("Session UL:DL Ratio (Total): %s"), s_sessionUpload, s_sessionDownload), 3);

	tmpRoot1 = s_statTree->AddChild(new CStatTreeItemBase(wxTRANSLATE("Connection")));
	s_runningAverageDown = (CStatTreeItemRateCounter*)tmpRoot1->AddChild(new CStatTreeItemRateCounter(wxTRANSLATE("Average Downloadrate (Session): %s"), false,300000, true));
	s_runningAverageUp = (CStatTreeItemRateCounter*)tmpRoot1->AddChild(new CStatTreeItemRateCounter(wxTRANSLATE("Average Uploadrate (Session): %s"), false, 300000, true));
	s_downloadrate = (CStatTreeItemRateCounter*)tmpRoot1->AddChild(new CStatTreeItemRateCounter(wxTRANSLATE("Max Downloadrate (Session): %s"), true, 30000));
	s_uploadrate = (CStatTreeItemRateCounter*)tmpRoot1->AddChild(new CStatTreeItemRateCounter(wxTRANSLATE("Max Uploadrate (Session): %s"), true, 30000));
	s_reconnects = (CStatTreeItemReconnects*)tmpRoot1->AddChild(new CStatTreeItemReconnects(wxTRANSLATE("Reconnects: %i")));
	s_sinceFirstTransfer = (CStatTreeItemTimer*)tmpRoot1->AddChild(new CStatTreeItemTimer(wxTRANSLATE("Time Since First Transfer: %s"), stHideIfZero));
	s_sinceConnected = (CStatTreeItemTimer*)tmpRoot1->AddChild(new CStatTreeItemTimer(wxTRANSLATE("Connected To Server Since: %s")));
	s_activeConnections = (CStatTreeItemCounterMax*)tmpRoot1->AddChild(new CStatTreeItemCounterMax(wxTRANSLATE("Active Connections (estimate): %i")));
	s_limitReached = (CStatTreeItemMaxConnLimitReached*)tmpRoot1->AddChild(new CStatTreeItemMaxConnLimitReached(wxTRANSLATE("Max Connection Limit Reached: %s")));
	s_avgConnections = (CStatTreeItemSimple*)tmpRoot1->AddChild(new CStatTreeItemSimple(wxTRANSLATE("Average Connections (estimate): %g")));
	s_avgConnections->SetValue(0.0);
	tmpRoot1->AddChild(new CStatTreeItemPeakConnections(wxTRANSLATE("Peak Connections (estimate): %i")));

	s_clients = (CStatTreeItemHiddenCounter*)s_statTree->AddChild(new CStatTreeItemHiddenCounter(wxTRANSLATE("Clients"), stSortChildren | stSortByValue));
	s_unknown = (CStatTreeItemCounter*)s_clients->AddChild(new CStatTreeItemCounter(wxTRANSLATE("Unknown") wxT(": %s")), 6);
	//s_lowID = (CStatTreeItem*)s_clients->AddChild(new CStatTreeItem(wxTRANSLATE("LowID: %u (%.2f%% Total %.2f%% Known)")), 5);
	//s_secIdentOnOff = (CStatTreeItem*)s_clients->AddChild(new CStatTreeItem(wxTRANSLATE("SecIdent On/Off: %u (%.2f%%) : %u (%.2f%%)")), 4);
#ifdef __DEBUG__
	s_hasSocket = (CStatTreeItemNativeCounter*)s_clients->AddChild(new CStatTreeItemNativeCounter(wxT("HasSocket: %s")), 3);
#endif
	s_filtered = (CStatTreeItemNativeCounter*)s_clients->AddChild(new CStatTreeItemNativeCounter(wxTRANSLATE("Filtered") wxT(": %s")), 2);
	s_banned = (CStatTreeItemNativeCounter*)s_clients->AddChild(new CStatTreeItemNativeCounter(wxTRANSLATE("Banned") wxT(": %s")), 1);
	s_clients->AddChild(new CStatTreeItemTotalClients(wxTRANSLATE("Total: %i Known: %i"), s_clients, s_unknown), 0x80000000);

	// TODO: Use counters?
	tmpRoot1 = s_statTree->AddChild(new CStatTreeItemBase(wxTRANSLATE("Servers")));
	s_workingServers = (CStatTreeItemSimple*)tmpRoot1->AddChild(new CStatTreeItemSimple(wxTRANSLATE("Working Servers: %i")));
	s_failedServers = (CStatTreeItemSimple*)tmpRoot1->AddChild(new CStatTreeItemSimple(wxTRANSLATE("Failed Servers: %i")));
	s_totalServers = (CStatTreeItemNativeCounter*)tmpRoot1->AddChild(new CStatTreeItemNativeCounter(wxTRANSLATE("Total: %s")));
	s_deletedServers = (CStatTreeItemNativeCounter*)tmpRoot1->AddChild(new CStatTreeItemNativeCounter(wxTRANSLATE("Deleted Servers: %s")));
	s_filteredServers = (CStatTreeItemNativeCounter*)tmpRoot1->AddChild(new CStatTreeItemNativeCounter(wxTRANSLATE("Filtered Servers: %s")));
	s_usersOnWorking = (CStatTreeItemSimple*)tmpRoot1->AddChild(new CStatTreeItemSimple(wxTRANSLATE("Users on Working Servers: %i")));
	s_filesOnWorking = (CStatTreeItemSimple*)tmpRoot1->AddChild(new CStatTreeItemSimple(wxTRANSLATE("Files on Working Servers: %i")));
	s_totalUsers = (CStatTreeItemSimple*)tmpRoot1->AddChild(new CStatTreeItemSimple(wxTRANSLATE("Total Users: %i")));
	s_totalFiles = (CStatTreeItemSimple*)tmpRoot1->AddChild(new CStatTreeItemSimple(wxTRANSLATE("Total Files: %i")));
	s_serverOccupation = (CStatTreeItemSimple*)tmpRoot1->AddChild(new CStatTreeItemSimple(wxTRANSLATE("Server Occupation: %.2f%%")));
	s_serverOccupation->SetValue(0.0);

	tmpRoot1 = s_statTree->AddChild(new CStatTreeItemBase(wxTRANSLATE("Shared Files")));
	s_numberOfShared = (CStatTreeItemCounter*)tmpRoot1->AddChild(new CStatTreeItemCounter(wxTRANSLATE("Number of Shared Files: %s")));
	s_sizeOfShare = (CStatTreeItemCounter*)tmpRoot1->AddChild(new CStatTreeItemCounter(wxTRANSLATE("Total size of Shared Files: %s")));
	s_sizeOfShare->SetDisplayMode(dmBytes);
	tmpRoot1->AddChild(new CStatTreeItemAverage(wxTRANSLATE("Average filesize: %s"), s_sizeOfShare, s_numberOfShared, dmBytes));
}


void CStatistics::UpdateStatsTree()
{
	// get sort orders right
	s_sessionUpload->ReSortChildren();
	s_sessionDownload->ReSortChildren();
	s_clients->ReSortChildren();
	s_foundSources->ReSortChildren();
	// TODO: sort OS_Info subtrees.

	s_avgConnections->SetValue(theApp.listensocket->GetAverageConnections());

#if 0
	(*cli13) = wxString::Format(_("LowID: %u (%.2f%% Total %.2f%% Known)"),#lowid , (#total>0)?((double)100*#lowid/#total):0, (double)100*#knownLowID/#known);
	(*cli14) = wxString::Format(_("SecIdent On/Off: %u (%.2f%%) : %u (%.2f%%)"), #secOn , ((#eMule+#aMule)>0)?((double)100*#secOn / (#eMule+#aMule)):0, #secOff, ((#eMule+#aMule)>0)?((double)100*#secOff /(#eMule+#aMule) ):0);
#endif

	// get serverstats
	// TODO: make these realtime, too
	uint32 servfail;
	uint32 servuser;
	uint32 servfile;
	uint32 servtuser;
	uint32 servtfile;
	float servocc;
	theApp.serverlist->GetStatus(servfail, servuser, servfile, servtuser, servtfile, servocc);
	s_workingServers->SetValue((uint64)((*s_totalServers)-servfail));
	s_failedServers->SetValue((uint64)servfail);
	s_usersOnWorking->SetValue((uint64)servuser);
	s_filesOnWorking->SetValue((uint64)servfile);
	s_totalUsers->SetValue((uint64)servtuser);
	s_totalFiles->SetValue((uint64)servtfile);
	s_serverOccupation->SetValue(servocc);
}


void CStatistics::AddSourceOrigin(unsigned origin)
{
	CStatTreeItemNativeCounter* counter = (CStatTreeItemNativeCounter*)s_foundSources->GetChildById(0x0100 + origin);
	if (counter) {
		++(*counter);
	} else {
		counter = new CStatTreeItemNativeCounter(OriginToText(origin) + wxT(": %s"), stHideIfZero | stShowPercent);
		++(*counter);
		s_foundSources->AddChild(counter, 0x0100 + origin);
	}
}

void CStatistics::RemoveSourceOrigin(unsigned origin)
{
	CStatTreeItemNativeCounter* counter = (CStatTreeItemNativeCounter*)s_foundSources->GetChildById(0x0100 + origin);
	wxASSERT(counter);
	--(*counter);
}


// Yes, I know this is dirty, but I really don't want to #include "updownclient.h" here
#define	SO_EMULE		0
#define SO_AMULE		3
#define	SO_SHAREAZA		4
#define SO_HYDRANODE		6
#define	SO_NEW2_MLDONKEY	10
#define	SO_NEW_SHAREAZA		68
#define	SO_OLDEMULE		53
#define	SO_NEW_MLDONKEY		152

uint32 GetSoftID(uint8 SoftType)
{
	// prevent appearing multiple tree entries with the same name
	// this should be kept in sync with GetSoftName().
	switch (SoftType) {
		case SO_OLDEMULE:
			return 0x0100 + SO_EMULE;
		case SO_NEW_SHAREAZA:
			return 0x0100 + SO_SHAREAZA;
		case SO_NEW2_MLDONKEY:
			return 0x0100 + SO_NEW_MLDONKEY;
		default:
			return 0x0100 + SoftType;
	}
}

void CStatistics::AddDownloadFromSoft(uint8 SoftType, uint32 bytes)
{
	AddReceivedBytes(bytes);

	uint32 id = GetSoftID(SoftType);

	if (s_sessionDownload->HasChildWithId(id)) {
		(*((CStatTreeItemCounter*)s_sessionDownload->GetChildById(id))) += bytes;
	} else {
		CStatTreeItemCounter* tmp = new CStatTreeItemCounter(GetSoftName(SoftType) + wxT(": %s"));
		tmp->SetDisplayMode(dmBytes);
		(*tmp) += bytes;
		s_sessionDownload->AddChild(tmp, id);
	}
}

void CStatistics::AddUploadToSoft(uint8 SoftType, uint32 bytes)
{
	uint32 id = GetSoftID(SoftType);

	if (s_sessionUpload->HasChildWithId(id)) {
		(*((CStatTreeItemCounter*)s_sessionUpload->GetChildById(id))) += bytes;
	} else {
		CStatTreeItemCounter* tmp = new CStatTreeItemCounter(GetSoftName(SoftType) + wxT(": %s"));
		tmp->SetDisplayMode(dmBytes);
		(*tmp) += bytes;
		s_sessionUpload->AddChild(tmp, id);
	}
}

inline bool SupportsOSInfo(unsigned clientSoft)
{
	return (clientSoft == SO_AMULE) || (clientSoft == SO_HYDRANODE);
}

// Do some random black magic to strings to get a relatively unique number for them.
uint32 GetIdFromString(const wxString& str)
{
	uint32 id = 0;
	for (unsigned i = 0; i < str.Length(); ++i) {
                unsigned old_id = id;
                id += (uint32)str.GetChar(i);
                id <<= 2;
                id ^= old_id;
                id -= old_id;
        }
	return ((id >> 1) + id | 0x00000100) & 0x7fffffff;
}

void CStatistics::AddKnownClient(CUpDownClient *pClient, uint32 clientSoft, uint32 clientVersion)
{
	++(*s_clients);

	uint32 id = GetSoftID(clientSoft);

	CStatTreeItemCounter *client;

	if (s_clients->HasChildWithId(id)) {
		client = (CStatTreeItemCounter*)s_clients->GetChildById(id);
		++(*client);
	} else {
		uint32 flags = stSortChildren | stShowPercent | stHideIfZero;
		if (!SupportsOSInfo(clientSoft)) {
			flags |= stCapChildren;
		}
		client = new CStatTreeItemCounter(GetSoftName(clientSoft) + wxT(": %s"), flags);
		++(*client);
		s_clients->AddChild(client, id);
		if (SupportsOSInfo(clientSoft)) {
			client->AddChild(new CStatTreeItemBase(wxTRANSLATE("Version"), stSortChildren | stCapChildren), 2);
			client->AddChild(new CStatTreeItemBase(wxTRANSLATE("Operating System"), stSortChildren | stSortByValue), 1);
		}
	}

	CStatTreeItemBase *versionRoot = SupportsOSInfo(clientSoft) ? client->GetChildById(2) : client;

	if (versionRoot->HasChildWithId(clientVersion)) {
		CStatTreeItemCounter *version = (CStatTreeItemCounter*)versionRoot->GetChildById(clientVersion);
		++(*version);
	} else {
		wxString versionStr;
		GetClientDetails(pClient, NULL, &versionStr, NULL);
		if (versionStr.IsEmpty()) {
			versionStr = wxTRANSLATE("Unknown");
		}
		CStatTreeItemCounter *version = new CStatTreeItemCounter(versionStr + wxT(": %s"), stShowPercent | stHideIfZero);
		++(*version);
		versionRoot->AddChild(version, clientVersion, SupportsOSInfo(clientSoft));
	}

	if (SupportsOSInfo(clientSoft)) {
		wxString OSInfo = GetClientDetails(pClient, NULL, NULL, NULL);
		uint32 OS_ID;
		if (OSInfo.IsEmpty()) {
			OSInfo = wxTRANSLATE("Not Received");
			OS_ID = 0;
		} else {
			OS_ID = GetIdFromString(OSInfo);
		}
		CStatTreeItemBase* OSRoot = client->GetChildById(1);
		CStatTreeItemCounter* OSNode = (CStatTreeItemCounter*)OSRoot->GetChildById(OS_ID);
		if (OSNode) {
			++(*OSNode);
		} else {
			OSNode = new CStatTreeItemCounter(OSInfo + wxT(": %s"), stShowPercent | stHideIfZero);
			++(*OSNode);
			OSRoot->AddChild(OSNode, OS_ID, true);
		}
	}
}

void CStatistics::RemoveKnownClient(uint32 clientSoft, uint32 clientVersion, const wxString& OSInfo)
{
	--(*s_clients);

	uint32 id = GetSoftID(clientSoft);

	CStatTreeItemCounter *client = (CStatTreeItemCounter*)s_clients->GetChildById(id);
	wxASSERT(client);
	--(*client);

	CStatTreeItemBase *versionRoot = SupportsOSInfo(clientSoft) ? client->GetChildById(2) : client;

	CStatTreeItemCounter *version = (CStatTreeItemCounter*)versionRoot->GetChildById(clientVersion);
	wxASSERT(version);
	--(*version);

	if (SupportsOSInfo(clientSoft)) {
		uint32 OS_ID = OSInfo.IsEmpty() ? 0 : GetIdFromString(OSInfo);
		CStatTreeItemCounter* OSNode = (CStatTreeItemCounter*)client->GetChildById(1)->GetChildById(OS_ID);
		wxASSERT(OSNode);
		--(*OSNode);
	}
}

#else /* EC_REMOTE (CLIENT_GUI) */

CStatistics::CStatistics(CRemoteConnect* conn)
	: m_conn(conn)
{
	s_start_time = GetTickCount64();

	// Init Tree
	s_statTree = new CStatTreeItemBase(_("Statistics"));

	// Clear stat data container
	for (int i = 0; i < sdTotalItems; ++i) {
		s_statData[i] = 0;
	}
}


CStatistics::~CStatistics()
{
	delete s_statTree;
}


void CStatistics::UpdateStats(CECPacket* stats)
{
	s_statData[sdUpload] = stats->GetTagByNameSafe(EC_TAG_STATS_UL_SPEED)->GetInt32Data();
	s_statData[sdUpOverhead] = stats->GetTagByNameSafe(EC_TAG_STATS_UP_OVERHEAD)->GetInt32Data();
	s_statData[sdDownload] = stats->GetTagByNameSafe(EC_TAG_STATS_DL_SPEED)->GetInt32Data();
	s_statData[sdDownOverhead] = stats->GetTagByNameSafe(EC_TAG_STATS_DOWN_OVERHEAD)->GetInt32Data();
	s_statData[sdWaitingClients] = stats->GetTagByNameSafe(EC_TAG_STATS_UL_QUEUE_LEN)->GetInt32Data();
	s_statData[sdBannedClients] = stats->GetTagByNameSafe(EC_TAG_STATS_BANNED_COUNT)->GetInt32Data();
};


void CStatistics::UpdateStatsTree()
{
	CECPacket request(EC_OP_GET_STATSTREE);
	if (thePrefs::GetMaxClientVersions() != 0) {
		request.AddTag(CECTag(EC_TAG_STATTREE_CAPPING, (uint8)thePrefs::GetMaxClientVersions()));
	}
	CECPacket* reply = m_conn->SendRecv(&request);
	if (reply) {
		CECTag* treeRoot = reply->GetTagByName(EC_TAG_STATTREE_NODE);
		if (treeRoot) {
			delete s_statTree;
			s_statTree = new CStatTreeItemBase(treeRoot);
		}
	}
	delete reply;
}

#endif /* !EC_REMOTE */
