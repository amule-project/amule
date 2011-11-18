//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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


#include "EMSocket.h"		// Interface declarations.

#include <protocol/Protocols.h>
#include <protocol/ed2k/Constants.h>

#include "Packet.h"		// Needed for CPacket
#include "amule.h"
#include "GetTickCount.h"
#include "UploadBandwidthThrottler.h"
#include "Logger.h"
#include "Preferences.h"
#include "ScopedPtr.h"


const uint32 MAX_PACKET_SIZE = 2000000;

CEMSocket::CEMSocket(const CProxyData *ProxyData)
	: CEncryptedStreamSocket(wxSOCKET_NOWAIT, ProxyData)
{
	// If an interface has been specified,
	// then we need to bind to it.
	if (!thePrefs::GetAddress().IsEmpty()) {
		amuleIPV4Address host;
		
		// No need to warn here, in case of failure to
		// assign the hostname. That is already done
		// in amule.cpp when starting ...
		if (host.Hostname(thePrefs::GetAddress())) {
			SetLocal(host);
		}
	}

	byConnected = ES_NOTCONNECTED;
	m_uTimeOut = CONNECTION_TIMEOUT; // default timeout for ed2k sockets

	// Download (pseudo) rate control	
	downloadLimit = 0;
	downloadLimitEnable = false;
	pendingOnReceive = false;

	// Download partial header
	pendingHeaderSize = 0;

	// Download partial packet
	pendingPacket = NULL;
	pendingPacketSize = 0;

	// Upload control
	sendbuffer = NULL;
	sendblen = 0;
	sent = 0;

    m_currentPacket_is_controlpacket = false;
	m_currentPackageIsFromPartFile = false;

    m_numberOfSentBytesCompleteFile = 0;
    m_numberOfSentBytesPartFile = 0;
    m_numberOfSentBytesControlPacket = 0;

    lastCalledSend = ::GetTickCount();
    lastSent = ::GetTickCount()-1000;

	m_bAccelerateUpload = false;

    m_actualPayloadSize = 0;
    m_actualPayloadSizeSent = 0;

    m_bBusy = false;
    m_hasSent = false;

	lastFinishedStandard = 0;

	DoingDestroy = false;
	
}

CEMSocket::~CEMSocket()
{
    // need to be locked here to know that the other methods
    // won't be in the middle of things
    {
    	wxMutexLocker lock(m_sendLocker);
		byConnected = ES_DISCONNECTED;
	}

    // now that we know no other method will keep adding to the queue
    // we can remove ourself from the queue
	if (theApp->uploadBandwidthThrottler) {
	    theApp->uploadBandwidthThrottler->RemoveFromAllQueues(this);
	}

    ClearQueues();
	
	SetNotify(0);
	Notify(FALSE);
}

void CEMSocket::Destroy() {
	if (!DoingDestroy) {		
		DoingDestroy = true;
		wxSocketClient::Destroy();
	}	
}


void CEMSocket::ClearQueues()
{
	wxMutexLocker lock(m_sendLocker);

	DeleteContents(m_control_queue);

	{
		CStdPacketQueue::iterator it = m_standard_queue.begin();
		for (; it != m_standard_queue.end(); ++it) {
			delete it->packet;
		}
		m_standard_queue.clear();
	}
	
	// Download (pseudo) rate control	
	downloadLimit = 0;
	downloadLimitEnable = false;
	pendingOnReceive = false;

	// Download partial header
	pendingHeaderSize = 0;

	// Download partial packet
	delete[] pendingPacket;
	pendingPacket = NULL;
	pendingPacketSize = 0;

	// Upload control
	delete[] sendbuffer;
	sendbuffer = NULL;
	sendblen = 0;
	sent = 0;
}


void CEMSocket::OnClose(int WXUNUSED(nErrorCode))
{
    // need to be locked here to know that the other methods
    // won't be in the middle of things
    {
    	wxMutexLocker lock(m_sendLocker);
		byConnected = ES_DISCONNECTED;
	}

    // now that we know no other method will keep adding to the queue
    // we can remove ourself from the queue
    theApp->uploadBandwidthThrottler->RemoveFromAllQueues(this);
	
	ClearQueues();
}


