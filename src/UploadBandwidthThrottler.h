//
// This file is part of eMule
// 
// Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef UPLOADBANDWIDTHTHROTTLER_H
#define UPLOADBANDWIDTHTHROTTLER_H


#include <wx/thread.h>

#include <deque>

#include "types.h"

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

    void QueueForSendingControlPacket(ThrottledControlSocket* socket, bool hasSent = false);
    void RemoveFromAllQueues(ThrottledControlSocket* socket);
    void RemoveFromAllQueues(ThrottledFileSocket* socket);

    void SetAllowedDataRate(uint32 newValue);

    void AddToStandardList(uint32 index, ThrottledFileSocket* socket);
    bool RemoveFromStandardList(ThrottledFileSocket* socket);


    void Pause(bool paused);
    void EndThread();
	
private:
    void DoRemoveFromAllQueues(ThrottledControlSocket* socket);
    bool RemoveFromStandardListNoLock(ThrottledFileSocket* socket);

    void* Entry();
	
    bool m_doRun;


    wxMutex m_sendLocker;
    wxMutex m_tempQueueLocker;
    wxMutex m_pauseLocker;
	
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

    uint32 m_allowedDataRate;
};


#endif
