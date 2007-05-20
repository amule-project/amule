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

/* This class supports obfuscation and encryption for a *Mule tcp connection.
   Right now only basic obfusication is supported, but this can be expanded, as their is a
   dedicated handshake to negotiate the encryption method used.

   Please note, even if obfuscation uses encryption methods, it does not fulfill cryptographic standards since it
   doesn't use secret (and for rc4 important: unique) keys
*/

#ifndef __ENCRYPTEDSTREAMSOCKET_H__
#define __ENCRYPTEDSTREAMSOCKET_H__

#include <wx/string.h>

// cryptoPP used for DH integer calculations
#include "CryptoPP_Inc.h"	// Needed for Crypto functions
#include "Types.h"
#include "Proxy.h"

#include "RC4Encrypt.h"

#define ERR_WRONGHEADER				0x01
#define ERR_TOOBIG					0x02
#define ERR_ENCRYPTION				0x03
#define ERR_ENCRYPTION_NOTALLOWED	0x04

enum EStreamCryptState {
	ECS_NONE = 0,			// Disabled or not available
	ECS_UNKNOWN,			// Incoming connection, will test the first incoming data for encrypted protocol
	ECS_PENDING,			// Outgoing connection, will start sending encryption protocol
	ECS_PENDING_SERVER,		// Outgoing serverconnection, will start sending encryption protocol
	ECS_NEGOTIATING,		// Encryption supported, handshake still uncompleted
	ECS_ENCRYPTING			// Encryption enabled
};

enum ENegotiatingState {
	ONS_NONE,
	
	ONS_BASIC_CLIENTA_RANDOMPART,
	ONS_BASIC_CLIENTA_MAGICVALUE,
	ONS_BASIC_CLIENTA_METHODTAGSPADLEN,
	ONS_BASIC_CLIENTA_PADDING,
	
	ONS_BASIC_CLIENTB_MAGICVALUE,
	ONS_BASIC_CLIENTB_METHODTAGSPADLEN,
	ONS_BASIC_CLIENTB_PADDING,

	ONS_BASIC_SERVER_DHANSWER,
	ONS_BASIC_SERVER_MAGICVALUE,
	ONS_BASIC_SERVER_METHODTAGSPADLEN,
	ONS_BASIC_SERVER_PADDING,
	ONS_BASIC_SERVER_DELAYEDSENDING,

	ONS_COMPLETE
};

enum EEncryptionMethods {
	ENM_OBFUSCATION = 0x00
};

class CRC4EncryptableBuffer;

class CEncryptedStreamSocket : public CSocketClientProxy
{
	DECLARE_DYNAMIC_CLASS(CEncryptedStreamSocket)
public:
	CEncryptedStreamSocket(wxSocketFlags flags = wxSOCKET_NONE, const CProxyData *proxyData = NULL);
	virtual ~CEncryptedStreamSocket();

	void	SetConnectionEncryption(bool bEnabled, const uint8* pTargetClientHash, bool bServerConnection);
	uint32	GetRealReceivedBytes() const		{ return m_nObfusicationBytesReceived; } // indicates how many bytes were received including obfusication so that the parent knows if the receive limit was reached
	bool	IsObfusicating() const				{ return m_StreamCryptState == ECS_ENCRYPTING && m_EncryptionMethod == ENM_OBFUSCATION; }
	
	bool	IsServerCryptEnabledConnection() const { return m_bServerCrypt; }	

	uint8	m_dbgbyEncryptionSupported;
	uint8	m_dbgbyEncryptionRequested;
	uint8	m_dbgbyEncryptionMethodSet;

protected:
	int Write(const void* lpBuf, wxUint32 nBufLen);
	int Read(void* lpBuf, wxUint32 nBufLen);
	virtual void OnError(int nErrorCode) {};
	virtual void	OnSend(int nErrorCode);
	wxString			DbgGetIPString();
	void			CryptPrepareSendData(uint8* pBuffer, uint32 nLen);
	bool			IsEncryptionLayerReady();
	uint8			GetSemiRandomNotProtocolMarker() const;
	
	uint32	m_nObfusicationBytesReceived;
	EStreamCryptState	m_StreamCryptState;
	EEncryptionMethods  m_EncryptionMethod;
	bool	m_bFullReceive;
	bool	m_bServerCrypt;

private:
	int		Negotiate(const uint8* pBuffer, uint32 nLen);
	void	StartNegotiation(bool bOutgoing);
	int		SendNegotiatingData(const void* lpBuf, uint32 nBufLen, uint32 nStartCryptFromByte = 0, bool bDelaySend = false);

	ENegotiatingState	m_NegotiatingState;
	CRC4EncryptableBuffer		m_pfiReceiveBuffer;
	uint32				m_nReceiveBytesWanted;
	CRC4EncryptableBuffer		m_pfiSendBuffer;
	uint32				m_nRandomKeyPart;
	CryptoPP::Integer	m_cryptDHA;

};

#endif // __ENCRYPTEDSTREAMSOCKET_H__
