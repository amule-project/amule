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

#include "ClientCreditsList.h"	// Interface declarations

#include <cmath>
#include <ctime>
#include <wx/utils.h>
#include <wx/intl.h>		// Needed for _
#include <wx/textfile.h>

#include "GetTickCount.h"	// Needed for GetTickCount
#include "Preferences.h"	// Needed for thePrefs
#include "ClientCredits.h"	// Needed for CClientCredits
#include "OPCodes.h"		// Needed for CREDITFILE_VERSION
#include "amule.h"			// Needed for theApp
#include "CFile.h"			// Needed for CFile
#include "Logger.h"			// Needed for Add(Debug)LogLine
#include "FileFunctions.h"	// Needed for GetFileSize
#include "CryptoPP_Inc.h"	// Needed for Crypto functions

#define CLIENTS_MET_FILENAME		wxT("clients.met")
#define CLIENTS_MET_BAK_FILENAME	wxT("clients.met.BAK")
#define CRYPTKEY_FILENAME			wxT("cryptkey.dat")

CClientCreditsList::CClientCreditsList()
{
	m_nLastSaved = ::GetTickCount();
	LoadList();
	
	InitalizeCrypting();
}

CClientCreditsList::~CClientCreditsList()
{
	
	ClientMap::iterator it = m_mapClients.begin();
	for ( ; it != m_mapClients.end(); ++it ){
		delete it->second;
	}
	m_mapClients.clear();
	if (m_pSignkey){
		delete (CryptoPP::RSASSA_PKCS1v15_SHA_Signer*)m_pSignkey;
		m_pSignkey = NULL;
	}
}

void CClientCreditsList::LoadList()
{
	CFile file;
	wxString strFileName(theApp.ConfigDir + CLIENTS_MET_FILENAME);
	if (!::wxFileExists(strFileName)) {
		AddDebugLogLineM( true, logCredits, wxT("Failed to load creditfile"));
		return;
	}	
	
	try {
		file.Open(strFileName, CFile::read);
	
		if (file.ReadUInt8() != CREDITFILE_VERSION) {
			AddDebugLogLineM( true, logCredits, wxT("Creditfile is out of date and will be replaced") );
			file.Close();
			return;
		}

		// everything is ok, lets see if the backup exist...
		wxString strBakFileName(theApp.ConfigDir + CLIENTS_MET_BAK_FILENAME);
	
		bool bCreateBackup = TRUE;
		if (wxFileExists(strBakFileName)) {
			// Ok, the backup exist, get the size
			CFile hBakFile(strBakFileName);
			if ( hBakFile.GetLength() > file.GetLength()) {
				// the size of the backup was larger then the org. file, something is wrong here, don't overwrite old backup..
				bCreateBackup = FALSE;
			}
			// else: backup is smaller or the same size as org. file, proceed with copying of file
		}
	
		//else: the backup doesn't exist, create it
		if (bCreateBackup) {
			file.Close(); // close the file before copying
			// safe? you bet it is
			if (!wxCopyFile(strFileName,strBakFileName)) {
				AddDebugLogLineM( true, logCredits, wxT("Could not create backup file ") + strFileName );
			}
			// reopen file
			if (!file.Open(strFileName, CFile::read)) {
				AddDebugLogLineM( true, logCredits, wxT("Failed to load creditfile") );
				return;
			}

			file.Seek(1);
		}	
	
	
		uint32 count = file.ReadUInt32();

		const uint32 dwExpired = time(NULL) - 12960000; // today - 150 day
		uint32 cDeleted = 0;
		for (uint32 i = 0; i < count; i++){
			CreditStruct* newcstruct = new CreditStruct();

			newcstruct->key					= file.ReadHash();
			newcstruct->nUploadedLo         = file.ReadUInt32();
			newcstruct->nDownloadedLo       = file.ReadUInt32();
			newcstruct->nLastSeen           = file.ReadUInt32();
			newcstruct->nUploadedHi         = file.ReadUInt32();
			newcstruct->nDownloadedHi       = file.ReadUInt32();
			newcstruct->nReserved3          = file.ReadUInt16();
			newcstruct->nKeySize            = file.ReadUInt8();
			file.Read(newcstruct->abySecureIdent, MAXPUBKEYSIZE);
		
			if ( newcstruct->nKeySize > MAXPUBKEYSIZE ) {
				// Oh dear, this is bad mojo, the file is most likely corrupt
				// We can no longer assume that any of the clients in the file are valid
				// and will have to discard it.
				delete newcstruct;
				
				// Remove already read, and possibly invalid, entries
				ClientMap::iterator it = m_mapClients.begin();
				for ( ; it != m_mapClients.end(); ++it ){
					delete it->second;
				}
				m_mapClients.clear();
				
				AddDebugLogLineM( true, logCredits, wxT("WARNING: Corruptions found while reading Creditfile!") );
				return;	
			}
		
			if (newcstruct->nLastSeen < dwExpired){
				cDeleted++;
				delete newcstruct;
				continue;
			}

			CClientCredits* newcredits = new CClientCredits(newcstruct);
			m_mapClients[newcredits->GetKey()] = newcredits;
		}

		AddLogLineM(false, wxString::Format(_("Creditfile loaded, %u clients are known"),count-cDeleted) );
	
		if (cDeleted) {
			AddLogLineM(false, wxString::Format(_(" - Credits expired for %u clients!"),cDeleted));
		}
	} catch (const CSafeIOException& e) {
		AddDebugLogLineM(true, logCredits, wxT("IO erro while loading clients.met file: ") + e.what());
	}
}


