//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
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

#ifndef OTHERFUNCTIONS_H
#define OTHERFUNCTIONS_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/string.h>		// Needed for wxString

#include "types.h"		// Needed for uint16, uint32 and uint64
#include "filefn.h"		// Needed for wxCopyFile_fat32
#include "endianfix.h"

class CString;
class CPartFile;

#define ROUND(x) (floor((float)x+0.5f))
#define ARRSIZE(x)  (int) (sizeof(x)/sizeof(x[0]))
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

#define _WINVER_NT4_	0x0004
#define _WINVER_95_	0x0004
#define _WINVER_98_	0x0A04
#define _WINVER_ME_	0x5A04
#define _WINVER_2K_	0x0005
#define _WINVER_XP_	0x0105

CString CastItoXBytes(uint64 count);
CString CastItoIShort(uint64 number);
CString CastSecondsToHM(sint32 seconds); //<<--9/21/02
CString LeadingZero(uint32 units);
void ShellOpenFile(CString name); //<<--9/21/02
void MakeFoldername(char* path);
/* Creteil BEGIN */
CString DbgGetFileInfo(const unsigned char* hash);
/* Creteil END */

// Make a malloc'd decoded strnig from an URL encoded string (with escaped spaces '%20' and  the like
void URLDecode(wxString& result, const char* buff);

wxString URLEncode(wxString sIn);
inline BYTE toHex(const BYTE &x);
wxString GetFiletypeByName(wxString infile);

// Barry - Allow forced update without prompt
bool Ask4RegFix(bool checkOnly, bool dontAsk = false);

// Barry - Store previous values
void BackupReg(void);

// Barry - Restore previous values
void RevertReg(void);
int GetMaxConnections();
CString MakeStringEscaped(CString in);
//void RunURL(CAbstractFile* file,CString urlpattern);
void HexDump(const void *buffer, unsigned long buflen);

bool BackupFile(const wxString& filename, const wxString& appendix);
bool FS_wxCopyFile(const wxString& file1, const wxString& file2,bool overwrite = TRUE);
bool FS_wxRenameFile(const wxString& file1, const wxString& file2);

CString GetCatTitle(int catid);
bool CheckShowItemInGivenCat(CPartFile* file,int inCategory);

WORD	DetectWinVersion();
//uint64	GetFreeDiskSpaceX(LPCTSTR pDirectory);
//uint64	GetFreeDiskSpaceX(char* pDirectory);
//_int64	GetFreeDiskSpaceX(PCHAR pDirectory);
//For Rate File
CString GetRateString(uint16 rate);
//void	UpdateURLMenu(CMenu &menu, int &counter);

// From Gnucleus project [found by Tarod]
CString EncodeBase32(const unsigned char* buffer, unsigned int bufLen);
CString EncodeBase16(const unsigned char* buffer, unsigned int bufLen);
int	DecodeLengthBase16(int base16Length);
void	DecodeBase16(const char *base16Buffer, unsigned int base16BufLen, unsigned char *buffer);

__inline char* nstrdup(const char* src)
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
__inline int md4cmp(const void* hash1, const void* hash2) {
	return !(((uint32*)hash1)[0] == ((uint32*)hash2)[0] &&
		     ((uint32*)hash1)[1] == ((uint32*)hash2)[1] &&
		     ((uint32*)hash1)[2] == ((uint32*)hash2)[2] &&
		     ((uint32*)hash1)[3] == ((uint32*)hash2)[3]);
}

// md4clr -- replacement for memset(hash,0,16)
__inline void md4clr(const void* hash) {
	((uint32*)hash)[0] = ((uint32*)hash)[1] = ((uint32*)hash)[2] = ((uint32*)hash)[3] = 0;
}

// md4cpy -- replacement for memcpy(dst,src,16)
__inline void md4cpy(const void* dst, const void* src) {
	((uint32*)dst)[0] = ((uint32*)src)[0];
	((uint32*)dst)[1] = ((uint32*)src)[1];
	((uint32*)dst)[2] = ((uint32*)src)[2];
	((uint32*)dst)[3] = ((uint32*)src)[3];
}

#define	MAX_HASHSTR_SIZE (16*2+1)
CString md4str(const unsigned char* hash);
void md4str(const unsigned char* hash, char* pszHash);

/* GetTickCount() was moved to "GetTickCount.h" */

bool IsGoodIP(uint32 nIP);
bool IsGoodIPPort(uint32 nIP, uint16 nPort);

// For uint16 arrays sorting

int wxCMPFUNC_CONV Uint16CompareValues(uint16* first, uint16* second);

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
