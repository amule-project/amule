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


#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for wxGetTranslation
#include <wx/utils.h>

#include "otherfunctions.h"	// Interface declarations
#include "Preferences.h"	// Needed for CPreferences
#include "CamuleAppBase.h"	// Needed for theApp
#include "PartFile.h"		// Needed for CPartFile
#include "KnownFile.h"		// Needed for CAbstractFile
#include "CString.h"	// Needed for CString
#include "amule.h"		// Needed for theApp


// Base chars for encode an decode functions
static byte base16Chars[17] = "0123456789ABCDEF";
static byte base32Chars[33] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
#define BASE16_LOOKUP_MAX 23
static byte base16Lookup[BASE16_LOOKUP_MAX][2] = {
    { '0', 0x0 },
    { '1', 0x1 },
    { '2', 0x2 },
    { '3', 0x3 },
    { '4', 0x4 },
    { '5', 0x5 },
    { '6', 0x6 },
    { '7', 0x7 },
    { '8', 0x8 },
    { '9', 0x9 },
	{ ':', 0x9 },
    { ';', 0x9 },
    { '<', 0x9 },
    { '=', 0x9 },
    { '>', 0x9 },
    { '?', 0x9 },
    { '@', 0x9 },
    { 'A', 0xA },
    { 'B', 0xB },
    { 'C', 0xC },
    { 'D', 0xD },
    { 'E', 0xE },
    { 'F', 0xF }
};

CString CastItoXBytes(uint64 count){
	CString buffer;
	if (count < 1024)
		buffer.Format("%.0f %s",(float)count,CString(_("Bytes")).GetData());
	else if (count < 1048576)
		buffer.Format("%.0f %s",(float)count/1024,CString(_("KB")).GetData());
	else if (count < 1073741824)
		buffer.Format("%.2f %s",(float)count/1048576,CString(_("MB")).GetData());
	else if (count < 1099511627776LL)
		buffer.Format("%.2f %s",(float)count/1073741824,CString(_("GB")).GetData());
	else 
		buffer.Format("%.3f %s",(float)count/1099511627776LL,CString(_("TB")).GetData());
	return buffer;
}

CString CastItoIShort(uint64 count){
	CString output;
	if (count < 1000)
		output.Format("%i",count);
	else if (count < 1000000)
		output.Format("%.0f%s",(float)count/1000, CString(_("K")).GetBuffer());
	else if (count < 1000000000)
		output.Format("%.2f%s",(float)count/1000000, CString(_("M")).GetBuffer());
	else if (count < 1000000000000LL)
		output.Format("%.2f%s",(float)count/1000000000LL, CString(_("G")).GetBuffer());
	else if (count < 1000000000000000LL)
		output.Format("%.2f%s",(float)count/1000000000000LL, CString(_("T")).GetBuffer());
	return output;
}

CString CastSecondsToHM(sint32 count){
	CString buffer;
	if (count < 0)
		buffer = "?"; 
	else if (count < 60)
		buffer.Format("%i %s",count,CString(_("secs")).GetData()); 
	else if (count < 3600) 
		buffer.Format("%i:%s %s",count/60,LeadingZero(count-(count/60)*60).GetData(),CString(_("mins")).GetData());
	else if (count < 86400) 
		buffer.Format("%i:%s %s",count/3600,LeadingZero((count-(count/3600)*3600)/60).GetData(),CString(_("h")).GetData());
	else 
		buffer.Format("%i %s %i %s",count/86400,CString(_("D")).GetData(),(count-(count/86400)*86400)/3600,CString(_("h")).GetData()); 
	return buffer;
} 

CString LeadingZero(uint32 units) {
	CString temp;
	if (units<10) temp.Format("0%i",units); else temp.Format("%i",units);
	return temp;
}

//<<--9/21/02
void ShellOpenFile(wxString name){ 
  //ShellExecute(NULL, "open", name, NULL, NULL, SW_SHOW); 
  printf("todo. shellopen\n");
} 

namespace {
	bool IsHexDigit(int c) {
		switch (c) {
		case '0': return true;
		case '1': return true;
		case '2': return true;
		case '3': return true;
		case '4': return true;
		case '5': return true;
		case '6': return true;
		case '7': return true;
		case '8': return true;
		case '9': return true;
		case 'A': return true;
		case 'B': return true;
		case 'C': return true;
		case 'D': return true;
		case 'E': return true;
		case 'F': return true;
		case 'a': return true;
		case 'b': return true;
		case 'c': return true;
		case 'd': return true;
		case 'e': return true;
		case 'f': return true;
		default: return false;
		}
	}
}

