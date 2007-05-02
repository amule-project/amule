//
// This file is part of the aMule Project.
//
// Copyright (C) 2007 Johannes Krampf <wuischke@amule.org>
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
#include <string>
#include <vector>


CollectionFile::CollectionFile(
	const std::string &fileName,
	uint64_t fileSize,
	const std::string &fileHash)
:
m_fileName(fileName),
m_fileSize(fileSize),
m_fileHash(fileHash)
{
}


CMuleCollection::CMuleCollection()
:
vCollection(0)
{
}


CMuleCollection::~CMuleCollection()
{
}


bool CMuleCollection::Open(const std::string &File)
{
	return OpenBinary(File) || OpenText(File);
}


std::string CMuleCollection::GetEd2kLink(size_t index) const
{
	if (index >= GetFileCount()) {
		return "Invalid Index!";
	}
	
	std::stringstream retvalue;
	// ed2k://|file|fileName|fileSize|fileHash|/
	retvalue
		<< "ed2k://|file|" << GetFileName(index)
		<< "|" << GetFileSize(index)
		<< "|" << GetFileHash(index)
		<< "|/";
		
	return retvalue.str();
}


std::string CMuleCollection::GetFileName(size_t index) const
{
	if (index >= GetFileCount()) {
		return "Invalid Index!";
	}

	std::string retvalue = vCollection[index].m_fileName;
	if (retvalue.empty()) {
		return "Empty String!";
	}
			
	return retvalue;
}


uint64_t CMuleCollection::GetFileSize(size_t index) const
{
	if (index >= GetFileCount()) {
		return 0;
	}
	
	return vCollection[index].m_fileSize;
}


std::string CMuleCollection::GetFileHash(size_t index) const
{
	if (index >= GetFileCount()) {
		return "Invalid Index!";
	}
	std::string retvalue = vCollection[index].m_fileHash;
	if (retvalue.empty()) {
		return "Empty String!";
	}
			
	return retvalue;
}


