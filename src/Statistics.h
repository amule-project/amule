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

#ifndef STATISTICS_H
#define STATISTICS_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "Statistics.h"
#endif

#include "tree.hh"
#include "CTypedPtrList.h"
#include "Types.h"
#include "GetTickCount.h"
#include "updownclient.h"

#include <wx/string.h>

#include <deque>


typedef tree<wxString> StatsTree;
typedef StatsTree::iterator StatsTreeNode;
typedef StatsTree::sibling_iterator StatsTreeSiblingIterator;	

typedef struct {
	StatsTreeNode TreeItem;
	bool active;
} StatsTreeVersionItem;
	
enum StatsGraphType {
	GRAPH_INVALID = 0,
	GRAPH_DOWN,
	GRAPH_UP,
	GRAPH_CONN
};

typedef struct UpdateInfo {
	double timestamp;
	float downloads[3];
	float uploads[3];
	float connections[3];
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
} HR;

class CStatistics {

public: 

	CStatistics();
	
	~CStatistics();

	/* Statistics graph functions */

	void		RecordHistory();
	unsigned GetHistoryForWeb(unsigned cntPoints, double sStep, double *sStart, uint32 **graphData);
	unsigned GetHistory(unsigned cntPoints, double sStep, double sFinal, float **ppf, StatsGraphType which_graph);
	GraphUpdateInfo GetPointsForUpdate();

	/* Statistics tree functions */
	
	void UpdateStatsTree();

	/* Statistics tree vars */
	
	StatsTree statstree; // Tree is public 'cos we want to retrieve to send stats tree.

	/* Common functions inlined for speed */
	/* Called VERY often */

	uint64 GetSessionReceivedBytes() {
		return stat_sessionReceivedBytes;
	}

	uint64 GetSessionSentBytes() {
		return stat_sessionSentBytes;
	}

	void SetServerConnectTime(uint64 conn_time) {
		stat_serverConnectTime = conn_time;
	}	
	
	void AddReconnect() { stat_reconnects++; };
	
	void AddFilteredClient() { stat_filteredclients++; };
	
	// Updates the number of received bytes and marks when transfers first began
	void UpdateReceivedBytes(int32 bytesToAdd) {
		if (!stat_transferStarttime) {
			// Saves the time where transfers were started and calucated the time before
			stat_transferStarttime = GetTickCount64();
			sTransferDelay = (stat_transferStarttime - Start_time)/1000.0;			
		}
		stat_sessionReceivedBytes += bytesToAdd;
	}


	// Updates the number of received bytes and marks when transfers first began
	void UpdateSentBytes(int32 bytesToAdd) {
		if (!stat_transferStarttime) {
			// Saves the time where transfers were started and calucated the time before
			stat_transferStarttime = GetTickCount64();
			sTransferDelay = (stat_transferStarttime - Start_time)/1000.0;			
		}
		stat_sessionSentBytes += bytesToAdd;
	}

	// Returns the uptime in millie-seconds
	uint64 GetUptimeMsecs() {
		return GetTickCount64() - Start_time;
	}


	// Returns the uptime in seconds
	uint32 GetUptimeSecs() {
		return GetUptimeMsecs() / 1000;
	}
	
	void AddDownloadFromSoft(uint8 SoftType, uint32 bytes);
	
	// Download related
	void	AddDownDataOverheadSourceExchange(uint32 data)	{ m_nDownDataRateMSOverhead += data;
															  m_nDownDataOverheadSourceExchange += data;
															  m_nDownDataOverheadSourceExchangePackets++;}
	void	AddDownDataOverheadFileRequest(uint32 data)		{ m_nDownDataRateMSOverhead += data;
															  m_nDownDataOverheadFileRequest += data;
															  m_nDownDataOverheadFileRequestPackets++;}
	void	AddDownDataOverheadServer(uint32 data)			{ m_nDownDataRateMSOverhead += data;
															  m_nDownDataOverheadServer += data;
															  m_nDownDataOverheadServerPackets++;}
	void	AddDownDataOverheadOther(uint32 data)			{ m_nDownDataRateMSOverhead += data;
															  m_nDownDataOverheadOther += data;
															  m_nDownDataOverheadOtherPackets++;}
	
