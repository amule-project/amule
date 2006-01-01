//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef EMSOCKET_H
#define EMSOCKET_H

#include "Proxy.h"				// Needed for CSocketClientProxy

#include "Types.h"				// Needed for uint8 and uint32
#include "ThrottledSocket.h"	// Needed for ThrottledFileSocket

#include <list>


class CPacket;


#define ERR_WRONGHEADER		0x01
#define ERR_TOOBIG			0x02

#define	ES_DISCONNECTED		0xFF
#define	ES_NOTCONNECTED		0x00
#define	ES_CONNECTED		0x01


const sint32 PACKET_HEADER_SIZE	= 6;


class CEMSocket : public CSocketClientProxy, public ThrottledFileSocket
{
	DECLARE_DYNAMIC_CLASS(CEMSocket)
	
public:
	CEMSocket(const CProxyData *ProxyData = NULL);
	virtual ~CEMSocket();
	
	virtual void 	SendPacket(CPacket* packet, bool delpacket = true, bool controlpacket = true, uint32 actualPayloadSize = 0);
    bool    HasQueues();
	bool	IsConnected() { return byConnected==ES_CONNECTED;};
	uint8	GetConState()	{return byConnected;}
	void	SetDownloadLimit(uint32 limit);
	void	DisableDownloadLimit();

	virtual uint32	GetTimeOut() const;
	virtual void	SetTimeOut(uint32 uTimeOut);
	
    uint32	GetLastCalledSend() { return lastCalledSend; }

    uint64	GetSentBytesCompleteFileSinceLastCallAndReset();
    uint64	GetSentBytesPartFileSinceLastCallAndReset();
    uint64	GetSentBytesControlPacketSinceLastCallAndReset();
    uint64	GetSentPayloadSinceLastCallAndReset();
    void	TruncateQueues();

    virtual SocketSentBytes SendControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize) { return Send(maxNumberOfBytesToSend, minFragSize, true); };
    virtual SocketSentBytes SendFileAndControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize) { return Send(maxNumberOfBytesToSend, minFragSize, false); };

    uint32	GetNeededBytes();
	
	void	Destroy();
	bool OnDestroy() { return DoingDestroy; };
	
	//protected:
	// these functions are public on our code because of the amuleDlg::socketHandler
	virtual void	OnError(int WXUNUSED(nErrorCode)) { };
	virtual void	OnSend(int nErrorCode);	
	virtual void	OnReceive(int nErrorCode);
	
protected:
	bool RecievePending() { return pendingOnReceive; }
	virtual bool	PacketReceived(CPacket* WXUNUSED(packet)) { return false; };
	virtual void	OnClose(int nErrorCode);
	
	uint8	byConnected;
	uint32	m_uTimeOut;

private:
    virtual SocketSentBytes Send(uint32 maxNumberOfBytesToSend, uint32 minFragSize, bool onlyAllowedToSendControlPacket);
	void	ClearQueues();	

    uint32	GetNextFragSize(uint32 current, uint32 minFragSize);
    bool    HasSent() { return m_hasSent; }
	
	// Download (pseudo) rate control	
	uint32	downloadLimit;
	bool	downloadLimitEnable;
	bool	pendingOnReceive;

	// Download partial header
	// actually, this holds only 'PACKET_HEADER_SIZE-1' bytes.
	char	pendingHeader[PACKET_HEADER_SIZE];
	uint32	pendingHeaderSize;

	// Download partial packet
	CPacket* pendingPacket;
	uint32  pendingPacketSize;

	// Upload control
	char*	sendbuffer;
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

	wxMutex	m_sendLocker;

	uint64 m_numberOfSentBytesCompleteFile;
    uint64 m_numberOfSentBytesPartFile;
    uint64 m_numberOfSentBytesControlPacket;
    bool m_currentPackageIsFromPartFile;

	bool	m_bAccelerateUpload;
	uint32	lastCalledSend;
    uint32	lastSent;
	uint32	lastFinishedStandard;

    uint32 m_actualPayloadSize;
    uint32 m_actualPayloadSizeSent;

    bool m_bBusy;
    bool m_hasSent;

	bool DoingDestroy;
};


#endif // EMSOCKET_H
