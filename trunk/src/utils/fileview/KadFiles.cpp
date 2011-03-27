//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 Dévai Tamás ( gonosztopi@amule.org )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "KadFiles.h"
#include "Print.h"
#include "../../SafeFile.h"
#include "../../kademlia/kademlia/SearchManager.h"	// for CSearchManager::GetInvalidKeywordChars()
#include <wx/tokenzr.h>

void DecodePreferencesKadDat(const CFileDataIO& file)
{
	cout << "IP      : " << CKadIP(file.ReadUInt32()) << '\n';
	cout << "(unused): " << file.ReadUInt16() << '\n';
	cout << "ClientID: " << file.ReadUInt128() << '\n';
}

void DecodeLoadIndexDat(const CFileDataIO& file)
{
	cout << "Version   : " << file.ReadUInt32();
	cout << "\n(savetime): " << CTimeT(file.ReadUInt32());
	uint32_t numLoad = file.ReadUInt32();
	cout << "\nnumLoad   : " << numLoad << '\n';
	for (uint32_t i = 0; i < numLoad; i++) {
		cout << "\t{ " << file.ReadUInt128();
		cout << ", " << CTimeT(file.ReadUInt32()) << " }\n";
	}
}

// from Kademlia.cpp
#include "../../CryptoPP_Inc.h"
void KadGetKeywordHash(const wxString& rstrKeyword, Kademlia::CUInt128* pKadID)
{
	byte Output[16];
#ifdef CRYPTOPP_ENABLE_NAMESPACE_WEAK
	CryptoPP::Weak::MD4 md4_hasher;
#else
	CryptoPP::MD4 md4_hasher;
#endif

	// This should be safe - we assume rstrKeyword is ANSI anyway.
	Unicode2CharBuf ansi_buffer(unicode2UTF8(rstrKeyword));
	
	md4_hasher.CalculateDigest(Output, (const byte *) (const char *) ansi_buffer, strlen(ansi_buffer));
	
	pKadID->SetValueBE(Output);
}

// code from CSearchManager::GetWords(const wxString& str, WordList *words)
bool IdentifyKeyword(const Kademlia::CUInt128& keyID, const wxString& str, wxString& keyword)
{
	wxStringTokenizer tkz(str, Kademlia::CSearchManager::GetInvalidKeywordChars());
	while (tkz.HasMoreTokens()) {
		wxString current_word = tkz.GetNextToken();
		
		if (current_word.Length() > 2) {
			current_word.MakeLower();
			Kademlia::CUInt128 currentID;
			KadGetKeywordHash(current_word, &currentID);
			if (currentID == keyID) {
				keyword = current_word;
				return true;
			}
		}
	}
	return false;
}

void DecodeKeyIndexDat(const CFileDataIO& file)
{
	uint32_t version;
	uint32_t numKeys;
	uint32_t numSource;
	uint32_t numName;
	uint8_t tagCount;

	cout << "Version : " << (version = file.ReadUInt32());
	cout << "\nSaveTime: " << CTimeT(file.ReadUInt32());
	cout << "\nID      : " << file.ReadUInt128();
	cout << "\nnumKeys : " << (numKeys = file.ReadUInt32()) << '\n';
	for (uint32_t ik = 0; ik < numKeys; ik++) {
		Kademlia::CUInt128 keyID = file.ReadUInt128();
		bool identified = false;
		cout << "\tKeyID    : " << keyID;
		cout << "\n\tnumSource: " << (numSource = file.ReadUInt32()) << '\n';
		for (uint32_t is = 0; is < numSource; is++) {
			cout << "\t\tSourceID: " << file.ReadUInt128();
			cout << "\n\t\tnumName : " << (numName = file.ReadUInt32()) << '\n';
			for (uint32_t iN = 0; iN < numName; iN++) {
				cout << "\t\t\tLifeTime : " <<  CTimeT(file.ReadUInt32()) << '\n';
				if (version >= 3) {
					uint32_t count;
					cout << "\t\t\tnameCount: " << (count = file.ReadUInt32()) << '\n';
					for (uint32_t i = 0; i < count; i++) {
						wxString name = file.ReadString(true, 2);
						cout << "\t\t\t\t{ " << MakePrintableString(name);
						cout << ", " << file.ReadUInt32() << " }\n";
						wxString keyword;
						if (!identified && IdentifyKeyword(keyID, name, keyword)) {
							cout << "\tKeyword: " << MakePrintableString(keyword) << '\n';
							identified = true;
						}
					}
					cout << "\t\t\tipCount  : " << (count = file.ReadUInt32()) << '\n';
					for (uint32_t i = 0; i < count; i++) {
						cout << "\t\t\t\t{ " << CKadIP(file.ReadUInt32());
						cout << ", " << CTimeT(file.ReadUInt32()) << " }\n";
					}
				}
				cout << "\t\t\ttagCount : " << (uint32)(tagCount = file.ReadUInt8()) << '\n';
				for (uint32_t it = 0; it < tagCount; it++) {
					CTag *tag = file.ReadTag();
					cout << "\t\t\t\t" << *tag << '\n';
					delete tag;
				}
			}
		}
	}
}

