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

#include "amule.h"
#include "UploadBandwidthThrottler.h"
#include "EMSocket.h"
#include "opcodes.h"
#include "UploadQueue.h"
#include "LastCommonRouteFinder.h"
#include "OtherFunctions.h"
#include "amuledlg.h"

#if 0
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#endif

/**
 * The constructor starts the thread.
 */
UploadBandwidthThrottler::UploadBandwidthThrottler(void) {
    m_SentBytesSinceLastCall = 0;
    m_SentBytesSinceLastCallExcludingOverhead = 0;
    m_highestNumberOfFullyActivatedSlots = 0;

    threadEndedEvent = new CEvent(0, 1);
    doRun = true;
    AfxBeginThread(RunProc, (LPVOID)this);
}

/**
 * The destructor stops the thread. If the thread has already stoppped, destructor does nothing.
 */
UploadBandwidthThrottler::~UploadBandwidthThrottler(void) {
    EndThread();
    
    delete threadEndedEvent;
}

void UploadBandwidthThrottler::SetAllowedDataRate(uint32 newValue) {
    sendLocker.Lock();

    m_allowedDataRate = newValue;

    sendLocker.Unlock();
}

/**
 * Find out how many bytes that has been put on the sockets since the last call to this
 * method. Includes overhead of control packets.
 *
 * @return the number of bytes that has been put on the sockets since the last call
 */
uint64 UploadBandwidthThrottler::GetNumberOfSentBytesSinceLastCallAndReset() {
    sendLocker.Lock();
    
    uint64 numberOfSentBytesSinceLastCall = m_SentBytesSinceLastCall;
    m_SentBytesSinceLastCall = 0;

    sendLocker.Unlock();

    return numberOfSentBytesSinceLastCall;
}

/**
 * Find out how many bytes that has been put on the sockets since the last call to this
 * method. Excludes overhead of control packets.
 *
 * @return the number of bytes that has been put on the sockets since the last call
 */
uint64 UploadBandwidthThrottler::GetNumberOfSentBytesExcludingOverheadSinceLastCallAndReset() {
    sendLocker.Lock();
    
    uint64 numberOfSentBytesSinceLastCall = m_SentBytesSinceLastCallExcludingOverhead;
    m_SentBytesSinceLastCallExcludingOverhead = 0;

    sendLocker.Unlock();

    return numberOfSentBytesSinceLastCall;
}

/**
 * Find out the highest number of slots that has been fed data in the normal standard loop
 * of the thread since the last call of this method. This means all slots that haven't
 * been in the trickle state during the entire time since the last call.
 *
 * @return the highest number of fully activated slots during any loop since last call
 */
uint32 UploadBandwidthThrottler::GetHighestNumberOfFullyActivatedSlotsSinceLastCallAndReset() {
    sendLocker.Lock();
    
    uint64 highestNumberOfFullyActivatedSlots = m_highestNumberOfFullyActivatedSlots;
    m_highestNumberOfFullyActivatedSlots = 0;

    sendLocker.Unlock();

    return highestNumberOfFullyActivatedSlots;
}

/**
 * Add a socket to the list of sockets that have upload slots. The main thread will
 * continously call send on these sockets, to give them chance to work off their queues.
 * The sockets are called in the order they exist in the list, so the top socket (index 0)
 * will be given a chance first to use bandwidth, and then the next socket (index 1) etc.
 *
 * It is possible to add a socket several times to the list without removing it inbetween,
 * but that should be avoided.
 *
 * @param index insert the socket at this place in the list. An index that is higher than the
 *              current number of sockets in the list will mean that the socket should be inserted
 *              last in the list.
 *
 * @param socket the address to the socket that should be added to the list. If the address is NULL,
 *               this method will do nothing.
 */
void UploadBandwidthThrottler::AddToStandardList(uint32 index, CEMSocket* socket) {
    if(socket != NULL) {
        sendLocker.Lock();

        RemoveFromStandardListNoLock(socket);

        if(index > (uint32)m_StandardOrder_list.GetSize()) {
            index = m_StandardOrder_list.GetSize();
        }
        m_StandardOrder_list.InsertAt(index, socket);

        sendLocker.Unlock();
    } else {
		//if (thePrefs.GetVerbose())
		//	theApp.emuledlg->AddDebugLogLine(true,"Tried to add NULL socket to UploadBandwidthThrottler Standard list! Prevented.");
    }
}

/**
 * Remove a socket from the list of sockets that have upload slots.
 *
 * If the socket has mistakenly been added several times to the list, this method
 * will return all of the entries for the socket.
 *
 * @param socket the address of the socket that should be removed from the list. If this socket
 *               does not exist in the list, this method will do nothing.
 */
