//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
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

#ifndef EMSOCKET_H
#define EMSOCKET_H

#include <mutex>

#include "EncryptedStreamSocket.h"				// Needed for CEncryptedStreamSocket

#include "ThrottledSocket.h"	// Needed for ThrottledFileSocket

class CPacket;

#define ERR_WRONGHEADER		0x01
#define ERR_TOOBIG			0x02

#define	ES_DISCONNECTED		0xFF
#define	ES_NOTCONNECTED		0x00
#define	ES_CONNECTED		0x01


const uint32 PACKET_HEADER_SIZE	= 6;


class CEMSocket : public CEncryptedStreamSocket, public ThrottledFileSocket
{
public:
	CEMSocket(const CProxyData *ProxyData = NULL);
	virtual ~CEMSocket();

	virtual void	SendPacket(CPacket* packet, bool delpacket = true, bool controlpacket = true, uint32 actualPayloadSize = 0);
	bool	IsConnected() { return byConnected==ES_CONNECTED;};
	uint8	GetConState()	{return byConnected;}
	// Re-trigger OnReceive if this socket suspended its read loop last
	// tick because CDownloadBandwidthThrottler's bucket was empty.
	// Called once per tick from CPartFile::Process via
	// CUpDownClient::TickDownloadAndMeasure.
	void	WakeIfPaused();

	virtual uint64	GetTimeOut() const;
	virtual void	SetTimeOut(uint64 uTimeOut);

    uint64	GetLastCalledSend() override { return lastCalledSend; }

    uint64	GetSentBytesCompleteFileSinceLastCallAndReset();
    uint64	GetSentBytesPartFileSinceLastCallAndReset();
    uint64	GetSentBytesControlPacketSinceLastCallAndReset();
    uint64	GetSentPayloadSinceLastCallAndReset();
    uint64	PeekSentPayload();   // Non-resetting peek — for disk I/O thread buffer check
    void	TruncateQueues();

    SocketSentBytes SendControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize) override { return Send(maxNumberOfBytesToSend, minFragSize, true); };
    SocketSentBytes SendFileAndControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize) override { return Send(maxNumberOfBytesToSend, minFragSize, false); };

    uint32	GetNeededBytes() override;
    bool    HasSent() { return m_hasSent; }  // eMule ref: used by CUploadDiskIOThread to detect socket starvation
    bool    HasQueues(bool bOnlyStandardPackets = false) const;
    bool    IsBusyQuickCheck() const { return m_bBusy; }

	// Whether OnReceive should gate reads through the global
	// download bandwidth budget. Peer file-transfer sockets do;
	// server-control sockets do not, since their traffic is tiny
	// and latency-sensitive and would stall silently under a
	// download cap tight enough to exhaust the throttler bucket.
	virtual bool	IsDownloadThrottled() const { return true; }

	//protected:
	// these functions are public on our code because of the amuleDlg::socketHandler
	void	OnError(int WXUNUSED(nErrorCode)) override { };
	void	OnSend(int nErrorCode) override;
	void	OnReceive(int nErrorCode) override;
	void	OnConnect(int nErrorCode) override = 0;

	// The Asio reactor's HandleRead dispatches peer FIN / RST via
	// CLibSocket::OnLost(int), whose default is an empty no-op. Without
	// an override on this path, the eD2k server socket (CServerSocket)
	// and peer socket (CClientTCPSocket) never see the close event:
	// CServerSocket stays at CS_CONNECTED after a server disappears
	// (#905, #393), and peer sockets sit in CLOSE_WAIT forever as
	// silent half-open sources. Forward to OnClose(int) so virtual
	// dispatch reaches the per-class teardown that was already there
	// for the (now-defunct) wxSocket close path.
	void OnLost(int nErrorCode) override { OnClose(nErrorCode); }

protected:

	virtual bool	PacketReceived(CPacket* WXUNUSED(packet)) { return false; };
	virtual void	OnClose(int nErrorCode);

	uint8	byConnected;
	uint32	m_uTimeOut;

private:
    virtual SocketSentBytes Send(uint32 maxNumberOfBytesToSend, uint32 minFragSize, bool onlyAllowedToSendControlPacket);
	void	ClearQueues();

    uint32	GetNextFragSize(uint32 current, uint32 minFragSize);

	// Download rate control: the global cap is enforced by
	// CDownloadBandwidthThrottler. pendingOnReceive is the only
	// per-socket bit -- set when OnReceive() suspended its read loop
	// because the throttler's bucket was empty, cleared on the next
	// successful read or when WakeIfPaused() retries the loop.
	bool	pendingOnReceive;

	// Download partial header
	uint8	pendingHeader[PACKET_HEADER_SIZE];
	uint32	pendingHeaderSize;

	// Download partial packet
	uint8*	pendingPacket;
	uint32	pendingPacketSize;

	// Upload control
	uint8*	sendbuffer;
	uint32	sendblen;
	uint32	sent;

	typedef std::list<CPacket*> CPacketQueue;
	CPacketQueue m_control_queue;

	struct StandardPacketQueueEntry
	{
		uint32 actualPayloadSize;
		CPacket* packet;
	};

	typedef	std::list<StandardPacketQueueEntry> CStdPacketQueue;
	CStdPacketQueue m_standard_queue;

    bool m_currentPacket_is_controlpacket;

	std::mutex	m_sendLocker;

	uint64 m_numberOfSentBytesCompleteFile;
    uint64 m_numberOfSentBytesPartFile;
    uint64 m_numberOfSentBytesControlPacket;
    bool m_currentPackageIsFromPartFile;

	bool	m_bAccelerateUpload;
	uint64	lastCalledSend;
	uint64	lastSent;
	uint64	lastFinishedStandard;

    uint32 m_actualPayloadSize;
    uint32 m_actualPayloadSizeSent;

    bool m_bBusy;
    bool m_hasSent;
};


#endif // EMSOCKET_H
// File_checked_for_headers