void DecodeSourceIndexDat(const CFileDataIO& file)
{
	uint32_t numKeys;
	uint32_t numSource;
	uint32_t numName;
	uint8_t tagCount;

	cout << "Version : " << file.ReadUInt32();
	cout << "\nSaveTime: " << CTimeT(file.ReadUInt32());
	cout << "\nnumKeys : " << (numKeys = file.ReadUInt32()) << '\n';
	for (uint32_t ik = 0; ik < numKeys; ik++) {
		cout << "\tKeyID    : " << file.ReadUInt128();
		cout << "\n\tnumSource: " << (numSource = file.ReadUInt32()) << '\n';
		for (uint32_t is = 0; is < numSource; is++) {
			cout << "\t\tSourceID: " << file.ReadUInt128();
			cout << "\n\t\tnumName : " << (numName = file.ReadUInt32()) << '\n';
			for (uint32_t iN = 0; iN < numName; iN++) {
				cout << "\t\t\tLifeTime: " << CTimeT(file.ReadUInt32());
				cout << "\n\t\t\ttagCount: " << (tagCount = file.ReadUInt8()) << '\n';
				for (uint32_t it = 0; it < tagCount; it++) {
					CTag *tag = file.ReadTag();
					cout << "\t\t\t\t" << *tag << '\n';
					delete tag;
				}
			}
		}
	}
}

void DecodeNodesDat(const CFileDataIO& file)
{
	uint32_t numContacts = file.ReadUInt32();
	uint32_t fileVersion = 0;

	cout << "NumContacts #1  : " << numContacts << '\n';
	if (numContacts == 0) {
		cout << "FileVersion     : " << (fileVersion = file.ReadUInt32()) << '\n';
		if (fileVersion == 3) {
			cout << "BootstrapEdition: " << file.ReadUInt32() << '\n';
		}
		if (fileVersion >= 1 && fileVersion <= 3) {
			cout << "NumContacts #2  : " << (numContacts = file.ReadUInt32()) << '\n';
		}
	}
	for (uint32_t i = 0; i < numContacts; i++) {
		cout << wxString::Format(wxT("#%u\tID       : "), i) << file.ReadUInt128();
		cout << "\n\tIP       : " << CKadIP(file.ReadUInt32());
		cout << "\n\tUDP Port : " << file.ReadUInt16();
		cout << "\n\tTCP Port : " << file.ReadUInt16();
		if (fileVersion >= 1) {
			cout << "\n\tVersion  : ";
		} else {
			cout << "\n\tType     : ";
		}
		cout << (unsigned)file.ReadUInt8();
		if (fileVersion >= 2) {
			cout << "\n\tUDP Key  : { " << hex(file.ReadUInt32());
			cout << ", " << CKadIP(file.ReadUInt32());
			cout << " }\n\tVerified : " << (file.ReadUInt8() != 0 ? "true" : "false");
		}
		cout << '\n';
	}
}