void CEMSocket::OnReceive(int nErrorCode)
{
	if(nErrorCode) {
		uint32 error = LastError(); 
		if (error != wxSOCKET_WOULDBLOCK) {
			OnError(nErrorCode);
			return;
		}
	}
	
	// Check current connection state
	if (byConnected == ES_DISCONNECTED) {
		return;
	} else {	
		byConnected = ES_CONNECTED; // ES_DISCONNECTED, ES_NOTCONNECTED, ES_CONNECTED
	}

	uint32 ret;
	do {
		// CPU load improvement
		if (downloadLimitEnable && downloadLimit == 0){
			pendingOnReceive = true;
			return;
		}

		uint32 readMax;
		byte *buf;
		if (pendingHeaderSize < PACKET_HEADER_SIZE) {
			delete[] pendingPacket;
			pendingPacket = NULL;
			buf = pendingHeader + pendingHeaderSize;
			readMax = PACKET_HEADER_SIZE - pendingHeaderSize;
		} else if (pendingPacket == NULL) {
			pendingPacketSize = 0;
			readMax = CPacket::GetPacketSizeFromHeader(pendingHeader);
			if (readMax > MAX_PACKET_SIZE) {
				pendingHeaderSize = 0;
				OnError(ERR_TOOBIG);
				return;
			}
			pendingPacket = new byte[readMax + 1];
			buf = pendingPacket;
		} else {
			buf = pendingPacket + pendingPacketSize;
			readMax = CPacket::GetPacketSizeFromHeader(pendingHeader) - pendingPacketSize;
		}

		if (downloadLimitEnable && readMax > downloadLimit) {
			readMax = downloadLimit;
		}

		ret = 0;
		if (readMax) {
			wxMutexLocker lock(m_sendLocker);
			ret = Read(buf, readMax);
			if (Error() || (ret == 0)) {
				if (LastError() == wxSOCKET_WOULDBLOCK) {
					pendingOnReceive = true;
				}
				return;
			}
		}

		// Bandwidth control
		if (downloadLimitEnable) {
			// Update limit
			if (ret >= downloadLimit) {
				downloadLimit = 0;
			} else {
				downloadLimit -= ret;
			}
		}

		// CPU load improvement
		// Detect if the socket's buffer is empty (or the size did match...)
		pendingOnReceive = (ret == readMax);

		if (pendingHeaderSize >= PACKET_HEADER_SIZE) {
			pendingPacketSize += ret;
			if (pendingPacketSize >= CPacket::GetPacketSizeFromHeader(pendingHeader)) {
				CScopedPtr<CPacket> packet(new CPacket(pendingHeader, pendingPacket));
				pendingPacket = NULL;
				pendingPacketSize = 0;
				pendingHeaderSize = 0;

				// Bugfix We still need to check for a valid protocol
				// Remark: the default eMule v0.26b had removed this test......
				switch (packet->GetProtocol()){
					case OP_EDONKEYPROT:
					case OP_PACKEDPROT:
					case OP_EMULEPROT:
					case OP_ED2KV2HEADER:
					case OP_ED2KV2PACKEDPROT:
						break;
					default:
						OnError(ERR_WRONGHEADER);
						return;
				}

				// Process packet
				PacketReceived(packet.get());
			}
		} else {
			pendingHeaderSize += ret;
		}
	} while (ret && pendingHeaderSize >= PACKET_HEADER_SIZE);
}


void CEMSocket::SetDownloadLimit(uint32 limit)
{
	downloadLimit = limit;
	downloadLimitEnable = true;	
	
	// CPU load improvement
	if(limit > 0 && pendingOnReceive == true){
		OnReceive(0);
	}
}


void CEMSocket::DisableDownloadLimit()
{
	downloadLimitEnable = false;

	// CPU load improvement
	if (pendingOnReceive == true){
		OnReceive(0);
	}
}