void URLDecode(wxString& result, const char* buff)
{
	int buflen = (int)strlen(buff);
	int x;
	int y;
	char* buff2 = nstrdup(buff); // length of buff2 will be less or equal to length of buff
	for (x = 0, y = 0; x < buflen ; ++x )
	{
		if ( buff[x] == '%' && x+2 < buflen && IsHexDigit(buff[x+1]) && IsHexDigit(buff[x+2]) ) {
			char hexstr[3];
			// Copy the two bytes following the %
			strncpy(hexstr, &buff[x + 1], 2);

			// Skip over the hex
			x = x + 2;

			// Convert the hex to ASCII
			buff2[y++] = (unsigned char)strtoul(hexstr, NULL, 16);
		}
		else {
			buff2[y++] = buff[x];
			break;
		}
	}
	result = buff2;
	delete[] buff2;
}

wxString URLEncode(wxString sIn){
    wxString sOut;
	
    const int nLen = sIn.Length() + 1;

    register LPBYTE pOutTmp = NULL;
    LPBYTE pOutBuf = NULL;
    register LPBYTE pInTmp = NULL;
    LPBYTE pInBuf =(LPBYTE)sIn.GetWriteBuf(nLen); //GetData(); //GetBuffer(nLen);
	
    //alloc out buffer
    pOutBuf = (LPBYTE)sOut.GetWriteBuf(nLen*3-2); //GetData(); //GetBuffer(nLen  * 3 - 2);//new BYTE [nLen  * 3];

    if(pOutBuf)
    {
        pInTmp	= pInBuf;
	pOutTmp = pOutBuf;
		
	// do encoding
	while (*pInTmp)
	{
	    if(isalnum(*pInTmp))
	        *pOutTmp++ = *pInTmp;
	    else
	        if(isspace(*pInTmp))
		    *pOutTmp++ = '+';
		else
		{
		    *pOutTmp++ = '%';
		    *pOutTmp++ = toHex(*pInTmp>>4);
		    *pOutTmp++ = toHex(*pInTmp%16);
		}
	    pInTmp++;
	}
	*pOutTmp = '\0';
	sOut=pOutBuf;
	//delete [] pOutBuf;
	//Out.ReleaseBuffer();
	sOut.UngetWriteBuf();
    }
    //sIn.ReleaseBuffer();
    sIn.UngetWriteBuf();
    return sOut;
}

inline BYTE toHex(const BYTE &x){
	return x > 9 ? x + 55: x + 48;
}

// Returns the Typename, examining the extention of the given filename
wxString GetFiletypeByName(wxString infile) {
	wxString ext;

	if(infile.Find('.',TRUE)==-1) return wxString(_("Any"));

	ext= infile.Right(infile.Length()-infile.Find('.',TRUE)).MakeLower() ;
	
	if (ext==".mpc" || ext==".mp4" || ext==".aac" || ext==".ape" || ext==".mp3" || 
	    ext==".mp2" || ext==".wav" || ext==".au" || ext==".ogg" || ext==".wma") return wxString(_("Audio"));

	if (ext==".jpg" || ext==".jpeg" || ext==".bmp" || ext==".gif" || ext==".tif" ||
	    ext==".png") return wxString(_("Pictures"));

	if (ext==".avi" || ext==".mpg" || ext==".mpeg" || ext==".ram" || ext==".rm" || ext==".asf" ||
	    ext==".vob" || ext==".divx" || ext==".vivo" || ext==".ogm" || ext==".mov" || ext==".wmv") return wxString(_("Videos"));

	if (ext==".gz" || ext==".zip" || ext==".ace" || ext==".rar") return wxString(_("Archives"));

	if (ext==".exe" || ext==".com") return wxString(_("Programs")); 

	if (ext==".ccd"|| ext==".sub" || ext==".cue" || ext==".bin" || ext==".iso" || ext==".nrg" ||
	    ext==".img" || ext==".bwa" || ext==".bwi" || ext==".bws" || ext==".bwt" || ext==".mds" || ext==".mdf")
	  return wxString(_("CD-Images"));
  
	return wxString(_("Any"));
}


CString MakeStringEscaped(CString in) {
	in.Replace("&","&&");
	
	return in;
}

