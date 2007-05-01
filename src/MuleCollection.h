/*This file is part of amule-emc, emulecollection support for aMule.
Copyright (C) 2007  Johannes Krampf <wuischke@amule.org>

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

#ifndef __MULECOLLECTION_H__
#define __MULECOLLECTION_H__

#include <string>
#include <vector>

#include <stdint.h> // uint64_t compilation fix for older gcc versions

class CMuleCollection {
	public:
		CMuleCollection();
		~CMuleCollection();
		
		bool		Open( std::string File );
		size_t		GetFileCount();
								// Return values on error:
		std::string	GetEd2kLink( size_t index );	// "Invalid Index"
		std::string	GetFileName( size_t index );	// "Empty String", "Invalid Index"
		uint64_t	GetFileSize( size_t index );	// 0
		std::string	GetFileHash( size_t index );	// "Empty String", "Invalid Index"
	private:
		bool		OpenBinary( std::string File );
		bool		OpenText( std::string File );
		
		bool		AddLink( std::string Link );
		bool		AddFile( std::string	FileName,
					 uint64_t	FileSize,
				 	 std::string	FileHash
				);
		
		bool		IsValidHash( std::string FileHash );
				 		
		typedef struct{
			std::string	FileName;
			uint64_t	FileSize;
			std::string	FileHash; 
		} CollectionFile;
		
		std::vector<CollectionFile> vCollection;
			
};

#endif // __MULECOLLECTION_H__