void CClientCreditsList::SaveList()
{
	AddDebugLogLineM( false, logCredits, wxT("Saved Credit list"));
	m_nLastSaved = ::GetTickCount();

	wxString name(theApp.ConfigDir + CLIENTS_MET_FILENAME);
	CFile file;

	if ( !file.Create(name, true) ) {
		AddDebugLogLineM( true, logCredits, wxT("Failed to create creditfile") );
		return;
	}
	
	if ( file.Open(name, CFile::write) ) {
		try {
			uint32 count = 0;

			file.WriteUInt8( CREDITFILE_VERSION );
			// Temporary place-holder for number of stucts
			file.WriteUInt32( 0 );

			ClientMap::iterator it = m_mapClients.begin();
			for ( ; it != m_mapClients.end(); ++it ) {	
				CClientCredits* cur_credit = it->second;
		
				if ( cur_credit->GetUploadedTotal() || cur_credit->GetDownloadedTotal() ) {
					const CreditStruct* const cstruct = cur_credit->GetDataStruct();
					file.WriteHash(cstruct->key);
					file.WriteUInt32(cstruct->nUploadedLo);
					file.WriteUInt32(cstruct->nDownloadedLo);
					file.WriteUInt32(cstruct->nLastSeen);
					file.WriteUInt32(cstruct->nUploadedHi);
					file.WriteUInt32(cstruct->nDownloadedHi);
					file.WriteUInt16(cstruct->nReserved3);
					file.WriteUInt8(cstruct->nKeySize);
					// Doesn't matter if this saves garbage, will be fixed on load.
					file.Write(cstruct->abySecureIdent, MAXPUBKEYSIZE);
					count++;
				}
			}
		
			// Write the actual number of structs
			file.Seek( 1 );
			file.WriteUInt32( count );
		} catch (const CIOFailureException& e) {
			AddDebugLogLineM(true, logCredits, wxT("IO failure while saving clients.met: ") + e.what());
		}
	} else {
		AddDebugLogLineM(true, logCredits, wxT("Failed to open existing creditfile!"));
	}
}


CClientCredits* CClientCreditsList::GetCredit(const CMD4Hash& key)
{
	CClientCredits* result;

	ClientMap::iterator it = m_mapClients.find( key );

	
	if ( it == m_mapClients.end() ){
		result = new CClientCredits(key);
		m_mapClients[result->GetKey()] = result;
	} else {
		result = it->second;
	}
	
	result->SetLastSeen();
	
	return result;
}

void CClientCreditsList::Process()
{
	if (::GetTickCount() - m_nLastSaved > MIN2MS(13))
		SaveList();
}

bool CClientCreditsList::CreateKeyPair()
{
	try{
		CryptoPP::AutoSeededRandomPool rng;
		CryptoPP::InvertibleRSAFunction privkey;
		privkey.Initialize(rng,RSAKEYSIZE);

		// Nothing we can do against this unicode2char :/
		CryptoPP::Base64Encoder privkeysink(new CryptoPP::FileSink(unicode2char(theApp.ConfigDir + CRYPTKEY_FILENAME)));
		
		privkey.DEREncode(privkeysink);
		
		privkeysink.MessageEnd();

		AddDebugLogLineM( true, logCredits, wxT("Created new RSA keypair"));
	} catch(const CryptoPP::Exception& e) {
		AddDebugLogLineM(true, logCredits, wxString(wxT("Failed to create new RSA keypair: ")) + char2unicode(e.what()));
		wxASSERT(false);
 		return false;
 	}
	
 	return true;
}


