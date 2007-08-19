//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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

/* Basic Obfusicated Handshake Protocol UDP:
	see EncryptedStreamSocket.h

****************************** ED2K Packets

-Keycreation Client <-> Clinet:
	- Client A (Outgoing connection):
		Sendkey: Md5(<UserHashClientB 16><IPClientA 4><MagicValue91 1><RandomKeyPartClientA 2>)  23
	- Client B (Incomming connection):
		Receivekey: Md5(<UserHashClientB 16><IPClientA 4><MagicValue91 1><RandomKeyPartClientA 2>)  23
	 - Note: The first 1024 Bytes will be _NOT_ discarded for UDP keys to safe CPU time

	- Handshake
		-> The handshake is encrypted - except otherwise noted - by the Keys created above
		-> Padding is cucrently not used for UDP meaning that PaddingLen will be 0, using PaddingLens up to 16 Bytes is acceptable however
		Client A: <SemiRandomNotProtocolMarker 7 Bits[Unencrypted]><ED2K Marker 1Bit = 1><RandomKeyPart 2[Unencrypted]><MagicValue 4><PaddingLen 1><RandomBytes PaddingLen%16>	
	
	- Additional Comments:
		- For obvious reasons the UDP handshake is actually no handshake. If a different Encryption method (or better a different Key) is to be used this has to be negotiated in a TCP connection
		- SemiRandomNotProtocolMarker is a Byte which has a value unequal any Protocol header byte. This is a compromiss, turning in complete randomness (and nice design) but gaining a lower CPU usage
		- Kad/Ed2k Marker are only indicators, which possibility could be tried first, and should not be trusted

****************************** Server Packets

-Keycreation Client <-> Server:
	- Client A (Outgoing connection client -> server):
		Sendkey: Md5(<BaseKey 4><MagicValueClientServer 1><RandomKeyPartClientA 2>)  7
	- Client B (Incomming connection):
		Receivekey: Md5(<BaseKey 4><MagicValueServerClient 1><RandomKeyPartClientA 2>)  7
	- Note: The first 1024 Bytes will be _NOT_ discarded for UDP keys to safe CPU time

	- Handshake
		-> The handshake is encrypted - except otherwise noted - by the Keys created above
		-> Padding is cucrently not used for UDP meaning that PaddingLen will be 0, using PaddingLens up to 16 Bytes is acceptable however
		Client A: <SemiRandomNotProtocolMarker 1[Unencrypted]><RandomKeyPart 2[Unencrypted]><MagicValue 4><PaddingLen 1><RandomBytes PaddingLen%16>	

	- Overhead: 8 Bytes per UDP Packet
	
	- Security for Basic Obfusication:
		- Random looking packets, very limited protection against passive eavesdropping single packets
	
	- Additional Comments:
		- For obvious reasons the UDP handshake is actually no handshake. If a different Encryption method (or better a different Key) is to be used this has to be negotiated in a TCP connection
		- SemiRandomNotProtocolMarker is a Byte which has a value unequal any Protocol header byte. This is a compromiss, turning in complete randomness (and nice design) but gaining a lower CPU usage

****************************** KAD Packets
			  
-Keycreation Client <-> Client:
	- Client A (Outgoing connection):
		Sendkey: Md5(<KadID 16><RandomKeyPartClientA 2>)  18
	- Client B (Incomming connection):
		Receivekey: Md5(<KadID 16><RandomKeyPartClientA 2>)  18
	- Note: The first 1024 Bytes will be _NOT_ discarded for UDP keys to safe CPU time

	- Handshake
		-> The handshake is encrypted - except otherwise noted - by the Keys created above
		-> Padding is cucrently not used for UDP meaning that PaddingLen will be 0, using PaddingLens up to 16 Bytes is acceptable however
		Client A: <SemiRandomNotProtocolMarker 7 Bits[Unencrypted]><Kad Marker 1Bit = 0><RandomKeyPart 2[Unencrypted]><MagicValue 4><PaddingLen 1><RandomBytes PaddingLen%16><ReceiverVerifyKey 2><SenderVerifyKey 2>

	- Overhead: 12 Bytes per UDP Packet
	
	- Additional Comments:
		- For obvious reasons the UDP handshake is actually no handshake. If a different Encryption method (or better a different Key) is to be used this has to be negotiated in a TCP connection
		- SemiRandomNotProtocolMarker is a Byte which has a value unequal any Protocol header byte. This is a compromiss, turning in complete randomness (and nice design) but gaining a lower CPU usage
		- Kad/Ed2k Marker are only indicators, which possibility could be tried first, and should not be trusted
*/

