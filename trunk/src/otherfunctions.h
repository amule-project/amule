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


/*
	Functions in this file. Please update this list if you change something.

	
	Hash functions:
		From Gnucleus project [found by Tarod]:
		These functions deal with the base16 hashes that aMule uses.
		
		CString EncodeBase16( buffer, bufLen )
			Converts 'buffer' with length 'bufLen' to a CString
				
		void DecodeBase16( base16Buffer,  base16BufLen,  buffer )
			Converts the string 'base16Buffer' with length 'base16BufLen' 
			to a hash in 'buffer'.

		int md4cmp( hash1,  hash2 )
			Compares two hashes. Returns 0 on equality and !0 otherwise.
			
		void md4clr( hash )
			Clears a hash
			
		void md4cpy( dst,  src )
			Copies the hash 'src' to the hash 'dst'
				

	Cast function:
		These functions make a specific type of data usable for displaying.
	
		CString CastItoXBytes( count )
			Converts the number of bytes to human readable form.
			
		CString CastItoIShort( number )
			Converts the possibly large number 'count', to human readable form.
			
		CString CastSecondsToHM( seconds)
			Converts the number seconds to human readable time.
		
		
	Informational functions:
		These functions return various information.
		
		wxString GetFiletypeByName( infile )
			Identify the filetype of a file, by looking at the extension.
			
		int GetMaxConnections()
			Returns the max number of connections the current OS can handle.
			Currently anything but windows will return the default value -1;

		CString GetCatTitle( catid )
			Returns the string assosiated with a category value.

		bool IsGoodIP( nIP )
			Checks an ip to see if it is valid, depending on current preferences.
		
		CString GetRateString( rate )
			Returns the string assosiated with a file-rating value.


	String manipulation:
		These functions perform various tasks on strings
		
		wxString URLEncode( sIn )
			Makes sIn suitable for inclusion in an URL, by escaping all chars
			that could cause trouble.
			
		CString MakeStringEscaped( in )
			Replaces "&" with "&&" in 'in' for use with text-labels
			
		void MakeFoldername(char* path);


	File manipulation:
		Functions that act on files.
		
		bool BackupFile( filename,  appendix )
			Makes a backup of a file, by copying the original file to filename + appendix

		bool FS_wxCopyFile( file1, file2, overwrite )
			This function is a replacement for wxCopyFile, with the added feature,
			that chmoding of the target file can be disabled. The reason for this
			is, that FAT partitons under linux generate warnings when chmoding.
			
		bool FS_wxRenameFile( file1,  file2 );
			Same as above, but renames rather than copies.


	Other functions:
	
	void HexDump( buffer,  buflen )
	
	bool CheckShowItemInGivenCat( file, inCategory )
	
	int wxCMPFUNC_CONV Uint16CompareValues( first, second );
		Compares first and second. For uint16 arrays sorting.

*/



#ifndef OTHERFUNCTIONS_H
#define OTHERFUNCTIONS_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/string.h>		// Needed for wxString

#include "types.h"		// Needed for uint16, uint32 and uint64
#include "filefn.h"		// Needed for wxCopyFile_fat32
#include "endianfix.h"

class CString;
class CPartFile;


/* Hash functions */
// From Gnucleus project [found by Tarod]
CString EncodeBase16(const unsigned char* buffer, unsigned int bufLen);
void DecodeBase16(const char *base16Buffer, unsigned int base16BufLen, unsigned char *buffer);


/* Cast function */
CString CastItoXBytes(uint64 count);
CString CastItoIShort(uint64 number);
CString CastSecondsToHM(sint32 seconds);
CString GetRateString(uint16 rate);


/* Informational functions */
wxString GetFiletypeByName(wxString infile);
int GetMaxConnections();
CString GetCatTitle(int catid);
bool IsGoodIP(uint32 nIP);


/* String manipulation */
wxString URLEncode(wxString sIn);
CString MakeStringEscaped(CString in);
void MakeFoldername(char* path);


/* File manipulation */
bool BackupFile(const wxString& filename, const wxString& appendix);
bool FS_wxCopyFile(const wxString& file1, const wxString& file2,bool overwrite = TRUE);
bool FS_wxRenameFile(const wxString& file1, const wxString& file2);


/* Other */
void HexDump(const void *buffer, unsigned long buflen);
bool CheckShowItemInGivenCat(CPartFile* file,int inCategory);
// For uint16 arrays sorting
int wxCMPFUNC_CONV Uint16CompareValues(uint16* first, uint16* second);


#define ARRSIZE(x) (int) (sizeof(x)/sizeof(x[0]))
#define itemsof(x) (sizeof(x)/sizeof(x[0]))
#define ELEMENT_COUNT(X) (sizeof(X) / sizeof(X[0]))


enum EED2KFileType {
	ED2KFT_ANY,
	ED2KFT_AUDIO,
	ED2KFT_VIDEO,
	ED2KFT_IMAGE,
	ED2KFT_PROGRAM,
	ED2KFT_DOCUMENT,
	ED2KFT_ARCHIVE,
	ED2KFT_CDIMAGE
};


const uint8 PMT_UNKNOWN=0;
const uint8 PMT_DEFAULTOLD=1;
const uint8 PMT_SPLITTED=2;
const uint8 PMT_NEWOLD=3;


// Duplicates a string
inline char* nstrdup(const char* src)
{
	int len = (src ? strlen(src) : 0) + 1;
	char *res = new char[len];
	if ( src ) strcpy(res, src);
	res[len-1] = 0;
	return res;
}


// md4cmp -- replacement for memcmp(hash1,hash2,16)
// Like 'memcmp' this function returns 0, if hash1==hash2, and !0, if hash1!=hash2.
// NOTE: Do *NOT* use that function for determining if hash1<hash2 or hash1>hash2.
inline int md4cmp(const void* hash1, const void* hash2) {
	return !(((uint32*)hash1)[0] == ((uint32*)hash2)[0] &&
		     ((uint32*)hash1)[1] == ((uint32*)hash2)[1] &&
		     ((uint32*)hash1)[2] == ((uint32*)hash2)[2] &&
		     ((uint32*)hash1)[3] == ((uint32*)hash2)[3]);
}


// md4clr -- replacement for memset(hash,0,16)
inline void md4clr(const void* hash) {
	((uint32*)hash)[0] = ((uint32*)hash)[1] = ((uint32*)hash)[2] = ((uint32*)hash)[3] = 0;
}


// md4cpy -- replacement for memcpy(dst,src,16)
inline void md4cpy(const void* dst, const void* src) {
	((uint32*)dst)[0] = ((uint32*)src)[0];
	((uint32*)dst)[1] = ((uint32*)src)[1];
	((uint32*)dst)[2] = ((uint32*)src)[2];
	((uint32*)dst)[3] = ((uint32*)src)[3];
}

// DumpMem ... Dumps mem ;)
void DumpMem(const void* where, uint32 size);

#ifdef __FreeBSD__
inline long long atoll(char const* s) {
  return (long long) strtoll(s, (char **)NULL, 10);
}
#endif /* __FreeBSD__ */

/*         BSD based OS support for gethostname_r       */
#if 	defined(__FreeBSD__) || defined(__OpenBSD__) \
	|| defined(__NetBSD__)  || defined(__DARWIN__)

#include <wx/thread.h>		// Needed for wxMutex

static wxMutex s_mutexProtectingGetHostByName;

struct hostent* gethostbyname_r(
	char const* name,
	struct hostent* result,
	char* buffer,
	int buflen,
	int* h_errnop
);

#endif /* BSD supoprt */


#endif // OTHERFUNCTIONS_H
