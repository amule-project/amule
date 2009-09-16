//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2009 Dévai Tamás ( gonosztopi@amule.org )
// Copyright (c) 2004-2009 aMule Team ( admin@amule.org / http://www.amule.org )
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

void DecodePreferencesKadDat(const CFileDataIO& file)
{
	DoPrint(wxT("IP      : ")); Print(CKadIP(file.ReadUInt32())); DoPrint(wxT("\n"));
	DoPrint(wxT("(unused): ")); Print(file.ReadUInt16()); DoPrint(wxT("\n"));
	DoPrint(wxT("ClientID: ")); Print(file.ReadUInt128()); DoPrint(wxT("\n"));
}

void DecodeLoadIndexDat(const CFileDataIO& file)
{
	DoPrint(wxT("Version   : ")); Print(file.ReadUInt32()); DoPrint(wxT("\n"));
	DoPrint(wxT("(savetime): ")); Print((time_t)file.ReadUInt32()); DoPrint(wxT("\n"));
	uint32_t numLoad = file.ReadUInt32();
	DoPrint(wxT("numLoad   : ")); Print(numLoad); DoPrint(wxT("\n"));
	for (uint32_t i = 0; i < numLoad; i++) {
		DoPrint(wxString::Format(wxT("\t: { "), i)); Print(file.ReadUInt128()); DoPrint(wxT(", ")); Print((time_t)file.ReadUInt32()); DoPrint(wxT(" }\n"));
	}
}

void DecodeKeyIndexDat(const CFileDataIO& file)
{
	uint32_t version;
	uint32_t numKeys;
	uint32_t numSource;
	uint32_t numName;
	uint8_t tagCount;

	DoPrint(wxT("Version : ")); Print(version = file.ReadUInt32()); DoPrint(wxT("\n"));
	DoPrint(wxT("SaveTime: ")); Print((time_t)file.ReadUInt32()); DoPrint(wxT("\n"));
	DoPrint(wxT("ID      : ")); Print(file.ReadUInt128()); DoPrint(wxT("\n"));
	DoPrint(wxT("numKeys : ")); Print(numKeys = file.ReadUInt32()); DoPrint(wxT("\n"));
	for (uint32_t ik = 0; ik < numKeys; ik++) {
		DoPrint(wxT("\tKeyID    : ")); Print(file.ReadUInt128()); DoPrint(wxT("\n"));
		DoPrint(wxT("\tnumSource: ")); Print(numSource = file.ReadUInt32()); DoPrint(wxT("\n"));
		for (uint32_t is = 0; is < numSource; is++) {
			DoPrint(wxT("\t\tSourceID: ")); Print(file.ReadUInt128()); DoPrint(wxT("\n"));
			DoPrint(wxT("\t\tnumName : ")); Print(numName = file.ReadUInt32()); DoPrint(wxT("\n"));
			for (uint32_t iN = 0; iN < numName; iN++) {
				DoPrint(wxT("\t\t\tLifeTime : ")); Print((time_t)file.ReadUInt32()); DoPrint(wxT("\n"));
				if (version >= 3) {
					uint32_t count;
					DoPrint(wxT("\t\t\tnameCount: ")); Print(count = file.ReadUInt32()); DoPrint(wxT("\n"));
					for (uint32_t i = 0; i < count; i++) {
						DoPrint(wxT("\t\t\t\t{ ")); Print(file.ReadString(true, 2)); DoPrint(wxT(", "));
						Print(file.ReadUInt32()); DoPrint(wxT(" }\n"));
					}
					DoPrint(wxT("\t\t\tipCount : ")); Print(count = file.ReadUInt32()); DoPrint(wxT("\n"));
					for (uint32_t i = 0; i < count; i++) {
						DoPrint(wxT("\t\t\t\t{ ")); Print(CKadIP(file.ReadUInt32())); DoPrint(wxT(", "));
						Print((time_t)file.ReadUInt32());
						DoPrint(wxT(" }\n"));
					}
				}
				DoPrint(wxT("\t\t\ttagCount: ")); Print(tagCount = file.ReadUInt8()); DoPrint(wxT("\n"));
				for (uint32_t it = 0; it < tagCount; it++) {
					DoPrint(wxT("\t\t\t\t"));
					CTag *tag = file.ReadTag();
					Print(*tag);
					delete tag;
					DoPrint(wxT("\n"));
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

	DoPrint(wxT("Version : ")); Print(file.ReadUInt32()); DoPrint(wxT("\n"));
	DoPrint(wxT("SaveTime: ")); Print((time_t)file.ReadUInt32()); DoPrint(wxT("\n"));
	DoPrint(wxT("numKeys : ")); Print(numKeys = file.ReadUInt32()); DoPrint(wxT("\n"));
	for (uint32_t ik = 0; ik < numKeys; ik++) {
		DoPrint(wxT("\tKeyID    : ")); Print(file.ReadUInt128()); DoPrint(wxT("\n"));
		DoPrint(wxT("\tnumSource: ")); Print(numSource = file.ReadUInt32()); DoPrint(wxT("\n"));
		for (uint32_t is = 0; is < numSource; is++) {
			DoPrint(wxT("\t\tSourceID: ")); Print(file.ReadUInt128()); DoPrint(wxT("\n"));
			DoPrint(wxT("\t\tnumName : ")); Print(numName = file.ReadUInt32()); DoPrint(wxT("\n"));
			for (uint32_t iN = 0; iN < numName; iN++) {
				DoPrint(wxT("\t\t\tLifeTime: ")); Print((time_t)file.ReadUInt32()); DoPrint(wxT("\n"));
				DoPrint(wxT("\t\t\ttagCount: ")); Print(tagCount = file.ReadUInt8()); DoPrint(wxT("\n"));
				for (uint32_t it = 0; it < tagCount; it++) {
					DoPrint(wxT("\t\t\t\t"));
					CTag *tag = file.ReadTag();
					Print(*tag);
					delete tag;
					DoPrint(wxT("\n"));
				}
			}
		}
	}
}

void DecodeNodesDat(const CFileDataIO& file)
{
	uint32_t numContacts = file.ReadUInt32();
	uint32_t fileVersion = 0;

	DoPrint(wxT("NumContacts #1: ")); Print(numContacts); DoPrint(wxT("\n"));
	if (numContacts == 0) {
		DoPrint(wxT("FileVersion   : ")); Print(fileVersion = file.ReadUInt32()); DoPrint(wxT("\n"));
		if (fileVersion >= 1) {
			DoPrint(wxT("NumContacts #2: ")); Print(numContacts = file.ReadUInt32()); DoPrint(wxT("\n"));
		}
	}
	for (uint32_t i = 0; i < numContacts; i++) {
		DoPrint(wxT("\t{ "));
		Print(file.ReadUInt128()); DoPrint(wxT(", "));
		Print(CKadIP(file.ReadUInt32())); DoPrint(wxT(", "));
		Print(file.ReadUInt16()); DoPrint(wxT(", "));
		Print(file.ReadUInt16()); DoPrint(wxT(", "));
		Print(file.ReadUInt8());
		if (fileVersion >= 2) {
			DoPrint(wxT(", { "));
			PrintHex(file.ReadUInt32());
			DoPrint(wxT(", "));
			Print(CKadIP(file.ReadUInt32()));
			DoPrint(wxT(" }, "));
			Print(file.ReadUInt8() != 0);
		}
		DoPrint(wxT(" }\n"));
	}
}
