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

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for wxGetTranslation
#include <wx/utils.h>
#include <wx/tokenzr.h>
#include <wx/filename.h>

#ifdef __WXMSW__
    #include <wx/msw/winundef.h>
	#include <wx/msw/registry.h>
#endif

#include "otherfunctions.h"	// Interface declarations
#include "config.h"			// Needed for VERSION
#include "SHAHashSet.h"

#include <cctype>



wxString GetMuleVersion()
{
	wxString ver;
	
	ver += wxString::Format(wxT("aMule %s using "), wxT(VERSION));

	// Figure out the wx build-type
	#ifdef __WXGTK__
		#ifdef __WXGTK20__
			ver += wxT("wxGTK2");
		#else
			ver += wxT("wxGTK1");
		#endif
	#elif defined(__WXMAC__)
		ver += wxT("wxMac");
	#elif defined(__WXMSW__)
		ver += wxT("wxWin");
	#endif

	ver += wxString::Format(wxT(" v%d.%d.%d"), wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER );

#if wxUSE_UNICODE
	ver += wxT(" (Unicoded)");
#endif
	
	return ver;
}



// Formats a filesize in bytes to make it suitable for displaying
wxString CastItoXBytes( uint64 count )
{

	if (count < 1024)
		return wxString::Format( wxT("%.0f %s"), (float)count, _("Bytes") );
	else if (count < 1048576)
		return wxString::Format( wxT("%.0f %s"), (float)count/1024, _("KB") );
	else if (count < 1073741824)
		return wxString::Format( wxT("%.2f %s"), (float)count/1048576, _("MB") );
	else if (count < 1099511627776LL)
		return wxString::Format( wxT("%.2f %s"), (float)count/1073741824, _("GB") );
	else
		return wxString::Format( wxT("%.3f %s"), (float)count/1099511627776LL, _("TB") );

	return _("Error");
}


wxString CastItoIShort(uint64 count)
{

	if (count < 1000)
		return wxString::Format(wxT("%llu"), count);
	else if (count < 1000000)
		return wxString::Format(wxT("%.0f%s"),(float)count/1000, _("K") );
	else if (count < 1000000000)
		return wxString::Format(wxT("%.2f%s"),(float)count/1000000, _("M") );
	else if (count < 1000000000000LL)
		return wxString::Format(wxT("%.2f%s"),(float)count/1000000000LL, _("G") );
	else if (count < 1000000000000000LL)
		return wxString::Format(wxT("%.2f%s"),(float)count/1000000000000LL, _("T") );

	return _("Error");
}


// Make a time value in millieseconds suitable for displaying
wxString CastSecondsToHM(sint32 count)
{
	
	if (count < 0)
		return wxT("?");
	else if (count < 60)
		return wxString::Format( wxT("%02i %s"), count, _("secs") );
	else if (count < 3600)
		return wxString::Format( wxT("%i:%02i %s"), count/60, (count % 60), _("mins") );
	else if (count < 86400)
		return wxString::Format( wxT("%i:%02i %s"), count/3600, (count % 3600)/60, _("h") );
	else
		return wxString::Format( wxT("%i %s %02i %s"), count/86400, _("D") , (count % 86400) / 3600, _("h") );
		
	return _("Error");
}


