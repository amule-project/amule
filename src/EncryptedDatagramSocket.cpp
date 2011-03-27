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

/* Basic Obfuscated Handshake Protocol UDP:
	see EncryptedStreamSocket.h

****************************** ED2K Packets

- Keycreation Client <-> Client:
	- Client A (Outgoing connection):
		Sendkey: Md5(<UserHashClientB 16><IPClientA 4><MagicValue91 1><RandomKeyPartClientA 2>)  23
	- Client B (Incoming connection):
		Receivekey: Md5(<UserHashClientB 16><IPClientA 4><MagicValue91 1><RandomKeyPartClientA 2>)  23
	- Note: The first 1024 Bytes will be _NOT_ discarded for UDP keys to save CPU time

	- Handshake
		-> The handshake is encrypted - except otherwise noted - by the Keys created above
		-> Padding is currently not used for UDP meaning that PaddingLen will be 0, using PaddingLens up to 16 Bytes is acceptable however
		Client A: <SemiRandomNotProtocolMarker 7 Bits[Unencrypted]><ED2K Marker 1Bit = 1><RandomKeyPart 2[Unencrypted]><MagicValue 4><PaddingLen 1><RandomBytes PaddingLen%16>	

	- Additional Comments:
		- For obvious reasons the UDP handshake is actually no handshake. If a different Encryption method (or better a different Key) is to be used this has to be negotiated in a TCP connection
		- SemiRandomNotProtocolMarker is a Byte which has a value unequal any Protocol header byte. This is a compromise, turning in complete randomness (and nice design) but gaining a lower CPU usage
		- Kad/Ed2k Marker are only indicators, which possibility could be tried first, and should not be trusted

****************************** Server Packets

- Keycreation Client <-> Server:
	- Client A (Outgoing connection client -> server):
		Sendkey: Md5(<BaseKey 4><MagicValueClientServer 1><RandomKeyPartClientA 2>)  7
	- Client B (Incoming connection):
		Receivekey: Md5(<BaseKey 4><MagicValueServerClient 1><RandomKeyPartClientA 2>)  7
	- Note: The first 1024 Bytes will be _NOT_ discarded for UDP keys to save CPU time

	- Handshake
		-> The handshake is encrypted - except otherwise noted - by the Keys created above
		-> Padding is currently not used for UDP meaning that PaddingLen will be 0, using PaddingLens up to 16 Bytes is acceptable however
		Client A: <SemiRandomNotProtocolMarker 1[Unencrypted]><RandomKeyPart 2[Unencrypted]><MagicValue 4><PaddingLen 1><RandomBytes PaddingLen%16>	

	- Overhead: 8 Bytes per UDP Packet

	- Security for Basic Obfuscation:
		- Random looking packets, very limited protection against passive eavesdropping single packets

	- Additional Comments:
		- For obvious reasons the UDP handshake is actually no handshake. If a different Encryption method (or better a different Key) is to be used this has to be negotiated in a TCP connection
		- SemiRandomNotProtocolMarker is a Byte which has a value unequal any Protocol header byte. This is a compromise, turning in complete randomness (and nice design) but gaining a lower CPU usage

****************************** KAD Packets
			  
- Keycreation Client <-> Client:
	- Client A (Outgoing connection):
		Sendkey: Md5(<KadID 16><RandomKeyPartClientA 2>)  18
	- Client B (Incoming connection):
		Receivekey: Md5(<KadID 16><RandomKeyPartClientA 2>)  18
	- Note: The first 1024 Bytes will be _NOT_ discarded for UDP keys to save CPU time

	- Handshake
		-> The handshake is encrypted - except otherwise noted - by the Keys created above
		-> Padding is currently not used for UDP meaning that PaddingLen will be 0, using PaddingLens up to 16 Bytes is acceptable however
		Client A: <SemiRandomNotProtocolMarker 7 Bits[Unencrypted]><Kad Marker 1Bit = 0><RandomKeyPart 2[Unencrypted]><MagicValue 4><PaddingLen 1><RandomBytes PaddingLen%16><ReceiverVerifyKey 2><SenderVerifyKey 2>

	- Overhead: 12 Bytes per UDP Packet

	- Additional Comments:
		- For obvious reasons the UDP handshake is actually no handshake. If a different Encryption method (or better a different Key) is to be used this has to be negotiated in a TCP connection
		- SemiRandomNotProtocolMarker is a Byte which has a value unequal any Protocol header byte. This is a compromise, turning in complete randomness (and nice design) but gaining a lower CPU usage
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
#include "Statistics.h"

#include <protocol/Protocols.h>
#include <common/MD5Sum.h>

// random generator
#include "CryptoPP_Inc.h"	// Needed for Crypto functions

#define CRYPT_HEADER_WITHOUTPADDING		    8
#define	MAGICVALUE_UDP						91
#define MAGICVALUE_UDP_SYNC_CLIENT			0x395F2EC1
#define MAGICVALUE_UDP_SYNC_SERVER			0x13EF24D5
#define	MAGICVALUE_UDP_SERVERCLIENT			0xA5
#define	MAGICVALUE_UDP_CLIENTSERVER			0x6B

CEncryptedDatagramSocket::CEncryptedDatagramSocket(wxIPaddress &address, wxSocketFlags flags,	const CProxyData *proxyData)
	: CDatagramSocketProxy(address, flags, proxyData)
{}

CEncryptedDatagramSocket::~CEncryptedDatagramSocket()
{}

int CEncryptedDatagramSocket::DecryptReceivedClient(uint8_t *bufIn, int bufLen, uint8_t **bufOut, uint32_t ip, uint32_t *receiverVerifyKey, uint32_t *senderVerifyKey)
{
	int result = bufLen;
	*bufOut = bufIn;

	if (receiverVerifyKey == NULL || senderVerifyKey == NULL) {
		wxFAIL;
		return result;
	}

	*receiverVerifyKey = 0;
	*senderVerifyKey = 0;

	if (result <= CRYPT_HEADER_WITHOUTPADDING /*|| !thePrefs.IsClientCryptLayerSupported()*/) {
		return result;
	}

	switch (bufIn[0]) {
		case OP_EMULEPROT:
		case OP_KADEMLIAPACKEDPROT:
		case OP_KADEMLIAHEADER:
		case OP_UDPRESERVEDPROT1:
		case OP_UDPRESERVEDPROT2:
		case OP_PACKEDPROT:
			return result; // no encrypted packet (see description on top)
		default:
			;
	}

	// might be an encrypted packet, try to decrypt
	CRC4EncryptableBuffer receivebuffer;
	uint32_t value = 0;
	// check the marker bit which type this packet could be and which key to test first, this is only an indicator since old clients have it set random
	// see the header for marker bits explanation
	uint8_t currentTry = ((bufIn[0] & 0x03) == 3) ? 1 : (bufIn[0] & 0x03);
	uint8_t tries;
	if (Kademlia::CKademlia::GetPrefs() == NULL) {
		// if kad never run, no point in checking anything except for ed2k encryption
		tries = 1;
		currentTry = 1;
	} else {
		tries = 3;
	}
	bool kadRecvKeyUsed = false;
	bool kad = false;
	do {
		receivebuffer.FullReset();
		tries--;
		MD5Sum md5;

		if (currentTry == 0) {
			// kad packet with NodeID as key
			kad = true;
			kadRecvKeyUsed = false;
			if (Kademlia::CKademlia::GetPrefs()) {
				uint8_t keyData[18];
				Kademlia::CKademlia::GetPrefs()->GetKadID().StoreCryptValue((uint8_t *)&keyData);
				memcpy(keyData + 16, bufIn + 1, 2); // random key part sent from remote client
				md5.Calculate(keyData, sizeof(keyData));
			}
		} else if (currentTry == 1) {
			// ed2k packet
			kad = false;
			kadRecvKeyUsed = false;
			uint8_t keyData[23];
			md4cpy(keyData, thePrefs::GetUserHash().GetHash());
			keyData[20] = MAGICVALUE_UDP;
			PokeUInt32(keyData + 16, ip);
			memcpy(keyData + 21, bufIn + 1, 2); // random key part sent from remote client
			md5.Calculate(keyData, sizeof(keyData));
		} else if (currentTry == 2) {
			// kad packet with ReceiverKey as key
			kad = true;
			kadRecvKeyUsed = true;
			if (Kademlia::CKademlia::GetPrefs()) {
				uint8_t keyData[6];
				PokeUInt32(keyData, Kademlia::CPrefs::GetUDPVerifyKey(ip));
				memcpy(keyData + 4, bufIn + 1, 2); // random key part sent from remote client
				md5.Calculate(keyData, sizeof(keyData));
			}
		} else {
			wxFAIL;
		}

		receivebuffer.SetKey(md5, true);
		receivebuffer.RC4Crypt(bufIn + 3, (uint8_t*)&value, sizeof(value));
		ENDIAN_SWAP_I_32(value);

		currentTry = (currentTry + 1) % 3;
	} while (value != MAGICVALUE_UDP_SYNC_CLIENT && tries > 0); // try to decrypt as ed2k as well as kad packet if needed (max 3 rounds)

	if (value == MAGICVALUE_UDP_SYNC_CLIENT) {
		// yup this is an encrypted packet
// 		// debugoutput notices
// 		// the following cases are "allowed" but shouldn't happen given that there is only our implementation yet
// 		if (bKad && (pbyBufIn[0] & 0x01) != 0)
// 			DebugLog(_T("Received obfuscated UDP packet from clientIP: %s with wrong key marker bits (kad packet, ed2k bit)"), ipstr(dwIP));
// 		else if (bKad && !bKadRecvKeyUsed && (pbyBufIn[0] & 0x02) != 0)
// 			DebugLog(_T("Received obfuscated UDP packet from clientIP: %s with wrong key marker bits (kad packet, nodeid key, recvkey bit)"), ipstr(dwIP));
// 		else if (bKad && bKadRecvKeyUsed && (pbyBufIn[0] & 0x02) == 0)
// 			DebugLog(_T("Received obfuscated UDP packet from clientIP: %s with wrong key marker bits (kad packet, recvkey key, nodeid bit)"), ipstr(dwIP));

		uint8_t padLen;
		receivebuffer.RC4Crypt(bufIn + 7, (uint8_t*)&padLen, 1);
		result -= CRYPT_HEADER_WITHOUTPADDING;

		if (result <= padLen) {
			//DebugLogError(_T("Invalid obfuscated UDP packet from clientIP: %s, Paddingsize (%u) larger than received bytes"), ipstr(dwIP), byPadLen);
			return bufLen; // pass through, let the Receivefunction do the errorhandling on this junk
		}

		if (padLen > 0) {
			receivebuffer.RC4Crypt(NULL, NULL, padLen);
		}

		result -= padLen;

		if (kad) {
			if (result <= 8) {
				//DebugLogError(_T("Obfuscated Kad packet with mismatching size (verify keys missing) received from clientIP: %s"), ipstr(dwIP));
				return bufLen; // pass through, let the Receivefunction do the errorhandling on this junk;
			}
			// read the verify keys
			receivebuffer.RC4Crypt(bufIn + CRYPT_HEADER_WITHOUTPADDING + padLen, (uint8_t*)receiverVerifyKey, 4);
			receivebuffer.RC4Crypt(bufIn + CRYPT_HEADER_WITHOUTPADDING + padLen + 4, (uint8_t*)senderVerifyKey, 4);
			ENDIAN_SWAP_I_32(*receiverVerifyKey);
			ENDIAN_SWAP_I_32(*senderVerifyKey);
			result -= 8;
		}

		*bufOut = bufIn + (bufLen - result);

		receivebuffer.RC4Crypt((uint8_t*)*bufOut, (uint8_t*)*bufOut, result);
		theStats::AddDownOverheadCrypt(bufLen - result);
		return result; // done
	} else {
		//DebugLogWarning(_T("Obfuscated packet expected but magicvalue mismatch on UDP packet from clientIP: %s"), ipstr(dwIP));
		return bufLen; // pass through, let the Receivefunction do the errorhandling on this junk
	}
}