bool CMuleCollection::OpenBinary(const std::string &File)
{
	std::ifstream infile;
	
	infile.open(File.c_str(), std::ifstream::in|std::ifstream::binary);
	if(!infile.is_open()) {
		return false;
	}

	uint32_t cVersion;
	infile.read(reinterpret_cast<char *>(&cVersion),sizeof(uint32_t));
		
	if (!infile.good() || // Read error or EOF
	    ( cVersion != 0x01 && cVersion != 0x02)) { // Invalid cVersion
			infile.close();
			return false;
	}
	
	uint32_t hTagCount;
	infile.read(reinterpret_cast<char *>(&hTagCount),sizeof(uint32_t));
	if (!infile.good() ||	// Read error or EOF
	    hTagCount > 3) {	// Catch bad values
		infile.close();
		return false;
	}
	
	for (size_t hTi = 0; hTi < hTagCount;hTi++) {
		// int hTagType =
			infile.get(); // unused variable
		
		uint16_t hTagFormat;	// hTagFormat == 1 -> FT-value is given
		infile.read (reinterpret_cast<char *>(&hTagFormat),sizeof(uint16_t));
		if (hTagFormat != 0x0001) {	// invalid string format
			infile.close();
			return false;
		}			

		int hTag = infile.get();
		if (!infile.good()) { // Read error or EOF
			infile.close();
			return false;
		}
		switch (hTag) {
		// FT_FILENAME
		case 0x01: {
			uint16_t hTagStringSize;
			infile.read (reinterpret_cast<char *>(&hTagStringSize),sizeof(uint16_t));
			if( !infile.good() ) { 	// Read error or EOF
				infile.close();
				return false;
			}
			std::vector<char> buffer(hTagStringSize);
			infile.read(&buffer[0], hTagStringSize);
			std::string fileName = buffer.empty() ?
				std::string() :
				std::string (buffer.begin(), buffer.end());
			// Value is currently not processed.
			break;
		}
		// FT_COLLECTIONAUTHOR
		case 0x31: {
			uint16_t hTagStringSize;
			infile.read (reinterpret_cast<char *>(&hTagStringSize),sizeof(uint16_t));
			if (!infile.good()) { // Read error or EOF
				infile.close();
				return false;
			}
			std::vector<char> buffer(hTagStringSize);
			infile.read(&buffer[0], hTagStringSize);
			std::string CollectionAuthor = buffer.empty() ?
				std::string() :
				std::string (buffer.begin(), buffer.end());
			// Value is currently not processed.
			break;
		}
		// FT_COLLECTIONAUTHORKEY
		case 0x32: {
			uint32_t hTagBlobSize;
			infile.read (reinterpret_cast<char *>(&hTagBlobSize),sizeof(uint32_t));
			if (!infile.good()) { // Read error or EOF
				infile.close();
				return false;
			}
			std::vector<char> CollectionAuthorKey(hTagBlobSize);
			infile.read(&CollectionAuthorKey[0], hTagBlobSize);
			// Value is currently not processed.			
			break;
		}
		// UNDEFINED TAG
		default:
			if (!infile.good()) { // Read error or EOF
				infile.close();
				return false;
			}
			break;
		}
	}

	uint32_t cFileCount;
	infile.read(reinterpret_cast<char *>(&cFileCount),sizeof(uint32_t));
	
	/*
	softlimit is set to 1024 to avoid problems with big uint32_t values
	I don't believe anyone would want to use an emulecollection file
	to store more than 1024 files, but just raise below value in case
	you know someone who does.
	*/

	if(!infile.good() ||	// Read error or EOF
	   cFileCount > 1024) { // Catch bad values
		infile.close();
		return false;
	}
	
	for (size_t cFi = 0; cFi < cFileCount; ++cFi) {
		uint32_t fTagCount;
		infile.read(reinterpret_cast<char *>(&fTagCount),sizeof(uint32_t));

		if (!infile.good() ||	// Read error or EOF
		    fTagCount > 5) { // Catch bad values
			infile.close();
			return false;
		}
		
		std::string fileHash = std::string(32, '0');
		uint64_t fileSize;			
		std::string fileName;
		std::string FileComment; // unused variable
		for(size_t fTi = 0; fTi < fTagCount; ++fTi) {
			int fTagType = infile.get();
			if (!infile.good()) { // Read error or EOF
				infile.close();
				return false;
			}

			int fTag = infile.get();
			if (!infile.good()) { // Read error or EOF
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
			// FT_FILESIZE
			case 0x02: {
				switch(fTagType) {
				case 0x83: {
					uint32_t fsUint32_t;
					infile.read (reinterpret_cast<char *>(&fsUint32_t), sizeof(uint32_t));
					fileSize = fsUint32_t;
					break;
				}
				case 0x88: {
					uint16_t fsUint16_t;
					infile.read (reinterpret_cast<char *>(&fsUint16_t), sizeof(uint16_t));
					fileSize = fsUint16_t;
					break;
				}
				case 0x89: {
					uint8_t fsUint8_t = infile.get();
					fileSize = fsUint8_t;
					break;
				}
				case 0x8b: {
					infile.read(reinterpret_cast<char *>(&fileSize), sizeof(uint64_t));
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
				if (fTagType >= 0x91 && fTagType <= 0xa0) {
					std::vector<char> buffer(fTagType - 0x90);
					infile.read(&buffer[0], fTagType - 0x90);
					fileName = buffer.empty() ?
						std::string() :
						std::string (buffer.begin(), buffer.end());
				} else if (fTagType == 0x82) { // TAGTYPE_STR
					uint16_t fTagStringSize;
					infile.read(reinterpret_cast<char *>(&fTagStringSize),sizeof(uint16_t));
					std::vector<char> buffer (fTagStringSize);
					infile.read(&buffer[0], fTagStringSize);
					fileName = buffer.empty() ?
						std::string() :
						std::string (buffer.begin(), buffer.end());
				} else {
					infile.close();
					return false;
				}
				break;
			}
			// FT_FILECOMMENT
			case 0xF6: {
				if (fTagType == 0x82) { // TAGTYPE_STR
					uint16_t fTagStringSize;
					infile.read (reinterpret_cast<char *>(&fTagStringSize),sizeof(uint16_t));
					std::vector<char> buffer(fTagStringSize);
					infile.read(&buffer[0], fTagStringSize);
					FileComment = buffer.empty() ?
						std::string() :
						std::string (buffer.begin(), buffer.end());
				} else {
					infile.close();
					return false;
				}
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
			if( !infile.good() ) {	// Read error or EOF
				infile.close();
				return false;
			}
		}
		AddFile(fileName, fileSize, fileHash);
	}
	infile.close();

	return true;
}


bool CMuleCollection::OpenText(const std::string &File)
{
	int numLinks = 0;
	std::string line;
	std::ifstream infile;
	
	infile.open(File.c_str(), std::ifstream::in);
	if (!infile.is_open()) {
		return false;
	}
  	
  	while (getline(infile, line)) {
		if(AddLink(line)) {
			numLinks++;
		}
	}
	infile.close();
	
	if(numLinks == 0) {
		return false;
	}
		
	return true;
}


bool CMuleCollection::AddLink(const std::string &Link)
{
	// 12345678901234       56       7 + 32 + 89 = 19+32=51
	// ed2k://|file|fileName|fileSize|fileHash|/
	if (Link.size() < 51 ||
	    Link.substr(0,13) != "ed2k://|file|" ||
	    Link.substr(Link.size()-2) != "|/") {
		return false;
	}
	
	size_t iName = Link.find("|",13);
	if (iName == std::string::npos) {
		return false;
	}
	std::string fileName = Link.substr(13,iName-13);

	size_t iSize = Link.find("|",iName+1);
	if (iSize == std::string::npos) {
		return false;
	}
	std::stringstream sFileSize;
	sFileSize << Link.substr(iName+1,iSize-iName-1);
	uint64_t fileSize;
	if ((sFileSize >> std::dec >> fileSize).fail()) {
		return false;
	}

	size_t iHash = Link.find("|",iSize+1);
	if (iHash == std::string::npos) {
		return false;
	}
	std::string fileHash = Link.substr(iSize+1,32);

	return AddFile(fileName, fileSize, fileHash);
}


bool CMuleCollection::AddFile(
	const std::string &fileName,
	uint64_t fileSize,
	const std::string &fileHash)
{
	if (fileName == "" ||
	    fileSize == 0 ||
	    fileSize > 0xffffffffLL ||
	    !IsValidHash(fileHash)) {
		return false;
	}
		
	vCollection.push_back(
		CollectionFile(fileName, fileSize, fileHash));
	return true;
}


bool CMuleCollection::IsValidHash(const std::string &fileHash)
{
	if (fileHash.size() != 32 || fileHash == "") {
		return false;
	}
	
	// fileHash needs to be a valid MD4Hash
	std::string hex = "0123456789abcdefABCDEF";
	for(size_t i = 0; i < fileHash.size(); ++i) {
		if (hex.find(fileHash[i]) == std::string::npos) {
			return false;
		}
	}
	return true;	
}