	double	GetDownDatarateOverhead()			{return m_nDownDatarateOverhead;}
	uint64	GetDownDataOverheadSourceExchange()		{return m_nDownDataOverheadSourceExchange;}
	uint64	GetDownDataOverheadFileRequest()		{return m_nDownDataOverheadFileRequest;}
	uint64	GetDownDataOverheadServer()			{return m_nDownDataOverheadServer;}
	uint64	GetDownDataOverheadOther()			{return m_nDownDataOverheadOther;}
	uint64	GetDownDataOverheadSourceExchangePackets()	{return m_nDownDataOverheadSourceExchangePackets;}
	uint64	GetDownDataOverheadFileRequestPackets()		{return m_nDownDataOverheadFileRequestPackets;}
	uint64	GetDownDataOverheadServerPackets()		{return m_nDownDataOverheadServerPackets;}
	uint64	GetDownDataOverheadOtherPackets()		{return m_nDownDataOverheadOtherPackets;}
	void	CompDownDatarateOverhead();

	// Upload related
	void	AddUpDataOverheadSourceExchange(uint32 data)	{ m_nUpDataRateMSOverhead += data;
															  m_nUpDataOverheadSourceExchange += data;
															  m_nUpDataOverheadSourceExchangePackets++;}
	void	AddUpDataOverheadFileRequest(uint32 data)		{ m_nUpDataRateMSOverhead += data;
															  m_nUpDataOverheadFileRequest += data;
															  m_nUpDataOverheadFileRequestPackets++;}
	void	AddUpDataOverheadServer(uint32 data)			{ m_nUpDataRateMSOverhead += data;
															  m_nUpDataOverheadServer += data;
															  m_nUpDataOverheadServerPackets++;}
	void	AddUpDataOverheadOther(uint32 data)				{ m_nUpDataRateMSOverhead += data;
															  m_nUpDataOverheadOther += data;
															  m_nUpDataOverheadOtherPackets++;}
	double	GetUpDatarateOverhead()						{return m_nUpDatarateOverhead;}
	uint64	GetUpDataOverheadSourceExchange()			{return m_nUpDataOverheadSourceExchange;}
	uint64	GetUpDataOverheadFileRequest()				{return m_nUpDataOverheadFileRequest;}
	uint64	GetUpDataOverheadServer()					{return m_nUpDataOverheadServer;}
	uint64	GetUpDataOverheadOther()					{return m_nUpDataOverheadOther;}
	uint64	GetUpDataOverheadSourceExchangePackets()	{return m_nUpDataOverheadSourceExchangePackets;}
	uint64	GetUpDataOverheadFileRequestPackets()		{return m_nUpDataOverheadFileRequestPackets;}
	uint64	GetUpDataOverheadServerPackets()			{return m_nUpDataOverheadServerPackets;}
	uint64	GetUpDataOverheadOtherPackets()				{return m_nUpDataOverheadOtherPackets;}
	void	CompUpDatarateOverhead();
private:

	/* Graph-related functions */

	void ComputeAverages(HR **pphr, POSITION pos, unsigned cntFilled, double sStep, float **ppf, StatsGraphType which_graph);

	// ComputeSessionAvg and ComputeRunningAvg are used to assure consistent computations across
	// RecordHistory and ComputeAverages; see note in RecordHistory on the use of double and float 
	void ComputeSessionAvg(float& kBpsSession, float& kBpsCur, double& kBytesTrans, double& sCur, double& sTrans);

	void ComputeRunningAvg(float& kBpsRunning, float& kBpsSession, double& kBytesTrans, 
							double& kBytesTransPrev, double& sTrans, double& sPrev, float& sAvg);
	
	int GetPointsPerRange(){
		return (1280/2) - 80; // This used to be a calc. based on GUI width
	};
	
	void VerifyHistory(bool bMsgIfOk = false);

