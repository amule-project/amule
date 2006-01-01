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

#ifndef CLIENTCREDITS_H
#define CLIENTCREDITS_H

#include "Types.h"		// Needed for uint16 and uint32
#include "MD4Hash.h"	// Needed for CMD4Hash

#define	 MAXPUBKEYSIZE		80

#define CRYPT_CIP_REMOTECLIENT	10
#define CRYPT_CIP_LOCALCLIENT	20
#define CRYPT_CIP_NONECLIENT	30

struct CreditStruct
{
	CreditStruct();
	
	CMD4Hash	key;
	uint32		nUploadedLo;	// uploaded TO him
	uint32		nDownloadedLo;	// downloaded from him
	uint32		nLastSeen;
	uint32		nUploadedHi;	// upload high 32
	uint32		nDownloadedHi;	// download high 32
	uint16		nReserved3;
	uint8		nKeySize;
	byte		abySecureIdent[MAXPUBKEYSIZE];
};

enum EIdentState{
	IS_NOTAVAILABLE,
	IS_IDNEEDED,
	IS_IDENTIFIED,
	IS_IDFAILED,
	IS_IDBADGUY
};

class CClientCredits
{

public:
	CClientCredits(CreditStruct* in_credits);
	CClientCredits(const CMD4Hash& key);
	~CClientCredits();

	const CMD4Hash& GetKey() const 			{return m_pCredits->key;}
	const byte*	GetSecureIdent() const	{return m_abyPublicKey;}
	uint8	GetSecIDKeyLen() const 			{return m_nPublicKeyLen;}
	const CreditStruct* GetDataStruct() const	{return m_pCredits;}
	void	ClearWaitStartTime();
	void	AddDownloaded(uint32 bytes, uint32 dwForIP, bool cryptoavail);
	void	AddUploaded(uint32 bytes, uint32 dwForIP, bool cryptoavail);
	uint64	GetUploadedTotal() const;
	uint64	GetDownloadedTotal() const;
	float	GetScoreRatio(uint32 dwForIP, bool cryptoavail);
	void	SetLastSeen();
	bool	SetSecureIdent(const byte* pachIdent, uint8 nIdentLen); // Public key cannot change, use only if there is not public key yet
	uint32	m_dwCryptRndChallengeFor;
	uint32	m_dwCryptRndChallengeFrom;
	EIdentState	GetCurrentIdentState(uint32 dwForIP) const; // can be != m_identState
	uint32	GetSecureWaitStartTime(uint32 dwForIP);
	void	SetSecWaitStartTime(uint32 dwForIP);
	void	Verified(uint32 dwForIP);
	EIdentState GetIdentState() const { return m_identState; }
	void	SetIdentState(EIdentState state) { m_identState = state; }
	
private:
	EIdentState		m_identState;
	void			InitalizeIdent();
	CreditStruct*	m_pCredits;
	byte			m_abyPublicKey[80];			// even keys which are not verified will be stored here, and - if verified - copied into the struct
	uint8			m_nPublicKeyLen;
	uint32			m_dwIdentIP;
	uint32			m_dwWaitTime;
	uint32			m_dwSecureWaitTime;
	uint32			m_dwUnSecureWaitTime;
	uint32			m_dwWaitTimeIP;			   // client IP assigned to the waittime
};

#endif // CLIENTCREDITS_H