#include "EncryptedDatagramSocket.h"
#include "amule.h"
#include "Logger.h"
#include "Preferences.h"
#include "RC4Encrypt.h"
#include "./kademlia/kademlia/Prefs.h"
#include "./kademlia/kademlia/Kademlia.h"
#include "RandomFunctions.h"

#include <include/protocol/Protocols.h>
#include <common/MD5Sum.h>

// random generator
#include "CryptoPP_Inc.h"	// Needed for Crypto functions

#define CRYPT_HEADER_WITHOUTPADDING		    8
#define	MAGICVALUE_UDP						91
#define MAGICVALUE_UDP_SYNC_CLIENT			0x395F2EC1
#define MAGICVALUE_UDP_SYNC_SERVER			0x13EF24D5
#define	MAGICVALUE_UDP_SERVERCLIENT			0xA5
#define	MAGICVALUE_UDP_CLIENTSERVER			0x6B

CEncryptedDatagramSocket::CEncryptedDatagramSocket( wxIPaddress &address, wxSocketFlags flags,	const CProxyData *proxyData) : CDatagramSocketProxy(address, flags, proxyData)
{
	
}

CEncryptedDatagramSocket::~CEncryptedDatagramSocket()
{

}

int CEncryptedDatagramSocket::DecryptReceivedClient(uint8* pbyBufIn, int nBufLen, uint8** ppbyBufOut, uint32 dwIP, uint16* nReceiverVerifyKey, uint16* nSenderVerifyKey) {
	int nResult = nBufLen;
	*ppbyBufOut = pbyBufIn;
	
	if (nResult <= CRYPT_HEADER_WITHOUTPADDING /*|| !thePrefs.IsClientCryptLayerSupported()*/)
		return nResult;

	if (nReceiverVerifyKey == NULL || nSenderVerifyKey == NULL){
		wxASSERT( false );
		return nResult;
	}
	
	switch (pbyBufIn[0]){
		case OP_EMULEPROT:
		case OP_KADEMLIAPACKEDPROT:
		case OP_KADEMLIAHEADER:
		case OP_UDPRESERVEDPROT1:
		case OP_UDPRESERVEDPROT2:
		case OP_PACKEDPROT:
			return nResult; // no encrypted packet (see description on top)
		default:
			;
	}
	
	bool bKad = (pbyBufIn[0] & 0x01) == 0; // check the marker bit if this is a kad or ed2k packet, this is only an indicator since old clients have it set random
	// might be an encrypted packet, try to decrypt
	
	CRC4EncryptableBuffer receivebuffer;
	uint32 dwValue = 0;
	bool bFlipTry = false;
	do{
		bKad = bFlipTry ? !bKad : bKad;
		MD5Sum md5;
		
		if (bKad){
			if (Kademlia::CKademlia::GetPrefs()) {
				uint8 achKeyData[18];
				memcpy(achKeyData, Kademlia::CKademlia::GetPrefs()->GetKadID().GetData(), 16);
				memcpy(achKeyData + 16, pbyBufIn + 1, 2); // random key part sent from remote client
				md5.Calculate(achKeyData, sizeof(achKeyData));
			}
		} else{
			uint8 achKeyData[23];
			md4cpy(achKeyData, thePrefs::GetUserHash().GetHash());
			achKeyData[20] = MAGICVALUE_UDP;
			memcpy(achKeyData + 16, &dwIP, 4);
			memcpy(achKeyData + 21, pbyBufIn + 1, 2); // random key part sent from remote client
			md5.Calculate(achKeyData, sizeof(achKeyData));
		}
		
		receivebuffer.SetKey(md5, true);
		receivebuffer.RC4Crypt(pbyBufIn + 3, (uint8*)&dwValue, sizeof(dwValue));
		bFlipTry = !bFlipTry; // next round try the other possibility
	} while (dwValue != MAGICVALUE_UDP_SYNC_CLIENT && bFlipTry); // try to decrypt as ed2k as well as kad packet if needed (max 2 rounds)
	
	if (dwValue == MAGICVALUE_UDP_SYNC_CLIENT){
		// Yup this is an encrypted packet
		//DEBUG_ONLY( DebugLog(_T("Received obfuscated UDP packet from clientIP: %s"), ipstr(dwIP)) );
		uint8 byPadLen;
		
		receivebuffer.RC4Crypt(pbyBufIn + 7, (uint8*)&byPadLen, 1);
		nResult -= CRYPT_HEADER_WITHOUTPADDING;
		
		if (nResult <= byPadLen){
			//DebugLogError(_T("Invalid obfuscated UDP packet from clientIP: %s, Paddingsize (%u) larger than received bytes"), ipstr(dwIP), byPadLen);
			return nBufLen; // pass through, let the Receivefunction do the errorhandling on this junk
		}
		
		if (byPadLen > 0) {
			receivebuffer.RC4Crypt(NULL, NULL, byPadLen);
		}
		
		nResult -= byPadLen;

		if (bKad){
			if (nResult <= 4){
				//DebugLogError(_T("Obfuscated Kad packet with mismatching size (verify keys missing) received from clientIP: %s"), ipstr(dwIP));
				return nBufLen; // pass through, let the Receivefunction do the errorhandling on this junk;
			}
			// read the verify keys
			*nReceiverVerifyKey = PeekUInt16(pbyBufIn + CRYPT_HEADER_WITHOUTPADDING + byPadLen);
			*nSenderVerifyKey = PeekUInt16(pbyBufIn + CRYPT_HEADER_WITHOUTPADDING + byPadLen + 2);
			nResult -= 4;
		} else {
			*nReceiverVerifyKey = 0;
			*nSenderVerifyKey = 0;
		}
		
		*ppbyBufOut = pbyBufIn + (nBufLen - nResult);
		
		receivebuffer.RC4Crypt((uint8*)*ppbyBufOut, (uint8*)*ppbyBufOut, nResult);
		//theStats.AddDownDataOverheadCrypt(nBufLen - nResult);
		return nResult; // done
	} else{
		//DebugLogWarning(_T("Obfuscated packet expected but magicvalue mismatch on UDP packet from clientIP: %s"), ipstr(dwIP));
		return nBufLen; // pass through, let the Receivefunction do the errorhandling on this junk
	}
}