// Examins a filename and determines the filetype
FileType GetFiletype(const wxString& filename)
{
	if( filename.Find('.' ) == -1 )
		return ftAny;

	wxString ext = filename.AfterLast('.').MakeLower();

	if ( ext==wxT("avi")  || ext==wxT("mpg")  || ext==wxT("mpeg") || ext==wxT("ogm")  ||
		 ext==wxT("ram")  || ext==wxT("rm")   || ext==wxT("asf")  || ext==wxT("vob")  ||
		 ext==wxT("divx") || ext==wxT("vivo") || ext==wxT("mov")  || ext==wxT("wmv")  ||
		 ext==wxT("m2v")  || ext==wxT("swf")  || ext==wxT("qt")   || ext==wxT("mkv") )
		 return ftVideo;
		 
	if ( ext==wxT("cue")  || ext==wxT("bin")  || ext==wxT("iso")  || ext==wxT("ccd")  ||
		 ext==wxT("sub")  || ext==wxT("nrg")  || ext==wxT("img")  || ext==wxT("bwa")  ||
		 ext==wxT("bwi")  || ext==wxT("bws")  || ext==wxT("bwt")  || ext==wxT("mds")  ||
		 ext==wxT("mdf") )
	  return ftCDImage;
		
	if ( ext==wxT("mpc")  || ext==wxT("mp4")  || ext==wxT("aac")  || ext==wxT("ape")  ||
	     ext==wxT("mp3")  || ext==wxT("mp2")  || ext==wxT("wav")  || ext==wxT("au")   ||
		 ext==wxT("ogg")  || ext==wxT("wma")  || ext==wxT("rma")  || ext==wxT("mid") )
		 return ftAudio;

	if ( ext==wxT("jpg")  || ext==wxT("jpeg") || ext==wxT("bmp")  || ext==wxT("gif")  ||
	     ext==wxT("tiff") || ext==wxT("png")  || ext==wxT("rle")  || ext==wxT("psp")  ||
		 ext==wxT("tga")  || ext==wxT("wmf")  || ext==wxT("xpm")  || ext==wxT("pcx") )
		 return ftPicture;

	if ( ext==wxT("rar")  || ext==wxT("zip")  || ext==wxT("ace")  || ext==wxT("gz")   ||
	     ext==wxT("bz2")  || ext==wxT("tar")  || ext==wxT("arj")  || ext==wxT("lhz")  ||
		 ext==wxT("bz") )
		return ftArchive;

	if ( ext==wxT("exe")  || ext==wxT("com") )
		return ftProgram;

	if ( ext==wxT("txt")  || ext==wxT("html") || ext==wxT("htm")  || ext==wxT("doc")  ||
	     ext==wxT("pdf")  || ext==wxT("ps")   || ext==wxT("sxw")  || ext==wxT("log") )
		return ftText;

	return ftAny;
}


// Returns the (translated) description assosiated with a FileType
wxString GetFiletypeDesc(FileType type)
{
	switch ( type ) {
		case ftVideo:	return wxString( _("Videos") );
		case ftAudio:	return wxString( _("Audio") );
		case ftArchive:	return wxString( _("Archives") );
		case ftCDImage:	return wxString( _("CD-Images") );
		case ftPicture:	return wxString( _("Pictures") );
		case ftText:	return wxString( _("Texts") ); // ?
		case ftProgram:	return wxString( _("Programs") );
		default:		return wxString(_("Any"));
	}
}


// Returns the Typename, examining the extention of the given filename
wxString GetFiletypeByName(const wxString& filename)
{
	return GetFiletypeDesc( GetFiletype( filename ) );
}

// Get the max number of connections that the OS supports, or -1 for default
int GetMaxConnections() {

#ifdef __WXMSW__

	int os = wxGetOsVersion();
		
	// Try to get the max connection value in the registry
	wxRegKey key( "HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Services\\VxD\\MSTCP\\MaxConnections" );
	
	wxString value;
	if ( key.Exists() )
		value = key.QueryDefaultValue();

	if ( !value.IsEmpty() && value.IsNumber() ) {
		long maxconn = -1;

		value.ToLong( &maxconn );

		return maxconn;		
	} else {
		switch ( os ) {
			case wxWIN95:		return 50;	// This includes all Win9x versions
			case wxWINDOWS_NT:	return 500;	// This includes NT based windows
			default:			return -1;	// Anything else. Let aMule decide...
		}
	}

#endif

	// Any other OS can just use the default number of connections
	return -1;
}


// Return the text assosiated with a rating of a file
wxString GetRateString(uint16 rate)
{
	switch ( rate ) {
		case 0: return _("Not rated");
		case 1: return _("Invalid / Corrupt / Fake");
		case 2: return _("Poor");
		case 3: return _("Good");
		case 4: return _("Fair");
		case 5: return _("Excellent");
		default: return _("Not rated");
	}
}

/**
 * Return the size in bytes of the given size-type
 *
 * @param type The type (as an int) where: 0 = Byte, 1 = KB, 2 = MB, 3 = GB
 *
 * @return The amount of Bytes the provided size-type represents
 *
 * Values over GB aren't handled since the amount of Bytes 1TB represents
 * is over the uint32 capacity
 */

uint32 GetTypeSize(uint8 type)
{
        enum {Bytes, KB, MB, GB};
        int size;

        switch(type) {
                case Bytes: size = 1; break;
                case KB: size = 1024; break;
                case MB: size = 1048576; break;
                case GB: size = 1073741824; break;
                default: size = -1; break;
        }
        return size;
}

// Base16 chars for encode an decode functions
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


// Make string safe for use in URLs
wxString URLEncode(wxString sIn)
{
	wxString sOut;
	unsigned char curChar;
	
	for ( unsigned int i = 0; i < sIn.Length(); i ++ ) {
		curChar = sIn.GetChar( i );

		if ( isalnum( curChar ) ) {
	        sOut += curChar;
	    } else if( isspace ( curChar ) ) {
		    sOut += wxT("+");
		} else {
			sOut += wxT("%");
			sOut += base16Chars[ curChar >> 4];
			sOut += base16Chars[ curChar & 0xf];
		}

	}

	return sOut;
}


