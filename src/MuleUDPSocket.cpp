//
// This file is part of the aMule Project.
//
// Copyright (C) 2005-2006aMule Team ( admin@amule.org / http://www.amule.org )
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


#include "MuleUDPSocket.h"	// Interface declarations

#include <include/protocol/ed2k/Constants.h>

#include "Logger.h"			// Needed for AddDebugLogLineM
#include "amule.h"			// Needed for theApp
#include "GetTickCount.h"	// Needed for GetTickCount()
#include "Packet.h"			// Needed for CPacket
#include <common/StringFunctions.h>// Needed for unicode2char
#include "Proxy.h"			// Needed for CDatagramSocketProxy
#include "Logger.h"			// Needed for AddDebugLogLineM
#include "UploadBandwidthThrottler.h"


CMuleUDPSocket::CMuleUDPSocket(const wxString& name, int id, const amuleIPV4Address& address, const CProxyData* ProxyData)
	: m_busy(false),
	  m_name(name),
	  m_id(id),
	  m_addr(address),
	  m_proxy(ProxyData),
	  m_socket(NULL)
{
}


CMuleUDPSocket::~CMuleUDPSocket()
{
    theApp.uploadBandwidthThrottler->RemoveFromAllQueues(this);

	if (m_socket) {
		DestroySocket();
	}
}


void CMuleUDPSocket::CreateSocket()
{
	wxCHECK_RET(!m_socket, wxT("Socket already opened."));
	
	m_socket = new CDatagramSocketProxy(m_addr, wxSOCKET_NOWAIT, m_proxy);
	m_socket->SetClientData(this);
	m_socket->SetEventHandler(theApp, m_id);
	m_socket->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG);
	m_socket->Notify(true);

	if (!m_socket->Ok()) {
		AddDebugLogLineM(true, logMuleUDP, wxT("Failed to create valid ") + m_name);
		DestroySocket();
	} else {
		AddDebugLogLineM(true, logMuleUDP, 
			wxString(wxT("Created ")) << m_name << wxT(" at port ") << m_addr.Service());
	}
}


void CMuleUDPSocket::DestroySocket()
{
	wxCHECK_RET(m_socket, wxT("Cannot destroy non-existing socket."));
	
	AddDebugLogLineM(true, logMuleUDP, wxT("Shutting down ") + m_name);
	m_socket->SetNotify(0);
	m_socket->Notify(false);
	m_socket->Close();
	m_socket->Destroy();
	m_socket = NULL;
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

	theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this);
}


const unsigned UDP_BUFFER_SIZE = 8192;


void CMuleUDPSocket::OnReceive(int errorCode)
{
	AddDebugLogLineM(false, logMuleUDP, wxString::Format(wxT("Got UDP callback for read: Error %i Socket state %i"),errorCode, Ok() ? 1 : 0));
	
	if (errorCode or !Ok() or !m_socket) {
		if (m_socket) {
			DestroySocket();
		}
		CreateSocket();
		return;
	}
	
	char buffer[UDP_BUFFER_SIZE];
	amuleIPV4Address addr;
	unsigned length = 0;
	bool error = false;
	int lastError = 0;
	
	{
		wxMutexLocker lock(m_mutex);
		
		length = m_socket->RecvFrom(addr, buffer, UDP_BUFFER_SIZE).LastCount();
		error = m_socket->Error();
		lastError = m_socket->LastError();
	}
	
	if (error) {
		AddDebugLogLineM(false, logMuleUDP, (m_name + wxT(": Error while reading: ")) << lastError);
		return;
	} else if (length < 2) {
		// 2 bytes (protocol and opcode) is the smallets possible packet.
		AddDebugLogLineM(false, logMuleUDP, m_name + wxT(": Invalid Packet received by "));
		return;
	} else if (!StringIPtoUint32(addr.IPAddress())) {
		// wxASSERT(0);
		printf("Unknown ip receiving on UDP packet! Ignoring: '%s'\n", (const char*)unicode2char(addr.IPAddress()));
		return;
	} else if (!addr.Service()) {
		// wxASSERT(0);
		printf("Unknown port receiving an UDP packet! Ignoring\n");
		return;
	}

	AddDebugLogLineM(false, logMuleUDP, (m_name + wxT(": Packet received (")) 
		<< addr.IPAddress() << wxT(":") << addr.Service() << wxT("): ")
		<< length << wxT("b"));
	
	OnPacketReceived(addr, (byte*)buffer, length);
}