int CEncryptedDatagramSocket::EncryptSendClient(uint8** ppbyBuf, int nBufLen, const uint8* pachClientHashOrKadID, bool bKad, uint16 nReceiverVerifyKey, uint16 nSenderVerifyKey) {
	wxASSERT( theApp->GetPublicIP() != 0 || bKad );
	wxASSERT( thePrefs::IsClientCryptLayerSupported() );

	uint8 byPadLen = 0;			// padding disabled for UDP currently
	const uint32 nCryptHeaderLen = byPadLen + CRYPT_HEADER_WITHOUTPADDING + (bKad ? 4 : 0);
	uint32 nCryptedLen = nBufLen + nCryptHeaderLen;
	uint8* pachCryptedBuffer = new uint8[nCryptedLen];
	
	uint16 nRandomKeyPart = GetRandomUint16();
	CRC4EncryptableBuffer sendbuffer;
	MD5Sum md5;
	if (bKad) {
		uint8 achKeyData[18];
		md4cpy(achKeyData, pachClientHashOrKadID);
		memcpy(achKeyData+16, &nRandomKeyPart, 2);
		md5.Calculate(achKeyData, sizeof(achKeyData));
	} else {
		uint8 achKeyData[23];
		md4cpy(achKeyData, pachClientHashOrKadID);
		uint32 dwIP = theApp->GetPublicIP();
		memcpy(achKeyData+16, &dwIP, 4);
		memcpy(achKeyData+21, &nRandomKeyPart, 2);
		achKeyData[20] = MAGICVALUE_UDP;
		md5.Calculate(achKeyData, sizeof(achKeyData));
	}
	
	sendbuffer.SetKey(md5, true);

	// create the semi random byte encryption header
	uint8 bySemiRandomNotProtocolMarker = 0;
	int i;
	for (i = 0; i < 128; i++){
		bySemiRandomNotProtocolMarker = GetRandomUint8();
		bySemiRandomNotProtocolMarker = bKad ? (bySemiRandomNotProtocolMarker & 0xFE) : (bySemiRandomNotProtocolMarker | 0x01); // set the ed2k/kad marker bit

		bool bOk = false;
		switch (bySemiRandomNotProtocolMarker){ // not allowed values
			case OP_EMULEPROT:
			case OP_KADEMLIAPACKEDPROT:
			case OP_KADEMLIAHEADER:
			case OP_UDPRESERVEDPROT1:
			case OP_UDPRESERVEDPROT2:
			case OP_PACKEDPROT:
				break;
			default:
				bOk = true;
		}
		
		if (bOk) {
			break;
		}
	}
	
	if (i >= 128){
		// either we have _real_ bad luck or the randomgenerator is a bit messed up
		wxASSERT( false );
		bySemiRandomNotProtocolMarker = 0x01;
	}

	uint32 dwMagicValue = MAGICVALUE_UDP_SYNC_CLIENT;
	pachCryptedBuffer[0] = bySemiRandomNotProtocolMarker;
	memcpy(pachCryptedBuffer + 1, &nRandomKeyPart, 2);
	sendbuffer.RC4Crypt((uint8*)&dwMagicValue, pachCryptedBuffer + 3, 4);
	sendbuffer.RC4Crypt((uint8*)&byPadLen, pachCryptedBuffer + 7, 1);

	for (int j = 0; j < byPadLen; j++){
		uint8 byRand = (uint8)rand();	// they actually dont really need to be random, but it doesn't hurts either
		sendbuffer.RC4Crypt((uint8*)&byRand, pachCryptedBuffer + CRYPT_HEADER_WITHOUTPADDING + j, 1);
	}

	if (bKad){
		sendbuffer.RC4Crypt((uint8*)&nReceiverVerifyKey, pachCryptedBuffer + CRYPT_HEADER_WITHOUTPADDING + byPadLen, 2);
		sendbuffer.RC4Crypt((uint8*)&nSenderVerifyKey, pachCryptedBuffer + CRYPT_HEADER_WITHOUTPADDING + byPadLen + 2, 2);
	}

	sendbuffer.RC4Crypt(*ppbyBuf, pachCryptedBuffer + nCryptHeaderLen, nBufLen);
	delete[] *ppbyBuf;
	*ppbyBuf = pachCryptedBuffer;

	//theStats.AddUpDataOverheadCrypt(nCryptedLen - nBufLen);
	return nCryptedLen;
}