/**
 * Queues up the packet to be sent. Another thread will actually send the packet.
 *
 * If the packet is not a control packet, and if the socket decides that its queue is
 * full and forceAdd is false, then the socket is allowed to refuse to add the packet
 * to its queue. It will then return false and it is up to the calling thread to try
 * to call SendPacket for that packet again at a later time.
 *
 * @param packet address to the packet that should be added to the queue
 *
 * @param delpacket if true, the responsibility for deleting the packet after it has been sent
 *                  has been transferred to this object. If false, don't delete the packet after it
 *                  has been sent.
 *
 * @param controlpacket the packet is a controlpacket
 *
 * @param forceAdd this packet must be added to the queue, even if it is full. If this flag is true
 *                 then the method can not refuse to add the packet, and therefore not return false.
 *
 * @return true if the packet was added to the queue, false otherwise
 */
void CEMSocket::SendPacket(CPacket* packet, bool delpacket, bool controlpacket, uint32 actualPayloadSize)
{
	//printf("* SendPacket called on socket %p\n", this);
	wxMutexLocker lock(m_sendLocker);

	if (byConnected == ES_DISCONNECTED) {
		//printf("* Disconnected, drop packet\n");
        if(delpacket) {
			delete packet;
        }
    } else {
        if (!delpacket){
            packet = new CPacket(*packet);
	    }

        if (controlpacket) {
			//printf("* Adding a control packet\n");
	        m_control_queue.push_back(packet);

            // queue up for controlpacket
            theApp->uploadBandwidthThrottler->QueueForSendingControlPacket(this, HasSent());
	    } else {
			//printf("* Adding a normal packet to the queue\n");
            bool first = !((sendbuffer && !m_currentPacket_is_controlpacket) || !m_standard_queue.empty());
            StandardPacketQueueEntry queueEntry = { actualPayloadSize, packet };
		    m_standard_queue.push_back(queueEntry);

            // reset timeout for the first time
            if (first) {
                lastFinishedStandard = ::GetTickCount();
                m_bAccelerateUpload = true;	// Always accelerate first packet in a block
            }
	    }
    }
}


uint64 CEMSocket::GetSentBytesCompleteFileSinceLastCallAndReset()
{
	wxMutexLocker lock( m_sendLocker );

	uint64 sentBytes = m_numberOfSentBytesCompleteFile;
    m_numberOfSentBytesCompleteFile = 0;

    return sentBytes;
}


uint64 CEMSocket::GetSentBytesPartFileSinceLastCallAndReset()
{
	wxMutexLocker lock( m_sendLocker );

	uint64 sentBytes = m_numberOfSentBytesPartFile;
    m_numberOfSentBytesPartFile = 0;

    return sentBytes;
}

uint64 CEMSocket::GetSentBytesControlPacketSinceLastCallAndReset()
{
	wxMutexLocker lock( m_sendLocker );

	uint64 sentBytes = m_numberOfSentBytesControlPacket;
    m_numberOfSentBytesControlPacket = 0;

    return sentBytes;
}

uint64 CEMSocket::GetSentPayloadSinceLastCallAndReset()
{
	wxMutexLocker lock( m_sendLocker );

	uint64 sentBytes = m_actualPayloadSizeSent;
    m_actualPayloadSizeSent = 0;

    return sentBytes;
}


void CEMSocket::OnSend(int nErrorCode)
{
    if (nErrorCode){
		OnError(nErrorCode);
		return;
	}

	CEncryptedStreamSocket::OnSend(0);	
	
	wxMutexLocker lock( m_sendLocker );
    m_bBusy = false;

    if (byConnected != ES_DISCONNECTED) {
		byConnected = ES_CONNECTED;

	    if (m_currentPacket_is_controlpacket) {
	        // queue up for control packet
    	    theApp->uploadBandwidthThrottler->QueueForSendingControlPacket(this, HasSent());
		}
    }
}