void CClientCreditsList::InitalizeCrypting()
{
	m_nMyPublicKeyLen = 0;
	memset(m_abyMyPublicKey,0,80); // not really needed; better for debugging tho
	m_pSignkey = NULL;

	if (!thePrefs::IsSecureIdentEnabled()) {
		return;
	}

 
	try {
		// check if keyfile is there
 		if (wxFileExists(theApp.ConfigDir + CRYPTKEY_FILENAME)) {
			off_t keySize = GetFileSize(theApp.ConfigDir + CRYPTKEY_FILENAME);
			
			if (keySize < 0) {
				AddDebugLogLineM(true, logCredits, wxT("Cannot access 'cryptkey.dat', please check permissions."));
				return;
			} else if (keySize == 0) {
				AddDebugLogLineM(true, logCredits, wxT("'cryptkey.dat' is empty, recreating keypair."));
				CreateKeyPair();
 			}
 		} else {
			AddLogLineM( false, _("No 'cryptkey.dat' file found, creating.") );
 			CreateKeyPair();
 		}
			
 		// load private key
 		CryptoPP::FileSource filesource(unicode2char(theApp.ConfigDir + CRYPTKEY_FILENAME), true,new CryptoPP::Base64Decoder);
 		m_pSignkey = new CryptoPP::RSASSA_PKCS1v15_SHA_Signer(filesource);
 		// calculate and store public key
		CryptoPP::RSASSA_PKCS1v15_SHA_Verifier pubkey(*((CryptoPP::RSASSA_PKCS1v15_SHA_Signer*)m_pSignkey));
		CryptoPP::ArraySink asink(m_abyMyPublicKey, 80);
 		pubkey.DEREncode(asink);
 		m_nMyPublicKeyLen = asink.TotalPutLength();
 		asink.MessageEnd();
	} catch (const CryptoPP::Exception& e) {
		delete (CryptoPP::RSASSA_PKCS1v15_SHA_Signer*)m_pSignkey;
		m_pSignkey = NULL;
		
		AddDebugLogLineM(true, logCredits, wxString(wxT("Error while initializing encryption keys: ")) + char2unicode(e.what()));
 	}
}


uint8 CClientCreditsList::CreateSignature(CClientCredits* pTarget, byte* pachOutput, uint8 nMaxSize, uint32 ChallengeIP, uint8 byChaIPKind, void* sigkey)
{	
	CryptoPP::RSASSA_PKCS1v15_SHA_Signer* signer = (CryptoPP::RSASSA_PKCS1v15_SHA_Signer*)sigkey;
	// signer param is used for debug only
	if (signer == NULL)
		signer = (CryptoPP::RSASSA_PKCS1v15_SHA_Signer*)m_pSignkey;

	// create a signature of the public key from pTarget
	wxASSERT( pTarget );
	wxASSERT( pachOutput );
	
	if ( !CryptoAvailable() ) {
 		return 0;
	}
	
	try {		
		CryptoPP::SecByteBlock sbbSignature(signer->SignatureLength());
		CryptoPP::AutoSeededRandomPool rng;
		byte abyBuffer[MAXPUBKEYSIZE+9];
		uint32 keylen = pTarget->GetSecIDKeyLen();
		memcpy(abyBuffer,pTarget->GetSecureIdent(),keylen);
		// 4 additional bytes random data send from this client
		uint32 challenge = pTarget->m_dwCryptRndChallengeFrom;
		wxASSERT ( challenge != 0 );		
		PokeUInt32(abyBuffer+keylen,challenge);
		
		uint16 ChIpLen = 0;
		if ( byChaIPKind != 0){
			ChIpLen = 5;
			PokeUInt32(abyBuffer+keylen+4, ChallengeIP);
			PokeUInt8(abyBuffer+keylen+4+4,byChaIPKind);
		}
 		signer->SignMessage(rng, abyBuffer ,keylen+4+ChIpLen , sbbSignature.begin());
 		CryptoPP::ArraySink asink(pachOutput, nMaxSize);
 		asink.Put(sbbSignature.begin(), sbbSignature.size());
		
		return asink.TotalPutLength();			
	} catch (const CryptoPP::Exception& e) {
		AddDebugLogLineM(true, logCredits, wxString(wxT("Error while creating signature: ")) + char2unicode(e.what()));
		wxASSERT(false);
		
		return 0;
 	}
}


