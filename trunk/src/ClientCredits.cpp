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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ClientCredits.h"
#endif

#include <cmath>
#include <ctime>
#include <sys/types.h>		//
#include <sys/stat.h>		// These three are needed for open(2)
#include <fcntl.h>		//
#include <unistd.h>		// Needed for close(2)
#include <wx/utils.h>
#include <wx/intl.h>		// Needed for _
#include <wx/textfile.h>

#include "Preferences.h"	// Needed for CPreferences

#include "opcodes.h"		// Needed for CREDITFILE_VERSION
#include "GetTickCount.h"	// Needed for GetTickCount
#include "ClientCredits.h"	// Interface declarations
#include "amule.h"			// Needed for theApp
#include "SafeFile.h"		// Needed for CSafeFile
#include "StringFunctions.h"	// Needed for unicode2char
#include "otherfunctions.h" 	// Needed for md4cpy

#include "sockets.h"
#include "CFile.h"

#include "CryptoPP_Inc.h"

//#include "StdAfx.h"


#define CLIENTS_MET_FILENAME		wxT("clients.met")
#define CLIENTS_MET_BAK_FILENAME	wxT("clients.met.BAK")
#define CRYPTKEY_FILENAME			wxT("cryptkey.dat")

using namespace otherfunctions;

CClientCredits::CClientCredits(CreditStruct* in_credits)
{
	m_pCredits = in_credits;
	InitalizeIdent();
	m_dwUnSecureWaitTime = 0;
	m_dwSecureWaitTime = 0;
	m_dwWaitTimeIP = 0;
}

CClientCredits::CClientCredits(const CMD4Hash& key)
{
	m_pCredits = new CreditStruct;
	memset(m_pCredits, 0, sizeof(CreditStruct));
	md4cpy(m_pCredits->abyKey, key);
	InitalizeIdent();
	m_dwUnSecureWaitTime = ::GetTickCount();
	m_dwSecureWaitTime = ::GetTickCount();
	m_dwWaitTimeIP = 0;
}

CClientCredits::~CClientCredits()
{
	delete m_pCredits;
}

void CClientCredits::AddDownloaded(uint32 bytes, uint32 dwForIP) {
	if ( ( GetCurrentIdentState(dwForIP) == IS_IDFAILED || GetCurrentIdentState(dwForIP) == IS_IDBADGUY || GetCurrentIdentState(dwForIP) == IS_IDNEEDED) && theApp.clientcredits->CryptoAvailable() ){
		return;
	}
	//encode
	uint64 current=m_pCredits->nDownloadedHi;
	current=(current<<32)+ m_pCredits->nDownloadedLo + bytes ;

	//recode
	m_pCredits->nDownloadedLo=(uint32)current;
	m_pCredits->nDownloadedHi=(uint32)(current>>32);
}

void CClientCredits::AddUploaded(uint32 bytes, uint32 dwForIP) {
	if ( ( GetCurrentIdentState(dwForIP) == IS_IDFAILED || GetCurrentIdentState(dwForIP) == IS_IDBADGUY || GetCurrentIdentState(dwForIP) == IS_IDNEEDED) && theApp.clientcredits->CryptoAvailable() ){
		return;
	}
	//encode
	uint64 current=m_pCredits->nUploadedHi;
	current=(current<<32)+ m_pCredits->nUploadedLo + bytes ;

	//recode
	m_pCredits->nUploadedLo=(uint32)current;
	m_pCredits->nUploadedHi=(uint32)(current>>32);
}

uint64	CClientCredits::GetUploadedTotal() const {
	return ( (uint64)m_pCredits->nUploadedHi<<32)+m_pCredits->nUploadedLo;
}

uint64	CClientCredits::GetDownloadedTotal() const {
	return ( (uint64)m_pCredits->nDownloadedHi<<32)+m_pCredits->nDownloadedLo;
}

