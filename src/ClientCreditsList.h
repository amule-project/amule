//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef CLIENTCREDITSLIST_H
#define CLIENTCREDITSLIST_H

#include "Types.h"		// Needed for uint16 and uint32
#include "MD4Hash.h"	// Needed for CMD4Hash

#include <map>

class CClientCredits;

class CClientCreditsList
{
public:
	CClientCreditsList();
	~CClientCreditsList();
	
			// return signature size, 0 = Failed | use sigkey param for debug only
	uint8	CreateSignature(CClientCredits* pTarget, byte* pachOutput, uint8 nMaxSize, uint32 ChallengeIP, uint8 byChaIPKind, void* sigkey = NULL);
	bool	VerifyIdent(CClientCredits* pTarget, const byte* pachSignature, uint8 nInputSize, uint32 dwForIP, uint8 byChaIPKind);	

	CClientCredits* GetCredit(const CMD4Hash& key);
	void	Process();
	uint8	GetPubKeyLen() const 			{return m_nMyPublicKeyLen;}
	const byte*	GetPublicKey() const		{return m_abyMyPublicKey;}
	bool	CryptoAvailable() const;
	void	SaveList();
protected:
	void	LoadList();
	void	InitalizeCrypting();
	bool	CreateKeyPair();
#ifdef _DEBUG
	bool	Debug_CheckCrypting();
#endif
private:
	typedef std::map<CMD4Hash, CClientCredits*> ClientMap;
	ClientMap m_mapClients;
	uint32			m_nLastSaved;
	// A void* to avoid having to include the large CryptoPP.h file
	void*		m_pSignkey;
	byte			m_abyMyPublicKey[80];
	uint8			m_nMyPublicKeyLen;
};

#endif // CLIENTCREDITSLIST_H