/**
 * Try to put queued up data on the socket.
 *
 * Control packets have higher priority, and will be sent first, if possible.
 * Standard packets can be split up in several package containers. In that case
 * all the parts of a split package must be sent in a row, without any control packet
 * in between.
 *
 * @param maxNumberOfBytesToSend This is the maximum number of bytes that is allowed to be put on the socket
 *                               this call. The actual number of sent bytes will be returned from the method.
 *
 * @param onlyAllowedToSendControlPacket This call we only try to put control packets on the sockets.
 *                                       If there's a standard packet "in the way", and we think that this socket
 *                                       is no longer an upload slot, then it is ok to send the standard packet to
 *                                       get it out of the way. But it is not allowed to pick a new standard packet
 *                                       from the queue during this call. Several split packets are counted as one
 *                                       standard packet though, so it is ok to finish them all off if necessary.
 *
 * @return the actual number of bytes that were put on the socket.
 */
SocketSentBytes CEMSocket::Send(uint32 maxNumberOfBytesToSend, uint32 minFragSize, bool onlyAllowedToSendControlPacket)
{
	wxMutexLocker lock(m_sendLocker);

	//printf("* Attempt to send a packet on socket %p\n", this);
	
	if (byConnected == ES_DISCONNECTED) {
		//printf("* Disconnected socket %p\n", this);
        SocketSentBytes returnVal = { false, 0, 0 };
        return returnVal;
    } else if (m_bBusy && onlyAllowedToSendControlPacket) {
		//printf("* Busy socket %p\n", this);
        SocketSentBytes returnVal = { true, 0, 0 };
        return returnVal;
    }	

    bool anErrorHasOccured = false;
    uint32 sentStandardPacketBytesThisCall = 0;
    uint32 sentControlPacketBytesThisCall = 0;
	
    if (byConnected == ES_CONNECTED && IsEncryptionLayerReady() && (!m_bBusy || onlyAllowedToSendControlPacket)) {

		//printf("* Internal attemptto send on %p\n", this);
		
		if(minFragSize < 1) {
			minFragSize = 1;
		}
	
		maxNumberOfBytesToSend = GetNextFragSize(maxNumberOfBytesToSend, minFragSize);
	
		bool bWasLongTimeSinceSend = (::GetTickCount() - lastSent) > 1000;
	
		lastCalledSend = ::GetTickCount();
	
			
		while(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall < maxNumberOfBytesToSend && anErrorHasOccured == false && // don't send more than allowed. Also, there should have been no error in earlier loop
			  (!m_control_queue.empty() || !m_standard_queue.empty() || sendbuffer != NULL) && // there must exist something to send
			  (onlyAllowedToSendControlPacket == false || // this means we are allowed to send both types of packets, so proceed
			   (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall > 0 && (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) % minFragSize != 0) ||
			   (sendbuffer == NULL && !m_control_queue.empty()) || // There's a control packet in queue, and we are not currently sending anything, so we will handle the control packet next
			   (sendbuffer != NULL && m_currentPacket_is_controlpacket == true) || // We are in the progress of sending a control packet. We are always allowed to send those
			   (sendbuffer != NULL && m_currentPacket_is_controlpacket == false && bWasLongTimeSinceSend && !m_control_queue.empty() && m_standard_queue.empty() && (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) < minFragSize) // We have waited to long to clean the current packet (which may be a standard packet that is in the way). Proceed no matter what the value of onlyAllowedToSendControlPacket.
			  )
			 ) {
	
			// If we are currently not in the progress of sending a packet, we will need to find the next one to send
			if(sendbuffer == NULL) {
				CPacket* curPacket = NULL;
				if(!m_control_queue.empty()) {
					// There's a control packet to send
					m_currentPacket_is_controlpacket = true;
					curPacket = m_control_queue.front();
					m_control_queue.pop_front();
				} else if(!m_standard_queue.empty() /*&& onlyAllowedToSendControlPacket == false*/) {
					// There's a standard packet to send
					m_currentPacket_is_controlpacket = false;
					StandardPacketQueueEntry queueEntry = m_standard_queue.front();
					m_standard_queue.pop_front();
					curPacket = queueEntry.packet;
					m_actualPayloadSize = queueEntry.actualPayloadSize;
	
					// remember this for statistics purposes.
					m_currentPackageIsFromPartFile = curPacket->IsFromPF();
				} else {
					// Just to be safe. Shouldn't happen?
					// if we reach this point, then there's something wrong with the while condition above!
					wxFAIL;
					AddDebugLogLineC(logGeneral, wxT("EMSocket: Couldn't get a new packet! There's an error in the first while condition in EMSocket::Send()"));
	
					SocketSentBytes returnVal = { true, sentStandardPacketBytesThisCall, sentControlPacketBytesThisCall };
					return returnVal;
				}
	
				// We found a packet to send. Get the data to send from the
				// package container and dispose of the container.
				sendblen = curPacket->GetRealPacketSize();
				sendbuffer = curPacket->DetachPacket();
				sent = 0;
				delete curPacket;
				
				CryptPrepareSendData((byte*)sendbuffer, sendblen);
			}
	
			// At this point we've got a packet to send in sendbuffer. Try to send it. Loop until entire packet
			// is sent, or until we reach maximum bytes to send for this call, or until we get an error.
			// NOTE! If send would block (returns WOULDBLOCK), we will return from this method INSIDE this loop.
			while (sent < sendblen &&
				   sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall < maxNumberOfBytesToSend &&
				   (
					onlyAllowedToSendControlPacket == false || // this means we are allowed to send both types of packets, so proceed
					m_currentPacket_is_controlpacket ||
					(bWasLongTimeSinceSend && (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) < minFragSize) ||
					(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) % minFragSize != 0
				   ) &&
				   anErrorHasOccured == false) {
				uint32 tosend = sendblen-sent;
				if(!onlyAllowedToSendControlPacket || m_currentPacket_is_controlpacket) {
					if (maxNumberOfBytesToSend >= sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall && tosend > maxNumberOfBytesToSend-(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall))
						tosend = maxNumberOfBytesToSend-(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall);
				} else if(bWasLongTimeSinceSend && (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) < minFragSize) {
					if (minFragSize >= sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall && tosend > minFragSize-(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall))
						tosend = minFragSize-(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall);
				} else {
					uint32 nextFragMaxBytesToSent = GetNextFragSize(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall, minFragSize);
					if (nextFragMaxBytesToSent >= sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall && tosend > nextFragMaxBytesToSent-(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall))
						tosend = nextFragMaxBytesToSent-(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall);
				}
				wxASSERT(tosend != 0 && tosend <= sendblen-sent);
				
				//DWORD tempStartSendTick = ::GetTickCount();
	
				lastSent = ::GetTickCount();
	
				uint32 result = CEncryptedStreamSocket::Write(sendbuffer+sent,tosend);
				
				if (Error()){
					
					uint32 error = LastError();
					if (error == wxSOCKET_WOULDBLOCK){
						m_bBusy = true;
	
						SocketSentBytes returnVal = { true, sentStandardPacketBytesThisCall, sentControlPacketBytesThisCall };
						
						return returnVal; // Send() blocked, onsend will be called when ready to send again
					} else{
						// Send() gave an error
						anErrorHasOccured = true;
					}
				} else {
					// we managed to send some bytes. Perform bookkeeping.
					m_bBusy = false;
					m_hasSent = true;
	
					sent += result;
	
					// Log send bytes in correct class
					if(m_currentPacket_is_controlpacket == false) {
						sentStandardPacketBytesThisCall += result;
	
						if(m_currentPackageIsFromPartFile == true) {
							m_numberOfSentBytesPartFile += result;
						} else {
							m_numberOfSentBytesCompleteFile += result;
						}
					} else {
						sentControlPacketBytesThisCall += result;
						m_numberOfSentBytesControlPacket += result;
					}
				}
			}
	
			if (sent == sendblen){
				// we are done sending the current packet. Delete it and set
				// sendbuffer to NULL so a new packet can be fetched.
				delete[] sendbuffer;
				sendbuffer = NULL;
				sendblen = 0;
	
				if(!m_currentPacket_is_controlpacket) {
					m_actualPayloadSizeSent += m_actualPayloadSize;
					m_actualPayloadSize = 0;
	
					lastFinishedStandard = ::GetTickCount(); // reset timeout
					m_bAccelerateUpload = false; // Safe until told otherwise
				}
	
				sent = 0;
			}
		}
	}
	
	if(onlyAllowedToSendControlPacket && (!m_control_queue.empty() || (sendbuffer != NULL && m_currentPacket_is_controlpacket))) {
		// enter control packet send queue
		// we might enter control packet queue several times for the same package,
		// but that costs very little overhead. Less overhead than trying to make sure
		// that we only enter the queue once.
		//printf("* Requeueing control packet on %p\n", this);
		theApp->uploadBandwidthThrottler->QueueForSendingControlPacket(this, HasSent());
	}

	//printf("* Finishing send debug on %p\n",this);
	
	SocketSentBytes returnVal = { !anErrorHasOccured, sentStandardPacketBytesThisCall, sentControlPacketBytesThisCall };
	
    return returnVal;
}