float CClientCredits::GetScoreRatio(uint32 dwForIP)
{
	// check the client ident status
	if ( ( GetCurrentIdentState(dwForIP) == IS_IDFAILED || GetCurrentIdentState(dwForIP) == IS_IDBADGUY || GetCurrentIdentState(dwForIP) == IS_IDNEEDED) && theApp.clientcredits->CryptoAvailable() ){
		// bad guy - no credits for you
		return 1;
	}

	if (GetDownloadedTotal() < 1000000)
		return 1;
	float result = 0;
	if (!GetUploadedTotal())
		result = 10;
	else
		result = (float)(((double)GetDownloadedTotal()*2.0)/(double)GetUploadedTotal());
	float result2 = 0;
	result2 = (float)GetDownloadedTotal()/1048576.0;
	result2 += 2;
	result2 = (double)sqrt((double)result2);

	if (result > result2)
		result = result2;

	if (result < 1)
		return 1;
	else if (result > 10)
		return 10;
	return result;
}


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
	
	CSafeFile file;
	wxString strFileName(theApp.ConfigDir + CLIENTS_MET_FILENAME);
	if (!::wxFileExists(strFileName)) {
		AddLogLineM(true, _("Failed to load creditfile"));
		return;
	}	
	
	try {
	
		file.Open(strFileName, CFile::read);
	
		uint8 version;
		file.Read(&version, 1);
		if ( version != CREDITFILE_VERSION ){
			AddLogLineM(false, _("Creditfile is out of date and will be replaced"));
			file.Close();
			return;
		}

		// everything is ok, lets see if the backup exist...
		wxString strBakFileName(theApp.ConfigDir + CLIENTS_MET_BAK_FILENAME);
	
		bool bCreateBackup = TRUE;
		if (wxFileExists(strBakFileName)) {
			// Ok, the backup exist, get the size
			CFile hBakFile(strBakFileName);
			if ( hBakFile.Length() > file.Length()) {
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
				AddLogLineM(true, _("Could not create backup file ") + strFileName);
			}
			// reopen file
			if (!file.Open(strFileName, CFile::read)) {
				AddLogLineM(true, _("Failed to load creditfile"));
				return;
			}
			file.Seek(1);
		}	
	
	
		uint32 count = file.ReadUInt32();

		const uint32 dwExpired = time(NULL) - 12960000; // today - 150 day
		uint32 cDeleted = 0;
		bool error = false;
		for (uint32 i = 0; i < count; i++){
			CreditStruct* newcstruct = new CreditStruct;
			memset(newcstruct, 0, sizeof(CreditStruct));
			file.Read(newcstruct, sizeof(CreditStruct));
		
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
				
				error = true;
				
				break;
			}
		
			if (newcstruct->nLastSeen < dwExpired){
				cDeleted++;
				delete newcstruct;
				continue;
			}

			CClientCredits* newcredits = new CClientCredits(newcstruct);
			m_mapClients[ CMD4Hash(newcredits->GetKey()) ] = newcredits;
		}
		file.Close();

		GUIEvent event(ADDLOGLINE);
		
		if ( error ) {
			printf("WARNING: Corruptions found while reading Creditfile!\n");
			
			event.string_value = _("WARNING: Failed to load Creditfile due to corruptions!");	
			event.byte_value = true;
		} else {
			event.string_value = wxString::Format(_("Creditfile loaded, %u clients are known"),count-cDeleted);	
			event.byte_value = false;
	
			if (cDeleted) {
				event.string_value += wxString::Format(_(" - Credits expired for %u clients!"),cDeleted);
			}
		}
	
		
		theApp.NotifyEvent(event);
		
	} catch (wxString error) {
		AddLogLineM(true, _("Unable to load clients.met file! ") + error);
	} catch (...) {
		AddLogLineM(true, _("Unable to load clients.met file! - Unknown Error"));
	}

}

void CClientCreditsList::SaveList()
{
	AddDebugLogLineM(false, wxT("Saved Credit list"));
	m_nLastSaved = ::GetTickCount();

	wxString name(theApp.ConfigDir + CLIENTS_MET_FILENAME);
	CSafeFile file;

	if ( !file.Create(name, true) ) {
		AddLogLineM(true, _("Failed to create creditfile"));
		return;
	}
	
	if ( file.Open(name, CFile::write) ) {
		uint32 count = 0;

		file.WriteUInt8( CREDITFILE_VERSION );

		// Temporary place-holder for number of stucts
		file.WriteUInt32( 0 );

		ClientMap::iterator it = m_mapClients.begin();
		for ( ; it != m_mapClients.end(); ++it ) {	
			CClientCredits* cur_credit = it->second;
		
			if ( cur_credit->GetUploadedTotal() || cur_credit->GetDownloadedTotal() ) {
				file.Write( cur_credit->GetDataStruct(), sizeof(CreditStruct) );
				count++;
			}
		}
		
		// Write the actual number of structs
		file.Seek( 1 );
		file.WriteUInt32( count );

		file.Flush();
		file.Close();
	} else {
		AddLogLineM(true, _("Failed to open existing creditfile!"));
	}
}

