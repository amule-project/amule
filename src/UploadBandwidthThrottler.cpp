//
// This file is part of the aMule Project.
// 
// Copyright (C) 2005-2006aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <cmath>
#include "UploadBandwidthThrottler.h"
#include "EMSocket.h"
#include "OPCodes.h"
#include "OtherFunctions.h"
#include "ThrottledSocket.h"
#include "Logger.h"
#include "Preferences.h"
#include "amule.h"
#include "Statistics.h"

#include <algorithm>
#include <limits>

#ifdef _UI64_MAX
#undef _UI64_MAX
#endif

#ifdef _I64_MAX
#undef _I64_MAX
#endif

const uint32 _UI32_MAX = std::numeric_limits<uint32>::max();
const sint32 _I32_MAX = std::numeric_limits<sint32>::max();
const uint64 _UI64_MAX = std::numeric_limits<uint64>::max();
const sint64 _I64_MAX = std::numeric_limits<sint64>::max();

/////////////////////////////////////


/**
 * The constructor starts the thread.
 */
UploadBandwidthThrottler::UploadBandwidthThrottler()
		: wxThread( wxTHREAD_JOINABLE )
{
	m_SentBytesSinceLastCall = 0;
	m_SentBytesSinceLastCallOverhead = 0;
	m_highestNumberOfFullyActivatedSlots = 0;

	m_doRun = true;

	Create();
	Run();
}


/**
 * The destructor stops the thread. If the thread has already stoppped, destructor does nothing.
 */
UploadBandwidthThrottler::~UploadBandwidthThrottler()
{
	EndThread();
}


/**
 * Find out how many bytes that has been put on the sockets since the last call to this
 * method. Includes overhead of control packets.
 *
 * @return the number of bytes that has been put on the sockets since the last call
 */
uint64 UploadBandwidthThrottler::GetNumberOfSentBytesSinceLastCallAndReset()
{
	wxMutexLocker lock( m_sendLocker );

	uint64 numberOfSentBytesSinceLastCall = m_SentBytesSinceLastCall;
	m_SentBytesSinceLastCall = 0;

	return numberOfSentBytesSinceLastCall;
}

/**
 * Find out how many bytes that has been put on the sockets since the last call to this
 * method. Excludes overhead of control packets.
 *
 * @return the number of bytes that has been put on the sockets since the last call
 */
uint64 UploadBandwidthThrottler::GetNumberOfSentBytesOverheadSinceLastCallAndReset()
{
	wxMutexLocker lock( m_sendLocker );

	uint64 numberOfSentBytesSinceLastCall = m_SentBytesSinceLastCallOverhead;
	m_SentBytesSinceLastCallOverhead = 0;

	return numberOfSentBytesSinceLastCall;
}


/**
 * Find out the highest number of slots that has been fed data in the normal standard loop
 * of the thread since the last call of this method. This means all slots that haven't
 * been in the trickle state during the entire time since the last call.
 *
 * @return the highest number of fully activated slots during any loop since last call
 */
uint32 UploadBandwidthThrottler::GetHighestNumberOfFullyActivatedSlotsSinceLastCallAndReset()
{
	wxMutexLocker lock( m_sendLocker );
	
	uint32 highestNumberOfFullyActivatedSlots = m_highestNumberOfFullyActivatedSlots;
	m_highestNumberOfFullyActivatedSlots = 0;

	return highestNumberOfFullyActivatedSlots;
}