void CMuleUDPSocket::SendPacket(CPacket* packet, uint32 IP, uint16 port)
{
	wxCHECK_RET(packet, wxT("Invalid packet."));
	/*wxCHECK_RET(port, wxT("Invalid port."));
	wxCHECK_RET(IP, wxT("Invalid IP."));
	*/

	if (!port || !IP) return;
	
	if (!Ok()) {
		AddDebugLogLineM(false, logMuleUDP, (m_name + wxT(": Packet discarded (socket not Ok): ")) 
			<< Uint32toStringIP(IP) << wxT(":") << port << wxT(" ") << packet->GetPacketSize()
			<< wxT("b"));
	
		delete packet;
		return;
	}
	
	AddDebugLogLineM(false, logMuleUDP, (m_name + wxT(": Packet queued: ")) 
		<< Uint32toStringIP(IP) << wxT(":") << port << wxT(" ") 
		<< packet->GetPacketSize() << wxT("b"));
		
	
	UDPPack newpending;
	newpending.IP = IP;
	newpending.port = port;
	newpending.packet = packet;
	newpending.time = GetTickCount();
    
	{
		wxMutexLocker lock(m_mutex);		
		m_queue.push_back(newpending);
	}
	
	theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this);
}


bool CMuleUDPSocket::Ok()
{
    wxMutexLocker lock(m_mutex);

	return m_socket and m_socket->Ok();
}


SocketSentBytes CMuleUDPSocket::SendControlData(uint32 maxNumberOfBytesToSend, uint32 WXUNUSED(minFragSize))
{
    wxMutexLocker lock(m_mutex);
    uint32 sentBytes = 0;

	while (!m_queue.empty() && !m_busy && (sentBytes < maxNumberOfBytesToSend)) {
		UDPPack item = m_queue.front();
		CPacket* packet = item.packet;
		
		if (GetTickCount() - item.time < UDPMAXQUEUETIME) {
			char sendbuffer[packet->GetPacketSize() + 2];
			memcpy(sendbuffer, packet->GetUDPHeader(), 2);
			memcpy(sendbuffer + 2, packet->GetDataBuffer(), packet->GetPacketSize());

            if (SendTo(sendbuffer, packet->GetPacketSize() + 2, item.IP, item.port)){
                sentBytes += packet->GetPacketSize() + 2; 

				m_queue.pop_front();
				delete packet;
            } else {
				// TODO: Needs better error handling, see SentTo
				break;
			}
		} else {
			m_queue.pop_front();
			delete packet;
		}
	}

    if (!m_busy && !m_queue.empty()) {
        theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this);
    }

    SocketSentBytes returnVal = { true, 0, sentBytes };
    return returnVal;
}


bool CMuleUDPSocket::SendTo(char* buffer, uint32 length, uint32 ip, uint16 port)
{
	// Just pretend that we sent the packet in order to avoid infinete loops.
	if (!(m_socket and m_socket->Ok())) {
		return true;
	}
	
	amuleIPV4Address addr;
	addr.Hostname(ip);
	addr.Service(port);

	// We better clear this flag here, status might have been changed
	// between the U.B.T. adition and the real sending happening later
	m_busy = false; 
	bool sent = false;
	
	m_socket->SendTo(addr, buffer, length);

	if (m_socket->Error()) {
		wxSocketError error = m_socket->LastError();
			
		switch (error) {
			case wxSOCKET_WOULDBLOCK:
				// Socket is busy and can't send this data right now,
				// so we just return not sent and set the wouldblock 
				// flag so it gets resent when socket is ready.
				m_busy = true;
				break;
				
			case wxSOCKET_INVSOCK:
				// Once in a while, wxDatagram sockets have a tendency
				// to suddenly become invalid. In order to work around
				// this problem, we simply create a replacement socket.
				AddDebugLogLineM(true, logMuleUDP, wxT("Socket died. Recreating socket."));
				
				DestroySocket();
				CreateSocket();
				break;
								
			default:
				// An error which we can't handle happended, so we drop 
				// the packet rather than risk entering an infinite loop.
				printf("WARNING! %s discarded packet due to errors (%i) while sending.\n", (const char*)unicode2char(m_name), error);
				sent = true;
				break;

		}
	} else {
		AddDebugLogLineM(false, logMuleUDP, (m_name + wxT(": Packet sent to ")) 
			<< ip << wxT(":") << port << wxT(": ")
			<< length << wxT("b"));
		
		sent = true;
	}

	return sent;
}
// File_checked_for_headers
