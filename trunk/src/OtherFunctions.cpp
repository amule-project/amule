//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "OtherFunctions.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for wxGetTranslation
#include <wx/utils.h>
#include <wx/tokenzr.h>
#include <wx/file.h>		// Needed for wxFile

#ifdef __WXMSW__
	#include <wx/msw/winundef.h>
	#include <wx/msw/registry.h>
#endif

#include "OtherFunctions.h"	// Interface declarations

#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for VERSION
#endif

#include <cctype>

namespace otherfunctions {

wxString GetMuleVersion()
{
	wxString ver;
	
	ver += wxString::Format(wxT("%s using "), wxT(VERSION));

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
	#elif defined(__WXCOCOA__)
		ver += wxT("wxCocoa");
	#endif

	ver += wxString::Format(wxT(" v%d.%d.%d"), wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER );

#if wxUSE_UNICODE
	ver += wxT(" (Unicoded)");
#endif
	
#ifdef CVSDATE
	ver += wxString::Format( wxT(" (Snapshot: %s)"), wxT(CVSDATE));
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
wxString GetFiletypeDesc(FileType type, bool translated)
{
	switch ( type ) {
		case ftVideo:	
			if (translated) {
				return _("Videos");
			} else {
				return wxT("Videos");
			}
			break;
		case ftAudio:
			if (translated) {
				return _("Audio");
			} else {
				return wxT("Audio");
			}
			break;			
		case ftArchive:	
			if (translated) {
				return _("Archives");
			} else {
				return wxT("Archives");
			}
			break;			
		case ftCDImage:
			if (translated) {
				return _("CD-Images");
			} else {
				return wxT("CD-Images");
			}
			break;			
		case ftPicture:
			if (translated) {
				return _("Pictures");
			} else {
				return wxT("Pictures");
			}
			break;			
		case ftText:
			if (translated) {
				return _("Texts");
			} else {
				return wxT("Texts");
			}
			break;			
		case ftProgram:
			if (translated) {
				return _("Programs");
			} else {
				return wxT("Programs");
			}
			break;			
		default:
			if (translated) {
				return _("Any");
			} else {
				return wxT("Any");
			}
			break;			
	}
}


// Returns the Typename, examining the extention of the given filename
wxString GetFiletypeByName(const wxString& filename, bool translated)
{
	return GetFiletypeDesc( GetFiletype( filename ), translated );
}

/** 
 * Return a boolean meaning whether the file has contents or not (doesn't
 * matter if it exists)
 *
 * @param filename The filename of the file to evaluate (as a wxString)
 *
 * @return Boolean value TRUE when it has no contents (file doesn't exists
 * or it's size is 0bytes). Any othe case, FALSE
 */

bool IsEmptyFile(const wxString& filename)
{
	if (wxFile::Exists(filename)) {
		wxFile file(filename);
		if (file.IsOpened()) {
			return ( file.Length() == 0 );
		}
	}
	return true;
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
static wxChar base16Chars[17] = wxT("0123456789ABCDEF");
static wxChar base32Chars[33] = wxT("ABCDEFGHIJKLMNOPQRSTUVWXYZ234567");
#define BASE16_LOOKUP_MAX 23
static wxChar base16Lookup[BASE16_LOOKUP_MAX][2] = {
	{ wxT('0'), 0x0 },
	{ wxT('1'), 0x1 },
	{ wxT('2'), 0x2 },
	{ wxT('3'), 0x3 },
	{ wxT('4'), 0x4 },
	{ wxT('5'), 0x5 },
	{ wxT('6'), 0x6 },
	{ wxT('7'), 0x7 },
	{ wxT('8'), 0x8 },
	{ wxT('9'), 0x9 },
	{ wxT(':'), 0x9 },
	{ wxT(';'), 0x9 },
	{ wxT('<'), 0x9 },
	{ wxT('='), 0x9 },
	{ wxT('>'), 0x9 },
	{ wxT('?'), 0x9 },
	{ wxT('@'), 0x9 },
	{ wxT('A'), 0xA },
	{ wxT('B'), 0xB },
	{ wxT('C'), 0xC },
	{ wxT('D'), 0xD },
	{ wxT('E'), 0xE },
	{ wxT('F'), 0xF }
};


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

	for(unsigned int i = 0; i < bufLen; ++i) {
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
unsigned int DecodeBase16(const wxString &base16Buffer, unsigned int base16BufLen, byte *buffer)
{
	if (base16BufLen & 1) {
		return 0;
	}
	unsigned int ret = base16BufLen >> 1;
	memset( buffer, 0,  ret);
	for(unsigned int i = 0; i < base16BufLen; ++i) {
		int lookup = toupper(base16Buffer[i]) - wxT('0');
		// Check to make sure that the given word falls inside a valid range
		byte word = (lookup < 0 || lookup >= BASE16_LOOKUP_MAX) ?
			0xFF : base16Lookup[lookup][1];
		unsigned idx = i >> 1;
		buffer[idx] = (i & 1) ? // odd or even?
			(buffer[idx] | word) : (word << 4);
	}

	return ret;
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
			if (i < bufLen - 1) {
				word |= buffer[i + 1] >> (8 - index);
			}
			++i;
		} else {
			word = (buffer[i] >> (8 - (index + 5))) & 0x1F;
			index = (index + 5) % 8;
			if (index == 0) {
				++i;
			}
		}
		Base32Buff += (char) base32Chars[word];
	}

	return Base32Buff;
}

// Decodes a BASE32 string into a byte array
//
// [In]
//   base32Buffer: String containing BASE32
//   base32BufLen: Lenght BASE32 coded string's length
//
// [Out]
//   buffer: byte array containing decoded string
// [Return]
//   nDecodeLen:
unsigned int DecodeBase32(const wxString &base32Buffer, unsigned int base32BufLen, unsigned char *buffer)
{
	size_t nInputLen = base32Buffer.Length();
	uint32 nDecodeLen = (nInputLen * 5) / 8;
	if ((nInputLen * 5) % 8 > 0) {
		++nDecodeLen;
	}
	if (base32BufLen == 0) {
		return nDecodeLen;
	}
	if (nDecodeLen > base32BufLen) {
		return 0;
	}

	DWORD nBits = 0;
	int nCount = 0;
	for (size_t i = 0; i < nInputLen; ++i)
	{
		if (base32Buffer[i] >= wxT('A') && base32Buffer[i] <= wxT('Z')) {
			nBits |= ( base32Buffer[i] - wxT('A') );
		}
		else if (base32Buffer[i] >= wxT('a') && base32Buffer[i] <= wxT('z')) {
			nBits |= ( base32Buffer[i] - wxT('a') );
		}
		else if (base32Buffer[i] >= wxT('2') && base32Buffer[i] <= wxT('7')) {
			nBits |= ( base32Buffer[i] - wxT('2') + 26 );
		} else {
			return 0;
		}
		nCount += 5;
		if (nCount >= 8)
		{
			*buffer++ = (BYTE)( nBits >> (nCount - 8) );
			nCount -= 8;
		}
		nBits <<= 5;
	}

	return nDecodeLen;
}

/*
 * base64.c
 *
 * Base64 encoding/decoding command line filter
 *
 * Copyright (c) 2002 Matthias Gaertner 29.06.2002
 * Adapted by (c) 2005 Phoenix to use wxWidgets.
 *
 */
static const wxString to_b64(
	/*   0000000000111111111122222222223333333333444444444455555555556666 */
	/*   0123456789012345678901234567890123456789012345678901234567890123 */
	wxT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"));

/* Option variables */
static bool g_fUseCRLF = false;
static unsigned int g_nCharsPerLine = 72;
static wxString strHeaderLine;

wxString EncodeBase64(const char *pbBufferIn, unsigned int bufLen)
{
	wxString pbBufferOut;
	wxString strHeader;
	
	if( !strHeaderLine.IsEmpty() ) {
		strHeader = wxT("-----BEGIN ") + strHeaderLine + wxT("-----");
		if( g_fUseCRLF ) {
			strHeader += wxT("\r");
		}
		strHeader += wxT("\n");
	}

	unsigned long nDiv = ((unsigned long)bufLen) / 3;
	unsigned long nRem = ((unsigned long)bufLen) % 3;
	unsigned int NewLineSize = g_fUseCRLF ? 2 : 1;
	
	// Allocate enough space in the output buffer to speed up things
	pbBufferOut.Alloc(
		strHeader.Len() * 2 +		// header/footer
		(bufLen * 4) / 3 + 1 + 		// Number of codes
		nDiv           * NewLineSize + 	// Number of new lines
		(nRem ? 1 : 0) * NewLineSize );	// Last line
	pbBufferOut = strHeader;

	unsigned long nChars = 0;
	const unsigned char *pIn = (unsigned char*)pbBufferIn;
	while( nDiv > 0 ) {
		pbBufferOut += to_b64[ (pIn[0] >> 2) & 0x3f];
		pbBufferOut += to_b64[((pIn[0] << 4) & 0x30) | ((pIn[1] >> 4) & 0xf)];
		pbBufferOut += to_b64[((pIn[1] << 2) & 0x3c) | ((pIn[2] >> 6) & 0x3)];
		pbBufferOut += to_b64[  pIn[2] & 0x3f];
		pIn += 3;
		nDiv--;
		nChars += 4;
		if( nChars >= g_nCharsPerLine && g_nCharsPerLine != 0 ) {
			nChars = 0;
			if( g_fUseCRLF ) {
				pbBufferOut += wxT("\r");
			}
			pbBufferOut += wxT("\n");
		}
	}
	switch( nRem ) {
	case 2:
		pbBufferOut += to_b64[ (pIn[0] >> 2) & 0x3f];
		pbBufferOut += to_b64[((pIn[0] << 4) & 0x30) | ((pIn[1] >> 4) & 0xf)];
		pbBufferOut += to_b64[ (pIn[1] << 2) & 0x3c];
		pbBufferOut += wxT("=");
		nChars += 4;
		if( nChars >= g_nCharsPerLine && g_nCharsPerLine != 0 ) {
			nChars = 0;
			if( g_fUseCRLF ) {
				pbBufferOut += wxT("\r");
			}
			pbBufferOut += wxT("\n");
		}
		break;
	case 1:
		pbBufferOut += to_b64[ (pIn[0] >> 2) & 0x3f];
		pbBufferOut += to_b64[ (pIn[0] << 4) & 0x30];
		pbBufferOut += wxT("=");
		pbBufferOut += wxT("=");
		nChars += 4;
		if( nChars >= g_nCharsPerLine && g_nCharsPerLine != 0 ) {
			nChars = 0;
			if( g_fUseCRLF ) {
				pbBufferOut += wxT("\r");
			}
			pbBufferOut += wxT("\n");
		}
		break;
	}

	if( nRem > 0 ) {
		if( nChars > 0 ) {
			if( g_fUseCRLF ) {
				pbBufferOut += wxT("\r");
			}
			pbBufferOut += wxT("\n");
		}
	}

	if( !strHeaderLine.IsEmpty() ) {
		pbBufferOut = wxT("-----END ") + strHeaderLine + wxT("-----");
		if( g_fUseCRLF ) {
			pbBufferOut += wxT("\r");
		}
		pbBufferOut += wxT("\n");
	}
	
	return pbBufferOut;
}

unsigned int DecodeBase64(const wxString &base64Buffer, unsigned int base64BufLen, unsigned char *buffer)
{
	int z = 0;  // 0 Normal, 1 skip MIME separator (---) to end of line
	unsigned int nData = 0;
	unsigned int i = 0;

	if (base64BufLen == 0) {
		*buffer = 0;
		nData = 1;
	}
	
	for(unsigned int j = 0; j < base64BufLen; ++j) {
		wxChar c = base64Buffer[j];
		wxChar bits = wxT('z');
		if( z > 0 ) {
			if(c == wxT('\n')) {
				z = 0;
			}
		}
		else if(c >= wxT('A') && c <= wxT('Z')) {
			bits = c - wxT('A');
		}
		else if(c >= wxT('a') && c <= wxT('z')) {
			bits = c - wxT('a') + (wxChar)26;
		}
		else if(c >= wxT('0') && c <= wxT('9')) {
			bits = c - wxT('0') + (wxChar)52;
		}
		else if(c == wxT('+')) {
			bits = (wxChar)62;
		}
		else if(c == wxT('/')) {
			bits = (wxChar)63;
		}
		else if(c == wxT('-')) {
			z = 1;
		}
		else if(c == wxT('=')) {
			break;
		} else {
			bits = wxT('y');
		}

		// Skips anything that was not recognized
		// as a base64 valid char ('y' or 'z')
		if (bits < (wxChar)64) {
			switch (nData++) {
			case 0:
				buffer[i+0] = (bits << 2) & 0xfc;
				break;
			case 1:
				buffer[i+0] |= (bits >> 4) & 0x03;
				buffer[i+1] = (bits << 4) & 0xf0;
				break;
			case 2:
				buffer[i+1] |= (bits >> 2) & 0x0f;
				buffer[i+2] = (bits << 6) & 0xc0;
				break;
			case 3:
				buffer[i+2] |= bits & 0x3f;
				break;
			}
			if (nData == 4) {
				nData = 0;
				i += 3;
			}
		}
	}
	if (nData == 1) {
		// Syntax error or buffer was empty
		*buffer = 0;
		nData = 0;
		i = 0;
	} else {
		buffer[i+nData] = 0;
	}
	
	return i + nData;
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




int wxCMPFUNC_CONV Uint16CompareValues(uint16* first, uint16* second) {
	   return (((int)*first) - ((int)*second)) ;
}	  


struct SED2KFileType
{
	wxChar* pszExt;
	EED2KFileType iFileType;
} _aED2KFileTypes[] = 
{
    { wxT(".669"),   ED2KFT_AUDIO },
    { wxT(".aac"),   ED2KFT_AUDIO },
    { wxT(".aif"),   ED2KFT_AUDIO },
    { wxT(".aiff"),  ED2KFT_AUDIO },
    { wxT(".amf"),   ED2KFT_AUDIO },
    { wxT(".ams"),   ED2KFT_AUDIO },
    { wxT(".ape"),   ED2KFT_AUDIO },
    { wxT(".au"),    ED2KFT_AUDIO },
    { wxT(".dbm"),   ED2KFT_AUDIO },
    { wxT(".dmf"),   ED2KFT_AUDIO },
    { wxT(".dsm"),   ED2KFT_AUDIO },
    { wxT(".far"),   ED2KFT_AUDIO },
    { wxT(".flac"),  ED2KFT_AUDIO },
    { wxT(".it"),    ED2KFT_AUDIO },
    { wxT(".mdl"),   ED2KFT_AUDIO },
    { wxT(".med"),   ED2KFT_AUDIO },
    { wxT(".mid"),   ED2KFT_AUDIO },
    { wxT(".midi"),  ED2KFT_AUDIO },
    { wxT(".mod"),   ED2KFT_AUDIO },
    { wxT(".mol"),   ED2KFT_AUDIO },
    { wxT(".mp1"),   ED2KFT_AUDIO },
    { wxT(".mp2"),   ED2KFT_AUDIO },
    { wxT(".mp3"),   ED2KFT_AUDIO },
    { wxT(".mp4"),   ED2KFT_AUDIO },
    { wxT(".mpa"),   ED2KFT_AUDIO },
    { wxT(".mpc"),   ED2KFT_AUDIO },
    { wxT(".mpp"),   ED2KFT_AUDIO },
    { wxT(".mtm"),   ED2KFT_AUDIO },
    { wxT(".nst"),   ED2KFT_AUDIO },
    { wxT(".ogg"),   ED2KFT_AUDIO },
    { wxT(".okt"),   ED2KFT_AUDIO },
    { wxT(".psm"),   ED2KFT_AUDIO },
    { wxT(".ptm"),   ED2KFT_AUDIO },
    { wxT(".ra"),    ED2KFT_AUDIO },
    { wxT(".rmi"),   ED2KFT_AUDIO },
    { wxT(".s3m"),   ED2KFT_AUDIO },
    { wxT(".stm"),   ED2KFT_AUDIO },
    { wxT(".ult"),   ED2KFT_AUDIO },
    { wxT(".umx"),   ED2KFT_AUDIO },
    { wxT(".wav"),   ED2KFT_AUDIO },
    { wxT(".wma"),   ED2KFT_AUDIO },
    { wxT(".wow"),   ED2KFT_AUDIO },
    { wxT(".xm"),    ED2KFT_AUDIO },

    { wxT(".asf"),   ED2KFT_VIDEO },
    { wxT(".avi"),   ED2KFT_VIDEO },
    { wxT(".divx"),  ED2KFT_VIDEO },
    { wxT(".m1v"),   ED2KFT_VIDEO },
    { wxT(".m2v"),   ED2KFT_VIDEO },
    { wxT(".mkv"),   ED2KFT_VIDEO },
    { wxT(".mov"),   ED2KFT_VIDEO },
    { wxT(".mp1v"),  ED2KFT_VIDEO },
    { wxT(".mp2v"),  ED2KFT_VIDEO },
    { wxT(".mpe"),   ED2KFT_VIDEO },
    { wxT(".mpeg"),  ED2KFT_VIDEO },
    { wxT(".mpg"),   ED2KFT_VIDEO },
    { wxT(".mps"),   ED2KFT_VIDEO },
    { wxT(".mpv"),   ED2KFT_VIDEO },
    { wxT(".mpv1"),  ED2KFT_VIDEO },
    { wxT(".mpv2"),  ED2KFT_VIDEO },
    { wxT(".ogm"),   ED2KFT_VIDEO },
    { wxT(".qt"),    ED2KFT_VIDEO },
    { wxT(".ram"),   ED2KFT_VIDEO },
    { wxT(".rm"),    ED2KFT_VIDEO },
    { wxT(".rv"),    ED2KFT_VIDEO },
    { wxT(".rv9"),   ED2KFT_VIDEO },
    { wxT(".ts"),    ED2KFT_VIDEO },
    { wxT(".vivo"),  ED2KFT_VIDEO },
    { wxT(".vob"),   ED2KFT_VIDEO },
    { wxT(".wmv"),   ED2KFT_VIDEO },
    { wxT(".xvid"),  ED2KFT_VIDEO },

    { wxT(".bmp"),   ED2KFT_IMAGE },
    { wxT(".dcx"),   ED2KFT_IMAGE },
    { wxT(".emf"),   ED2KFT_IMAGE },
    { wxT(".gif"),   ED2KFT_IMAGE },
    { wxT(".ico"),   ED2KFT_IMAGE },
    { wxT(".jpeg"),  ED2KFT_IMAGE },
    { wxT(".jpg"),   ED2KFT_IMAGE },
    { wxT(".pct"),   ED2KFT_IMAGE },
    { wxT(".pcx"),   ED2KFT_IMAGE },
    { wxT(".pic"),   ED2KFT_IMAGE },
    { wxT(".pict"),  ED2KFT_IMAGE },
    { wxT(".png"),   ED2KFT_IMAGE },
    { wxT(".psd"),   ED2KFT_IMAGE },
    { wxT(".psp"),   ED2KFT_IMAGE },
    { wxT(".tga"),   ED2KFT_IMAGE },
    { wxT(".tif"),   ED2KFT_IMAGE },
    { wxT(".tiff"),  ED2KFT_IMAGE },
    { wxT(".wmf"),   ED2KFT_IMAGE },
    { wxT(".xif"),   ED2KFT_IMAGE },

    { wxT(".7z"),	ED2KFT_ARCHIVE },
    { wxT(".ace"),   ED2KFT_ARCHIVE },
    { wxT(".arj"),   ED2KFT_ARCHIVE },
    { wxT(".bz2"),   ED2KFT_ARCHIVE },
    { wxT(".cab"),   ED2KFT_ARCHIVE },
    { wxT(".gz"),    ED2KFT_ARCHIVE },
    { wxT(".hqx"),   ED2KFT_ARCHIVE },
    { wxT(".lha"),   ED2KFT_ARCHIVE },
    { wxT(".msi"),   ED2KFT_ARCHIVE },
    { wxT(".rar"),   ED2KFT_ARCHIVE },
    { wxT(".sea"),   ED2KFT_ARCHIVE },
    { wxT(".sit"),   ED2KFT_ARCHIVE },
    { wxT(".tar"),   ED2KFT_ARCHIVE },
    { wxT(".tgz"),   ED2KFT_ARCHIVE },
    { wxT(".uc2"),   ED2KFT_ARCHIVE },
    { wxT(".zip"),   ED2KFT_ARCHIVE },

    { wxT(".bat"),   ED2KFT_PROGRAM },
    { wxT(".cmd"),   ED2KFT_PROGRAM },
    { wxT(".com"),   ED2KFT_PROGRAM },
    { wxT(".exe"),   ED2KFT_PROGRAM },

    { wxT(".bin"),   ED2KFT_CDIMAGE },
    { wxT(".bwa"),   ED2KFT_CDIMAGE },
    { wxT(".bwi"),   ED2KFT_CDIMAGE },
    { wxT(".bws"),   ED2KFT_CDIMAGE },
    { wxT(".bwt"),   ED2KFT_CDIMAGE },
    { wxT(".ccd"),   ED2KFT_CDIMAGE },
    { wxT(".cue"),   ED2KFT_CDIMAGE },
    { wxT(".dmg"),   ED2KFT_CDIMAGE },
    { wxT(".dmz"),   ED2KFT_CDIMAGE },
    { wxT(".img"),   ED2KFT_CDIMAGE },
    { wxT(".iso"),   ED2KFT_CDIMAGE },
    { wxT(".mdf"),   ED2KFT_CDIMAGE },
    { wxT(".mds"),   ED2KFT_CDIMAGE },
    { wxT(".nrg"),   ED2KFT_CDIMAGE },
    { wxT(".sub"),   ED2KFT_CDIMAGE },
    { wxT(".toast"), ED2KFT_CDIMAGE },

    { wxT(".chm"),   ED2KFT_DOCUMENT },
    { wxT(".css"),   ED2KFT_DOCUMENT },
    { wxT(".diz"),   ED2KFT_DOCUMENT },
    { wxT(".doc"),   ED2KFT_DOCUMENT },
    { wxT(".dot"),   ED2KFT_DOCUMENT },
    { wxT(".hlp"),   ED2KFT_DOCUMENT },
    { wxT(".htm"),   ED2KFT_DOCUMENT },
    { wxT(".html"),  ED2KFT_DOCUMENT },
    { wxT(".nfo"),   ED2KFT_DOCUMENT },
    { wxT(".pdf"),   ED2KFT_DOCUMENT },
    { wxT(".pps"),   ED2KFT_DOCUMENT },
    { wxT(".ppt"),   ED2KFT_DOCUMENT },
    { wxT(".ps"),    ED2KFT_DOCUMENT },
    { wxT(".rtf"),   ED2KFT_DOCUMENT },
    { wxT(".wri"),   ED2KFT_DOCUMENT },
    { wxT(".txt"),   ED2KFT_DOCUMENT },
    { wxT(".xls"),   ED2KFT_DOCUMENT },
    { wxT(".xlt"),   ED2KFT_DOCUMENT }
};

#if 0
// Needs a lot of work and I ahve to go to work NOW

int CompareE2DKFileType(const void* p1, const void* p2)
{
	return strncmp( ((const SED2KFileType*)p1)->pszExt, ((const SED2KFileType*)p2)->pszExt );
}

EED2KFileType GetED2KFileTypeID(LPCTSTR pszFileName)
{
	LPCTSTR pszExt = _tcsrchr(pszFileName, wxT('.'));
	if (pszExt == NULL) {
		return ED2KFT_ANY;
	}
	wxString strExt(pszExt);
	strExt.MakeLower();

	SED2KFileType ft;
	ft.pszExt = strExt;
	ft.iFileType = ED2KFT_ANY;
	const SED2KFileType* pFound = (SED2KFileType*)bsearch(&ft, _aED2KFileTypes, ARRSIZE(_aED2KFileTypes), sizeof _aED2KFileTypes[0], CompareE2DKFileType);
	if (pFound != NULL)
		return pFound->iFileType;
	return ED2KFT_ANY;
}

// Retuns the ed2k file type term which is to be used in server searches
LPCSTR GetED2KFileTypeSearchTerm(EED2KFileType iFileID)
{
	if (iFileID == ED2KFT_AUDIO)		return ED2KFTSTR_AUDIO;
	if (iFileID == ED2KFT_VIDEO)		return ED2KFTSTR_VIDEO;
	if (iFileID == ED2KFT_IMAGE)		return ED2KFTSTR_IMAGE;
	if (iFileID == ED2KFT_DOCUMENT)		return ED2KFTSTR_DOCUMENT;
	if (iFileID == ED2KFT_PROGRAM)		return ED2KFTSTR_PROGRAM;
	// NOTE: Archives and CD-Images are published with file type "Pro"
	if (iFileID == ED2KFT_ARCHIVE)		return ED2KFTSTR_PROGRAM;
	if (iFileID == ED2KFT_CDIMAGE)		return ED2KFTSTR_PROGRAM;
	return NULL;
}

// Returns a file type which is used eMule internally only, examining the extention of the given filename
CString GetFileTypeByName(LPCTSTR pszFileName)
{
	EED2KFileType iFileType = GetED2KFileTypeID(pszFileName);
	switch (iFileType) {
		case ED2KFT_AUDIO:		return wxT(ED2KFTSTR_AUDIO);
		case ED2KFT_VIDEO:		return wxT(ED2KFTSTR_VIDEO);
		case ED2KFT_IMAGE:		return wxT(ED2KFTSTR_IMAGE);
		case ED2KFT_DOCUMENT:	return wxT(ED2KFTSTR_DOCUMENT);
		case ED2KFT_PROGRAM:	return wxT(ED2KFTSTR_PROGRAM);
		case ED2KFT_ARCHIVE:	return wxT(ED2KFTSTR_ARCHIVE);
		case ED2KFT_CDIMAGE:	return wxT(ED2KFTSTR_CDIMAGE);
		default:				return wxT("");
	}
}

// Returns a file type which is used eMule internally only (GUI)
CString GetFileTypeDisplayStrFromED2KFileType(LPCTSTR pszED2KFileType)
{
	ASSERT( pszED2KFileType != NULL );
	if (pszED2KFileType != NULL)
	{
		if (_tcscmp(pszED2KFileType, wxT(ED2KFTSTR_AUDIO)) == 0)			return GetResString(IDS_SEARCH_AUDIO);
		else if (_tcscmp(pszED2KFileType, wxT(ED2KFTSTR_VIDEO)) == 0)    return GetResString(IDS_SEARCH_VIDEO);
		else if (_tcscmp(pszED2KFileType, wxT(ED2KFTSTR_IMAGE)) == 0)    return GetResString(IDS_SEARCH_PICS);
		else if (_tcscmp(pszED2KFileType, wxT(ED2KFTSTR_DOCUMENT)) == 0)	return GetResString(IDS_SEARCH_DOC);
		else if (_tcscmp(pszED2KFileType, wxT(ED2KFTSTR_PROGRAM)) == 0)  return GetResString(IDS_SEARCH_PRG);
		else if (_tcscmp(pszED2KFileType, wxT(ED2KFTSTR_ARCHIVE)) == 0)	return GetResString(IDS_SEARCH_ARC);
		else if (_tcscmp(pszED2KFileType, wxT(ED2KFTSTR_CDIMAGE)) == 0)  return GetResString(IDS_SEARCH_CDIMG);
	}
	return wxT("");
}

class CED2KFileTypes{
public:
	CED2KFileTypes(){
		qsort(_aED2KFileTypes, ARRSIZE(_aED2KFileTypes), sizeof _aED2KFileTypes[0], CompareE2DKFileType);
#ifdef _DEBUG
		// check for duplicate entries
		LPCTSTR pszLast = _aED2KFileTypes[0].pszExt;
		for (int i = 1; i < ARRSIZE(_aED2KFileTypes); i++){
			ASSERT( _tcscmp(pszLast, _aED2KFileTypes[i].pszExt) != 0 );
			pszLast = _aED2KFileTypes[i].pszExt;
		}
#endif
	}
};
CED2KFileTypes theED2KFileTypes; // get the list sorted *before* any code is accessing it


#endif

// DumpMem ... Dumps mem ;)
void DumpMem(const void* where, uint32 size) {
	for (uint32 i = 0; i< size; ++i) {
		printf("|%2x",(uint8)((char*)where)[i]);
		if ((i % 16) == 15) {
			printf("\n");
		}			
	}	
	printf("\n");	
}

//
// Dump mem in dword format
void DumpMem_DW(const uint32 *ptr, int count)
{
	for(int i = 0; i < count; i++) {
		printf("%08x ", ptr[i]);
		if ( (i % 4) == 3) printf("\n");
	}
	printf("\n");
}

/*
 * RLE encoder implementation. This is RLE implementation for very specific
 * purpose: encode DIFFERENCE between subsequent states of status bar.
 * 
 * This difference is calculated by xor-ing with previous data
 * 
 * We can't use implementation with "control char" since this encoder
 * will process binary data - not ascii (or unicode) strings
 */
 
RLE_Data::RLE_Data(int len, bool use_diff)
{
	// there's no point encoding with len <=3 
	//wxASSERT((len > 3));
	
	m_len = len;
	m_use_diff = use_diff;
	
	m_buff = new unsigned char[m_len];
	memset(m_buff, 0, m_len);
	//
	// in worst case 2-byte sequence encoded as 3. So, data can grow at 1/3
	m_enc_buff = new unsigned char[m_len*4/3 + 1];
}

RLE_Data::RLE_Data()
{
	m_buff = 0;
	m_enc_buff = 0;
	m_len = 0;
	m_use_diff = 0;
}

RLE_Data::RLE_Data(const RLE_Data &obj)
{
	m_len = obj.m_len;
	m_use_diff = obj.m_use_diff;

	m_buff = new unsigned char[m_len];
	memcpy(m_buff, obj.m_buff, m_len);
	
	m_enc_buff = new unsigned char[m_len*4/3 + 1];
}

RLE_Data &RLE_Data::operator=(const RLE_Data &obj)
{
	m_len = obj.m_len;
	
	m_use_diff = obj.m_use_diff;

	m_buff = new unsigned char[m_len];
	memcpy(m_buff, obj.m_buff, m_len);
	
	m_enc_buff = new unsigned char[m_len*4/3 + 1];
	
	return *this;
}

RLE_Data::~RLE_Data()
{
	if ( m_buff ) {
		delete [] m_buff;
	}
	if ( m_enc_buff ) {
		delete [] m_enc_buff;
	}
}

void RLE_Data::Realloc(int size)
{
	if ( size == m_len ) {
		return;
	}

	unsigned char *buff = new unsigned char[size];
	if ( size > m_len ) {
		memset(buff + m_len, 0, size - m_len);
		memcpy(buff, m_buff, m_len);
	} else {
		memcpy(buff, m_buff, size);
	}
	delete [] m_buff;
	m_buff = buff;
	
	buff = new unsigned char[size*4/3 + 1];
	if ( size > m_len ) {
		memset(buff + m_len*4/3 + 1, 0, (size - m_len)*4/3);
		memcpy(buff, m_enc_buff, m_len*4/3 + 1);
	} else {
		memcpy(buff, m_enc_buff, size*4/3 + 1);
	}
	delete [] m_enc_buff;
	m_enc_buff = buff;

	m_len = size;
}

const unsigned char *RLE_Data::Decode(const unsigned char *buff, int len)
{
	//
	// Open RLE
	//

	int i = 0, j = 0;
	while ( j != m_len ) {

		if ( i < (len -1) ) {
			if (buff[i+1] == buff[i]) {
				// this is sequence
				memset(m_enc_buff + j, buff[i], buff[i + 2]);
				j += buff[i + 2];
				i += 3;
			} else {
				// this is single byte
				m_enc_buff[j++] = buff[i++];
			}
		} else {
			// only 1 byte left in encoded data - it can't be sequence
			m_enc_buff[j++] = buff[i++];
			// if there's no more data, but buffer end is not reached,
			// it must be error in some point
			if ( j != m_len ) {
				printf("RLE_Data: decoding error. %d bytes decoded to %d instead of %d\n", len, j, m_len);
			}
			break;
		}
	}	
	//
	// Recreate data from diff
	//
	if ( m_use_diff ) {
		for (int i = 0; i < m_len; i++) {
			m_buff[i] ^= m_enc_buff[i];
		}
	}
		
	return m_buff;
}

void PartFileEncoderData::Decode(unsigned char *gapdata, int gaplen, unsigned char *partdata, int partlen)
{
	m_part_status.Decode(partdata, partlen);

	// in a first dword - real size
	uint32 gapsize = ENDIAN_NTOHL(*((uint32 *)gapdata));
	gapdata += sizeof(uint32);
	m_gap_status.Realloc(gapsize*2*sizeof(uint32));

	m_gap_status.Decode(gapdata, gaplen - sizeof(uint32));
}

} // End namespace