uint32 UploadBandwidthThrottler::GetStandardListSize()
{
	wxMutexLocker lock( m_sendLocker );
	
	return m_StandardOrder_list.size();
};


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
void UploadBandwidthThrottler::AddToStandardList(uint32 index, ThrottledFileSocket* socket)
{
	if ( socket ) {
		wxMutexLocker lock( m_sendLocker );

		RemoveFromStandardListNoLock(socket);
		if (index > (uint32)m_StandardOrder_list.size()) {
			index = m_StandardOrder_list.size();
		}
		
		m_StandardOrder_list.insert(m_StandardOrder_list.begin() + index, socket);
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
bool UploadBandwidthThrottler::RemoveFromStandardList(ThrottledFileSocket* socket)
{
	wxMutexLocker lock( m_sendLocker );

	return RemoveFromStandardListNoLock(socket);
}


/**
 * Remove a socket from the list of sockets that have upload slots. NOT THREADSAFE!
 * This is an internal method that doesn't take the necessary lock before it removes
 * the socket. This method should only be called when the current thread already owns
 * the m_sendLocker lock!
 *
 * @param socket address of the socket that should be removed from the list. If this socket
 *               does not exist in the list, this method will do nothing.
 */
bool UploadBandwidthThrottler::RemoveFromStandardListNoLock(ThrottledFileSocket* socket)
{
	// Find the slot
	bool foundSocket = EraseFirstValue( m_StandardOrder_list, socket );

	if ( foundSocket && m_highestNumberOfFullyActivatedSlots > m_StandardOrder_list.size()) {
		m_highestNumberOfFullyActivatedSlots = m_StandardOrder_list.size();
	}

	return foundSocket;
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
void UploadBandwidthThrottler::QueueForSendingControlPacket(ThrottledControlSocket* socket, bool hasSent)
{
	// Get critical section
	wxMutexLocker lock( m_tempQueueLocker );

	if ( m_doRun ) {
		if( hasSent ) {
			m_TempControlQueueFirst_list.push_back(socket);
		} else {
			m_TempControlQueue_list.push_back(socket);
		}
	}
}



/**
 * Remove the socket from all lists and queues. This will make it safe to
 * erase/delete the socket. It will also cause the main thread to stop calling
 * send() for the socket.
 *
 * @param socket address to the socket that should be removed
 */
void UploadBandwidthThrottler::DoRemoveFromAllQueues(ThrottledControlSocket* socket)
{
	if ( m_doRun ) {
		// Remove this socket from control packet queue
		EraseValue( m_ControlQueue_list, socket );
		EraseValue( m_ControlQueueFirst_list, socket );
		
		wxMutexLocker lock( m_tempQueueLocker );
		EraseValue( m_TempControlQueue_list, socket );
		EraseValue( m_TempControlQueueFirst_list, socket );
	}
}


void UploadBandwidthThrottler::RemoveFromAllQueues(ThrottledControlSocket* socket)
{
	wxMutexLocker lock( m_sendLocker );
	
	DoRemoveFromAllQueues( socket );
}


void UploadBandwidthThrottler::RemoveFromAllQueues(ThrottledFileSocket* socket)
{
	wxMutexLocker lock( m_sendLocker );
	
	if (m_doRun) {	
		DoRemoveFromAllQueues(socket);

		// And remove it from upload slots
		RemoveFromStandardListNoLock(socket);
	}
}


/**
 * Make the thread exit. This method will not return until the thread has stopped
 * looping. This guarantees that the thread will not access the CEMSockets after this
 * call has exited.
 */
void UploadBandwidthThrottler::EndThread()
{
	{
		wxMutexLocker lock(m_sendLocker);

		// signal the thread to stop looping and exit.
		m_doRun = false;
	}
	
	Wait();
}


/**
 * The thread method that handles calling send for the individual sockets.
 *
 * Control packets will always be tried to be sent first. If there is any bandwidth leftover
 * after that, send() for the upload slot sockets will be called in priority order until we have run
 * out of available bandwidth for this loop. Upload slots will not be allowed to go without having sent
 * called for more than a defined amount of time (i.e. two seconds).
 *
 * @return always returns 0.
 */
void* UploadBandwidthThrottler::Entry()
{
	const uint32 TIME_BETWEEN_UPLOAD_LOOPS = 1;
	
	uint32 lastLoopTick = ::GetTickCountFullRes();
	sint64 realBytesToSpend = 0;
	uint32 allowedDataRate = 0;
	uint32 rememberedSlotCounter = 0;
	uint32 lastTickReachedBandwidth = ::GetTickCountFullRes();
	uint32 extraSleepTime = TIME_BETWEEN_UPLOAD_LOOPS;
	
	while (m_doRun) {
		uint32 timeSinceLastLoop = ::GetTickCountFullRes() - lastLoopTick;

		// Get current speed from UploadSpeedSense
		if (thePrefs::GetMaxUpload() == UNLIMITED) {
			// Try to increase the upload rate
			if (theApp.uploadqueue) {
				allowedDataRate = (uint32)theStats::GetUploadRate() + 5 * 1024;
			} else {
				// App not created yet or already destroyed.
				allowedDataRate = (uint32)(-1);
			}
		} else {
			allowedDataRate = thePrefs::GetMaxUpload() * 1024;
		}

		uint32 minFragSize = 1300;
		uint32 doubleSendSize = minFragSize*2; // send two packages at a time so they can share an ACK
		if (allowedDataRate < 6*1024) {
			minFragSize = 536;
			doubleSendSize = minFragSize; // don't send two packages at a time at very low speeds to give them a smoother load
		}


		uint32 sleepTime;
		if(allowedDataRate == 0 || allowedDataRate == _UI32_MAX || realBytesToSpend >= 1000) {
			// we could send at once, but sleep a while to not suck up all cpu
			sleepTime = extraSleepTime;
		} else {
			// sleep for just as long as we need to get back to having one byte to send
			sleepTime = std::max((uint32)ceil((double)(-realBytesToSpend + 1000)/allowedDataRate), TIME_BETWEEN_UPLOAD_LOOPS);
		}

		if(timeSinceLastLoop < sleepTime) {
			Sleep(sleepTime-timeSinceLastLoop);
		}

		const uint32 thisLoopTick = ::GetTickCountFullRes();
		timeSinceLastLoop = thisLoopTick - lastLoopTick;

		// Calculate how many bytes we can spend
		sint64 bytesToSpend = 0;

		if(allowedDataRate != 0 && allowedDataRate != _UI32_MAX) {
			// prevent overflow
			if(timeSinceLastLoop == 0) {
				// no time has passed, so don't add any bytes. Shouldn't happen.
				bytesToSpend = 0; //realBytesToSpend/1000;
			} else if(_I64_MAX/timeSinceLastLoop > allowedDataRate && _I64_MAX-allowedDataRate*timeSinceLastLoop > realBytesToSpend) {
				if(timeSinceLastLoop > sleepTime + 2000) {
					AddDebugLogLineM(false, logGeneral, wxString::Format(wxT("UploadBandwidthThrottler: Time since last loop too long. time: %ims wanted: %ims Max: %ims"), timeSinceLastLoop, sleepTime, sleepTime + 2000));
		
					timeSinceLastLoop = sleepTime + 2000;
					lastLoopTick = thisLoopTick - timeSinceLastLoop;
				}

				realBytesToSpend += allowedDataRate*timeSinceLastLoop;

				bytesToSpend = realBytesToSpend/1000;
			} else {
				realBytesToSpend = _I64_MAX;
				bytesToSpend = _I32_MAX;
			}
		} else {
			realBytesToSpend = 0; //_I64_MAX;
			bytesToSpend = _I32_MAX;
		}

		lastLoopTick = thisLoopTick;

		if(bytesToSpend >= 1) {
			uint64 spentBytes = 0;
			uint64 spentOverhead = 0;

			wxMutexLocker sendLock(m_sendLocker);

			{
				wxMutexLocker queueLock(m_tempQueueLocker);

				// are there any sockets in m_TempControlQueue_list? Move them to normal m_ControlQueue_list;
				m_ControlQueueFirst_list.insert(	m_ControlQueueFirst_list.end(),
													m_TempControlQueueFirst_list.begin(),
													m_TempControlQueueFirst_list.end() );

				m_ControlQueue_list.insert( m_ControlQueue_list.end(), 
											m_TempControlQueue_list.begin(),
											m_TempControlQueue_list.end() );

				m_TempControlQueue_list.clear();
				m_TempControlQueueFirst_list.clear();
			}
	
			// Send any queued up control packets first
			while(bytesToSpend > 0 && spentBytes < (uint64)bytesToSpend && (!m_ControlQueueFirst_list.empty() || !m_ControlQueue_list.empty())) {
				ThrottledControlSocket* socket = NULL;
	
				if(!m_ControlQueueFirst_list.empty()) {
					socket = m_ControlQueueFirst_list.front();
					m_ControlQueueFirst_list.pop_front();
				} else if(!m_ControlQueue_list.empty()) {
					socket = m_ControlQueue_list.front();
					m_ControlQueue_list.pop_front();
				}
	
				if(socket != NULL) {
					SocketSentBytes socketSentBytes = socket->SendControlData(bytesToSpend-spentBytes, minFragSize);
					uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
					spentBytes += lastSpentBytes;
					spentOverhead += socketSentBytes.sentBytesControlPackets;
				}
			}
	
			// Check if any sockets haven't gotten data for a long time. Then trickle them a package.
			for ( uint32 slotCounter = 0; slotCounter < m_StandardOrder_list.size(); slotCounter++) {
				ThrottledFileSocket* socket = m_StandardOrder_list[ slotCounter ];
	
				if(socket != NULL) {
					if(thisLoopTick-socket->GetLastCalledSend() > SEC2MS(1)) {
						// trickle
						uint32 neededBytes = socket->GetNeededBytes();
	
						if(neededBytes > 0) {
							SocketSentBytes socketSentBytes = socket->SendFileAndControlData(neededBytes, minFragSize);
							uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
							spentBytes += lastSpentBytes;
							spentOverhead += socketSentBytes.sentBytesControlPackets;

							if(lastSpentBytes > 0 && slotCounter < m_highestNumberOfFullyActivatedSlots) {
								m_highestNumberOfFullyActivatedSlots = slotCounter;
							}
						}
					}
				} else {
					AddDebugLogLineM(false, logGeneral, wxString::Format( wxT("There was a NULL socket in the UploadBandwidthThrottler Standard list (trickle)! Prevented usage. Index: %i Size: %i"), slotCounter, m_StandardOrder_list.size()) );
				}
			}
	
			// Equal bandwidth for all slots
			uint32 maxSlot = m_StandardOrder_list.size();
			if(maxSlot > 0 && allowedDataRate/maxSlot < UPLOAD_CLIENT_DATARATE) {
				maxSlot = allowedDataRate/UPLOAD_CLIENT_DATARATE;
			}

			if(maxSlot > m_highestNumberOfFullyActivatedSlots) {
				m_highestNumberOfFullyActivatedSlots = maxSlot;
			}

			for(uint32 maxCounter = 0; maxCounter < std::min(maxSlot, (uint32)m_StandardOrder_list.size()) && bytesToSpend > 0 && spentBytes < (uint64)bytesToSpend; maxCounter++) {
				if(rememberedSlotCounter >= m_StandardOrder_list.size() ||
				   rememberedSlotCounter >= maxSlot) {
					rememberedSlotCounter = 0;
				}

				ThrottledFileSocket* socket = m_StandardOrder_list[ rememberedSlotCounter ];

				if(socket != NULL) {
					SocketSentBytes socketSentBytes = socket->SendFileAndControlData(std::min(doubleSendSize, (uint32)(bytesToSpend-spentBytes)), doubleSendSize);
					uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;

					spentBytes += lastSpentBytes;
					spentOverhead += socketSentBytes.sentBytesControlPackets;
				} else {
					AddDebugLogLineM(false, logGeneral, wxString::Format( wxT("There was a NULL socket in the UploadBandwidthThrottler Standard list (equal-for-all)! Prevented usage. Index: %i Size: %i"), rememberedSlotCounter, m_StandardOrder_list.size()));
				}

				rememberedSlotCounter++;
			}

			// Any bandwidth that hasn't been used yet are used first to last.
			for(uint32 slotCounter = 0; slotCounter < m_StandardOrder_list.size() && bytesToSpend > 0 && spentBytes < (uint64)bytesToSpend; slotCounter++) {
				ThrottledFileSocket* socket = m_StandardOrder_list[ slotCounter ];

				if(socket != NULL) {
					uint32 bytesToSpendTemp = bytesToSpend-spentBytes;
					SocketSentBytes socketSentBytes = socket->SendFileAndControlData(bytesToSpendTemp, doubleSendSize);
					uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
					spentBytes += lastSpentBytes;
					spentOverhead += socketSentBytes.sentBytesControlPackets;

					if(slotCounter+1 > m_highestNumberOfFullyActivatedSlots && (lastSpentBytes < bytesToSpendTemp || lastSpentBytes >= doubleSendSize)) { 
						m_highestNumberOfFullyActivatedSlots = slotCounter+1;
					}
				} else {
					AddDebugLogLineM( false, logGeneral, wxString::Format( wxT("There was a NULL socket in the UploadBandwidthThrottler Standard list (fully activated)! Prevented usage. Index: %i Size: %i"), slotCounter, m_StandardOrder_list.size()));
				}
			}
			realBytesToSpend -= spentBytes*1000;

			if(realBytesToSpend < -(((sint64)m_StandardOrder_list.size()+1)*minFragSize)*1000) {
				sint64 newRealBytesToSpend = -(((sint64)m_StandardOrder_list.size()+1)*minFragSize)*1000;
	
				realBytesToSpend = newRealBytesToSpend;
				lastTickReachedBandwidth = thisLoopTick;
			} else {
				uint64 bandwidthSavedTolerance = m_StandardOrder_list.size()*512*1000;
				if(realBytesToSpend > 0 && (uint64)realBytesToSpend > 999+bandwidthSavedTolerance) {
					sint64 newRealBytesToSpend = 999+bandwidthSavedTolerance;
					realBytesToSpend = newRealBytesToSpend;

					if(thisLoopTick-lastTickReachedBandwidth > std::max((uint32)1000, (uint32)timeSinceLastLoop*2)) {
						m_highestNumberOfFullyActivatedSlots = m_StandardOrder_list.size()+1;
						lastTickReachedBandwidth = thisLoopTick;
					}
				} else {
					lastTickReachedBandwidth = thisLoopTick;
				}
			}
			
			m_SentBytesSinceLastCall += spentBytes;
			m_SentBytesSinceLastCallOverhead += spentOverhead;

			if ((spentBytes == 0) and (spentOverhead == 0)) {
				extraSleepTime = std::min<uint32>(extraSleepTime * 5, 1000); // 1s at most
			} else {
				extraSleepTime = TIME_BETWEEN_UPLOAD_LOOPS;
			}
		}
	}

	{
		wxMutexLocker queueLock(m_tempQueueLocker);
		m_TempControlQueue_list.clear();
		m_TempControlQueueFirst_list.clear();
	}

	wxMutexLocker sendLock(m_sendLocker);
	m_ControlQueue_list.clear();
	m_StandardOrder_list.clear();

	return 0;
}
