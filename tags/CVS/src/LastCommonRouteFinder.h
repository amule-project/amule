// this file is part of aMule
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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
#pragma once

class CServer;
class CUpDownClient;
typedef CTypedPtrList<CPtrList, CUpDownClient*> CUpDownClientPtrList;

struct CurrentPingStruct {
	//uint32	datalen;
	uint32	latency;
	uint32	lowest;
};

class LastCommonRouteFinder :
    public CWinThread 
{
public:
    LastCommonRouteFinder();
    ~LastCommonRouteFinder();

    void EndThread();

    bool AddHostsToCheck(CTypedPtrList<CPtrList, CServer*> &list);
    bool AddHostsToCheck(CUpDownClientPtrList &list);

    //uint32 GetPingedHost();
    CurrentPingStruct GetCurrentPing();
    bool AcceptNewClient();

    void SetPrefs(bool pEnabled, uint32 pCurUpload, uint32 pMinUpload, uint32 pMaxUpload, double pPingTolerance, uint32 pGoingUpDivider, uint32 pGoingDownDivider, uint32 pNumberOfPingsForAverage, uint64 pLowestInitialPingAllowed);

    uint32 GetUpload();
private:
    static UINT RunProc(LPVOID pParam);
    UINT RunInternal();

    void SetUpload(uint32 newValue);

    bool doRun;
    bool acceptNewClient;
    bool m_enabled;
    bool needMoreHosts;

    CCriticalSection addHostLocker;
    CCriticalSection prefsLocker;
    CCriticalSection uploadLocker;
    CCriticalSection pingLocker;

    CEvent* threadEndedEvent;
    CEvent* newTraceRouteHostEvent;
    CEvent* prefsEvent;

    CList<uint32,uint32> hostsToTraceRoute;

    CList<uint32,uint32> pingDelays;
    uint64 pingDelaysTotal;

    uint32 minUpload;
    uint32 maxUpload;
    uint32 m_CurUpload;
    uint32 m_upload;

    double m_pingTolerance;
    uint32 m_goingUpDivider;
    uint32 m_goingDownDivider;
    uint32 m_iNumberOfPingsForAverage;

    uint32 m_pingAverage;
    uint32 m_lowestPing;
    uint64 m_LowestInitialPingAllowed;
};
