//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

#include <wx/thread.h>		// Needed for wxThread
#include "types.h"
#include "CTypedPtrList.h"

class CEMSocket;

class UploadBandwidthThrottler :
    public wxThread 
{
public:
    UploadBandwidthThrottler(void);
    ~UploadBandwidthThrottler(void);

    uint64 GetNumberOfSentBytesSinceLastCallAndReset();
    uint64 GetNumberOfSentBytesExcludingOverheadSinceLastCallAndReset();
    uint32 GetHighestNumberOfFullyActivatedSlotsSinceLastCallAndReset();

    void AddToStandardList(uint32 index, CEMSocket* socket);
    void RemoveFromStandardList(CEMSocket* socket);

    void QueueForSendingControlPacket(CEMSocket* socket);
    void RemoveFromAllQueues(CEMSocket* socket);

    void EndThread();

    void SetAllowedDataRate(uint32 newValue);
private:
    static UINT RunProc(LPVOID pParam);
    UINT RunInternal();

    void RemoveFromStandardListNoLock(CEMSocket* socket);

    CTypedPtrList<CPtrList, CEMSocket*> m_ControlQueue_list; // a queue for all the sockets that want to have Send() called on them.
    CTypedPtrList<CPtrList, CEMSocket*> m_TempControlQueue_list; // sockets that wants to enter m_ControlQueue_list

    CArray<CEMSocket*, CEMSocket*> m_StandardOrder_list; // sockets that have upload slots. Ordered so the most prioritized socket is first

    wxCriticalSection sendLocker;
    wxCriticalSection tempQueueLocker;

    wxEvent* threadEndedEvent;

    uint64 m_SentBytesSinceLastCall;
    uint64 m_SentBytesSinceLastCallExcludingOverhead;
    uint32 m_highestNumberOfFullyActivatedSlots;

    uint32 m_allowedDataRate;
    bool doRun;
};

#endif /* UPLOADBANDWIDTHTHROTTLER_H */