bool CClientCreditsList::VerifyIdent(CClientCredits* pTarget, const byte* pachSignature, uint8 nInputSize, uint32 dwForIP, uint8 byChaIPKind)
{
	wxASSERT( pTarget );
	wxASSERT( pachSignature );
	if ( !CryptoAvailable() ){
		pTarget->SetIdentState(IS_NOTAVAILABLE);
		return false;
	}
	bool bResult;
	try {
		CryptoPP::StringSource ss_Pubkey((byte*)pTarget->GetSecureIdent(),pTarget->GetSecIDKeyLen(),true,0);
		CryptoPP::RSASSA_PKCS1v15_SHA_Verifier pubkey(ss_Pubkey);
		// 4 additional bytes random data send from this client +5 bytes v2
		byte abyBuffer[MAXPUBKEYSIZE+9];
		memcpy(abyBuffer,m_abyMyPublicKey,m_nMyPublicKeyLen);
		uint32 challenge = pTarget->m_dwCryptRndChallengeFor;
		wxASSERT ( challenge != 0 );
		PokeUInt32(abyBuffer+m_nMyPublicKeyLen, challenge);
		
		// v2 security improvments (not supported by 29b, not used as default by 29c)
		uint8 nChIpSize = 0;
		if (byChaIPKind != 0){
			nChIpSize = 5;
			uint32 ChallengeIP = 0;
			switch (byChaIPKind){
				case CRYPT_CIP_LOCALCLIENT:
					ChallengeIP = dwForIP;
					break;
				case CRYPT_CIP_REMOTECLIENT:
					// Ignore local ip...
					if (!theApp.GetPublicIP(true)) {
						if (::IsLowID(theApp.GetED2KID())){
							AddDebugLogLineM( false, logCredits, wxT("Warning: Maybe SecureHash Ident fails because LocalIP is unknown"));
							// Fallback to local ip...
							ChallengeIP = theApp.GetPublicIP();
						} else {
							ChallengeIP = theApp.GetED2KID();
						}
					} else {
						ChallengeIP = theApp.GetPublicIP();
					}
					break;
				case CRYPT_CIP_NONECLIENT: // maybe not supported in future versions
					ChallengeIP = 0;
					break;
			}
			PokeUInt32(abyBuffer+m_nMyPublicKeyLen+4, ChallengeIP);
			PokeUInt8(abyBuffer+m_nMyPublicKeyLen+4+4, byChaIPKind);
		}
		//v2 end
		
 		bResult = pubkey.VerifyMessage(abyBuffer, m_nMyPublicKeyLen+4+nChIpSize, pachSignature, nInputSize);
	} catch (const CryptoPP::Exception& e) {
		AddDebugLogLineM(true, logCredits, wxString(wxT("Error while verifying identity: ")) + char2unicode(e.what()));
 		bResult = false;
 	}

	if (!bResult){
		if (pTarget->GetIdentState() == IS_IDNEEDED)
			pTarget->SetIdentState(IS_IDFAILED);
	} else {
		pTarget->Verified(dwForIP);
	}

	return bResult;
}


bool CClientCreditsList::CryptoAvailable() const
{
	return (m_nMyPublicKeyLen > 0 && m_pSignkey != 0 && thePrefs::IsSecureIdentEnabled());
}


#ifdef _DEBUG
bool CClientCreditsList::Debug_CheckCrypting(){
	// create random key
	CryptoPP::AutoSeededRandomPool rng;

	CryptoPP::RSASSA_PKCS1v15_SHA_Signer priv(rng, 384);
	CryptoPP::RSASSA_PKCS1v15_SHA_Verifier pub(priv);

	byte abyPublicKey[80];
	ArraySink asink(abyPublicKey, 80);
	pub.DEREncode(asink);
	int8 PublicKeyLen = asink.TotalPutLength();
	asink.MessageEnd();
	uint32 challenge = rand();
	// create fake client which pretends to be this emule
	CreditStruct* newcstruct = new CreditStruct();
	CClientCredits newcredits(newcstruct);
	newcredits.SetSecureIdent(m_abyMyPublicKey,m_nMyPublicKeyLen);
	newcredits.m_dwCryptRndChallengeFrom = challenge;
	// create signature with fake priv key
	byte pachSignature[200];
	memset(pachSignature,200,0);
	uint8 sigsize = CreateSignature(&newcredits,pachSignature,200,0,false, &priv);


	// next fake client uses the random created public key
	CreditStruct* newcstruct2 = new CreditStruct();
	CClientCredits newcredits2(newcstruct2);
	newcredits2.m_dwCryptRndChallengeFor = challenge;

	// if you uncomment one of the following lines the check has to fail
	//abyPublicKey[5] = 34;
	//m_abyMyPublicKey[5] = 22;
	//pachSignature[5] = 232;

	newcredits2.SetSecureIdent(abyPublicKey,PublicKeyLen);

	//now verify this signature - if it's true everything is fine
	return VerifyIdent(&newcredits2,pachSignature,sigsize,0,0);
}
#endif