void UploadBandwidthThrottler::RemoveFromStandardList(CEMSocket* socket) {
    sendLocker.Lock();

    RemoveFromStandardListNoLock(socket);

    sendLocker.Unlock();
}

/**
 * Remove a socket from the list of sockets that have upload slots. NOT THREADSAFE!
 * This is an internal method that doesn't take the necessary lock before it removes
 * the socket. This method should only be called when the current thread already owns
 * the sendLocker lock!
 *
 * @param socket address of the socket that should be removed from the list. If this socket
 *               does not exist in the list, this method will do nothing.
 */
void UploadBandwidthThrottler::RemoveFromStandardListNoLock(CEMSocket* socket) {
    // Find the slot
    int slotCounter = 0;
    bool foundSocket = false;
    while(slotCounter < m_StandardOrder_list.GetSize() && foundSocket == false) {
        if(m_StandardOrder_list.GetAt(slotCounter) == socket) {
            // Remove the slot
            m_StandardOrder_list.RemoveAt(slotCounter);
            foundSocket = true;
        } else {
            slotCounter++;
        }
    }
}

/**
* Notifies the send thread that it should try to call controlpacket send
* for the supplied socket. It is allowed to call this method several times
* for the same socket, without having controlpacket send called for the socket
* first. The doublette entries are never filtered, since it is incurs less cpu
* overhead to simply call Send() in the socket for each double. Send() will
* already have done its work when the second Send() is called, and will just
* return with little cpu overhead.
*
* @param socket address to the socket that requests to have controlpacket send
*               to be called on it
*/
void UploadBandwidthThrottler::QueueForSendingControlPacket(CEMSocket* socket) {
    // Get critical section
    tempQueueLocker.Lock();


    if(doRun) {
        m_TempControlQueue_list.AddTail(socket);
    }

    // End critical section
    tempQueueLocker.Unlock();
}

/**
 * Remove the socket from all lists and queues. This will make it safe to
 * erase/delete the socket. It will also cause the main thread to stop calling
 * send() for the socket.
 *
 * @param socket address to the socket that should be removed
 */
void UploadBandwidthThrottler::RemoveFromAllQueues(CEMSocket* socket) {
    // Get critical section
    sendLocker.Lock();

    if(doRun) {
        // Remove this socket from control packet queue
        {
            POSITION pos1, pos2;
	        for (pos1 = m_ControlQueue_list.GetHeadPosition();( pos2 = pos1 ) != NULL;) {
		        m_ControlQueue_list.GetNext(pos1);
		        CEMSocket* socketinQueue = m_ControlQueue_list.GetAt(pos2);

                if(socketinQueue == socket) {
                    m_ControlQueue_list.RemoveAt(pos2);
                }
            }
        }
        
        tempQueueLocker.Lock();
        {
            POSITION pos1, pos2;
	        for (pos1 = m_TempControlQueue_list.GetHeadPosition();( pos2 = pos1 ) != NULL;) {
		        m_TempControlQueue_list.GetNext(pos1);
		        CEMSocket* socketinQueue = m_TempControlQueue_list.GetAt(pos2);

                if(socketinQueue == socket) {
                    m_TempControlQueue_list.RemoveAt(pos2);
                }
            }
        }
        tempQueueLocker.Unlock();

        // And remove it from upload slots
        RemoveFromStandardListNoLock(socket);
    }

    // End critical section
    sendLocker.Unlock();
}

/**
 * Make the thread exit. This method will not return until the thread has stopped
 * looping. This guarantees that the thread will not access the CEMSockets after this
 * call has exited.
 */
void UploadBandwidthThrottler::EndThread() {
    sendLocker.Lock();

    // signal the thread to stop looping and exit.
    doRun = false;

    sendLocker.Unlock();

    // wait for the thread to signal that it has stopped looping.
    threadEndedEvent->Lock();
}

/**
 * Start the thread. Called from the constructor in this class.
 *
 * @param pParam
 *
 * @return
 */
UINT AFX_CDECL UploadBandwidthThrottler::RunProc(LPVOID pParam) {
	DbgSetThreadName("UploadBandwidthThrottler");
    UploadBandwidthThrottler* uploadBandwidthThrottler = (UploadBandwidthThrottler*)pParam;

    return uploadBandwidthThrottler->RunInternal();
}

/**
 * The thread method that handles calling send for the individual sockets.
 *
 * This method decides to which slot the currently available bandwidth chunk
 * will go to. There are several algorithms that could be inserted here. The
 * SlotFocus algorithm available in ZZUL tries to send as fast as possible to each
 * slot, before going to the next slot. The current experimental algorithm tries to
 * feed all slots an equal amount. If not all slots can get at least UPLOAD_CLIENT_DATARATE
 * each, the scheduler puts the last slots on trickle, and just gives UPLOAD_CLIENT_DATARATE
 * to as many slots as it has enough bandwidth for.
 *
 * Control packets will always be tried to be sent first.
 * 
 * Upload slots will not be allowed to go without having sent
 * called for more than a defined amount of time (i.e. two seconds).
 *
 * @return always returns 0.
 */
