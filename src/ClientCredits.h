// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef CLIENTCREDITS_H
#define CLIENTCREDITS_H

#include <cstddef>		// Needed for NULL
#include <ctime>		// Needed for time(2)

#ifdef __CRYPTO_DEBIAN_GENTOO__
	#include <crypto++/config.h>
	#include <crypto++/rsa.h>
#else
	#ifdef __CRYPTO_MDK_SUSE_FC__
		#include <cryptopp/config.h>
		#include <cryptopp/rsa.h>
	#else
		#ifdef __CRYPTO_SOURCE__
			#include <crypto-5.1/config.h>
			#include <crypto-5.1/rsa.h>
		#else //needed for standard path
			#include <cryptopp/config.h>
			#include <cryptopp/rsa.h>
		#endif
	#endif
#endif

#include "types.h"		// Needed for uint16 and uint32
#include <map>

#include "CMD4Hash.h"
#include "Preferences.h"
//#include "loggable.h"


#define	 MAXPUBKEYSIZE		80

#define CRYPT_CIP_REMOTECLIENT	10
#define CRYPT_CIP_LOCALCLIENT	20
#define CRYPT_CIP_NONECLIENT	30

#pragma pack(1)
struct CreditStruct{
	uchar		abyKey[16];
	uint32		nUploadedLo;	// uploaded TO him
	uint32		nDownloadedLo;	// downloaded from him
	uint32		nLastSeen;
	uint32		nUploadedHi;	// upload high 32
	uint32		nDownloadedHi;	// download high 32
	uint16		nReserved3;
	uint8		nKeySize;
	uchar		abySecureIdent[MAXPUBKEYSIZE];
};
#pragma pack()

enum EIdentState{
	IS_NOTAVAILABLE,
	IS_IDNEEDED,
	IS_IDENTIFIED,
	IS_IDFAILED,
	IS_IDBADGUY,
};

class CClientCredits
{
	friend class CClientCreditsList;
public:
	CClientCredits(CreditStruct* in_credits);
	CClientCredits(const CMD4Hash& key);
	~CClientCredits();

	const uchar* GetKey()					{return m_pCredits->abyKey;}
	uchar*	GetSecureIdent()				{return m_abyPublicKey;}
	uint8	GetSecIDKeyLen()				{return m_nPublicKeyLen;}
	CreditStruct* GetDataStruct()			{return m_pCredits;}
	void	ClearWaitStartTime();
	void	AddDownloaded(uint32 bytes, uint32 dwForIP);
	void	AddUploaded(uint32 bytes, uint32 dwForIP);
	uint64	GetUploadedTotal();
	uint64	GetDownloadedTotal();
	float	GetScoreRatio(uint32 dwForIP);
	void	SetLastSeen()					{m_pCredits->nLastSeen = time(NULL);}
	bool	SetSecureIdent(uchar* pachIdent, uint8 nIdentLen); // Public key cannot change, use only if there is not public key yet
	uint32	m_dwCryptRndChallengeFor;
	uint32	m_dwCryptRndChallengeFrom;
	EIdentState	GetCurrentIdentState(uint32 dwForIP); // can be != IdentState
	uint32	GetSecureWaitStartTime(uint32 dwForIP);
	void	SetSecWaitStartTime(uint32 dwForIP);
protected:
	void	Verified(uint32 dwForIP);
	EIdentState IdentState;
private:
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

class CClientCreditsList
{
public:
	CClientCreditsList(CPreferences* in_prefs);
	~CClientCreditsList();
	
			// return signature size, 0 = Failed | use sigkey param for debug only
	uint8	CreateSignature(CClientCredits* pTarget, uchar* pachOutput, uint8 nMaxSize, uint32 ChallengeIP, uint8 byChaIPKind, CryptoPP::RSASSA_PKCS1v15_SHA_Signer* sigkey = NULL);
	bool	VerifyIdent(CClientCredits* pTarget, uchar* pachSignature, uint8 nInputSize, uint32 dwForIP, uint8 byChaIPKind);	

	CClientCredits* GetCredit(const CMD4Hash& key);
	void	Process();
	uint8	GetPubKeyLen()					{return m_nMyPublicKeyLen;}
	byte*	GetPublicKey()					{return m_abyMyPublicKey;}
	bool	CryptoAvailable();
protected:
	void	LoadList();
	void	SaveList();
	void	InitalizeCrypting();
	bool	CreateKeyPair();
#ifdef _DEBUG
	bool	Debug_CheckCrypting();
#endif
private:
	std::map<CMD4Hash, CClientCredits*> m_mapClients;
	CPreferences*	m_pAppPrefs;
	uint32			m_nLastSaved;
	CryptoPP::RSASSA_PKCS1v15_SHA_Signer*		m_pSignkey;
	byte			m_abyMyPublicKey[80];
	uint8			m_nMyPublicKeyLen;
};

#endif // CLIENTCREDITS_H