CClientCredits* CClientCreditsList::GetCredit(const CMD4Hash& key)
{
	CClientCredits* result;

	ClientMap::iterator it = m_mapClients.find( key );

	
	if ( it == m_mapClients.end() ){
		result = new CClientCredits(key);
		m_mapClients[ CMD4Hash(result->GetKey()) ] = result;
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

void CClientCredits::InitalizeIdent(){
	if (m_pCredits->nKeySize == 0 ){
		memset(m_abyPublicKey,0,80); // for debugging
		m_nPublicKeyLen = 0;
		IdentState = IS_NOTAVAILABLE;
	}
	else{
		m_nPublicKeyLen = m_pCredits->nKeySize;
		memcpy(m_abyPublicKey, m_pCredits->abySecureIdent, m_nPublicKeyLen);
		IdentState = IS_IDNEEDED;
	}
	m_dwCryptRndChallengeFor = 0;
	m_dwCryptRndChallengeFrom = 0;
	m_dwIdentIP = 0;
}

void CClientCredits::Verified(uint32 dwForIP){
	m_dwIdentIP = dwForIP;
	// client was verified, copy the keyto store him if not done already
	if (m_pCredits->nKeySize == 0){
		m_pCredits->nKeySize = m_nPublicKeyLen; 
		memcpy(m_pCredits->abySecureIdent, m_abyPublicKey, m_nPublicKeyLen);
		if (GetDownloadedTotal() > 0){
			// for security reason, we have to delete all prior credits here
			m_pCredits->nDownloadedHi = 0;
			m_pCredits->nDownloadedLo = 1;
			m_pCredits->nUploadedHi = 0;
			m_pCredits->nUploadedLo = 1; // in order to safe this client, set 1 byte
			AddDebugLogLineM(false, wxT("Credits deleted due to new SecureIdent"));
		}
	}
	IdentState = IS_IDENTIFIED;
}

bool CClientCredits::SetSecureIdent(const uchar* pachIdent, uint8 nIdentLen){ // verified Public key cannot change, use only if there is not public key yet
	if (MAXPUBKEYSIZE < nIdentLen || m_pCredits->nKeySize != 0 )
		return false;
	memcpy(m_abyPublicKey,pachIdent, nIdentLen);
	m_nPublicKeyLen = nIdentLen;
	IdentState = IS_IDNEEDED;
	return true;
}

EIdentState	CClientCredits::GetCurrentIdentState(uint32 dwForIP) const {
	if (IdentState != IS_IDENTIFIED)
		return IdentState;
	else{
		if (dwForIP == m_dwIdentIP)
			return IS_IDENTIFIED;
		else
			return IS_IDBADGUY; 
			// mod note: clients which just reconnected after an IP change and have to ident yet will also have this state for 1-2 seconds
			//		 so don't try to spam such clients with "bad guy" messages (besides: spam messages are always bad)
	}
}


bool CClientCreditsList::CreateKeyPair(){
	try{
		CryptoPP::AutoSeededRandomPool rng;
		CryptoPP::InvertibleRSAFunction privkey;
		privkey.Initialize(rng,RSAKEYSIZE);

		CryptoPP::Base64Encoder privkeysink(new CryptoPP::FileSink(unicode2char(theApp.ConfigDir + CRYPTKEY_FILENAME)));
		
		privkey.DEREncode(privkeysink);
		
		privkeysink.MessageEnd();

		AddDebugLogLineM(false, wxT("Created new RSA keypair"));
	}
	catch(...)
	{
		AddDebugLogLineM(false, wxT("Failed to create new RSA keypair"));
		wxASSERT ( false );
		return false;
	}
	return true;
}


void CClientCreditsList::InitalizeCrypting(){
	m_nMyPublicKeyLen = 0;
	memset(m_abyMyPublicKey,0,80); // not really needed; better for debugging tho
	m_pSignkey = NULL;

	if (!thePrefs::IsSecureIdentEnabled()) {
		return;
	}
	
	// check if keyfile is there
	bool bCreateNewKey = false;

	CFile KeyFile;

	if (wxFileExists(theApp.ConfigDir + CRYPTKEY_FILENAME)) {
		KeyFile.Open(theApp.ConfigDir + CRYPTKEY_FILENAME,CFile::read);
		if (KeyFile.Length() == 0) {
			printf("cryptkey.dat is 0 size, creating\n");
			bCreateNewKey = true;
		}
		KeyFile.Close();
	}
	else {
		printf("No cryptkey.dat found, creating\n");
		bCreateNewKey = true;
	}
	
	if (bCreateNewKey)
		CreateKeyPair();
	
	// load key
	try{
		// load private key
		CryptoPP::FileSource filesource(unicode2char(theApp.ConfigDir + CRYPTKEY_FILENAME), true,new CryptoPP::Base64Decoder);
		m_pSignkey = new CryptoPP::RSASSA_PKCS1v15_SHA_Signer(filesource);
		// calculate and store public key
		CryptoPP::RSASSA_PKCS1v15_SHA_Verifier pubkey(*((CryptoPP::RSASSA_PKCS1v15_SHA_Signer*)m_pSignkey));
		CryptoPP::ArraySink asink(m_abyMyPublicKey, 80);
		pubkey.DEREncode(asink);
		m_nMyPublicKeyLen = asink.TotalPutLength();
		asink.MessageEnd();
	}
	catch(...)
	{
		if (m_pSignkey){
			delete (CryptoPP::RSASSA_PKCS1v15_SHA_Signer*)m_pSignkey;
			m_pSignkey = NULL;
		}
		AddLogLineM(false, wxT("IDS_CRYPT_INITFAILED\n"));
	}
	//Debug_CheckCrypting();
}

uint8 CClientCreditsList::CreateSignature(CClientCredits* pTarget, uchar* pachOutput, uint8 nMaxSize, uint32 ChallengeIP, uint8 byChaIPKind, void* sigkey){
	
	CryptoPP::RSASSA_PKCS1v15_SHA_Signer* signer = (CryptoPP::RSASSA_PKCS1v15_SHA_Signer*)sigkey;
	// signer param is used for debug only
	if (signer == NULL)
		signer = (CryptoPP::RSASSA_PKCS1v15_SHA_Signer*)m_pSignkey;

	// create a signature of the public key from pTarget
	wxASSERT( pTarget );
	wxASSERT( pachOutput );
	uint8 nResult;
	if ( !CryptoAvailable() )
		return 0;
	try{
		
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
		nResult = asink.TotalPutLength();			
	}
	catch(...)
	{
		wxASSERT ( false );
		nResult = 0;
	}
	return nResult;
}

bool CClientCreditsList::VerifyIdent(CClientCredits* pTarget, const uchar* pachSignature, uint8 nInputSize, uint32 dwForIP, uint8 byChaIPKind){
	wxASSERT( pTarget );
	wxASSERT( pachSignature );
	if ( !CryptoAvailable() ){
		pTarget->IdentState = IS_NOTAVAILABLE;
		return false;
	}
	bool bResult;
	try{
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
					if (theApp.serverconnect->GetClientID() == 0 || theApp.serverconnect->IsLowID()){
						AddDebugLogLineM(false, wxT("Warning: Maybe SecureHash Ident fails because LocalIP is unknown"));
						ChallengeIP = theApp.serverconnect->GetLocalIP();
					}
					else
						ChallengeIP = theApp.serverconnect->GetClientID();
					break;
				case CRYPT_CIP_NONECLIENT: // maybe not supported in future versions
					ChallengeIP = 0;
					break;
			}
			PokeUInt32(abyBuffer+m_nMyPublicKeyLen+4, ChallengeIP);
			PokeUInt8(abyBuffer+m_nMyPublicKeyLen+4+4, byChaIPKind);
		}
		//v2 end
		
		// Hello fellow traveler. If you are here because of a compilation
		// problem on this line, just comment that line and uncomment the other
		// Anyway, that means you have libcrypto 5.0 and it's UNDOCUMENTED and 
		// UNTESTED what will happen to your mule, compilation, linking, whatever.
		// libcrypto++ 5.1
		bResult = pubkey.VerifyMessage(abyBuffer, m_nMyPublicKeyLen+4+nChIpSize, pachSignature, nInputSize);
		// libcrypto++ 5.0
		//bResult = pubkey.VerifyMessage(abyBuffer, m_nMyPublicKeyLen+4+nChIpSize, pachSignature);		
	}
	catch(...)
	{
		bResult = false;
	}
	if (!bResult){
		if (pTarget->IdentState == IS_IDNEEDED)
			pTarget->IdentState = IS_IDFAILED;
	}
	else{
		pTarget->Verified(dwForIP);
	}
	return bResult;
}

