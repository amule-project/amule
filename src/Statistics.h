//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
// Copyright (C) 2005-2006Dévai Tamás ( gonosztopi@amule.org )
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

#ifndef STATISTICS_H
#define STATISTICS_H

#include "GetTickCount.h"	// Needed for GetTickCount64()
#include "StatTree.h"		// Needed for CStatTreeItem* classes


#include <deque>		// Needed for std::deque

enum StatsGraphType {
	GRAPH_INVALID = 0,
	GRAPH_DOWN,
	GRAPH_UP,
	GRAPH_CONN,
	GRAPH_KAD
};

typedef struct UpdateInfo {
	double timestamp;
	float downloads[3];
	float uploads[3];
	float connections[3];
	float kadnodes[3];
} GraphUpdateInfo;

typedef struct HistoryRecord {
	double		kBytesSent;
	double		kBytesReceived;
	float		kBpsUpCur;
	float		kBpsDownCur;
	double		sTimestamp;
	uint16		cntDownloads;
	uint16		cntUploads;
	uint16		cntConnections;
	uint16		kadNodesCur;
	uint64		kadNodesTotal;
} HR;


#ifndef EC_REMOTE

/**
 * Counts precise rate/average on added bytes/values.
 *
 * @note This class is MT-safe.
 */
class CPreciseRateCounter {
	friend class CStatistics;	// for playing dirty tricks to compute running average :P
 public:

	/**
	 * Constructor
	 *
	 * @param timespan Desired timespan for rate calculations.
	 * @param count_average Counts average instead of rate.
	 */
	CPreciseRateCounter(uint32_t timespan, bool count_average = false)
		: m_timespan(timespan), m_total(0), m_rate(0.0), m_max_rate(0.0), m_tmp_sum(0), m_count_average(count_average)
		{
			if (!count_average) {
				uint64_t cur_time = GetTickCount64();
				uint64_t target_time = cur_time - timespan;
				while (cur_time > target_time) {
					m_tick_history.push_front(cur_time);
					m_byte_history.push_front(0);
					cur_time -= 100;	// default update period
				}
				m_tick_history.push_front(cur_time);
			}
		}

	/**
	 * Calculate current rate.
	 *
	 * This function should be called reasonably often, to
	 * keep rates up-to-date, and prevent history growing
	 * to the skies.
	 */
	void	CalculateRate(uint64_t now);

	/**
	 * Get current rate.
	 *
	 * @return Current rate in bytes/second.
	 */
	double	GetRate()			{ wxMutexLocker lock(m_mutex); return m_rate; };

	/**
	 * Gets ever seen maximal rate.
	 *
	 * @return The maximal rate which occured.
	 */
	double	GetMaxRate()			{ wxMutexLocker lock(m_mutex); return m_max_rate; }

	/**
	 * Sets desired timespan for rate calculations.
	 *
	 * If new timespan is greater than the old was, then the change
	 * takes effect with time. The exact time needed for the change
	 * to take effect is new minus old value of the timespan.
	 *
	 * If the new timespan is lower than the old, the change takes
	 * effect immediately at the next call to CalculateRate().
	 */
	void	SetTimespan(uint32_t timespan)	{ wxMutexLocker lock(m_mutex); m_timespan = timespan; }

	/**
	 * Add bytes to be tracked for rate-counting.
	 */
	void	operator+=(uint32_t bytes)	{ wxMutexLocker lock(m_mutex); m_tmp_sum += bytes; }

 protected:

	std::deque<uint32>	m_byte_history;
	std::deque<uint64>	m_tick_history;
	uint32_t	m_timespan;
	uint32_t	m_total;
	double		m_rate;
	double		m_max_rate;
	uint32_t	m_tmp_sum;
	wxMutex		m_mutex;
	bool		m_count_average;
};


class CECTag;

/**
 * Stat tree item for rates/averages.
 */
class CStatTreeItemRateCounter : public CStatTreeItemBase, public CPreciseRateCounter {
 public:

	/**
	 * @see CStatTreeItemBase::CStatTreeItemBase, CPreciseRateCounter::CPreciseRateCounter
	 *
	 * @param show_maxrate If true, shows max rate instead of current rate.
	 */
	CStatTreeItemRateCounter(const wxString& label, bool show_maxrate, uint32_t timespan, bool count_average = false)
		: CStatTreeItemBase(label, stNone), CPreciseRateCounter(timespan, count_average), m_show_maxrate(show_maxrate)
		{}

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeItemBase::GetDisplayString()
	 */
	virtual wxString GetDisplayString() const;
#endif

 protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void	AddECValues(CECTag* tag) const;

	//! Whether to show max rate instead of actual rate.
	bool	m_show_maxrate;
};


/**
 * Stat tree item for Peak Connections.
 */
class CStatTreeItemPeakConnections : public CStatTreeItemBase {
 public:

	/**
	 * @see CStatTreeItemBase::CStatTreeItemBase
	 */
	CStatTreeItemPeakConnections(const wxString& label)
		: CStatTreeItemBase(label)
		{}

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeItemBase::GetDisplayString()
	 */
	virtual wxString GetDisplayString() const;
#endif

 protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void	AddECValues(CECTag* tag) const;
};


class CUpDownClient;

class CStatistics {
	friend class CStatisticsDlg;	// to access CStatistics::GetTreeRoot()
 public: 
	CStatistics();
	~CStatistics();

	/* Statistics graph functions */

	void	 RecordHistory();
	unsigned GetHistoryForWeb(unsigned cntPoints, double sStep, double *sStart, uint32 **graphData);
	unsigned GetHistory(unsigned cntPoints, double sStep, double sFinal, const std::vector<float *> &ppf, StatsGraphType which_graph);
	GraphUpdateInfo GetPointsForUpdate();

	/* Statistics tree functions */

	void UpdateStatsTree();

	/* Access to the tree */

	// uptime
	static	uint64	GetUptimeMillis() 			{ return s_uptime->GetTimerValue(); }
	static	uint64	GetUptimeSeconds()			{ return s_uptime->GetTimerSeconds(); }
	static	uint64	GetStartTime()				{ return s_uptime->GetTimerStart(); }

	// Upload
	static	uint64	GetSessionSentBytes()			{ return (*s_sessionUpload); }
	static	void	AddUpOverheadFileRequest(uint32 size)	{ (*s_fileReqUpOverhead) += size; (*s_upOverheadRate) += size; }
	static	void	AddUpOverheadSourceExchange(uint32 size){ (*s_sourceXchgUpOverhead) += size; (*s_upOverheadRate) += size; }
	static	void	AddUpOverheadServer(uint32 size)	{ (*s_serverUpOverhead) += size; (*s_upOverheadRate) += size; }
	static	void	AddUpOverheadKad(uint32 size)		{ (*s_kadUpOverhead) += size; (*s_upOverheadRate) += size; }
	static	void	AddUpOverheadOther(uint32 size)		{ (*s_totalUpOverhead) += size; (*s_upOverheadRate) += size; }
	static	double	GetUpOverheadRate()			{ return s_upOverheadRate->GetRate(); }
	static	void	AddSuccessfulUpload()			{ ++(*s_totalSuccUploads); }
	static	void	AddFailedUpload()			{ ++(*s_totalFailedUploads); }
	static	void	AddUploadTime(uint32 time)		{ (*s_totalUploadTime) += time; }
	static	void	AddUploadingClient()			{ ++(*s_activeUploads); }
	static	void	RemoveUploadingClient()			{ --(*s_activeUploads); }
	static	uint32	GetActiveUploadsCount()			{ return (*s_activeUploads); }
	static	void	AddWaitingClient()			{ ++(*s_waitingUploads); }
	static	void	RemoveWaitingClient()			{ --(*s_waitingUploads); }
	static	uint32	GetWaitingUserCount()			{ return (*s_waitingUploads); }
	static	double	GetUploadRate()				{ return s_uploadrate->GetRate(); }

