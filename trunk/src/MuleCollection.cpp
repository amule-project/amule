/*This file is part of amule-emc, emulecollection support for aMule.
Copyright (C) 2007  Johannes Krampf <wuischke@amule.org>

Other code by:
* Angel Vidal Veiga aka Kry <kry@amule.org>: changed class names
* Marcelo Malheiros <mgmalheiros@gmail.com>: fixed error with FT_FILEHASH
                                             added inital 5 tag/file support



This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "MuleCollection.h"

#include <vector>
#include <string>
#include <sstream>
#include <fstream>

#include <iostream>

CMuleCollection::CMuleCollection()
{
};

CMuleCollection::~CMuleCollection()
{
};
		
bool	CMuleCollection::Open( std::string File )
{
	if(!OpenBinary( File ))
		if(!OpenText( File ))
			return false;
	return true;
};

size_t	CMuleCollection::GetFileCount()
{
	return vCollection.size();
};

std::string	CMuleCollection::GetEd2kLink( size_t index )
{
	if (!(index < GetFileCount()))
		return "Invalid Index!";
	
	std::stringstream retvalue;
	// ed2k://|file|FileName|FileSize|FileHash|/
	retvalue	<< "ed2k://|file|" << GetFileName( index )
			<< "|" << GetFileSize( index )
			<< "|" << GetFileHash( index )
			<< "|/";
		
	return retvalue.str();
};

std::string	CMuleCollection::GetFileName( size_t index )
{
	if (!(index < GetFileCount()))
		return "Invalid Index!";

	std::string retvalue = vCollection[index].FileName;
	if (retvalue == "")
		return "Empty String!";
			
	return retvalue;
};

uint64_t	CMuleCollection::GetFileSize( size_t index )
{
	if (!(index < GetFileCount()))
		return 0;
	
	uint64_t retvalue = vCollection[index].FileSize;
	
	return retvalue;
};

std::string	CMuleCollection::GetFileHash( size_t index )
{
	if (!(index < GetFileCount()))
		return "Invalid Index!";
	std::string retvalue = vCollection[index].FileHash;
	if (retvalue == "")
		return "Empty String!";
			
	return retvalue;
};

bool	CMuleCollection::OpenBinary( std::string File )
{
	std::ifstream infile;
	
	infile.open(File.c_str(), std::ifstream::in|std::ifstream::binary);
		if(!infile.is_open())
			return false;

	uint32_t cVersion;
	infile.read (reinterpret_cast<char *>(&cVersion),sizeof(uint32_t));
		
	if(	(!infile.good()) ||	// Read error or EOF
		(	( cVersion != 0x01 ) &&
			( cVersion != 0x02 )
		)			// Invalid cVersion
		) {
			infile.close();
			return false;
		}
	
	uint32_t hTagCount;
	infile.read (reinterpret_cast<char *>(&hTagCount),sizeof(uint32_t));

		if(	(!infile.good()) ||	// Read error or EOF
			( hTagCount > 3 )	// Catch bad values
		) {
			infile.close();
			return false;
		}
	
	for (size_t hTi = 0; hTi < hTagCount;hTi++) {
		int hTagType = infile.get(); // unused variable
		
		uint16_t hTagFormat;	// hTagFormat == 1 -> FT-value is given
			infile.read (reinterpret_cast<char *>(&hTagFormat),sizeof(uint16_t));
			if( hTagFormat != 0x0001 ) {	// invalid string format
				infile.close();
				return false;
			}			

		int hTag = infile.get();

		if( !infile.good()) {	// Read error or EOF
			infile.close();
			return false;
		}
		switch(hTag)
		{
			case 0x01: {	//FT_FILENAME
				uint16_t hTagStringSize;
				infile.read (reinterpret_cast<char *>(&hTagStringSize),sizeof(uint16_t));
				if( !infile.good() ) { 	// Read error or EOF
					infile.close();
					return false;
				}

				std::vector<char> buffer (hTagStringSize);
				infile.read(&buffer[0], hTagStringSize);
				std::string FileName = (buffer.empty() ? std::string() : std::string (buffer.begin(), buffer.end()));				
				//Value is currently not processed.
			break;
			}
			case 0x31: {	//FT_COLLECTIONAUTHOR
				uint16_t hTagStringSize;
				infile.read (reinterpret_cast<char *>(&hTagStringSize),sizeof(uint16_t));
				if( !infile.good() ) { 	// Read error or EOF
					infile.close();
					return false;
				}

				std::vector<char> buffer (hTagStringSize);
				infile.read(&buffer[0], hTagStringSize);
				std::string CollectionAuthor = (buffer.empty() ? std::string() : std::string (buffer.begin(), buffer.end()));
				//Value is currently not processed.
			break;
			}
			case 0x32: {	//FT_COLLECTIONAUTHORKEY
				uint32_t hTagBlobSize;
				infile.read (reinterpret_cast<char *>(&hTagBlobSize),sizeof(uint32_t));
				if( !infile.good() ) { 	// Read error or EOF
					infile.close();
					return false;
				}

				std::vector<char> CollectionAuthorKey (hTagBlobSize);
				infile.read(&CollectionAuthorKey[0], hTagBlobSize);
				//Value is currently not processed.			

  			break;
			}
  			default:	//UNDEFINED TAG
  				if( !infile.good() ) {	// Read error or EOF
					infile.close();
					return false;
				}
			break;
		}
	}

	uint32_t cFileCount;
	infile.read (reinterpret_cast<char *>(&cFileCount),sizeof(uint32_t));
	
	/*
	softlimit is set to 1024 to avoid problems with big uint32_t values
	I don't believe anyone would want to use an emulecollection file
	to store more than 1024 files, but just raise below value in case
	you know someone who does.
	*/

	if(	(!infile.good()) ||	// Read error or EOF
		( cFileCount > 1024 )	// Catch bad values
	) {
		infile.close();
		return false;
	}
	
	for(size_t cFi = 0; cFi < cFileCount; cFi++) {
		uint32_t fTagCount;
		infile.read (reinterpret_cast<char *>(&fTagCount),sizeof(uint32_t));

		if(	(!infile.good()) ||	// Read error or EOF
			( fTagCount > 5 )	// Catch bad values
		) {
			infile.close();
			return false;
		}
		
		std::string FileHash = std::string(32, '0');
		uint64_t FileSize;			
		std::string FileName;
		std::string FileComment; // unused variable
		uint8_t FileRating; // unused variable
				
		for(size_t fTi = 0; fTi < fTagCount; fTi++) {

			int fTagType = infile.get();

			if( !infile.good()) {	// Read error or EOF
				infile.close();
				return false;
			}

			int fTag = infile.get();

			if( !infile.good()) {	// Read error or EOF
				infile.close();
				return false;
			}
			
			switch( fTag )
			{
				case 0x28: {	//FT_FILEHASH
					std::vector<char> bFileHash (16);
					infile.read(&bFileHash[0], 16);
										
					std::string hex = "0123456789abcdef";
					for(int pos = 0; pos < 16; pos++){
						FileHash[pos*2] = hex[((bFileHash[pos] >> 4) & 0xF)];
						FileHash[(pos*2) + 1] = hex[(bFileHash[pos]) & 0x0F];
					}
				break;
				}
				case 0x02: {	//FT_FILESIZE
					switch(fTagType) {
						case 0x83: {
							uint32_t fsUint32_t;
							infile.read (reinterpret_cast<char *>(&fsUint32_t),sizeof(uint32_t));
							FileSize = fsUint32_t;
						break;
						}
						case 0x88: {
							uint16_t fsUint16_t;
							infile.read (reinterpret_cast<char *>(&fsUint16_t),sizeof(uint16_t));
							FileSize = fsUint16_t;
						break;
						}
						case 0x89: {
							uint8_t fsUint8_t = infile.get();
							FileSize = fsUint8_t;
						break;
						}
						case 0x8b: {
							infile.read (reinterpret_cast<char *>(&FileSize),sizeof(uint64_t));
						break;
						}
						default:	// Invalid file structure
							infile.close();
							return false;
						break;
					}
				break;
				}
				case 0x01: {	//FT_FILENAME
					if (	( fTagType >= 0x91 ) &&
						( fTagType <= 0xa0)
					) {
						std::vector<char> buffer ( (fTagType - 0x90) );
						infile.read(&buffer[0], (fTagType - 0x90) );
						FileName = (buffer.empty() ? std::string() : std::string (buffer.begin(), buffer.end()));
					}else if (fTagType == 0x82) { //TAGTYPE_STR
						uint16_t fTagStringSize;
						infile.read (reinterpret_cast<char *>(&fTagStringSize),sizeof(uint16_t));

						std::vector<char> buffer (fTagStringSize);
						infile.read(&buffer[0], fTagStringSize);
						FileName = (buffer.empty() ? std::string() : std::string (buffer.begin(), buffer.end()));
					}else {
						infile.close();
						return false;
					}
				break;
				}
				case 0xF6: {	//FT_FILECOMMENT
					if (fTagType == 0x82) { //TAGTYPE_STR
						uint16_t fTagStringSize;
						infile.read (reinterpret_cast<char *>(&fTagStringSize),sizeof(uint16_t));

						std::vector<char> buffer (fTagStringSize);
						infile.read(&buffer[0], fTagStringSize);
						FileComment = (buffer.empty() ? std::string() : std::string (buffer.begin(), buffer.end()));
					}else {
						infile.close();
						return false;
					}
				break;
				}
				case 0xF7: {	//FT_FILERATING
					if (fTagType == 0x89) { //TAGTYPE_UINT8
							uint8_t FileRating = infile.get();
							
					}else {
						infile.close();
						return false;
					}
				break;
				}
				default:	//UNDEFINED TAG
					infile.close();
					return false;
				break;
			}
			if( !infile.good() ) {	// Read error or EOF
				infile.close();
				return false;
			}
		}

		AddFile( FileName, FileSize, FileHash );
	}
		
	infile.close();

	return true;
};