bool CClientCreditsList::CryptoAvailable() const {
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
	CreditStruct* newcstruct = new CreditStruct;
	memset(newcstruct, 0, sizeof(CreditStruct));
	CClientCredits* newcredits = new CClientCredits(newcstruct);
	newcredits->SetSecureIdent(m_abyMyPublicKey,m_nMyPublicKeyLen);
	newcredits->m_dwCryptRndChallengeFrom = challenge;
	// create signature with fake priv key
	uchar pachSignature[200];
	memset(pachSignature,200,0);
	uint8 sigsize = CreateSignature(newcredits,pachSignature,200,0,false, &priv);


	// next fake client uses the random created public key
	CreditStruct* newcstruct2 = new CreditStruct;
	memset(newcstruct2, 0, sizeof(CreditStruct));
	CClientCredits* newcredits2 = new CClientCredits(newcstruct2);
	newcredits2->m_dwCryptRndChallengeFor = challenge;

	// if you uncomment one of the following lines the check has to fail
	//abyPublicKey[5] = 34;
	//m_abyMyPublicKey[5] = 22;
	//pachSignature[5] = 232;

	newcredits2->SetSecureIdent(abyPublicKey,PublicKeyLen);

	//now verify this signature - if it's true everything is fine
	bool bResult = VerifyIdent(newcredits2,pachSignature,sigsize,0,0);

	delete newcredits;
	delete newcredits2;

	return bResult;
}
#endif

