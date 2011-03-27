//
// This file is part of the aMule Project.
//
// Copyright (c) 2005-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <wx/wx.h>
#include <algorithm>

#include "MuleUDPSocket.h"              // Interface declarations

#include <protocol/ed2k/Constants.h>

#include "amule.h"                      // Needed for theApp
#include "GetTickCount.h"               // Needed for GetTickCount()
#include "Packet.h"                     // Needed for CPacket
#include <common/StringFunctions.h>     // Needed for unicode2char
#include "Proxy.h"                      // Needed for CDatagramSocketProxy
#include "Logger.h"                     // Needed for AddDebugLogLine{C,N}
#include "UploadBandwidthThrottler.h"
#include "EncryptedDatagramSocket.h"
#include "OtherFunctions.h"
#include "kademlia/kademlia/Prefs.h"
#include "ClientList.h"


CMuleUDPSocket::CMuleUDPSocket(const wxString& name, int id, const amuleIPV4Address& address, const CProxyData* ProxyData)
:
m_busy(false),
m_name(name),
m_id(id),
m_addr(address),
m_proxy(ProxyData),
m_socket(NULL)
{
}


CMuleUDPSocket::~CMuleUDPSocket()
{
	theApp->uploadBandwidthThrottler->RemoveFromAllQueues(this);

	wxMutexLocker lock(m_mutex);
	DestroySocket();
}


void CMuleUDPSocket::CreateSocket()
{
	wxCHECK_RET(!m_socket, wxT("Socket already opened."));
	
	m_socket = new CEncryptedDatagramSocket(m_addr, wxSOCKET_NOWAIT, m_proxy);
	m_socket->SetClientData(this);
	m_socket->SetEventHandler(*theApp, m_id);
	m_socket->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_LOST_FLAG);
	m_socket->Notify(true);

	if (!m_socket->Ok()) {
		AddDebugLogLineC(logMuleUDP, wxT("Failed to create valid ") + m_name);
		DestroySocket();
	} else {
		AddLogLineN(wxString(wxT("Created ")) << m_name << wxT(" at port ") << m_addr.Service());
	}
}


void CMuleUDPSocket::DestroySocket()
{
	if (m_socket) {
		AddDebugLogLineN(logMuleUDP, wxT("Shutting down ") + m_name);
		m_socket->SetNotify(0);
		m_socket->Notify(false);
		m_socket->Close();
		m_socket->Destroy();
		m_socket = NULL;
	}
}	


void CMuleUDPSocket::Open()
{
	wxMutexLocker lock(m_mutex);

	CreateSocket();
}


void CMuleUDPSocket::Close()
{
	wxMutexLocker lock(m_mutex);

	DestroySocket();
}


void CMuleUDPSocket::OnSend(int errorCode)
{
	if (errorCode) {
		return;
	}
	
	{
		wxMutexLocker lock(m_mutex);
		m_busy = false;
		if (m_queue.empty()) {
			return;
		}
	}

	theApp->uploadBandwidthThrottler->QueueForSendingControlPacket(this);
}


const unsigned UDP_BUFFER_SIZE = 16384;


void CMuleUDPSocket::OnReceive(int errorCode)
{
	AddDebugLogLineN(logMuleUDP, CFormat(wxT("Got UDP callback for read: Error %i Socket state %i"))
		% errorCode % Ok());
	
	char buffer[UDP_BUFFER_SIZE];
	wxIPV4address addr;
	unsigned length = 0;
	bool error = false;
	int lastError = 0;
	
	{
		wxMutexLocker lock(m_mutex);

		if (errorCode || (m_socket == NULL) || !m_socket->Ok()) {
			DestroySocket();
			CreateSocket();

			return;
		}

		
		length = m_socket->RecvFrom(addr, buffer, UDP_BUFFER_SIZE).LastCount();
		error = m_socket->Error();
		lastError = m_socket->LastError();
	}
	
	const uint32 ip = StringIPtoUint32(addr.IPAddress());
	const uint16 port = addr.Service();
	if (error) {
		OnReceiveError(lastError, ip, port);
	} else if (length < 2) {
		// 2 bytes (protocol and opcode) is the smallets possible packet.
		AddDebugLogLineN(logMuleUDP, m_name + wxT(": Invalid Packet received"));
	} else if (!ip) {
		// wxFAIL;
		AddLogLineNS(wxT("Unknown ip receiving a UDP packet! Ignoring: '") + addr.IPAddress() + wxT("'"));
	} else if (!port) {
		// wxFAIL;
		AddLogLineNS(wxT("Unknown port receiving a UDP packet! Ignoring"));
	} else if (theApp->clientlist->IsBannedClient(ip)) {
		AddDebugLogLineN(logMuleUDP, m_name + wxT(": Dropped packet from banned IP ") + addr.IPAddress());
	} else {
		AddDebugLogLineN(logMuleUDP, (m_name + wxT(": Packet received ("))
			<< addr.IPAddress() << wxT(":") << port << wxT("): ")
			<< length << wxT("b"));
		OnPacketReceived(ip, port, (byte*)buffer, length);
	}
}


void CMuleUDPSocket::OnReceiveError(int errorCode, uint32 WXUNUSED(ip), uint16 WXUNUSED(port))
{
	AddDebugLogLineN(logMuleUDP, (m_name + wxT(": Error while reading: ")) << errorCode);
}


void CMuleUDPSocket::OnDisconnected(int WXUNUSED(errorCode))
{
	/* Due to bugs in wxWidgets, UDP sockets will sometimes
	 * be closed. This is caused by the fact that wx treats
	 * zero-length datagrams as EOF, which is only the case
	 * when dealing with streaming sockets. 
	 *
	 * This has been reported as patch #1885472:
	 * http://sourceforge.net/tracker/index.php?func=detail&aid=1885472&group_id=9863&atid=309863
	 */
	AddDebugLogLineC(logMuleUDP, m_name + wxT("Socket died, recreating."));
	DestroySocket();
	CreateSocket();
}


