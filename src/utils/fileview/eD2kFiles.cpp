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

#include "eD2kFiles.h"
#include "Print.h"
#include "../../SafeFile.h"
#include "../../ClientCredits.h"	// Needed for MAXPUBKEYSIZE
#include "../../OtherFunctions.h"	// Needed for CastItoXBytes
#include <protocol/ed2k/Constants.h>
#include <tags/FileTags.h>

enum VersionType {
	PrefFile,
	PartMetFile,
	CreditFile,
	KnownFileList
};

static wxString VersionInfo(uint8_t version, uint8_t type)
{
	wxString verStr = wxT("??");
	if (type == PrefFile) {
		if (version == PREFFILE_VERSION) {
			verStr = wxT("PREFFILE_VERSION");
		}
	} else if (type == PartMetFile) {
		if (version == PARTFILE_VERSION) {
			verStr = wxT("PARTFILE_VERSION");
		} else if (version == PARTFILE_SPLITTEDVERSION) {
			verStr = wxT("PARTFILE_SPLITTEDVERSION");
		} else if (version == PARTFILE_VERSION_LARGEFILE) {
			verStr = wxT("PARTFILE_VERSION_LARGEFILE");
		}
	} else if (type == CreditFile) {
		if (version == CREDITFILE_VERSION) {
			verStr = wxT("CREDITFILE_VERSION");
		}
	} else if (type == KnownFileList) {
		if (version == MET_HEADER) {
			verStr = wxT("MET_HEADER");
		} else if (version == MET_HEADER_WITH_LARGEFILES) {
			verStr = wxT("MET_HEADER_WITH_LARGEFILES");
		}
	}
	return hex(version) + wxT(' ') + verStr;
};

void DecodePreferencesDat(const CFileDataIO& file)
{
	cout << "(Version): " << VersionInfo(file.ReadUInt8(), PrefFile);
	cout << "\nUserHash : " << file.ReadHash() << '\n';
}

void DecodeFriendList(const CFileDataIO& file)
{
	uint8_t version = file.ReadUInt8();
	cout << "Version : " << VersionInfo(version, KnownFileList) << endl;
	if (version != MET_HEADER) {
		cerr << "File seems to be corrupt, invalid version!" << endl;
		return;
	}
	uint32_t records = file.ReadUInt32();
	cout << "Records : " << records << '\n';
	for (uint32_t i = 0; i < records; i++) {
		cout << "\tUserHash   : " << file.ReadHash();
		cout << "\n\tLasUsedIP  : " << CeD2kIP(file.ReadUInt32());
		cout << "\n\tLasUsedPort: " << file.ReadUInt16();
		cout << "\n\tLastSeen   : " << CTimeT(file.ReadUInt32());
		cout << "\n\tLastChatted: " << CTimeT(file.ReadUInt32());
		uint32_t tagCount = file.ReadUInt32();
		cout << "\n\ttagCount   : " << tagCount << '\n';
		for (; tagCount > 0; --tagCount) {
			CTag tag(file, true);
			cout << "\t\t" << CFriendTag(tag) << '\n';
		}
	}
}

void DecodeServerMet(const CFileDataIO& file)
{
	uint8_t version = file.ReadUInt8();
	cout << "Version : " << VersionInfo(version, KnownFileList)  << endl;
	if (version != 0xE0 && version != MET_HEADER) {
		cerr << "File seems to be corrupt, invalid version!" << endl;
		return;
	}
	uint32_t ServerCount = file.ReadUInt32();
	cout << "Count   : " << ServerCount << '\n';
	for (; ServerCount > 0; --ServerCount) {
		cout << "\tIP       : " << CeD2kIP(file.ReadUInt32()) << '\n';
		cout << "\tPort     : " << file.ReadUInt16() << '\n';
		uint32_t tagCount = file.ReadUInt32();
		cout << "\ttagCount : " << tagCount << '\n';
		for (; tagCount > 0; --tagCount) {
			CTag tag(file, true);
			cout << "\t\t" << CServerTag(tag) << '\n';
		}
	}
}

