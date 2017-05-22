//
// This file is part of the aMule Project.
//
// Copyright (c) 2007-2011 Johannes Krampf ( wuischke@amule.org )
//
// Other code by:
//
// Angel Vidal Veiga aka Kry <kry@amule.org>
// * changed class names
//
// Marcelo Malheiros <mgmalheiros@gmail.com>
// * fixed error with FT_FILEHASH
// * added inital 5 tag/file support
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


#include "MuleCollection.h"

#include <fstream>
#include <iostream>
#include <sstream>


bool CMuleCollection::Open(const std::string &File)
{
	return OpenBinary(File) || OpenText(File);
}


template <typename intType>
intType CMuleCollection::ReadInt(std::ifstream& infile)
{
	intType integer = 0;
	infile.read(reinterpret_cast<char *>(&integer),sizeof(intType));
	// TODO: byte-sex
	return integer;
}

std::string CMuleCollection::ReadString(std::ifstream& infile, int TagType = 0x02)
{
	if (TagType >= 0x11 && TagType <= 0x20) {
		std::vector<char> buffer(TagType - 0x10);
		infile.read(&buffer[0], TagType - 0x10);
		return buffer.empty() ?
			std::string() :
			std::string (buffer.begin(), buffer.end());
	}
	if (TagType == 0x02) {
		uint16_t TagStringSize = ReadInt<uint16_t>(infile);
		std::vector<char> buffer (TagStringSize);
		infile.read(&buffer[0], TagStringSize);
		return buffer.empty() ?
			std::string() :
			std::string (buffer.begin(), buffer.end());
	}
	return std::string();
}

bool CMuleCollection::OpenBinary(const std::string &File)
{
	std::ifstream infile;

	infile.open(File.c_str(), std::ifstream::in|std::ifstream::binary);
	if(!infile.is_open()) {
		return false;
	}

	uint32_t cVersion = ReadInt<uint32_t>(infile);

	if (!infile.good() ||
	    ( cVersion != 0x01 && cVersion != 0x02)) {
			infile.close();
			return false;
	}

	uint32_t hTagCount = ReadInt<uint32_t>(infile);
	if (!infile.good() ||
	    hTagCount > 3) {
		infile.close();
		return false;
	}

	for (size_t hTi = 0; hTi < hTagCount;hTi++) {
		 int hTagType = infile.get();

		// hTagFormat == 1 -> FT-value is given
		uint16_t hTagFormat = ReadInt<uint16_t>(infile);
		if (hTagFormat != 0x0001) {
			infile.close();
			return false;
		}

		int hTag = infile.get();
		if (!infile.good()) {
			infile.close();
			return false;
		}
		switch (hTag) {
		// FT_FILENAME
		case 0x01: {
			/*std::string fileName =*/ ReadString(infile, hTagType);
			break;
		}
		// FT_COLLECTIONAUTHOR
		case 0x31: {
			/*std::string CollectionAuthor =*/ ReadString(infile, hTagType);
			break;
		}
		// FT_COLLECTIONAUTHORKEY
		case 0x32: {
			uint32_t hTagBlobSize = ReadInt<uint32_t>(infile);
			if (!infile.good()) {
				infile.close();
				return false;
			}
			std::vector<char> CollectionAuthorKey(hTagBlobSize);
			infile.read(&CollectionAuthorKey[0], hTagBlobSize);
			break;
		}
		// UNDEFINED TAG
		default:
			if (!infile.good()) {
				infile.close();
				return false;
			}
			break;
		}
	}

	uint32_t cFileCount = ReadInt<uint32_t>(infile);

	/*
	softlimit is set to 1024 to avoid problems with big uint32_t values
	I don't believe anyone would want to use an emulecollection file
	to store more than 1024 files, but just raise below value in case
	you know someone who does.
	*/

	if(!infile.good() ||
	   cFileCount > 1024) {
		infile.close();
		return false;
	}

	vCollection.reserve(cFileCount);

	for (size_t cFi = 0; cFi < cFileCount; ++cFi) {
		uint32_t fTagCount = ReadInt<uint32_t>(infile);

		if (!infile.good() ||
		    fTagCount > 6) {
			infile.close();
			return false;
		}

		std::string fileHash = std::string(32, '0');
		uint64_t fileSize = 0;
		std::string fileName;
		std::string rootHash;
		for(size_t fTi = 0; fTi < fTagCount; ++fTi) {
			int fTagType = infile.get();
			if (!infile.good()) {
				infile.close();
				return false;
			}

			int fTag = infile.get();
			if (!infile.good()) {
				infile.close();
				return false;
			}

			switch (fTag) {
			// FT_FILEHASH
			case 0x28: {
				std::vector<char> bFileHash(16);
				infile.read(&bFileHash[0], 16);
				std::string hex = "0123456789abcdef";
				for (int pos = 0; pos < 16; pos++) {
					fileHash[pos*2] = hex[((bFileHash[pos] >> 4) & 0xF)];
					fileHash[(pos*2) + 1] = hex[(bFileHash[pos]) & 0x0F];
				}
				break;
			}
			// FT_AICH_FILEHASH
			case 0x27: {
				rootHash = ReadString(infile, 0x02);
				break;
			}
			// FT_FILESIZE
			case 0x02: {
				switch(fTagType) {
				case 0x83: {
					fileSize = ReadInt<uint32_t>(infile);
					break;
				}
				case 0x88: {
					fileSize = ReadInt<uint16_t>(infile);
					break;
				}
				case 0x89: {
					fileSize = infile.get();
					break;
				}
				case 0x8b: {
					fileSize = ReadInt<uint64_t>(infile);
					break;
				}
				default: // Invalid file structure
					infile.close();
					return false;
					break;
				}
				break;
			}
			// FT_FILENAME
			case 0x01: {
				fileName = ReadString(infile, fTagType^0x80);
				break;
			}
			// FT_FILECOMMENT
			case 0xF6: {
				/* std::string FileComment =*/ ReadString(infile, fTagType^0x80);
				break;
			}
			// FT_FILERATING
			case 0xF7: {
				if (fTagType == 0x89) { // TAGTYPE_UINT8
					// uint8_t FileRating =
						infile.get();

				} else {
					infile.close();
					return false;
				}
				break;
			}
			// UNDEFINED TAG
			default:
				infile.close();
				return false;
				break;
			}
			if( !infile.good() ) {
				infile.close();
				return false;
			}
		}

		if (!fileName.empty() && fileSize > 0) {
			std::stringstream link;
			// ed2k://|file|fileName|fileSize|fileHash|/
			link	<< "ed2k://|file|" << fileName
				<< "|" << fileSize
				<< "|" << fileHash;
			if (!rootHash.empty()) {
				link << "|h=" << rootHash;
			}
			link << "|/";
			vCollection.push_back(link.str());
		}
	}
	infile.close();

	return true;
}


bool CMuleCollection::OpenText(const std::string &File)
{
	std::string line;
	std::ifstream infile;

	infile.open(File.c_str(), std::ifstream::in|std::ifstream::binary);
	if (!infile.is_open()) {
		return false;
	}

	while (getline(infile, line, (char)10 /* LF */)) {
		size_t last = line.size()-1;
		if ((1 < last) && ((char)13 /* CR */ == line.at(last))) {
			line.erase(last);
		}
		if (line.size() > 50 &&
		    line.substr(0, 13) == "ed2k://|file|" &&
		    line.substr(line.size() - 2) == "|/") {
			vCollection.push_back(line);
		}
	}
	infile.close();

	return !vCollection.empty();
}