int CEncryptedDatagramSocket::DecryptReceivedServer(
	uint8* pbyBufIn,
	int nBufLen, uint8 **ppbyBufOut,
	uint32 dwBaseKey,
	uint32 /*dbgIP*/)
{
	int nResult = nBufLen;
	*ppbyBufOut = pbyBufIn;
	
	if (nResult <= CRYPT_HEADER_WITHOUTPADDING || !thePrefs::IsServerCryptLayerUDPEnabled() || dwBaseKey == 0) {
		return nResult;
	}
	
	if(pbyBufIn[0] == OP_EDONKEYPROT) {
		return nResult; // no encrypted packet (see description on top)
	}

	// might be an encrypted packet, try to decrypt
	uint8 achKeyData[7];
	memcpy(achKeyData, &dwBaseKey, 4);
	achKeyData[4] = MAGICVALUE_UDP_SERVERCLIENT;
	memcpy(achKeyData + 5, pbyBufIn + 1, 2); // random key part sent from remote server
	
	CRC4EncryptableBuffer receivebuffer;
	MD5Sum md5(achKeyData, sizeof(achKeyData));
	receivebuffer.SetKey(md5,true);
	
	uint32 dwValue;
	receivebuffer.RC4Crypt(pbyBufIn + 3, (uint8*)&dwValue, sizeof(dwValue));
	if (dwValue == MAGICVALUE_UDP_SYNC_SERVER){
		// yup this is an encrypted packet
		//DEBUG_ONLY( DebugLog(_T("Received obfuscated UDP packet from ServerIP: %s"), ipstr(dbgIP)) );
		uint8 byPadLen;
		receivebuffer.RC4Crypt(pbyBufIn + 7, (uint8*)&byPadLen, 1);
		byPadLen &= 15;
		nResult -= CRYPT_HEADER_WITHOUTPADDING;
		
		if (nResult <= byPadLen){
			//DebugLogError(_T("Invalid obfuscated UDP packet from ServerIP: %s, Paddingsize (%u) larger than received bytes"), ipstr(dbgIP), byPadLen);
			return nBufLen; // pass through, let the Receivefunction do the errorhandling on this junk
		}
		
		if (byPadLen > 0) {
			receivebuffer.RC4Crypt(NULL, NULL, byPadLen);
		}
		
		nResult -= byPadLen;
		*ppbyBufOut = pbyBufIn + (nBufLen - nResult);
		receivebuffer.RC4Crypt((uint8*)*ppbyBufOut, (uint8*)*ppbyBufOut, nResult);
		
		//theStats.AddDownDataOverheadCrypt(nBufLen - nResult);
		return nResult; // done
	} else {
		//DebugLogWarning(_T("Obfuscated packet expected but magicvalue mismatch on UDP packet from ServerIP: %s"), ipstr(dbgIP));
		return nBufLen; // pass through, let the Receivefunction do the errorhandling on this junk
	}
}

