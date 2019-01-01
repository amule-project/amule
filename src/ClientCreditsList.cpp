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

#include "ClientCreditsList.h"	// Interface declarations


#include <protocol/ed2k/Constants.h>
#include <common/Macros.h>
#include <common/DataFileVersion.h>
#include <common/FileFunctions.h>	// Needed for GetFileSize


#include "GetTickCount.h"	// Needed for GetTickCount
#include "Preferences.h"	// Needed for thePrefs
#include "ClientCredits.h"	// Needed for CClientCredits
#include "amule.h"		// Needed for theApp
#include "CFile.h"		// Needed for CFile
#include "Logger.h"		// Needed for Add(Debug)LogLine

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <openssl/ossl_typ.h>
#include <openssl/opensslv.h>

#include <errno.h>
#include <string.h>

#define CLIENTS_MET_FILENAME		wxT("clients.met")
#define CLIENTS_MET_BAK_FILENAME	wxT("clients.met.bak")
#define CRYPTKEY_STR			"cryptkey.pem"
#define CRYPTKEY_FILENAME		wxT(CRYPTKEY_STR)

namespace
{

// Current versions of OpenSSL doesn't allow generation of keys lesser than 512 bits
int rsa_keygen(RSA *r, int bits, unsigned long e)
{
	BIGNUM *r0 = NULL, *r1 = NULL, *r2 = NULL, *r3 = NULL, *tmp;
	BIGNUM *pr0, *d, *p;
	int i, bitsp, bitsq, ok = -1;
#if OPENSSL_VERSION_NUMBER < 0x10100000
#define rsa r
#else
	struct
	{
		BIGNUM *n;
		BIGNUM *e;
		BIGNUM *d;
		BIGNUM *p;
		BIGNUM *q;
		BIGNUM *dmp1;
		BIGNUM *dmq1;
		BIGNUM *iqmp;
	} rsa[1] = {0, 0, 0, 0, 0, 0, 0, 0};
#endif
	BN_CTX *ctx = BN_CTX_new();
	wxASSERT( ctx );
	BN_CTX_start(ctx);
	r0 = BN_CTX_get(ctx);
	r1 = BN_CTX_get(ctx);
	r2 = BN_CTX_get(ctx);
	r3 = BN_CTX_get(ctx);
	wxASSERT( r3 );

	bitsp = (bits + 1) / 2;
	bitsq = bits - bitsp;

	/* We need the RSA components non-NULL */
	if (!rsa->n && ((rsa->n = BN_new()) == NULL))
		goto err;
	if (!rsa->d && ((rsa->d = BN_new()) == NULL))
		goto err;
	if (!rsa->e && ((rsa->e = BN_new()) == NULL))
		goto err;
	if (!rsa->p && ((rsa->p = BN_new()) == NULL))
		goto err;
	if (!rsa->q && ((rsa->q = BN_new()) == NULL))
		goto err;
	if (!rsa->dmp1 && ((rsa->dmp1 = BN_new()) == NULL))
		goto err;
	if (!rsa->dmq1 && ((rsa->dmq1 = BN_new()) == NULL))
		goto err;
	if (!rsa->iqmp && ((rsa->iqmp = BN_new()) == NULL))
		goto err;

	for (i = 0; i < (int)sizeof(unsigned long) * 8; i++) {
		if (e & (1UL << i))
			if (BN_set_bit(rsa->e, i) == 0)
				goto err;
	}

	/* generate p and q */
	for (;;) {
		if (!BN_generate_prime_ex(rsa->p, bitsp, 0, NULL, NULL, NULL))
			goto err;
		if (!BN_sub(r2, rsa->p, BN_value_one()))
			goto err;
		if (!BN_gcd(r1, r2, rsa->e, ctx))
			goto err;
		if (BN_is_one(r1))
			break;
	}
	for (;;) {
		/*
		 * When generating ridiculously small keys, we can get stuck
		 * continually regenerating the same prime values. Check for this and
		 * bail if it happens 3 times.
		 */
		unsigned int degenerate = 0;
		do {
			if (!BN_generate_prime_ex(rsa->q, bitsq, 0, NULL, NULL, NULL))
				goto err;
		} while ((BN_cmp(rsa->p, rsa->q) == 0) && (++degenerate < 3));
		if (degenerate == 3) {
			ok = 0;             /* we set our own err */
			RSAerr(RSA_F_RSA_BUILTIN_KEYGEN, RSA_R_KEY_SIZE_TOO_SMALL);
			goto err;
		}
		if (!BN_sub(r2, rsa->q, BN_value_one()))
			goto err;
		if (!BN_gcd(r1, r2, rsa->e, ctx))
			goto err;
		if (BN_is_one(r1))
			break;
	}
	if (BN_cmp(rsa->p, rsa->q) < 0) {
		tmp = rsa->p;
		rsa->p = rsa->q;
		rsa->q = tmp;
	}

	/* calculate n */
	if (!BN_mul(rsa->n, rsa->p, rsa->q, ctx))
		goto err;

	/* calculate d */
	if (!BN_sub(r1, rsa->p, BN_value_one()))
		goto err;               /* p-1 */
	if (!BN_sub(r2, rsa->q, BN_value_one()))
		goto err;               /* q-1 */
	if (!BN_mul(r0, r1, r2, ctx))
		goto err;               /* (p-1)(q-1) */
	pr0 = r0;
	if (!BN_mod_inverse(rsa->d, rsa->e, pr0, ctx))
		goto err;               /* d */

	d = rsa->d;

	/* calculate d mod (p-1) */
	if (!BN_mod(rsa->dmp1, d, r1, ctx))
		goto err;

	/* calculate d mod (q-1) */
	if (!BN_mod(rsa->dmq1, d, r2, ctx))
		goto err;

	/* calculate inverse of q mod p */
	p = rsa->p;
	if (!BN_mod_inverse(rsa->iqmp, rsa->q, p, ctx))
		goto err;

	ok = 1;
#if OPENSSL_VERSION_NUMBER >= 0x10100000
	wxASSERT( RSA_set0_key(r, rsa->n, rsa->e, rsa->d) );
	wxASSERT( RSA_set0_factors(r, rsa->p, rsa->q) );
	wxASSERT( RSA_set0_crt_params(r, rsa->dmp1, rsa->dmq1, rsa->iqmp) );
#endif
err:
	if (ok == -1) {
		RSAerr(RSA_F_RSA_BUILTIN_KEYGEN, ERR_LIB_BN);
		ok = 0;
#if OPENSSL_VERSION_NUMBER >= 0x10100000
		BN_free(rsa->n);
		BN_free(rsa->d);
		BN_free(rsa->e);
		BN_free(rsa->p);
		BN_free(rsa->q);
		BN_free(rsa->dmp1);
		BN_free(rsa->dmq1);
		BN_free(rsa->iqmp);
#endif
	}
	if (ctx != NULL) {
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
	}
	return ok;
}

RSA* key2rsa(void* key)
{
	return static_cast<RSA*>(key);
}

wxString CryptoError()
{
	return wxString(char2unicode(ERR_error_string(ERR_get_error(), NULL)));
}

};