	// Download
	static	uint64	GetSessionReceivedBytes()		{ return (*s_sessionDownload); }
	static	void	AddDownOverheadFileRequest(uint32 size)	{ (*s_fileReqDownOverhead) += size; (*s_downOverheadRate) += size; }
	static	void	AddDownOverheadSourceExchange(uint32 size){ (*s_sourceXchgDownOverhead) += size; (*s_downOverheadRate) += size; }
	static	void	AddDownOverheadServer(uint32 size)	{ (*s_serverDownOverhead) += size; (*s_downOverheadRate) += size; }
	static	void	AddDownOverheadKad(uint32 size)		{ (*s_kadDownOverhead) += size; (*s_downOverheadRate) += size; }
	static	void	AddDownOverheadOther(uint32 size)	{ (*s_totalDownOverhead) += size; (*s_downOverheadRate) += size; }
	static	double	GetDownOverheadRate()			{ return s_downOverheadRate->GetRate(); }
	static	void	AddFoundSource()			{ ++(*s_foundSources); }
	static	void	RemoveFoundSource()			{ --(*s_foundSources); }
	static	uint32	GetFoundSources()			{ return (*s_foundSources); }
	static	void	AddSourceOrigin(unsigned origin);
	static	void	RemoveSourceOrigin(unsigned origin);
	static	void	AddDownloadingSource()			{ ++(*s_activeDownloads); }
	static	void	RemoveDownloadingSource()		{ --(*s_activeDownloads); }
	static	uint32	GetDownloadingSources()			{ return (*s_activeDownloads); }
	static	double	GetDownloadRate()			{ return s_downloadrate->GetRate(); }

	// Connection
	static	CStatTreeItemTimer* GetServerConnectTimer()	{ return s_sinceConnected; }
	static	void	AddReconnect()				{ ++(*s_reconnects); }
	static	void	AddActiveConnection()			{ ++(*s_activeConnections); }
	static	void	RemoveActiveConnection()		{ --(*s_activeConnections); }
	static	uint32	GetActiveConnections()			{ return s_activeConnections->GetValue(); }
	static	uint32	GetPeakConnections()			{ return s_activeConnections->GetMaxValue(); }
	static	void	AddMaxConnectionLimitReached()		{ ++(*s_limitReached); }

	// Clients
	static	void	AddFilteredClient()			{ ++(*s_filtered); }
	static	void	AddUnknownClient()			{ ++(*s_unknown); }
	static	void	RemoveUnknownClient()			{ --(*s_unknown); }
	static	void	AddKnownClient(CUpDownClient *pClient);
	static	void	RemoveKnownClient(uint32 clientSoft, uint32 clientVersion, const wxString& OSInfo);
#ifdef __DEBUG__
	static	void	SocketAssignedToClient()		{ ++(*s_hasSocket); }
	static	void	SocketUnassignedFromClient()		{ --(*s_hasSocket); }
#endif
	static	uint32	GetBannedCount()			{ return (*s_banned); }
	static	void	AddBannedClient()			{ ++(*s_banned); }
	static	void	RemoveBannedClient()			{ --(*s_banned); }

	// Servers
	static	void	AddServer()				{ ++(*s_totalServers); }
	static	void	DeleteServer()				{ ++(*s_deletedServers); --(*s_totalServers); }
	static	void	DeleteAllServers()			{ (*s_deletedServers) += (*s_totalServers); (*s_totalServers) = 0; }
	static	void	AddFilteredServer()			{ ++(*s_filteredServers); }

	// Shared files
	static	void	ClearSharedFilesInfo()			{ (*s_numberOfShared) = 0; (*s_sizeOfShare) = 0; }
	static	void	AddSharedFile(uint64 size)		{ ++(*s_numberOfShared); (*s_sizeOfShare) += size; }
	static	void	RemoveSharedFile(uint64 size)		{ --(*s_numberOfShared); (*s_sizeOfShare) -= size; }
	static	uint32	GetSharedFileCount()			{ return (*s_numberOfShared); }

	// Kad nodes
	static void		AddKadNode()					{ ++s_kadNodesCur; }
	static void		RemoveKadNode()					{ --s_kadNodesCur; }
	