#if 0
bool Ask4RegFix(bool checkOnly){
	// check registry if ed2k links is assigned to amule
	CRegKey regkey;
	regkey.Create(HKEY_CLASSES_ROOT,"ed2k\\shell\\open\\command");
	ULONG maxsize = 500;
	TCHAR rbuffer[500];
	char modbuffer[490];
	char regbuffer[520];
	regkey.QueryStringValue(0,rbuffer,&maxsize);
	::GetModuleFileName(0,modbuffer, 490);
	sprintf(regbuffer,"\"%s\" \"%%1\"",modbuffer);
	if (strcmp(rbuffer,regbuffer)){
		if (checkOnly) return true;
		if (MessageBox(0,GetResString(IDS_ASSIGNED2K),GetResString(IDS_ED2KLINKFIX),MB_ICONQUESTION|MB_YESNO) == IDYES){
			regkey.SetStringValue(0,regbuffer);	
			regkey.Create(HKEY_CLASSES_ROOT,"ed2k\\DefaultIcon" );// Added Shrink 
			regkey.SetStringValue(0,modbuffer);
			regkey.Create(HKEY_CLASSES_ROOT,"ed2k" );
			regkey.SetStringValue(0,"URL: ed2k Protocol");
			regkey.SetStringValue("URL Protocol","" );
		}
	}
	regkey.Close();
	return false;
}
#endif

int GetMaxConnections() {
#if 0
	OSVERSIONINFOEX osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if(!GetVersionEx((OSVERSIONINFO*)&osvi)) {
		//if OSVERSIONINFOEX doesn't work, try OSVERSIONINFO
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		if(!GetVersionEx((OSVERSIONINFO*)&osvi))
			return -1;  //shouldn't ever happen
	}

	if(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) // Windows NT product family
		return -1;  //no limits

	if(osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) { // Windows 95 product family

		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0) { //old school 95
			HKEY hKey;
			DWORD dwValue;
			DWORD dwLength = sizeof(dwValue);
			LONG lResult;

			RegOpenKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\VxD\\MSTCP",
				0, KEY_QUERY_VALUE, &hKey);
			lResult = RegQueryValueEx(hKey, TEXT("MaxConnections"), NULL, NULL,
				(LPBYTE)&dwValue, &dwLength);
			RegCloseKey(hKey);

			if(lResult != ERROR_SUCCESS || lResult < 1)
				return 100;  //the default for 95 is 100

			return dwValue;

		} else { //98 or ME
			HKEY hKey;
			TCHAR szValue[32];
			DWORD dwLength = sizeof(szValue);
			LONG lResult;

			RegOpenKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\VxD\\MSTCP",
				0, KEY_QUERY_VALUE, &hKey);
			lResult = RegQueryValueEx(hKey, TEXT("MaxConnections"), NULL, NULL,
				(LPBYTE)szValue, &dwLength);
			RegCloseKey(hKey);

			LONG lMaxConnections;
			if(lResult != ERROR_SUCCESS || (lMaxConnections = atoi(szValue)) < 1)
				return 100;  //the default for 98/ME is 100

			return lMaxConnections;
		}         
	}

	return -1;  //give the user the benefit of the doubt, most use NT+ anyway
#endif
	return -1;
}

CString   GetRateString(uint16 rate)   { 
      switch (rate){ 
      case 0: 
         return CString(_("Not rated")); 
         break; 
      case 1: 
         return CString(_("Invalid / Corrupt / Fake")); 
         break; 
      case 2: 
         return CString(_("Poor")); 
         break; 
      case 3: 
         return CString(_("Good")); 
         break; 
      case 4: 
         return CString(_("Fair")); 
         break; 
      case 5: 
         return CString(_("Excellent")); 
         break; 
      } 
      return CString(_("Not rated")); 
}


WORD DetectWinVersion()
{
  // we are at least this good
  return _WINVER_XP_;
}


// Returns a BASE32 encoded byte array
//
// [In]
//   buffer: Pointer to byte array
//   bufLen: Lenght of buffer array
//
// [Return]
//   CString object with BASE32 encoded byte array
CString EncodeBase32(const unsigned char* buffer, unsigned int bufLen)
{
	CString Base32Buff;
    
	unsigned int i, index;
    unsigned char word;

    for(i = 0, index = 0; i < bufLen;) {

		// Is the current word going to span a byte boundary?
        if (index > 3) {
            word = (buffer[i] & (0xFF >> index));
            index = (index + 5) % 8;
            word <<= index;
            if (i < bufLen - 1)
                word |= buffer[i + 1] >> (8 - index);

            i++;
        } else {
            word = (buffer[i] >> (8 - (index + 5))) & 0x1F;
            index = (index + 5) % 8;
            if (index == 0)
               i++;
        }

		Base32Buff += (char) base32Chars[word];
    }

    return Base32Buff;
}