CClientCreditsList::CClientCreditsList()
{
	m_nLastSaved = ::GetTickCount();
	LoadList();

	InitalizeCrypting();
}


CClientCreditsList::~CClientCreditsList()
{
	DeleteContents(m_mapClients);
	RSA_free(key2rsa(m_pSignkey));
}


void CClientCreditsList::LoadList()
{
	CFile file;
	CPath fileName = CPath(thePrefs::GetConfigDir() + CLIENTS_MET_FILENAME);

	if (!fileName.FileExists()) {
		return;
	}

	try {
		file.Open(fileName, CFile::read);

		if (file.ReadUInt8() != CREDITFILE_VERSION) {
			AddDebugLogLineC( logCredits, wxT("Creditfile is outdated and will be replaced") );
			file.Close();
			return;
		}

		// everything is ok, lets see if the backup exist...
		CPath bakFileName = CPath(thePrefs::GetConfigDir() + CLIENTS_MET_BAK_FILENAME);

		bool bCreateBackup = TRUE;
		if (bakFileName.FileExists()) {
			// Ok, the backup exist, get the size
			CFile hBakFile(bakFileName);
			if ( hBakFile.GetLength() > file.GetLength()) {
				// the size of the backup was larger then the
				// org. file, something is wrong here, don't
				// overwrite old backup..
				bCreateBackup = FALSE;
			}
			// else: backup is smaller or the same size as org.
			// file, proceed with copying of file
		}

		//else: the backup doesn't exist, create it
		if (bCreateBackup) {
			file.Close(); // close the file before copying
			if (!CPath::CloneFile(fileName, bakFileName, true)) {
				AddDebugLogLineC(logCredits,
					CFormat(wxT("Could not create backup file '%s'")) % fileName);
			}
			// reopen file
			if (!file.Open(fileName, CFile::read)) {
				AddDebugLogLineC( logCredits,
					wxT("Failed to load creditfile") );
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
			newcstruct->uploaded            = file.ReadUInt32();
			newcstruct->downloaded          = file.ReadUInt32();
			newcstruct->nLastSeen           = file.ReadUInt32();
			newcstruct->uploaded            += static_cast<uint64>(file.ReadUInt32()) << 32;
			newcstruct->downloaded          += static_cast<uint64>(file.ReadUInt32()) << 32;
			newcstruct->nReserved3          = file.ReadUInt16();
			newcstruct->nKeySize            = file.ReadUInt8();
			file.Read(newcstruct->abySecureIdent, MAXPUBKEYSIZE);

			if ( newcstruct->nKeySize > MAXPUBKEYSIZE ) {
				// Oh dear, this is bad mojo, the file is most likely corrupt
				// We can no longer assume that any of the clients in the file are valid
				// and will have to discard it.
				delete newcstruct;

				DeleteContents(m_mapClients);

				AddDebugLogLineC( logCredits,
					wxT("WARNING: Corruptions found while reading Creditfile!") );
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

		AddLogLineN(CFormat(wxPLURAL("Creditfile loaded, %u client is known", "Creditfile loaded, %u clients are known", count - cDeleted)) % (count - cDeleted));

		if (cDeleted) {
			AddLogLineN(CFormat(wxPLURAL(" - Credits expired for %u client!", " - Credits expired for %u clients!", cDeleted)) % cDeleted);
		}
	} catch (const CSafeIOException& e) {
		AddDebugLogLineC(logCredits, wxT("IO error while loading clients.met file: ") + e.what());
	}
}


void CClientCreditsList::SaveList()
{
	AddDebugLogLineN( logCredits, wxT("Saved Credit list"));
	m_nLastSaved = ::GetTickCount();

	wxString name(thePrefs::GetConfigDir() + CLIENTS_MET_FILENAME);
	CFile file;

	if ( !file.Create(name, true) ) {
		AddDebugLogLineC( logCredits, wxT("Failed to create creditfile") );
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
					file.WriteUInt32(static_cast<uint32>(cstruct->uploaded));
					file.WriteUInt32(static_cast<uint32>(cstruct->downloaded));
					file.WriteUInt32(cstruct->nLastSeen);
					file.WriteUInt32(static_cast<uint32>(cstruct->uploaded >> 32));
					file.WriteUInt32(static_cast<uint32>(cstruct->downloaded >> 32));
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
			AddDebugLogLineC(logCredits, wxT("IO failure while saving clients.met: ") + e.what());
		}
	} else {
		AddDebugLogLineC(logCredits, wxT("Failed to open existing creditfile!"));
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
	RSA *keypair = RSA_new();
	if (!rsa_keygen(keypair, RSAKEYSIZE, 3)) {
		AddDebugLogLineC(logCredits,
			wxString(wxT("Failed to create new RSA keypair: ")) + CryptoError());
		RSA_free(keypair);
		return false;
	}

	FILE *fp = fopen(unicode2UTF8(thePrefs::GetConfigDir() + CRYPTKEY_FILENAME), "w");
	if (!fp) {
		AddDebugLogLineC(logCredits,
			wxString(wxT("Failed to open '" CRYPTKEY_STR "': ")) +
			wxString(char2unicode(strerror(errno))));
		RSA_free(keypair);
		return false;
	}

	bool res = !!PEM_write_RSAPrivateKey(fp, keypair, 0, 0, 0, 0, 0);
	if (res) {
		AddDebugLogLineN(logCredits, wxT("Created new RSA keypair"));
	} else {
		AddDebugLogLineC(logCredits,
			wxString(wxT("Failed to write RSA keypair: ")) + CryptoError());
		wxFAIL;
	}
	RSA_free(keypair);
	fclose(fp);
	return res;
}


void CClientCreditsList::InitalizeCrypting()
{
	m_nMyPublicKeyLen = 0;
	memset(m_abyMyPublicKey,0,80); // not really needed; better for debugging tho
	m_pSignkey = NULL;

	if (!thePrefs::IsSecureIdentEnabled()) {
		return;
	}

	// check if keyfile is there
	if (wxFileExists(thePrefs::GetConfigDir() + CRYPTKEY_FILENAME)) {
		off_t keySize = CPath::GetFileSize(thePrefs::GetConfigDir() + CRYPTKEY_FILENAME);

		if (keySize == wxInvalidOffset) {
			AddDebugLogLineC(logCredits, wxT("Cannot access '" CRYPTKEY_STR "', please check permissions."));
			return;
		} else if (keySize == 0) {
			AddDebugLogLineC(logCredits, wxT("'" CRYPTKEY_STR ".dat' is empty, recreating keypair."));
			CreateKeyPair();
		}
	} else {
		AddLogLineN(_("No '" CRYPTKEY_STR "' file found, creating.") );
		CreateKeyPair();
	}

	// load private key
	FILE *fp = fopen(unicode2UTF8(thePrefs::GetConfigDir() + CRYPTKEY_FILENAME), "r");
	if (!fp) {
		AddDebugLogLineC(logCredits,
			wxString(wxT("Failed to open '" CRYPTKEY_STR "': ")) +
			wxString(char2unicode(strerror(errno))));
		return;
	}

	RSA* rsa = PEM_read_RSAPrivateKey(fp, 0, 0, 0);
	fclose(fp);
	if (!rsa) {
		AddDebugLogLineC(logCredits,
			wxString(wxT("Failed to read '" CRYPTKEY_STR "': ")) + CryptoError());
		return;
	}

	byte *p = m_abyMyPublicKey;
	m_nMyPublicKeyLen = i2d_RSA_PUBKEY(rsa, &p);
	wxASSERT( m_nMyPublicKeyLen );
	wxASSERT( m_nMyPublicKeyLen <= 80 );
	m_pSignkey = rsa;
}


uint8 CClientCreditsList::CreateSignature(CClientCredits* pTarget, byte* pachOutput, uint8 nMaxSize, uint32 ChallengeIP, uint8 byChaIPKind, void* sigkey)
{
	// signer param is used for debug only
	RSA* rsa = sigkey
		? static_cast<RSA*>(sigkey)
		: key2rsa(m_pSignkey);

	// create a signature of the public key from pTarget
	wxASSERT( pTarget );
	wxASSERT( pachOutput );

	if ( !CryptoAvailable() )
		return 0;

	byte abyBuffer[MAXPUBKEYSIZE+9];
	const uint32 keylen = pTarget->GetSecIDKeyLen();
	memcpy(abyBuffer,pTarget->GetSecureIdent(),keylen);
	// 4 additional bytes random data send from this client
	const uint32 challenge = pTarget->m_dwCryptRndChallengeFrom;
	wxASSERT ( challenge != 0 );
	PokeUInt32(abyBuffer+keylen,challenge);

	uint16 ChIpLen = 0;
	if ( byChaIPKind != 0) {
		ChIpLen = 5;
		PokeUInt32(abyBuffer+keylen+4, ChallengeIP);
		PokeUInt8(abyBuffer+keylen+4+4,byChaIPKind);
	}

	// first calculate message digest
	byte md[SHA_DIGEST_LENGTH];
	SHA1(abyBuffer, keylen+4+ChIpLen, md);

	unsigned int siglen = RSA_size(rsa);
	wxASSERT( nMaxSize >= siglen );
	wxASSERT( RSA_sign(NID_sha1, md, sizeof(md), pachOutput, &siglen, rsa) );

	return siglen;
}

namespace
{
bool Verify(const byte* myIdent, uint8 myIdentLen,
	const byte* targetIdent, uint8 targeIdenttLen,
	const byte* signature, uint8 signatureSize,
	uint32 challenge, uint32 ChallengeIP, uint8 byChaIPKind)
{
	const byte *k = targetIdent;
	RSA* rsa = d2i_RSA_PUBKEY(0, &k, targeIdenttLen);
	if (!rsa) {
		AddDebugLogLineC(logCredits, wxString(wxT("Error while verifying identity: ")) + CryptoError());
		return false;
	}

	if (RSA_size(rsa) != signatureSize) {
		AddDebugLogLineC(logCredits, CFormat(wxT("Wrong signature size %u, must be %i")) % signatureSize % RSA_size(rsa));
		RSA_free(rsa);
		return false;
	}

	byte abyBuffer[MAXPUBKEYSIZE+9];
	memcpy(abyBuffer,myIdent,myIdentLen);
	wxASSERT ( challenge != 0 );
	PokeUInt32(abyBuffer+myIdentLen, challenge);

	// v2 security improvments (not supported by 29b, not used as default by 29c)
	uint8 nChIpSize = 0;
	if (byChaIPKind != 0){
		nChIpSize = 5;
		PokeUInt32(abyBuffer+myIdentLen+4, ChallengeIP);
		PokeUInt8(abyBuffer+myIdentLen+4+4, byChaIPKind);
	}
	//v2 end

	byte md[SHA_DIGEST_LENGTH];
	SHA1(abyBuffer, myIdentLen+4+nChIpSize, md);
	bool valid = !!RSA_verify(NID_sha1, md, sizeof(md), signature, RSA_size(rsa), rsa);
	RSA_free(rsa);
	if (!valid) {
		AddDebugLogLineC(logCredits, wxString(wxT("Error while verifying identity: ")) + CryptoError());
	}
	return valid;
}

};

bool CClientCreditsList::VerifyIdent(CClientCredits* pTarget, const byte* pachSignature, uint8 nInputSize, uint32 dwForIP, uint8 byChaIPKind)
{
	wxASSERT( pTarget );
	wxASSERT( pachSignature );
	if ( !CryptoAvailable() ){
		pTarget->SetIdentState(IS_NOTAVAILABLE);
		return false;
	}

	uint32 ChallengeIP = 0;
	switch (byChaIPKind) {
		case 0:
		case CRYPT_CIP_NONECLIENT: // maybe not supported in future versions
			break;
		case CRYPT_CIP_LOCALCLIENT:
			ChallengeIP = dwForIP;
			break;
		case CRYPT_CIP_REMOTECLIENT:
			// Ignore local ip...
			if (!theApp->GetPublicIP(true)) {
				if (::IsLowID(theApp->GetED2KID())){
					AddDebugLogLineN(logCredits, wxT("Warning: Maybe SecureHash Ident fails because LocalIP is unknown"));
					// Fallback to local ip...
					ChallengeIP = theApp->GetPublicIP();
				} else {
					ChallengeIP = theApp->GetED2KID();
				}
			} else {
				ChallengeIP = theApp->GetPublicIP();
			}
			break;
	}
	const bool bResult = Verify(m_abyMyPublicKey, m_nMyPublicKeyLen,
		pTarget->GetSecureIdent(), pTarget->GetSecIDKeyLen(),
		pachSignature, nInputSize,
		pTarget->m_dwCryptRndChallengeFor, ChallengeIP, byChaIPKind);

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
	return m_nMyPublicKeyLen > 0 && m_pSignkey != NULL;
}


#ifdef _DEBUG
bool CClientCreditsList::Debug_CheckCrypting(){
	// create random key
	CryptoPP::AutoSeededX917RNG<CryptoPP::DES_EDE3> rng;

	CryptoPP::RSASSA_PKCS1v15_SHA_Signer priv(rng, 384);
	CryptoPP::RSASSA_PKCS1v15_SHA_Verifier pub(priv);

	byte abyPublicKey[80];
	CryptoPP::ArraySink asink(abyPublicKey, 80);
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
	memset(pachSignature,0,200);
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
// File_checked_for_headers