int CEncryptedDatagramSocket::EncryptSendServer(uint8** ppbyBuf, int nBufLen, uint32 dwBaseKey) {
	wxASSERT( thePrefs::IsServerCryptLayerUDPEnabled() );
	wxASSERT( dwBaseKey != 0 );
	
	uint8 byPadLen = 0;			// padding disabled for UDP currently
	uint32 nCryptedLen = nBufLen + byPadLen + CRYPT_HEADER_WITHOUTPADDING;
	uint8* pachCryptedBuffer = new uint8[nCryptedLen];
	
	uint16 nRandomKeyPart = GetRandomUint16();

	uint8 achKeyData[7];
	memcpy(achKeyData, &dwBaseKey, 4);
	achKeyData[4] = MAGICVALUE_UDP_CLIENTSERVER;
	memcpy(achKeyData + 5, &nRandomKeyPart, 2);
	MD5Sum md5(achKeyData, sizeof(achKeyData));
	CRC4EncryptableBuffer sendbuffer;
	sendbuffer.SetKey(md5);

	// create the semi random byte encryption header
	uint8 bySemiRandomNotProtocolMarker = 0;
	int i;
	
	for (i = 0; i < 128; i++){
		bySemiRandomNotProtocolMarker = GetRandomUint8();
		if (bySemiRandomNotProtocolMarker != OP_EDONKEYPROT) { // not allowed values
			break;
		}
	}
	
	if (i >= 128){
		// either we have _real_ bad luck or the randomgenerator is a bit messed up
		wxASSERT( false );
		bySemiRandomNotProtocolMarker = 0x01;
	}

	uint32 dwMagicValue = MAGICVALUE_UDP_SYNC_SERVER;
	pachCryptedBuffer[0] = bySemiRandomNotProtocolMarker;
	memcpy(pachCryptedBuffer + 1, &nRandomKeyPart, 2);
	sendbuffer.RC4Crypt((uint8*)&dwMagicValue, pachCryptedBuffer + 3, 4);
	sendbuffer.RC4Crypt((uint8*)&byPadLen, pachCryptedBuffer + 7, 1);

	for (int j = 0; j < byPadLen; j++){
		uint8 byRand = (uint8)rand();	// they actually dont really need to be random, but it doesn't hurts either
		sendbuffer.RC4Crypt((uint8*)&byRand, pachCryptedBuffer + CRYPT_HEADER_WITHOUTPADDING + j, 1);
	}
	sendbuffer.RC4Crypt(*ppbyBuf, pachCryptedBuffer + CRYPT_HEADER_WITHOUTPADDING + byPadLen, nBufLen);
	delete[] *ppbyBuf;
	*ppbyBuf = pachCryptedBuffer;

	//theStats.AddUpDataOverheadCrypt(nCryptedLen - nBufLen);
	return nCryptedLen;
}