uint32 CEMSocket::GetNextFragSize(uint32 current, uint32 minFragSize)
{
    if(current % minFragSize == 0) {
        return current;
    } else {
        return minFragSize*(current/minFragSize+1);
    }
}


/**
 * Decides the (minimum) amount the socket needs to send to prevent timeout.
 * 
 * @author SlugFiller
 */
uint32 CEMSocket::GetNeededBytes()
{
	uint32 sendgap;

	uint64 timetotal;
	uint64 timeleft;
	uint64 sizeleft, sizetotal;

	{
		wxMutexLocker lock(m_sendLocker);

		if (byConnected == ES_DISCONNECTED) {
			return 0;
		}
	
		if (!((sendbuffer && !m_currentPacket_is_controlpacket) || !m_standard_queue.empty())) {
			// No standard packet to send. Even if data needs to be sent to prevent timout, there's nothing to send.
			return 0;
		}
	
		if (((sendbuffer && !m_currentPacket_is_controlpacket)) && !m_control_queue.empty())
			m_bAccelerateUpload = true;	// We might be trying to send a block request, accelerate packet
	
		sendgap = ::GetTickCount() - lastCalledSend;
	
		timetotal = m_bAccelerateUpload?45000:90000;
		timeleft = ::GetTickCount() - lastFinishedStandard;
		if (sendbuffer && !m_currentPacket_is_controlpacket) {
			sizeleft = sendblen-sent;
			sizetotal = sendblen;
		} else {
			sizeleft = sizetotal = m_standard_queue.front().packet->GetRealPacketSize();
		}
	}

	if (timeleft >= timetotal)
		return sizeleft;
	timeleft = timetotal-timeleft;
	if (timeleft*sizetotal >= timetotal*sizeleft) {
		// don't use 'GetTimeOut' here in case the timeout value is high,
		if (sendgap > SEC2MS(20))
			return 1;	// Don't let the socket itself time out - Might happen when switching from spread(non-focus) slot to trickle slot
		return 0;
	}
	uint64 decval = timeleft*sizetotal/timetotal;
	if (!decval)
		return sizeleft;
	if (decval < sizeleft)
		return sizeleft-decval+1;	// Round up
	else
		return 1;
}


/**
 * Removes all packets from the standard queue that don't have to be sent for the socket to be able to send a control packet.
 *
 * Before a socket can send a new packet, the current packet has to be finished. If the current packet is part of
 * a split packet, then all parts of that split packet must be sent before the socket can send a control packet.
 *
 * This method keeps in standard queue only those packets that must be sent (rest of split packet), and removes everything
 * after it. The method doesn't touch the control packet queue.
 */
void CEMSocket::TruncateQueues()
{
	wxMutexLocker lock(m_sendLocker);

	// Clear the standard queue totally
    // Please note! There may still be a standardpacket in the sendbuffer variable!
	CStdPacketQueue::iterator it = m_standard_queue.begin();
	for (; it != m_standard_queue.end(); ++it) {
		delete it->packet;
	}
	
	m_standard_queue.clear();
}


uint32 CEMSocket::GetTimeOut() const
{
	return m_uTimeOut;
}


void CEMSocket::SetTimeOut(uint32 uTimeOut)
{
	m_uTimeOut = uTimeOut;
}
// File_checked_for_headers