uint32 CClientCredits::GetSecureWaitStartTime(uint32 dwForIP){
	if (m_dwUnSecureWaitTime == 0 || m_dwSecureWaitTime == 0)
		SetSecWaitStartTime(dwForIP);

	if (m_pCredits->nKeySize != 0){	// this client is a SecureHash Client
		if (GetCurrentIdentState(dwForIP) == IS_IDENTIFIED){ // good boy
			return m_dwSecureWaitTime;
		}
		else{	// not so good boy
			if (dwForIP == m_dwWaitTimeIP){
				return m_dwUnSecureWaitTime;
			}
			else{	// bad boy
				// this can also happen if the client has not identified himself yet, but will do later - so maybe he is not a bad boy :) .
				/*wxString buffer2, buffer;
				for (uint16 i = 0;i != 16;i++){
					buffer2.Printf("%02X",this->m_pCredits->abyKey[i]);
					buffer+=buffer2;
				}
				AddDebugLogLine(false,"Warning: WaitTime resetted due to Invalid Ident for Userhash %s",buffer.GetBuffer());*/
				
				m_dwUnSecureWaitTime = ::GetTickCount();
				m_dwWaitTimeIP = dwForIP;
				return m_dwUnSecureWaitTime;
			}	
		}
	}
	else{	// not a SecureHash Client - handle it like before for now (no security checks)
		return m_dwUnSecureWaitTime;
	}
}

void CClientCredits::SetSecWaitStartTime(uint32 dwForIP){
	m_dwUnSecureWaitTime = ::GetTickCount()-1;
	m_dwSecureWaitTime = ::GetTickCount()-1;
	m_dwWaitTimeIP = dwForIP;
}

void CClientCredits::ClearWaitStartTime(){
	m_dwUnSecureWaitTime = 0;
	m_dwSecureWaitTime = 0;
}