// Returns a BASE16 encoded byte array
//
// [In]
//   buffer: Pointer to byte array
//   bufLen: Lenght of buffer array
//
// [Return]
//   CString object with BASE16 encoded byte array
CString EncodeBase16(const unsigned char* buffer, unsigned int bufLen)
{
	CString Base16Buff;

	for(unsigned int i = 0; i < bufLen; i++) {
		Base16Buff += base16Chars[buffer[i] >> 4];
		Base16Buff += base16Chars[buffer[i] & 0xf];
	}

    return Base16Buff;
}

// Decodes a BASE16 string into a byte array
//
// [In]
//   base16Buffer: String containing BASE16
//   base16BufLen: Lenght BASE16 coded string's length
//
// [Out]
//   buffer: byte array containing decoded string
void DecodeBase16(const char *base16Buffer, unsigned int base16BufLen, byte *buffer)
{
    memset(buffer, 0, DecodeLengthBase16(base16BufLen));
  
    for(unsigned int i = 0; i < base16BufLen; i++) {
		int lookup = toupper(base16Buffer[i]) - '0';

        // Check to make sure that the given word falls inside a valid range
		byte word = 0;
        
		if ( lookup < 0 || lookup >= BASE16_LOOKUP_MAX)
           word = 0xFF;
        else
           word = base16Lookup[lookup][1];

		if(i % 2 == 0) {
			buffer[i/2] = word << 4;
		} else {
			buffer[(i-1)/2] |= word;
		}
	}
}

// Calculates length to decode from BASE16
//
// [In]
//   base16Length: Actual length of BASE16 string
//
// [Return]
//   New length of byte array decoded
int	DecodeLengthBase16(int base16Length)
{
	return base16Length / 2;
}


#if 0
void UpdateURLMenu(CMenu &menu,int &counter){
	counter=0;
	theApp.webservices.RemoveAll();
	CString name,url,sbuffer;
	char buffer[1024];
	int lenBuf = 1024;

	FILE* readFile= fopen(CString(theApp.glob_prefs->GetAppDir())+"webservices.dat", "r");
	if (readFile!=NULL) {
		while (!feof(readFile)) {
			if (fgets(buffer,lenBuf,readFile)==0) break;
			sbuffer=buffer;
			
			// ignore comments & too short lines
			if (sbuffer.GetAt(0) == '#' || sbuffer.GetAt(0) == '/' || sbuffer.GetLength()<5)
				continue;
			
			int pos=sbuffer.Find(',');
			if (pos>0) {
				counter++;
				menu.AppendMenu(MF_STRING,MP_WEBURL+(counter-1), sbuffer.Left(pos).Trim() );
				theApp.webservices.Add(sbuffer.Right(sbuffer.GetLength()-pos-1).Trim() );
			}
		}
		fclose(readFile);
	}
}
#endif

void RunURL(CAbstractFile* file, CString urlpattern)
{
#if 0
	// Convert hash to hexadecimal text and add it to the URL
	urlpattern.Replace("#hashid", EncodeBase16(file->GetFileHash(), 16));

	// Add file size to the URL
	CString temp;
	temp.Format("%u",file->GetFileSize());
	urlpattern.Replace("#filesize", temp);

	// add filename to the url
	urlpattern.Replace("#filename",URLEncode(file->GetFileName()));

	// Open URL
	ShellExecute(NULL, NULL, urlpattern, NULL, theApp.glob_prefs->GetAppDir(), SW_SHOWDEFAULT);
#endif
}
CString GetCatTitle(int catid) {
        switch (catid) {
                case 0 : return CString(_("all"));
                case 1 : return CString(_("all others"));
                case 2 : return CString(_("Incomplete"));
                case 3 : return CString(_("Completed"));
                case 4 : return CString(_("Waiting"));
                case 5 : return CString(_("Downloading"));
                case 6 : return CString(_("Erroneous"));
                case 7 : return CString(_("Paused"));
                case 8 : return CString(_("Stopped"));
                case 9 : return CString(_("Video"));
                case 10 : return CString(_("Audio"));
                case 11 : return CString(_("Archive"));
                case 12 : return CString(_("CD-Images"));
                case 13 : return CString(_("Pictures"));
                case 14 : return CString(_("Text"));
        }
        return CString(_("?"));
}
  