	// Other
	static	void	CalculateRates();

	static	void	AddReceivedBytes(uint32 bytes)
		{
			if (!s_sinceFirstTransfer->IsRunning()) {
				s_sinceFirstTransfer->StartTimer();
			}

			(*s_sessionDownload) += bytes;
			(*s_downloadrate) += bytes;
		}

	static	void	AddSentBytes(uint32 bytes)
		{
			if (!s_sinceFirstTransfer->IsRunning()) {
				s_sinceFirstTransfer->StartTimer();
			}

			(*s_sessionUpload) += bytes;
			(*s_uploadrate) += bytes;
		}

	static	void	AddDownloadFromSoft(uint8 SoftType, uint32 bytes);
	static	void	AddUploadToSoft(uint8 SoftType, uint32 bytes);

	// EC
	static	CECTag*	GetECStatTree(uint8 tree_capping_value)	{ return s_statTree->CreateECTag(tree_capping_value); }

	void SetAverageMinutes(uint8 minutes) { average_minutes = minutes; }
	
 private:
 	std::list<HR>	listHR;
	typedef std::list<HR>::iterator		listPOS;
	typedef std::list<HR>::reverse_iterator	listRPOS;

	/* Graph-related functions */

	void ComputeAverages(HR **pphr, listRPOS pos, unsigned cntFilled,
		double sStep, const std::vector<float *> &ppf, StatsGraphType which_graph);
 
	int GetPointsPerRange()
	{
		return (1280/2) - 80; // This used to be a calc. based on GUI width
	}

	/* Graphs-related vars */

	CPreciseRateCounter	m_graphRunningAvgDown;
	CPreciseRateCounter	m_graphRunningAvgUp;
	CPreciseRateCounter	m_graphRunningAvgKad;

	
	uint8 average_minutes;
	int	nHistRanges;
	int	bitsHistClockMask;
	int	nPointsPerRange;
	listPOS*	aposRecycle;

	HR hrInit;

	/* Rate/Average counters */
	static	CPreciseRateCounter*		s_upOverheadRate;
	static	CPreciseRateCounter*		s_downOverheadRate;
	static	CStatTreeItemRateCounter*	s_uploadrate;
	static	CStatTreeItemRateCounter*	s_downloadrate;

	/* Tree-related functions */
	
	static	void	InitStatsTree();

	static	CStatTreeItemBase*	GetTreeRoot()	{ return s_statTree; }

	/* Tree-related vars */

	// the tree
	static	CStatTreeItemBase*		s_statTree;

	// Uptime
	static	CStatTreeItemTimer*		s_uptime;

	// Upload
	static	CStatTreeItemUlDlCounter*	s_sessionUpload;
	static	CStatTreeItemPacketTotals*	s_totalUpOverhead;
	static	CStatTreeItemPackets*		s_fileReqUpOverhead;
	static	CStatTreeItemPackets*		s_sourceXchgUpOverhead;
	static	CStatTreeItemPackets*		s_serverUpOverhead;
	static	CStatTreeItemPackets*		s_kadUpOverhead;
	static	CStatTreeItemNativeCounter*	s_activeUploads;
	static	CStatTreeItemNativeCounter*	s_waitingUploads;
	static	CStatTreeItemCounter*		s_totalSuccUploads;
	static	CStatTreeItemCounter*		s_totalFailedUploads;
	static	CStatTreeItemCounter*		s_totalUploadTime;

	// Download
	static	CStatTreeItemUlDlCounter*	s_sessionDownload;
	static	CStatTreeItemPacketTotals*	s_totalDownOverhead;
	static	CStatTreeItemPackets*		s_fileReqDownOverhead;
	static	CStatTreeItemPackets*		s_sourceXchgDownOverhead;
	static	CStatTreeItemPackets*		s_serverDownOverhead;
	static	CStatTreeItemPackets*		s_kadDownOverhead;
	static	CStatTreeItemNativeCounter*	s_foundSources;
	static	CStatTreeItemNativeCounter*	s_activeDownloads;

