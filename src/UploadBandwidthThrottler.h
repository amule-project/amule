//
// This file is part of the aMule Project.
// 
// Copyright (c) 2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef UPLOADBANDWIDTHTHROTTLER_H
#define UPLOADBANDWIDTHTHROTTLER_H


#include <wx/thread.h>

#include <deque>

#include "Types.h"

class ThrottledControlSocket;
class ThrottledFileSocket;

class UploadBandwidthThrottler : public wxThread 
{
public:
    UploadBandwidthThrottler();
    ~UploadBandwidthThrottler();
    
	uint64 GetNumberOfSentBytesSinceLastCallAndReset();
    uint64 GetNumberOfSentBytesOverheadSinceLastCallAndReset();
    uint32 GetHighestNumberOfFullyActivatedSlotsSinceLastCallAndReset();
    
	uint32 GetStandardListSize();

    void AddToStandardList(uint32 index, ThrottledFileSocket* socket);
    bool RemoveFromStandardList(ThrottledFileSocket* socket);

    void QueueForSendingControlPacket(ThrottledControlSocket* socket, bool hasSent = false);
    void RemoveFromAllQueues(ThrottledControlSocket* socket);
    void RemoveFromAllQueues(ThrottledFileSocket* socket);

    void EndThread();
private:
    void DoRemoveFromAllQueues(ThrottledControlSocket* socket);
    bool RemoveFromStandardListNoLock(ThrottledFileSocket* socket);

    void* Entry();
	
    bool m_doRun;


    wxMutex m_sendLocker;
    wxMutex m_tempQueueLocker;
	
	typedef std::deque<ThrottledControlSocket*> SocketQueue;
	
	// a queue for all the sockets that want to have Send() called on them.
    SocketQueue m_ControlQueue_list;
	// a queue for all the sockets that want to have Send() called on them.
    SocketQueue m_ControlQueueFirst_list;
	// sockets that wants to enter m_ControlQueue_list 
    SocketQueue m_TempControlQueue_list;
	// sockets that wants to enter m_ControlQueue_list and has been able to send before 
    SocketQueue m_TempControlQueueFirst_list; 


	typedef std::deque<ThrottledFileSocket*> FileSocketQueue;
	// sockets that have upload slots. Ordered so the most prioritized socket is first
    FileSocketQueue m_StandardOrder_list; 

    uint64 m_SentBytesSinceLastCall;
    uint64 m_SentBytesSinceLastCallOverhead;
    uint32 m_highestNumberOfFullyActivatedSlots;
};


#endif