bool CheckShowItemInGivenCat(CPartFile* file,int inCategory) {
        // easy normal cases
		bool IsInCat;
		bool IsNotFiltered = true;
   
		IsInCat = ((inCategory==0) || (inCategory>0 && inCategory==file->GetCategory()));
                                                                                
        if (theApp.glob_prefs->GetAllcatType()>0)
                switch (theApp.glob_prefs->GetAllcatType()) {
                        case 1 : {
							IsNotFiltered = ((file->GetCategory()==0) || (inCategory>0));
							break;
						}
                        case 2 : {
							IsNotFiltered = (file->IsPartFile());
							break;
						}
                        case 3 : {
							IsNotFiltered = (!file->IsPartFile());
							break;
						}
                        case 4 : {
							IsNotFiltered = ((file->GetStatus()==PS_READY|| file->GetStatus()==PS_EMPTY) && file->GetTransferingSrcCount()==0);
							break;
						}
                        case 5 : {
							IsNotFiltered = ((file->GetStatus()==PS_READY|| file->GetStatus()==PS_EMPTY) && file->GetTransferingSrcCount()>0);
							break;
						}
                        case 6 : {
							IsNotFiltered = (file->GetStatus()==PS_ERROR);
							break;
						}
                        case 7 : {
							IsNotFiltered =  ((file->GetStatus()==PS_PAUSED) && (!file->IsStopped()));
							break;
						}
                        case 8 : {
							IsNotFiltered = file->IsStopped();
							break;
						}
                        case 9 : {
							IsNotFiltered = file->IsMovie();
							break;
						}
			  			case 10 : {
							IsNotFiltered = file->IsSound();
							break;
						}
                        case 11 : {
							IsNotFiltered = file->IsArchive();
							break;
						}
			  			case 12 : {
							IsNotFiltered = file->IsCDImage();
							break;
						}
			  			case 13 : {
							IsNotFiltered = file->IsImage();
							break;
						}
			  			case 14 : {
							IsNotFiltered =  file->IsText();
							break;
						}
                }
        return (IsNotFiltered && IsInCat);
}

void MakeFoldername(char* path){
	wxString string(path);
	if (string.Length()>0) if (string.Right(1)=='\\') string=string.Left(string.Length()-1);
	sprintf(path,"%s",string.GetData());
}

void HexDump(const void *buffer, unsigned long buflen)
{
	const uint8* cbuf = (const uint8*)buffer;

	for ( unsigned long ofs = 0; ofs < buflen; /* no increment here */ )
	{
		printf("%08lx", ofs);

		for ( unsigned long i = 0; (i<8) && (ofs<buflen); i++,ofs++ )
		{
			printf(" %02x", (int)cbuf[ofs]);
		}
		printf("\n");
	}
}

bool BackupFile(const wxString& filename, const wxString& appendix)
{

	if ( !FS_wxCopyFile(filename, filename + appendix) ) {
		printf("info: Could not create backup of '%s'\n",filename.c_str());
		return false;
	}
	
	// Kry - Safe Backup
	CFile safebackupfile;
	safebackupfile.Open(filename + appendix,CFile::read);
	safebackupfile.Flush();
	safebackupfile.Close();
	// Kry - Safe backup end
	
	return true;
}

// Kry - Added to get rid of fstab quiet flag on vfat
bool FS_wxCopyFile(const wxString& file1, const wxString& file2,bool overwrite) {
	bool result;
	if (theApp.use_chmod==1){
		result = wxCopyFile(file1,file2,overwrite);
	} else {
		result = wxCopyFile_fat32(file1,file2,overwrite);
	}
	return result;
}

bool FS_wxRenameFile(const wxString& file1, const wxString& file2) {
	bool result;
	if (theApp.use_chmod==1){
		result = wxRenameFile(file1,file2);
	} else {
		result = wxRenameFile_fat32(file1,file2);
	}
	return result;
}