bool	CMuleCollection::OpenText( std::string File )
{
	int		numLinks = 0;
	std::string	line;
	std::ifstream infile;
	
	infile.open(File.c_str(), std::ifstream::in);
		if(!infile.is_open())
			return false;
  	
  	while (getline(infile, line))
		if(AddLink(line))
			numLinks++;
	
	infile.close();
	
	if(numLinks == 0)
		return false;
		
	return true;
};

bool	CMuleCollection::AddLink( std::string Link )
{
	// 12345678901234       56       7 + 32 + 89 = 19+32=51
	// ed2k://|file|FileName|FileSize|FileHash|/
	if (	(Link.size() < 51)			||
		(Link.substr(0,13) != "ed2k://|file|")	||
		(Link.substr(Link.size()-2) != "|/")
	)
		return false;
	
	size_t iName = Link.find("|",13);
		if(iName == std::string::npos)
			return false;
	std::string FileName = Link.substr(13,iName-13);

	size_t iSize = Link.find("|",iName+1);
		if(iSize == std::string::npos)
			return false;
	std::stringstream sFileSize;
		sFileSize << Link.substr(iName+1,iSize-iName-1);
	uint64_t FileSize;
		if((sFileSize >> std::dec >> FileSize).fail())
			return false;
		
	size_t iHash = Link.find("|",iSize+1);
			if(iHash == std::string::npos)
			return false;
	std::string FileHash = Link.substr(iSize+1,32);

	return AddFile(FileName, FileSize, FileHash);;
};

bool	CMuleCollection::AddFile( std::string	FileName,
			     uint64_t		FileSize,
			     std::string	FileHash
				)
{
	if (	(FileName == "")		||
		(FileSize == 0)			||
		(FileSize > 0xffffffffLL)	||
		(!IsValidHash( FileHash) )
	)
		return false;
		
	vCollection.push_back( CollectionFile() );
	int index = vCollection.size()-1;
	vCollection[index].FileName = FileName;
	vCollection[index].FileSize = FileSize;
	vCollection[index].FileHash = FileHash;
	return true;
};

bool	CMuleCollection::IsValidHash( std::string FileHash )
{
	if (	(FileHash.size() != 32)	||
		(FileHash == "")
	)
		return false;
	
	//FileHash needs to be a valid MD4Hash
	std::string hex = "0123456789abcdefABCDEF";
	for(size_t i = 0; i < FileHash.size();i++)
	{
		if(hex.find(FileHash[i]) == std::string::npos)
			return false;
	}
	return true;	
};