UINT UploadBandwidthThrottler::RunInternal() {
    DWORD lastLoopTick = ::GetTickCount();

    sint64 bytesToSpend = 0;

    uint32 allowedDataRate = 0;

    while(doRun) {
        DWORD timeSinceLastLoop = ::GetTickCount() - lastLoopTick;

#define TIME_BETWEEN_UPLOAD_LOOPS 10
        if(timeSinceLastLoop < TIME_BETWEEN_UPLOAD_LOOPS) {
            Sleep(TIME_BETWEEN_UPLOAD_LOOPS-timeSinceLastLoop);
        }

        sendLocker.Lock();

        // PENDING: This would be used if UploadSpeedSense wasn't there.
        //          This direct connection could be removed between UploadSpeedSense
        //          and the throttler, by moving the value via CUploadQueue::UploadTimer,
        //          but I haven't decided about that yet.
        // allowedDataRate = m_allowedDataRate;

        // Get current speed from UploadSpeedSense
        allowedDataRate = theApp.lastCommonRouteFinder->GetUpload();

        //uint32 minFragSize = 512;
        uint32 minFragSize = allowedDataRate / 50;
        if(minFragSize < 512) {
            minFragSize = 512;
        } else if(minFragSize > 2800) {
            minFragSize = 2800;
        }

        const DWORD thisLoopTick = ::GetTickCount();
        timeSinceLastLoop = thisLoopTick - lastLoopTick;
        if(timeSinceLastLoop > 1*1000) {
//			theApp.emuledlg->QueueDebugLogLine(false,"UploadBandwidthThrottler: Time since last loop too long (%i).", timeSinceLastLoop);

            timeSinceLastLoop = 1*1000;
            lastLoopTick = thisLoopTick - timeSinceLastLoop;
        }

        // Calculate how many bytes we can spend
        if(allowedDataRate != 0) {
            bytesToSpend += allowedDataRate*(thisLoopTick-lastLoopTick)/1000;
        } else {
            bytesToSpend = _I64_MAX;
        }

        //lastLoopTick = thisLoopTick;

        uint64 spentBytes = 0;
        uint64 spentOverhead = 0;

        tempQueueLocker.Lock();

        // are there any sockets in m_TempControlQueue_list? Move them to normal m_ControlQueue_list;
        while(!m_TempControlQueue_list.IsEmpty()) {
            CEMSocket* moveSocket = m_TempControlQueue_list.RemoveHead();
            m_ControlQueue_list.AddTail(moveSocket);
        }

        tempQueueLocker.Unlock();
        
        // Send any queued up control packets first
        while(bytesToSpend > 0 && spentBytes <= (uint64)bytesToSpend && !m_ControlQueue_list.IsEmpty()) {
            CEMSocket* socket = m_ControlQueue_list.RemoveHead();

            if(socket != NULL) {
                SocketSentBytes socketSentBytes = socket->Send(bytesToSpend-spentBytes, true);
                spentBytes += socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
                spentOverhead += socketSentBytes.sentBytesControlPackets;
            }
        }

        // Check if any sockets haven't gotten data for a long time. Then trickle them a package.
        for(uint32 slotCounter = 0; slotCounter < (uint32)m_StandardOrder_list.GetSize(); ++slotCounter) {
            CEMSocket* socket = m_StandardOrder_list.GetAt(slotCounter);

            if(socket != NULL) {
                if((thisLoopTick-socket->GetLastCalledSend()) > 1000) {
                    // trickle
                    SocketSentBytes socketSentBytes = socket->Send(512);
                    spentBytes += socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
                    spentOverhead += socketSentBytes.sentBytesControlPackets;
                }
            } else {
				theApp.emuledlg->QueueDebugLogLine(false,"UploadBandwidthThrottler: There was a NULL socket in the standard list (trickle)! Prevented usage. Index: %i Size: %i", slotCounter, m_StandardOrder_list.GetSize());
            }
        }

        uint32 leftoverDueToRounding = 0;

        // how many slots are fully saturated?
        uint32 currentFullyActivatedSlots = 0;

        if(bytesToSpend > 0 && (uint64)bytesToSpend > spentBytes) {
            // calc number of clients to feed equally
            uint64 numberOfClientsToFeed = 0;
            if(thisLoopTick-lastLoopTick > 0) {
                numberOfClientsToFeed = (uint64)(bytesToSpend-spentBytes)*1000/(UPLOAD_CLIENT_DATARATE*(thisLoopTick-lastLoopTick));
            }
            if(numberOfClientsToFeed > (uint64)m_StandardOrder_list.GetSize()) {
                numberOfClientsToFeed = m_StandardOrder_list.GetSize();
            }

            uint64 bytesPerClient = bytesToSpend-spentBytes;
            
            if(numberOfClientsToFeed > 1) {
                bytesPerClient = (bytesToSpend-spentBytes)/numberOfClientsToFeed;
                leftoverDueToRounding = (bytesToSpend-spentBytes)%numberOfClientsToFeed;
            }

            // "Full" speed sockets
            for(uint32 slotCounter = 0; slotCounter < (uint32)m_StandardOrder_list.GetSize() &&  bytesToSpend > leftoverDueToRounding && spentBytes <= (uint64)bytesToSpend-leftoverDueToRounding; ++slotCounter) {
                CEMSocket* socket = m_StandardOrder_list.GetAt(slotCounter);

                if(socket != NULL) {
                    SocketSentBytes socketSentBytes = socket->Send(min(bytesPerClient, bytesToSpend-leftoverDueToRounding-spentBytes));
                    spentBytes += socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
                    spentOverhead += socketSentBytes.sentBytesControlPackets;
                } else {
					theApp.emuledlg->QueueDebugLogLine(false,"UploadBandwidthThrottler: There was a NULL socket in the standard list (full)! Prevented usage. Index: %i Size: %i", slotCounter, m_StandardOrder_list.GetSize());
                }
            }

            // Any data that is left over at this point is given to any slot that wants it. First come first serve.
            uint32 saturatedSlotCounter = 0;
            for(saturatedSlotCounter = 0; saturatedSlotCounter < (uint32)m_StandardOrder_list.GetSize() && bytesToSpend > leftoverDueToRounding+minFragSize && spentBytes <= (uint64)bytesToSpend-(leftoverDueToRounding+minFragSize); ++saturatedSlotCounter) {
                CEMSocket* socket = m_StandardOrder_list.GetAt(saturatedSlotCounter);

                if(socket != NULL) {
                    SocketSentBytes socketSentBytes = socket->Send(bytesToSpend-leftoverDueToRounding-spentBytes);
                    spentBytes += socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
                    spentOverhead += socketSentBytes.sentBytesControlPackets;
                } else {
					theApp.emuledlg->QueueDebugLogLine(false,"UploadBandwidthThrottler: There was a NULL socket in the standard list (leftovers)! Prevented usage. Index: %i Size: %i", saturatedSlotCounter, m_StandardOrder_list.GetSize());
                }
            }

            if(bytesToSpend-spentBytes > leftoverDueToRounding+minFragSize) {
                currentFullyActivatedSlots = saturatedSlotCounter;
            } else if(saturatedSlotCounter > 0) {
                currentFullyActivatedSlots = saturatedSlotCounter-1;
            }
            m_highestNumberOfFullyActivatedSlots = max(m_highestNumberOfFullyActivatedSlots, currentFullyActivatedSlots);
        }

        lastLoopTick = thisLoopTick;

        bytesToSpend -= spentBytes;


        // These are old limiting calculation valid for SlotFocus packet scheduler. I haven't redone them for
        // the "equal for all slots" packet scheduler yet, since that scheduler isn't finished yet.
        if(bytesToSpend < -((sint64)m_StandardOrder_list.GetSize()*minFragSize)) {
            sint64 newBytesToSpend = -((sint64)m_StandardOrder_list.GetSize()*minFragSize);

            TRACE("UploadBandwidthThrottler: Overcharged bytesToSpend. Limiting negative value. Old value: %I64i New value: %i\n", bytesToSpend, newBytesToSpend);

            bytesToSpend = newBytesToSpend;
        } else if(bytesToSpend > leftoverDueToRounding+minFragSize) {
            //theApp.emuledlg->QueueDebugLogLine(false,"UploadBandwidthThrottler::RunInternal(): Too much in bytesToSpend. Limiting positive value. Old value: %I64i New value: %i", bytesToSpend, leftoverDueToRounding+minFragSize);

            bytesToSpend = leftoverDueToRounding+minFragSize;

            //m_highestNumberOfFullyActivatedSlots = m_StandardOrder_list.GetSize();
        }

        m_SentBytesSinceLastCall += spentBytes;
        m_SentBytesSinceLastCallExcludingOverhead += spentBytes-spentOverhead;

        sendLocker.Unlock();
    }

    threadEndedEvent->SetEvent();

    tempQueueLocker.Lock();
    m_TempControlQueue_list.RemoveAll();
    tempQueueLocker.Unlock();

    sendLocker.Lock();

    m_ControlQueue_list.RemoveAll();
    m_StandardOrder_list.RemoveAll();
    sendLocker.Unlock();

    return 0;
}