wxString TruncateFilename(const wxString& filename, size_t length, bool isFilePath)
{
	// Check if there's anything to do
	if ( filename.Length() <= length )
		return filename;
	
	wxString file = filename;
	
	// If the filename is a path, then prefer to remove from the path, rather than the filename
	if ( isFilePath ) {
		wxString path = file.BeforeLast( wxFileName::GetPathSeparator() );
		file          = file.AfterLast( wxFileName::GetPathSeparator() );

		if ( path.Length() >= length ) {
			path.Clear();
		} else if ( file.Length() >= length ) {
			path.Clear();
		} else {
			// Minus 6 for "[...]" + seperator
			int pathlen = length - file.Length() - 6;
			
			if ( pathlen > 0 ) {
				path = wxT("[...]") + path.Right( pathlen );
			} else {
				path.Clear();
			}
		}
		
		if ( !path.IsEmpty() ) {
			file = path + wxFileName::GetPathSeparator() + file;
		}
	}

	if ( file.Length() > length ) {
		if ( length > 5 ) {		
			file = file.Left( length - 5 ) + wxT("[...]");
		} else {
			file.Clear();
		}
	}
	

	return file;
}


// Returns a BASE32 encoded byte array
//
// [In]
//   buffer: Pointer to byte array
//   bufLen: Lenght of buffer array
//
// [Return]
//   wxString object with BASE32 encoded byte array
wxString EncodeBase32(const unsigned char* buffer, unsigned int bufLen)
{
	wxString Base32Buff;
    
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
//   wxString object with BASE16 encoded byte array
wxString EncodeBase16(const unsigned char* buffer, unsigned int bufLen)
{
	wxString Base16Buff;

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
    memset( buffer, 0, base16BufLen / 2 );
	
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

uint32 DecodeBase32(const char* pszInput, uchar* paucOutput, uint32 nBufferLen)
{
	if (pszInput == NULL)
		return false;
	uint32 nDecodeLen = (strlen(pszInput)*5)/8;
	if ((strlen(pszInput)*5) % 8 > 0)
		nDecodeLen++;
	uint32 nInputLen = strlen( pszInput );
	if (paucOutput == NULL || nBufferLen == 0)
		return nDecodeLen;
	if (nDecodeLen > nBufferLen || paucOutput == NULL) 
		return 0;

	DWORD nBits	= 0;
	int nCount	= 0;

	for ( int nChars = nInputLen ; nChars-- ; pszInput++ )
	{
		if ( *pszInput >= 'A' && *pszInput <= 'Z' )
			nBits |= ( *pszInput - 'A' );
		else if ( *pszInput >= 'a' && *pszInput <= 'z' )
			nBits |= ( *pszInput - 'a' );
		else if ( *pszInput >= '2' && *pszInput <= '7' )
			nBits |= ( *pszInput - '2' + 26 );
		else
			return 0;
		
		nCount += 5;

		if ( nCount >= 8 )
		{
			*paucOutput++ = (BYTE)( nBits >> ( nCount - 8 ) );
			nCount -= 8;
		}

		nBits <<= 5;
	}

	return nDecodeLen;
}

uint32 DecodeBase32(const char* pszInput, CAICHHash& Hash){
	return DecodeBase32(pszInput, Hash.GetRawHash(), Hash.GetHashSize());
}


// Returns the text assosiated with a category type
wxString GetCatTitle(int catid)
{
	switch (catid) {
		case 0:	 return _("all");
		case 1:  return _("all others");
		case 2:  return _("Incomplete");
		case 3:  return _("Completed");
		case 4:  return _("Waiting");
		case 5:  return _("Downloading");
		case 6:  return _("Erroneous");
		case 7:  return _("Paused");
		case 8:  return _("Stopped");
		case 9:  return _("Video");
		case 10: return _("Audio");
		case 11: return _("Archive");
		case 12: return _("CD-Images");
		case 13: return _("Pictures");
		case 14: return _("Text");
		default: return wxT("?");
	}
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


int wxCMPFUNC_CONV Uint16CompareValues(uint16* first, uint16* second) {
       return (((int)*first) - ((int)*second)) ;
}      


// DumpMem ... Dumps mem ;)
void DumpMem(const void* where, uint32 size) {
	for (uint32 i = 0; i< size; i++) {
		printf("|%2x",(uint8)((char*)where)[i]);
		if ((i % 16) == 15) {
			printf("\n");
		}			
	}	
	printf("\n");	
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