void CMuleUDPSocket::SendPacket(CPacket* packet, uint32 IP, uint16 port, bool bEncrypt, const uint8* pachTargetClientHashORKadID, bool bKad, uint32 nReceiverVerifyKey)
{
	wxCHECK_RET(packet, wxT("Invalid packet."));
	/*wxCHECK_RET(port, wxT("Invalid port."));
	wxCHECK_RET(IP, wxT("Invalid IP."));
	*/

	if (!port || !IP) {
		return;
	}
	
	if (!Ok()) {
		AddDebugLogLineN(logMuleUDP, (m_name + wxT(": Packet discarded, socket not Ok ("))
			<< Uint32_16toStringIP_Port(IP, port) << wxT("): ") << packet->GetPacketSize() << wxT("b"));
		delete packet;

		return;
	}
	
	AddDebugLogLineN(logMuleUDP, (m_name + wxT(": Packet queued ("))
		<< Uint32_16toStringIP_Port(IP, port) << wxT("): ") << packet->GetPacketSize() << wxT("b"));
	
	UDPPack newpending;
	newpending.IP = IP;
	newpending.port = port;
	newpending.packet = packet;
	newpending.time = GetTickCount();
 	newpending.bEncrypt = bEncrypt && (pachTargetClientHashORKadID != NULL || (bKad && nReceiverVerifyKey != 0));
	newpending.bKad = bKad;
	newpending.nReceiverVerifyKey = nReceiverVerifyKey;   
	if (newpending.bEncrypt && pachTargetClientHashORKadID != NULL) {
		md4cpy(newpending.pachTargetClientHashORKadID, pachTargetClientHashORKadID);
	} else {
		md4clr(newpending.pachTargetClientHashORKadID);
	}

	{
		wxMutexLocker lock(m_mutex);		
		m_queue.push_back(newpending);
	}

	theApp->uploadBandwidthThrottler->QueueForSendingControlPacket(this);
}


bool CMuleUDPSocket::Ok()
{
	wxMutexLocker lock(m_mutex);

	return m_socket && m_socket->Ok();
}


SocketSentBytes CMuleUDPSocket::SendControlData(uint32 maxNumberOfBytesToSend, uint32 WXUNUSED(minFragSize))
{
	wxMutexLocker lock(m_mutex);
	uint32 sentBytes = 0;
	while (!m_queue.empty() && !m_busy && (sentBytes < maxNumberOfBytesToSend)) {
		UDPPack item = m_queue.front();
		CPacket* packet = item.packet;
		if (GetTickCount() - item.time < UDPMAXQUEUETIME) {
			uint32_t len = packet->GetPacketSize() + 2;
			uint8_t *sendbuffer = new uint8_t [len];
			memcpy(sendbuffer, packet->GetUDPHeader(), 2);
			memcpy(sendbuffer + 2, packet->GetDataBuffer(), packet->GetPacketSize());

			if (item.bEncrypt && (theApp->GetPublicIP() > 0 || item.bKad)) {
				len = CEncryptedDatagramSocket::EncryptSendClient(&sendbuffer, len, item.pachTargetClientHashORKadID, item.bKad, item.nReceiverVerifyKey, (item.bKad ? Kademlia::CPrefs::GetUDPVerifyKey(item.IP) : 0));
			}

			if (SendTo(sendbuffer, len, item.IP, item.port)) {
				sentBytes += len;
				m_queue.pop_front();
				delete packet;
				delete [] sendbuffer;
			} else {
				// TODO: Needs better error handling, see SentTo
				delete [] sendbuffer;
				break;
			}
		} else {
			m_queue.pop_front();
			delete packet;
		}
	}
	if (!m_busy && !m_queue.empty()) {
		theApp->uploadBandwidthThrottler->QueueForSendingControlPacket(this);
	}
	SocketSentBytes returnVal = { true, 0, sentBytes };

	return returnVal;
}


bool CMuleUDPSocket::SendTo(uint8_t *buffer, uint32_t length, uint32_t ip, uint16_t port)
{
	// Just pretend that we sent the packet in order to avoid infinite loops.
	if (!(m_socket && m_socket->Ok())) {
		return true;
	}
	
	amuleIPV4Address addr;
	addr.Hostname(ip);
	addr.Service(port);

	// We better clear this flag here, status might have been changed
	// between the U.B.T. addition and the real sending happening later
	m_busy = false; 
	bool sent = false;
	m_socket->SendTo(addr, buffer, length);
	if (m_socket->Error()) {
		wxSocketError error = m_socket->LastError();
		
		if (error == wxSOCKET_WOULDBLOCK) {
			// Socket is busy and can't send this data right now,
			// so we just return not sent and set the wouldblock 
			// flag so it gets resent when socket is ready.
			m_busy = true;
		} else {
			// An error which we can't handle happended, so we drop 
			// the packet rather than risk entering an infinite loop.
			AddLogLineN((wxT("WARNING! ") + m_name + wxT(": Packet to ")) 
				<< Uint32_16toStringIP_Port(ip, port)
				<< wxT(" discarded due to error (") << error << wxT(") while sending."));
			sent = true;
		}
	} else {
		AddDebugLogLineN(logMuleUDP, (m_name + wxT(": Packet sent ("))
			<< Uint32_16toStringIP_Port(ip, port) << wxT("): ")
			<< length << wxT("b"));
		sent = true;
	}

	return sent;
}

// File_checked_for_headers
