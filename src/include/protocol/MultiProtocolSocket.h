
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

#ifndef PROTOCOL_MULTIPROTOCOLSOCKET_H
#define PROTOCOL_MULTIPROTOCOLSOCKET_H

#include "../LibSocket.h"
#include "../Packet.h"
#include <memory>

namespace MultiProtocol {

// Protocol types supported by MultiProtocolSocket
enum class SocketProtocol {
    ED2K_TCP,
    KAD_UDP
};

// Forward declaration
class CPacket;

// MultiProtocolSocket class - supports multiple protocols
class MultiProtocolSocket : public CLibSocket {
public:
    explicit MultiProtocolSocket(SocketProtocol protocol);
    virtual ~MultiProtocolSocket();

    // Protocol handshake
    bool protocol_handshake();

    // Process incoming packet
    bool process_packet(CPacket* packet);

    // Virtual function overrides from CLibSocket
    virtual void OnConnect(int nErrorCode) override;
    virtual void OnSend(int nErrorCode) override;
    virtual void OnReceive(int nErrorCode) override;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace MultiProtocol

#endif // PROTOCOL_MULTIPROTOCOLSOCKET_H
