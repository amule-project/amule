//
// This file is part of the aMule Project.
//
// Copyright (c) 2005-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "UploadBandwidthThrottler.h"

#include <protocol/ed2k/Constants.h>
#include <common/Macros.h>
#include <common/Constants.h>

#include <cmath>
#include "OtherFunctions.h"
#include "ThrottledSocket.h"
#include "Logger.h"
#include "Preferences.h"
#include "Statistics.h"


/////////////////////////////////////


/**
 * The constructor starts the thread.
 */
UploadBandwidthThrottler::UploadBandwidthThrottler()
		: wxThread( wxTHREAD_JOINABLE )
{
	m_SentBytesSinceLastCall = 0;
	m_SentBytesSinceLastCallOverhead = 0;

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
	return (EraseFirstValue( m_StandardOrder_list, socket ) > 0);
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
	if (m_doRun) {	// do it only once
		{
			wxMutexLocker lock(m_sendLocker);

			// signal the thread to stop looping and exit.
			m_doRun = false;
		}

		Wait();
	}
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

	uint32 lastLoopTick = GetTickCountFullRes();
	// Bytes to spend in current cycle. If we spend more this becomes negative and causes a wait next time.
	sint32 bytesToSpend = 0;
	uint32 allowedDataRate;
	uint32 rememberedSlotCounter = 0;
	uint32 extraSleepTime = TIME_BETWEEN_UPLOAD_LOOPS;

	while (m_doRun && !TestDestroy()) {
		uint32 timeSinceLastLoop = GetTickCountFullRes() - lastLoopTick;

		// Calculate data rate
		if (thePrefs::GetMaxUpload() == UNLIMITED) {
			// Try to increase the upload rate from UploadSpeedSense
			allowedDataRate = (uint32)theStats::GetUploadRate() + 5 * 1024;
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
		if (bytesToSpend < 1) {
			// We have sent more than allowed in last cycle so we have to wait now
			// until we can send at least 1 byte.
			sleepTime = std::max((-bytesToSpend + 1) * 1000 / allowedDataRate + 2, // add 2 ms to allow for rounding inaccuracies
									extraSleepTime);
		} else {
			// We could send at once, but sleep a while to not suck up all cpu
			sleepTime = extraSleepTime;
		}

		if (timeSinceLastLoop < sleepTime) {
			Sleep(sleepTime-timeSinceLastLoop);
		}

		// Check after sleep in case the thread has been signaled to end
		if (!m_doRun || TestDestroy()) {
			break;
		}

		const uint32 thisLoopTick = GetTickCountFullRes();
		timeSinceLastLoop = thisLoopTick - lastLoopTick;
		lastLoopTick = thisLoopTick;

		if (timeSinceLastLoop > sleepTime + 2000) {
			AddDebugLogLineN(logGeneral, CFormat(wxT("UploadBandwidthThrottler: Time since last loop too long. time: %ims wanted: %ims Max: %ims"))
				% timeSinceLastLoop % sleepTime % (sleepTime + 2000));

			timeSinceLastLoop = sleepTime + 2000;
		}

		// Calculate how many bytes we can spend

		bytesToSpend += (sint32) (allowedDataRate / 1000.0 * timeSinceLastLoop);

		if (bytesToSpend >= 1) {
			sint32 spentBytes = 0;
			sint32 spentOverhead = 0;

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
			while (spentBytes < bytesToSpend && (!m_ControlQueueFirst_list.empty() || !m_ControlQueue_list.empty())) {
				ThrottledControlSocket* socket = NULL;

				if (!m_ControlQueueFirst_list.empty()) {
					socket = m_ControlQueueFirst_list.front();
					m_ControlQueueFirst_list.pop_front();
				} else if (!m_ControlQueue_list.empty()) {
					socket = m_ControlQueue_list.front();
					m_ControlQueue_list.pop_front();
				}

				if (socket != NULL) {
					SocketSentBytes socketSentBytes = socket->SendControlData(bytesToSpend-spentBytes, minFragSize);
					spentBytes += socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
					spentOverhead += socketSentBytes.sentBytesControlPackets;
				}
			}

			// Check if any sockets haven't gotten data for a long time. Then trickle them a package.
			uint32 slots = m_StandardOrder_list.size();
			for (uint32 slotCounter = 0; slotCounter < slots; slotCounter++) {
				ThrottledFileSocket* socket = m_StandardOrder_list[ slotCounter ];

				if (socket != NULL) {
					if (thisLoopTick-socket->GetLastCalledSend() > SEC2MS(1)) {
						// trickle
						uint32 neededBytes = socket->GetNeededBytes();

						if (neededBytes > 0) {
							SocketSentBytes socketSentBytes = socket->SendFileAndControlData(neededBytes, minFragSize);
							spentBytes += socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
							spentOverhead += socketSentBytes.sentBytesControlPackets;
						}
					}
				} else {
					AddDebugLogLineN(logGeneral, CFormat( wxT("There was a NULL socket in the UploadBandwidthThrottler Standard list (trickle)! Prevented usage. Index: %i Size: %i"))
						% slotCounter % m_StandardOrder_list.size());
				}
			}

			// Give available bandwidth to slots, starting with the one we ended with last time.
			// There are two passes. First pass gives packets of doubleSendSize, second pass
			// gives as much as possible.
			// Second pass starts with the last slot of the first pass actually.
			for (uint32 slotCounter = 0; (slotCounter < slots * 2) && spentBytes < bytesToSpend; slotCounter++) {
				if (rememberedSlotCounter >= slots) {	// wrap around pointer
					rememberedSlotCounter = 0;
				}

				uint32 data = (slotCounter < slots - 1)	? doubleSendSize				// pass 1
															: (bytesToSpend - spentBytes);	// pass 2

				ThrottledFileSocket* socket = m_StandardOrder_list[ rememberedSlotCounter ];

				if (socket != NULL) {
					SocketSentBytes socketSentBytes = socket->SendFileAndControlData(data, doubleSendSize);
					spentBytes += socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
					spentOverhead += socketSentBytes.sentBytesControlPackets;
				} else {
					AddDebugLogLineN(logGeneral, CFormat(wxT("There was a NULL socket in the UploadBandwidthThrottler Standard list (equal-for-all)! Prevented usage. Index: %i Size: %i"))
						% rememberedSlotCounter % m_StandardOrder_list.size());
				}

				rememberedSlotCounter++;
			}

			// Do some limiting of what we keep for the next loop.
			bytesToSpend -= spentBytes;
			sint32 minBytesToSpend = (slots + 1) * minFragSize;

			if (bytesToSpend < - minBytesToSpend) {
				bytesToSpend = - minBytesToSpend;
			} else {
				sint32 bandwidthSavedTolerance = slots * 512 + 1;
				if (bytesToSpend > bandwidthSavedTolerance) {
					bytesToSpend = bandwidthSavedTolerance;
				}
			}

			m_SentBytesSinceLastCall += spentBytes;
			m_SentBytesSinceLastCallOverhead += spentOverhead;

			if (spentBytes == 0) {	// spentBytes includes the overhead
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
// File_checked_for_headers