	/* Graphs-related vars */
	float kBpsUpCur;
	float kBpsUpAvg;
	float kBpsUpSession;
	float kBpsDownCur;
	float kBpsDownAvg;
	float kBpsDownSession;
	float maxDownavg;
	float maxDown;
		
	
 	CList<HR,HR>	listHR;	
	int	nHistRanges;
	int	bitsHistClockMask;
	int nPointsPerRange;
	POSITION*		aposRecycle;	

	HR hrInit;


	/* Tree-related functions */
	
	void InitStatsTree();

	/* Tree-related vars */

	uint64		Start_time;
	double		sTransferDelay;
	uint64		stat_sessionReceivedBytes;
	uint64		stat_sessionSentBytes;
	uint32		stat_reconnects;
	uint64		stat_transferStarttime;
	uint64		stat_serverConnectTime;
	uint32		stat_filteredclients;
	uint32		m_ilastMaxConnReached;


	StatsTreeNode h_shared,h_transfer,h_connection,h_clients,h_servers,h_upload,h_download,h_uptime;
	StatsTreeNode down1,down2,down3,down4,down5,down6,down7;
	StatsTreeNode	down1_1, down1_2, down1_3, down1_4, down1_5, down1_6, down1_7, down1_8;
	StatsTreeNode up1,up2,up3,up4,up5,up6,up7,up8,up9,up10;
	StatsTreeNode tran0;
	StatsTreeNode con1,con2,con3,con4,con5,con6,con7,con8,con9,con10,con11,con12,con13;
	StatsTreeNode shar1,shar2,shar3;
	StatsTreeNode cli1,cli2,cli3,cli4,cli5,cli6,cli7,cli8,cli9,cli10, cli10_1, cli10_2, cli11, cli12,cli13,cli14,cli15,cli16,cli17;	
	StatsTreeNode srv1,srv2,srv3,srv4,srv5,srv6,srv7,srv8,srv9;
	
	StatsTreeVersionItem cli_versions[18];
	
	/* Common functions inlined for speed */
	/* Called VERY often */

	// Returns the amount of time where transfers have been going on
	uint32 GetTransferSecs() {
		return ( GetTickCount64() - stat_transferStarttime ) / 1000;
	}

	// Returns the amount of time where we've been connected to a server
	uint32 GetServerSecs() {
		return ( GetTickCount64() - stat_serverConnectTime) / 1000;
	}
	
	
	// Download-related vars.
	long		m_nDownDatarateTotal;
	double		m_nDownDatarateOverhead;
	uint32		m_nDownDataRateMSOverhead;
	uint64		m_nDownDataOverheadSourceExchange;
	uint64		m_nDownDataOverheadSourceExchangePackets;
	uint64		m_nDownDataOverheadFileRequest;
	uint64		m_nDownDataOverheadFileRequestPackets;
	uint64		m_nDownDataOverheadServer;
	uint64		m_nDownDataOverheadServerPackets;
	uint64		m_nDownDataOverheadOther;
	uint64		m_nDownDataOverheadOtherPackets;
	std::deque<int>	m_AverageDDRO_list;
	
	uint64 downloaded_aMule;
	uint64 downloaded_eMule;
	uint64 downloaded_eDonkey;
	uint64 downloaded_eDonkeyHybrid;
	uint64 downloaded_Shareaza;
	uint64 downloaded_MLDonkey;
	uint64 downloaded_lxMule;
	uint64 downloaded_Other;

	// Upload-related vars.
	long	m_nUpDatarateTotal;
	double	m_nUpDatarateOverhead;
	uint32	m_nUpDataRateMSOverhead;
	uint64	m_nUpDataOverheadSourceExchange;
	uint64	m_nUpDataOverheadFileRequest;
	uint64	m_nUpDataOverheadServer;
	uint64	m_nUpDataOverheadOther;
	uint64	m_nUpDataOverheadSourceExchangePackets;
	uint64	m_nUpDataOverheadFileRequestPackets;
	uint64	m_nUpDataOverheadServerPackets;
	uint64	m_nUpDataOverheadOtherPackets;
	std::deque<int>	m_AverageUDRO_list;
};

#endif // STATISTICS_H