	// Connection
	static	CStatTreeItemReconnects*	s_reconnects;
	static	CStatTreeItemTimer*		s_sinceFirstTransfer;
	static	CStatTreeItemTimer*		s_sinceConnected;
	static	CStatTreeItemCounterMax*	s_activeConnections;
	static	CStatTreeItemMaxConnLimitReached* s_limitReached;
	static	CStatTreeItemSimple*		s_avgConnections;

	// Clients
	static	CStatTreeItemHiddenCounter*	s_clients;
	static	CStatTreeItemCounter*  		s_unknown;
	//static	CStatTreeItem			s_lowID;
	//static	CStatTreeItem			s_secIdentOnOff;
#ifdef __DEBUG__
	static	CStatTreeItemNativeCounter*	s_hasSocket;
#endif
	static	CStatTreeItemNativeCounter*	s_filtered;
	static	CStatTreeItemNativeCounter*	s_banned;

	// Servers
	static	CStatTreeItemSimple*		s_workingServers;
	static	CStatTreeItemSimple*		s_failedServers;
	static	CStatTreeItemNativeCounter*	s_totalServers;
	static	CStatTreeItemNativeCounter*	s_deletedServers;
	static	CStatTreeItemNativeCounter*	s_filteredServers;
	static	CStatTreeItemSimple*		s_usersOnWorking;
	static	CStatTreeItemSimple*		s_filesOnWorking;
	static	CStatTreeItemSimple*		s_totalUsers;
	static	CStatTreeItemSimple*		s_totalFiles;
	static	CStatTreeItemSimple*		s_serverOccupation;

	// Shared files
	static	CStatTreeItemCounter*		s_numberOfShared;
	static	CStatTreeItemCounter*		s_sizeOfShare;

	// Kad nodes
	static uint64 s_kadNodesTotal;
	static uint16 s_kadNodesCur;
};

#else /* EC_REMOTE == CLIENT_GUI */

class CECPacket;
class CRemoteConnect;

enum StatDataIndex {
	sdUpload,
	sdUpOverhead,
	sdDownload,
	sdDownOverhead,
	sdWaitingClients,
	sdBannedClients,

	sdTotalItems
};

class CStatistics {
	friend class CStatisticsDlg;	// to access CStatistics::GetTreeRoot()

private:
	CRemoteConnect &m_conn;
	static CStatTreeItemBase* s_statTree;
	static uint64 s_start_time;
	static uint64 s_statData[sdTotalItems];
	uint8 average_minutes;
	
 public:
	CStatistics(CRemoteConnect &conn);
	~CStatistics();

	static	uint64	GetUptimeMillis()			{ return GetTickCount64() - s_start_time; }
	static	uint64	GetUptimeSeconds()			{ return (GetTickCount64() - s_start_time) / 1000; }

	static	uint64	GetSessionSentBytes()			{ return 0; } // TODO
	static	double	GetUploadRate()				{ return (double)s_statData[sdUpload]; }
	static	double	GetUpOverheadRate()			{ return (double)s_statData[sdUpOverhead]; }

	static	uint64	GetSessionReceivedBytes()		{ return 0; } // TODO
	static	double	GetDownloadRate()			{ return (double)s_statData[sdDownload]; }
	static	double	GetDownOverheadRate()			{ return (double)s_statData[sdDownOverhead]; }

	static	uint32	GetWaitingUserCount()			{ return s_statData[sdWaitingClients]; }
	static	uint32	GetBannedCount()			{ return s_statData[sdBannedClients]; }

	static	uint32	GetSharedFileCount()			{ return 0; } // TODO

	static	void	UpdateStats(const CECPacket* stats);

		void	UpdateStatsTree();
		void	SetAverageMinutes(uint8 minutes)	{ average_minutes = minutes; }
	
 private:
	static	CStatTreeItemBase*	GetTreeRoot()		{ return s_statTree; }
};

#endif /* !EC_REMOTE / EC_REMOTE */


/**
 * Shortcut for CStatistics
 */
typedef	CStatistics	theStats;

#endif // STATISTICS_H
// File_checked_for_headers
