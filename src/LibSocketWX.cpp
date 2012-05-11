//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Stu Redman ( sturedman@amule.org )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

//
// Implementation of amuleIPV4Address for wxWidgets sockets
// (Implementation for Asio is in LibSocketAsio.cpp
//

#include "LibSocket.h"
#include "Logger.h"


class CamuleIPV4Endpoint : public wxIPV4address {
public:
	CamuleIPV4Endpoint() {}

	CamuleIPV4Endpoint(const CamuleIPV4Endpoint & impl) : wxIPV4address(impl) {}

///	operator wxSockAddress& () { return * this; }
};



bool CLibSocket::Connect(amuleIPV4Address& adr, bool wait)
{
	return wxSocketClient::Connect(adr.GetEndpoint(), wait);
}

bool CLibSocket::GetPeer(amuleIPV4Address& adr)
{
	return wxSocketClient::GetPeer(adr.GetEndpoint());
}

void CLibSocket::SetLocal(amuleIPV4Address& local)
{
	wxSocketClient::SetLocal(local.GetEndpoint());
}


CLibSocketServer::CLibSocketServer(const amuleIPV4Address &address,	wxSocketFlags flags) : wxSocketServer(address.GetEndpoint(), flags)
{
}

CLibUDPSocket::CLibUDPSocket(amuleIPV4Address &address, wxSocketFlags flags) : wxDatagramSocket(address.GetEndpoint(), flags)
{
}

uint32 CLibUDPSocket::RecvFrom(amuleIPV4Address& addr, void* buf, uint32 nBytes)
{
	wxDatagramSocket::RecvFrom(addr.GetEndpoint(), buf, nBytes);
	return wxDatagramSocket::LastCount();
}

uint32 CLibUDPSocket::SendTo(const amuleIPV4Address& addr, const void* buf, uint32 nBytes)
{
	wxDatagramSocket::SendTo(addr.GetEndpoint(), buf, nBytes);
	return wxDatagramSocket::LastCount();
}


amuleIPV4Address::amuleIPV4Address()
{
	m_endpoint = new CamuleIPV4Endpoint();
}

amuleIPV4Address::amuleIPV4Address(const amuleIPV4Address &a)
{
	*this = a;
}

amuleIPV4Address::~amuleIPV4Address()
{
	delete m_endpoint;
}

amuleIPV4Address& amuleIPV4Address::operator=(const amuleIPV4Address &a)
{
	m_endpoint = new CamuleIPV4Endpoint(* a.m_endpoint);
	return *this;
}

bool amuleIPV4Address::Hostname(const wxString& name)
{
	if (name.IsEmpty()) {
		return false;
	}
	return m_endpoint->Hostname(name);
}

bool amuleIPV4Address::Service(uint16 service)
{
	if (service == 0) {
		return false;
	}
	return m_endpoint->Service(service);
}

uint16 amuleIPV4Address::Service() const
{
	return m_endpoint->Service();
}

bool amuleIPV4Address::IsLocalHost() const
{
	return m_endpoint->IsLocalHost();
}

wxString amuleIPV4Address::IPAddress() const
{
	return m_endpoint->IPAddress();
}

// Set address to any of the addresses of the current machine.
bool amuleIPV4Address::AnyAddress()
{
	bool ret = m_endpoint->AnyAddress();
	AddDebugLogLineN(logGeneral, CFormat(wxT("AnyAddress() returned %s")) % IPAddress());
	return ret;
}

const CamuleIPV4Endpoint & amuleIPV4Address::GetEndpoint() const
{
	return * m_endpoint;
}

CamuleIPV4Endpoint & amuleIPV4Address::GetEndpoint()
{
	return * m_endpoint;
}

wxString MuleBoostVersion;