bool IsGoodIP(uint32 nIP)
{
	// always filter following IP's
	// -------------------------------------------
	//	 0.0.0.0
	// 127.*.*.*						localhost

	if (nIP==0 || (uint8)nIP==127)
		return false;

	if (!theApp.glob_prefs->FilterBadIPs())
		return true;

	// filter LAN IP's
	// -------------------------------------------
	//	0.*
	//	10.0.0.0 - 10.255.255.255		class A
	//	172.16.0.0 - 172.31.255.255		class B
	//	192.168.0.0 - 192.168.255.255	class C

	uint8 nFirst = (uint8)nIP;
	uint8 nSecond = (uint8)(nIP >> 8);

	if (nFirst==192 && nSecond==168) // check this 1st, because those LANs IPs are mostly spreaded
		return false;

	if (nFirst==172 && nSecond>=16 && nSecond<=31)
		return false;

	if (nFirst==0 || nFirst==10)
		return false;

	return true; 
}

bool IsGoodIPPort(uint32 nIP, uint16 nPort)
{
	return IsGoodIP(nIP) && nPort!=0;
}

int wxCMPFUNC_CONV Uint16CompareValues(uint16* first, uint16* second) {
	return (((int)*first) - ((int)*second))	;
}	

/*         BSD based OS support for gethostname_r       */

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)  || defined(__DARWIN__)
#include <sys/socket.h>
#include <netdb.h>

static int
convert (struct hostent *host, struct hostent *result,
       char *buf, int buflen, int *h_errnop)
{
  int len, i;

  if (!buf || !h_errnop) return -1;
  *h_errnop = h_errno;

  *result = *host;
  result->h_name = (char *) buf;
  /* This is the size. */
  len = strlen (host->h_name) + 1;
  if (len > buflen) return -1;
  buflen -= len;
  buf += len;
  strcpy ((char *) result->h_name, host->h_name);

  /* How many aliases and how big the buffer should be? There
     is always a NULL pointer. */
  for (len = sizeof (char *), i = 0; host->h_aliases [i]; i++)
  {
    /* It should be size of (char *) and the length of string
       plus 1. */
    len += strlen (host->h_aliases [i]) + 1 + sizeof (char *);
  }
  if (len > buflen) return -1;
  buflen -= len;

  /* This is an array of char * for h_aliases. */
#ifdef NEED_ALIGNED_ACCESS
  {
      int extra;
      extra = 4 - (((unsigned long) buf) & 3);
      if (extra != 4) {
         if (buflen < extra)
             return -1;
         buf = (char *) buf + extra;
      }
  }
#endif
  result->h_aliases = (char **) buf;
  buf += (i + 1) * sizeof (char *);

  /* We copy the aliases now. */
  for (i = 0; host->h_aliases [i]; i++)
  {
    result->h_aliases [i] = (char *) buf;
    strcpy (result->h_aliases [i], host->h_aliases [i]);
    buf += strlen (host->h_aliases [i]) + 1;
  }
  /* This is the last one */
  result->h_aliases [i] = NULL;

#if BSD >= 43 || defined(h_addr)
  for (len = sizeof (char *), i = 0; host->h_addr_list [i]; i++)
  {
    /* It should be size of (char *) and the length of string
       plus 1. */
    len += host->h_length + sizeof (char *);
  }
  if (len > buflen) return -1;

  /* This is an array of char * for h_addr_list. */
#ifdef NEED_ALIGNED_ACCESS
  {
      int extra;
      extra = 4 - (((unsigned long) buf) & 0x3);
      if (extra != 4) {
         if (buflen < extra)
             return -1;
         buf = ((char *) buf) + extra;
      }
  }
#endif
  result->h_addr_list = (char **) buf;
  buf += (i + 1) * sizeof (char *);

  /* We copy the h_addr_list now. */
  for (i = 0; host->h_addr_list [i]; i++)
  {
    result->h_addr_list [i] = (char *) buf;
    memcpy (result->h_addr_list [i], host->h_addr_list [i], host->h_length);
    buf += host->h_length;
  }
  /* This is the last one */
  result->h_addr_list [i] = NULL;
#else
  len = strlen (host->h_addr) + 1 + sizeof (char *);
  if (len > buflen) return -1;

  result->h_addr = (char *) buf;
  strcpy (result->h_addr, host->h_addr);
#endif
  return 0;
}

struct hostent *gethostbyname_r (const char *name, struct hostent *result, char *buffer, int buflen, int *h_errnop)
{
  struct hostent *host;

  s_mutexProtectingGetHostByName.Lock();

  host = gethostbyname (name);
  if (!host ||
       convert (host, result, buffer, buflen, h_errnop) != 0)
  {
    result = NULL;
  }

  s_mutexProtectingGetHostByName.Unlock();
  return result;
}

#endif /* __FreeBSD__ */