void DecodeClientsMet(const CFileDataIO& file)
{
	uint8_t version = file.ReadUInt8();
	cout << "Version : " << VersionInfo(version, CreditFile) << endl;
	if (version != CREDITFILE_VERSION) {
		cerr << "File seems to be corrupt, invalid version!" << endl;
		return;
	}
	uint32_t count = file.ReadUInt32();
	cout << "Count  : " << count << '\n';
	for (uint32_t i = 0; i < count; i++) {
		cout << wxString::Format(wxT("#%u\tKey          : "), i) << file.ReadHash();
		uint32_t uploaded = file.ReadUInt32();
		uint32_t downloaded = file.ReadUInt32();
		cout << "\n\tUploaded     : " << uploaded;
		cout << "\n\tDownloaded   : " << downloaded;
		cout << "\n\tLast Seen    : " << CTimeT(file.ReadUInt32());
		uint32_t uphi = file.ReadUInt32();
		uint32_t downhi = file.ReadUInt32();
		uint64_t totalup = (static_cast<uint64_t>(uphi) << 32) + uploaded;
		uint64_t totaldown = (static_cast<uint64_t>(downhi) << 32) + downloaded;
		cout << "\n\tUploadedHI   : " << uphi << "   Total : " << totalup << " (" << CastItoXBytes(totalup) << ')';
		cout << "\n\tDownloadedHI : " << downhi << "   Total : " << totaldown << " (" << CastItoXBytes(totaldown) << ')';
		cout << "\n\t(Reserved)   : " << file.ReadUInt16();
		uint8_t keysize = file.ReadUInt8();
		cout << "\n\tKeySize      : " << (unsigned)keysize;
		cout << "\n\tKey Data     : ";
		char buf[MAXPUBKEYSIZE];
		file.Read(buf, MAXPUBKEYSIZE);
		PrintByteArray(buf, MAXPUBKEYSIZE);
		cout << endl;
		if (keysize > MAXPUBKEYSIZE) {
			cerr << "\n*** Corruption found while reading credit file! ***\n" << endl;
			break;
		}
	}
}

static inline void PrintDateFromFile(const CFileDataIO& file, const wxString& prefix = wxEmptyString)
{
	cout << prefix << "LastChanged : " << CTimeT(file.ReadUInt32()) << '\n';
}

static void PrintHashsetFromFile(const CFileDataIO& file, const wxString& prefix = wxEmptyString)
{
	cout << prefix << "FileHash    : " << file.ReadHash() << '\n';

	uint16_t parts = file.ReadUInt16();
	cout << prefix << "Parts       : " << parts << '\n';

	cout << prefix << "HashSet     : ";
	for (uint16_t i = 0; i < parts; i++)
	{
		if (i) {
			cout << ", ";
		}
		cout << file.ReadHash();
	}
	cout << '\n';
}

void DecodeKnownMet(const CFileDataIO& file)
{
	uint8_t version = file.ReadUInt8();
	cout << "Version : " << VersionInfo(version, KnownFileList) << endl;
	if (version != MET_HEADER && version != MET_HEADER_WITH_LARGEFILES) {
		cerr << "File seems to be corrupt, invalid version!" << endl;
		return;
	}
	uint32_t records = file.ReadUInt32();
	cout << "Records : " << records << '\n';
	for (uint32_t i = 0; i < records; i++) {
		PrintDateFromFile(file, wxString::Format(wxT("#%u\t"), i));
		PrintHashsetFromFile(file, wxT("\t"));
		uint32_t tagCount = file.ReadUInt32();
		cout << "\tTagCount    : " << tagCount << '\n';
		for (; tagCount > 0; tagCount--) {
			CTag tag(file, true);
			cout << "\t\t" << tag << '\n';
		}
	}
}

void DecodePartMetFile(const CFileDataIO& file)
{
	uint16_t PartCount = 0;
	uint8_t version = file.ReadUInt8();
	cout << "Version     : " << VersionInfo(version, PartMetFile) << endl;
	if (version != PARTFILE_VERSION && version != PARTFILE_SPLITTEDVERSION && version != PARTFILE_VERSION_LARGEFILE) {
		cerr << "File seems to be corrupt, invalid version!" << endl;
		return;
	}
	bool isnewstyle = (version == PARTFILE_SPLITTEDVERSION);
	if (!isnewstyle) {
		uint8_t test[4];
		file.Seek(24, wxFromStart);
		file.Read(test, 4);
		file.Seek(1, wxFromStart);
		if (test[0] == 0 && test[1] == 0 && test[2] == 2 && test[3] == 1) {
			isnewstyle = true;
		}
	}
	if (isnewstyle) {
		uint32_t temp = file.ReadUInt32();
		if (temp == 0) {
			PrintHashsetFromFile(file);
		} else {
			file.Seek(2, wxFromStart);
			PrintDateFromFile(file);
			cout << "FileHash    : " << file.ReadHash() << '\n';
		}
	} else {
		PrintDateFromFile(file);
		PrintHashsetFromFile(file);
	}
	uint32_t tagCount = file.ReadUInt32();
	cout << "TagCount    : " << tagCount << '\n';
	for (; tagCount > 0; tagCount--) {
		CTag tag(file, true);
		if (tag.GetNameID() == FT_FILESIZE) {
			PartCount = (tag.GetInt() + (PARTSIZE - 1)) / PARTSIZE;
		}
		cout << '\t' << tag << '\n';
	}
	if (isnewstyle && (file.GetPosition() < file.GetLength())) {
		file.Seek(1, wxFromCurrent);
		cout << "HashSet     : ";
		for (uint16_t i = 0; i < PartCount && (file.GetPosition() + 16 < file.GetLength()); i++) {
			if (i) {
				cout << ", ";
			}
			cout << file.ReadHash();
		}
	}
	cout << '\n';
}