// Encrypt packet. Key used:
// clientHashOrKadID != NULL					-> clientHashOrKadID
// clientHashOrKadID == NULL && kad && receiverVerifyKey != 0	-> receiverVerifyKey
// else								-> ASSERT
int CEncryptedDatagramSocket::EncryptSendClient(uint8_t **buf, int bufLen, const uint8_t *clientHashOrKadID, bool kad, uint32_t receiverVerifyKey, uint32_t senderVerifyKey)
{
	wxASSERT(theApp->GetPublicIP() != 0 || kad);
	wxASSERT(thePrefs::IsClientCryptLayerSupported());
	wxASSERT(clientHashOrKadID != NULL || receiverVerifyKey != 0);
	wxASSERT((receiverVerifyKey == 0 && senderVerifyKey == 0) || kad);

	uint8_t padLen = 0;			// padding disabled for UDP currently
	const uint32_t cryptHeaderLen = padLen + CRYPT_HEADER_WITHOUTPADDING + (kad ? 8 : 0);
	uint32_t cryptedLen = bufLen + cryptHeaderLen;
	uint8_t *cryptedBuffer = new uint8_t[cryptedLen];
	bool kadRecvKeyUsed = false;

	uint16_t randomKeyPart = GetRandomUint16();
	CRC4EncryptableBuffer sendbuffer;
	MD5Sum md5;
	if (kad) {
		if ((clientHashOrKadID == NULL || CMD4Hash(clientHashOrKadID).IsEmpty()) && receiverVerifyKey != 0) {
			kadRecvKeyUsed = true;
			uint8_t keyData[6];
			PokeUInt32(keyData, receiverVerifyKey);
			PokeUInt16(keyData+4, randomKeyPart);
			md5.Calculate(keyData, sizeof(keyData));
			//DEBUG_ONLY( DebugLog(_T("Creating obfuscated Kad packet encrypted by ReceiverKey (%u)"), nReceiverVerifyKey) );  
		}
		else if (clientHashOrKadID != NULL && !CMD4Hash(clientHashOrKadID).IsEmpty()) {
			uint8_t keyData[18];
			md4cpy(keyData, clientHashOrKadID);
			PokeUInt16(keyData+16, randomKeyPart);
			md5.Calculate(keyData, sizeof(keyData));
			//DEBUG_ONLY( DebugLog(_T("Creating obfuscated Kad packet encrypted by Hash/NodeID %s"), md4str(pachClientHashOrKadID)) );  
		}
		else {
			wxFAIL;
			return bufLen;
		}
	} else {
		uint8_t keyData[23];
		md4cpy(keyData, clientHashOrKadID);
		PokeUInt32(keyData+16, theApp->GetPublicIP());
		PokeUInt16(keyData+21, randomKeyPart);
		keyData[20] = MAGICVALUE_UDP;
		md5.Calculate(keyData, sizeof(keyData));
	}

	sendbuffer.SetKey(md5, true);

	// create the semi random byte encryption header
	uint8_t semiRandomNotProtocolMarker = 0;
	int i;
	for (i = 0; i < 128; i++) {
		semiRandomNotProtocolMarker = GetRandomUint8();
		semiRandomNotProtocolMarker = kad ? (semiRandomNotProtocolMarker & 0xFE) : (semiRandomNotProtocolMarker | 0x01); // set the ed2k/kad marker bit
		if (kad) {
			// set the ed2k/kad and nodeid/recvkey markerbit
			semiRandomNotProtocolMarker = kadRecvKeyUsed ? ((semiRandomNotProtocolMarker & 0xFE) | 0x02) : (semiRandomNotProtocolMarker & 0xFC);
		} else {
			// set the ed2k/kad marker bit
			semiRandomNotProtocolMarker = (semiRandomNotProtocolMarker | 0x01);
		}

		bool bOk = false;
		switch (semiRandomNotProtocolMarker) { // not allowed values
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

	if (i >= 128) {
		// either we have _real_ bad luck or the randomgenerator is a bit messed up
		wxFAIL;
		semiRandomNotProtocolMarker = 0x01;
	}

	cryptedBuffer[0] = semiRandomNotProtocolMarker;
	PokeUInt16(cryptedBuffer + 1, randomKeyPart);

	uint32_t magicValue = ENDIAN_SWAP_32(MAGICVALUE_UDP_SYNC_CLIENT);
	sendbuffer.RC4Crypt((uint8_t*)&magicValue, cryptedBuffer + 3, 4);
	sendbuffer.RC4Crypt((uint8_t*)&padLen, cryptedBuffer + 7, 1);

	for (int j = 0; j < padLen; j++) {
		uint8_t byRand = (uint8_t)rand();	// they actually don't really need to be random, but it doesn't hurt either
		sendbuffer.RC4Crypt((uint8_t*)&byRand, cryptedBuffer + CRYPT_HEADER_WITHOUTPADDING + j, 1);
	}

	if (kad) {
		ENDIAN_SWAP_I_32(receiverVerifyKey);
		ENDIAN_SWAP_I_32(senderVerifyKey);
		sendbuffer.RC4Crypt((uint8_t*)&receiverVerifyKey, cryptedBuffer + CRYPT_HEADER_WITHOUTPADDING + padLen, 4);
		sendbuffer.RC4Crypt((uint8_t*)&senderVerifyKey, cryptedBuffer + CRYPT_HEADER_WITHOUTPADDING + padLen + 4, 4);
	}

	sendbuffer.RC4Crypt(*buf, cryptedBuffer + cryptHeaderLen, bufLen);
	delete [] *buf;
	*buf = cryptedBuffer;

	theStats::AddUpOverheadCrypt(cryptedLen - bufLen);
	return cryptedLen;
}

int CEncryptedDatagramSocket::DecryptReceivedServer(uint8_t* pbyBufIn, int nBufLen, uint8_t **ppbyBufOut, uint32_t dwBaseKey, uint32_t /*dbgIP*/)
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
	uint8_t achKeyData[7];
	PokeUInt32(achKeyData, dwBaseKey);
	achKeyData[4] = MAGICVALUE_UDP_SERVERCLIENT;
	memcpy(achKeyData + 5, pbyBufIn + 1, 2); // random key part sent from remote server

	CRC4EncryptableBuffer receivebuffer;
	MD5Sum md5(achKeyData, sizeof(achKeyData));
	receivebuffer.SetKey(md5,true);

	uint32_t dwValue;
	receivebuffer.RC4Crypt(pbyBufIn + 3, (uint8_t*)&dwValue, sizeof(dwValue));
	ENDIAN_SWAP_I_32(dwValue);
	if (dwValue == MAGICVALUE_UDP_SYNC_SERVER) {
		// yup this is an encrypted packet
		//DEBUG_ONLY( DebugLog(_T("Received obfuscated UDP packet from ServerIP: %s"), ipstr(dbgIP)) );
		uint8_t byPadLen;
		receivebuffer.RC4Crypt(pbyBufIn + 7, (uint8_t*)&byPadLen, 1);
		byPadLen &= 15;
		nResult -= CRYPT_HEADER_WITHOUTPADDING;

		if (nResult <= byPadLen) {
			//DebugLogError(_T("Invalid obfuscated UDP packet from ServerIP: %s, Paddingsize (%u) larger than received bytes"), ipstr(dbgIP), byPadLen);
			return nBufLen; // pass through, let the Receivefunction do the errorhandling on this junk
		}

		if (byPadLen > 0) {
			receivebuffer.RC4Crypt(NULL, NULL, byPadLen);
		}

		nResult -= byPadLen;
		*ppbyBufOut = pbyBufIn + (nBufLen - nResult);
		receivebuffer.RC4Crypt((uint8_t*)*ppbyBufOut, (uint8_t*)*ppbyBufOut, nResult);

		theStats::AddDownOverheadCrypt(nBufLen - nResult);
		return nResult; // done
	} else {
		//DebugLogWarning(_T("Obfuscated packet expected but magicvalue mismatch on UDP packet from ServerIP: %s"), ipstr(dbgIP));
		return nBufLen; // pass through, let the Receivefunction do the errorhandling on this junk
	}
}

int CEncryptedDatagramSocket::EncryptSendServer(uint8_t** ppbyBuf, int nBufLen, uint32_t dwBaseKey)
{
	wxASSERT( thePrefs::IsServerCryptLayerUDPEnabled() );
	wxASSERT( dwBaseKey != 0 );

	uint16_t nRandomKeyPart = GetRandomUint16();

	uint8_t achKeyData[7];
	PokeUInt32(achKeyData, dwBaseKey);
	achKeyData[4] = MAGICVALUE_UDP_CLIENTSERVER;
	PokeUInt16(achKeyData + 5, nRandomKeyPart);
	MD5Sum md5(achKeyData, sizeof(achKeyData));
	CRC4EncryptableBuffer sendbuffer;
	sendbuffer.SetKey(md5, true);

	// create the semi random byte encryption header
	uint8_t bySemiRandomNotProtocolMarker = 0;
	int i;

	for (i = 0; i < 128; i++) {
		bySemiRandomNotProtocolMarker = GetRandomUint8();
		if (bySemiRandomNotProtocolMarker != OP_EDONKEYPROT) { // not allowed values
			break;
		}
	}

	if (i >= 128) {
		// either we have _real_ bad luck or the randomgenerator is a bit messed up
		wxFAIL;
		bySemiRandomNotProtocolMarker = 0x01;
	}

	uint8_t byPadLen = 0;			// padding disabled for UDP currently
	uint32_t nCryptedLen = nBufLen + byPadLen + CRYPT_HEADER_WITHOUTPADDING;
	uint8_t* pachCryptedBuffer = new uint8_t[nCryptedLen];

	pachCryptedBuffer[0] = bySemiRandomNotProtocolMarker;
	PokeUInt16(pachCryptedBuffer + 1, nRandomKeyPart);

	uint32_t dwMagicValue = ENDIAN_SWAP_32(MAGICVALUE_UDP_SYNC_SERVER);
	sendbuffer.RC4Crypt((uint8_t*)&dwMagicValue, pachCryptedBuffer + 3, 4);

	sendbuffer.RC4Crypt((uint8_t*)&byPadLen, pachCryptedBuffer + 7, 1);

	for (int j = 0; j < byPadLen; j++){
		uint8_t byRand = (uint8_t)rand();	// they actually don't really need to be random, but it doesn't hurt either
		sendbuffer.RC4Crypt((uint8_t*)&byRand, pachCryptedBuffer + CRYPT_HEADER_WITHOUTPADDING + j, 1);
	}
	sendbuffer.RC4Crypt(*ppbyBuf, pachCryptedBuffer + CRYPT_HEADER_WITHOUTPADDING + byPadLen, nBufLen);
	delete[] *ppbyBuf;
	*ppbyBuf = pachCryptedBuffer;

	theStats::AddUpOverheadCrypt(nCryptedLen - nBufLen);
	return nCryptedLen;
}
