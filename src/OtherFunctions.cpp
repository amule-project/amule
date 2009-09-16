//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2009 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

// The backtrace functions contain modified code from libYaMa, (c) Venkatesha Murthy G.
// You can check libYaMa at http://personal.pavanashree.org/libyama/

#include <tags/FileTags.h>

#include <wx/filename.h>	// Needed for wxFileName
#include <wx/log.h>		// Needed for wxLogNull

#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for a number of defines
#endif

#include <wx/stdpaths.h> // Do_not_auto_remove
#include <common/StringFunctions.h>
#include <common/ClientVersion.h>	
#include <common/MD5Sum.h>
#include <common/Path.h>
#include "MD4Hash.h"
#include "Logger.h"

#include "OtherFunctions.h"	// Interface declarations

#include <map>

#ifdef __WXBASE__
	#include <cerrno>
#else
	#include <wx/utils.h>
#endif	


wxString GetMuleVersion()
{
	wxString ver(wxT(VERSION));
	
	ver += wxT(" using ");

	
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
		ver += wxT("wxMSW");
	#elif defined(__WXCOCOA__)
		ver += wxT("wxCocoa");
	#endif

	ver += wxString::Format(wxT(" v%d.%d.%d"), wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER );

#if defined(__WXDEBUG__)
	ver += wxT(" (Debugging)");
#endif
	
#ifdef SVNDATE
	ver += wxString::Format( wxT(" (Snapshot: %s)"), wxT(SVNDATE));
#endif
	
	return ver;
}


wxString GetFullMuleVersion()
{
#ifdef AMULE_DAEMON
	wxString app = wxT("aMuled");
#elif defined(CLIENT_GUI)
	wxString app = wxT("Remote aMule-GUI");
#else
	wxString app = wxT("aMule");
#endif

	return app + wxT(" ") + GetMuleVersion();
}


// Formats a filesize in bytes to make it suitable for displaying
wxString CastItoXBytes( uint64 count )
{

	if (count < 1024)
		return wxString::Format( wxT("%u "), (unsigned)count) + wxPLURAL("byte", "bytes", count) ;
	else if (count < 1048576)
		return wxString::Format( wxT("%u "), (unsigned)count >> 10) + _("kB") ;
	else if (count < 1073741824)
		return wxString::Format( wxT("%.2f "), (float)(uint32)count/1048576) + _("MB") ;
	else if (count < 1099511627776LL)
		return wxString::Format( wxT("%.3f "), (float)((uint32)(count/1024))/1048576) + _("GB") ;
	else
		return wxString::Format( wxT("%.3f "), (float)count/1099511627776LL) + _("TB") ;
}


wxString CastItoIShort(uint64 count)
{

	if (count < 1000)
		return wxString::Format(wxT("%u"), (uint32)count);
	else if (count < 1000000)
		return wxString::Format(wxT("%.0f"),(float)(uint32)count/1000) + _("k") ;
	else if (count < 1000000000)
		return wxString::Format(wxT("%.2f"),(float)(uint32)count/1000000) + _("M") ;
	else if (count < 1000000000000LL)
		return wxString::Format(wxT("%.2f"),(float)((uint32)(count/1000))/1000000) + _("G") ;
	else
		return wxString::Format(wxT("%.2f"),(float)count/1000000000000LL) + _("T");
}


wxString CastItoSpeed(uint32 bytes)
{
	if (bytes < 1024)
		return wxString::Format(wxT("%u "), bytes) + wxPLURAL("byte/sec", "bytes/sec", bytes);
	else if (bytes < 1048576)
		return wxString::Format(wxT("%.2f "), bytes / 1024.0) + _("kB/s");
	else
		return wxString::Format(wxT("%.2f "), bytes / 1048576.0) + _("MB/s");
}


// Make a time value in seconds suitable for displaying
wxString CastSecondsToHM(uint64 count, uint16 msecs)
{
	if (count < 60) {
		if (!msecs) {
			return wxString::Format(
				wxT("%02") wxLongLongFmtSpec wxT("u "),
				count) + _("secs");
		} else {
			return wxString::Format(
				wxT("%.3f"),
				(count + ((float)msecs/1000))) + _("secs");
		}
	} else if (count < 3600) {
		return wxString::Format(
			wxT("%")
			wxLongLongFmtSpec wxT("u:%02")
			wxLongLongFmtSpec wxT("u "), 
			count/60,
			(count % 60)) + _("mins");
	} else if (count < 86400) {
		return wxString::Format(
			wxT("%")
			wxLongLongFmtSpec wxT("u:%02")
			wxLongLongFmtSpec wxT("u "),
			count/3600,
			(count % 3600)/60) + _("hours");
	} else {
		return wxString::Format(
			wxT("%")
			wxLongLongFmtSpec wxT("u %s %02")
			wxLongLongFmtSpec wxT("u:%02")
			wxLongLongFmtSpec wxT("u "), 
			count/86400,
			_("Days"),
			(count % 86400)/3600,
			(count % 3600)/60) + _("hours");
	}
}


// Examines a filename and determines the filetype
FileType GetFiletype(const CPath& filename)
{
	// FIXME: WTF do we have two such functions in the first place?
	switch (GetED2KFileTypeID(filename)) {
		case ED2KFT_AUDIO:	return ftAudio;
		case ED2KFT_VIDEO:	return ftVideo;
		case ED2KFT_IMAGE:	return ftPicture;
		case ED2KFT_PROGRAM:	return ftProgram;
		case ED2KFT_DOCUMENT:	return ftText;
		case ED2KFT_ARCHIVE:	return ftArchive;
		case ED2KFT_CDIMAGE:	return ftCDImage;
		default:		return ftAny;
	}
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

wxString GetFiletypeByName(const CPath& filename, bool translated)
{
	return GetFiletypeDesc(GetFiletype(filename), translated);
}


// Return the text associated with a rating of a file
wxString GetRateString(uint16 rate)
{
	switch ( rate ) {
		case 0: return _("Not rated");
		case 1: return _("Invalid / Corrupt / Fake");
		case 2: return _("Poor");
		case 3: return _("Fair");
		case 4: return _("Good");
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

	uint32 nBits = 0;
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
			*buffer++ = (byte)( nBits >> (nCount - 8) );
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
 * Adapted by (C) 2005-2009 Phoenix to use wxWidgets.
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
		case 15: return _("Active");		
		default: return wxT("?");
	}
}


typedef std::map<wxString, EED2KFileTypeClass> SED2KFileTypeMap;
typedef SED2KFileTypeMap::value_type SED2KFileTypeMapElement;
static SED2KFileTypeMap ED2KFileTypesMap;


class CED2KFileTypes{
public:
	CED2KFileTypes()
	{
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".669"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".aac"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".aif"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".aiff"),  ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".amf"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ams"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ape"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".au"),    ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dbm"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dmf"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dsm"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".far"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".flac"),  ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".it"),    ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mdl"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".med"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mid"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".midi"),  ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mod"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mol"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mp1"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mp2"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mp3"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mp4"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpa"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpc"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpp"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mtm"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".nst"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ogg"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".okt"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".psm"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ptm"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ra"),    ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".rmi"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".s3m"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".stm"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ult"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".umx"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wav"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wma"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wow"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".xm"),    ED2KFT_AUDIO));

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".asf"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".avi"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".divx"),  ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".m1v"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".m2v"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mkv"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mov"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mp1v"),  ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mp2v"),  ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpe"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpeg"),  ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpg"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mps"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpv"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpv1"),  ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpv2"),  ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ogm"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".qt"),    ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ram"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".rm"),    ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".rv"),    ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".rv9"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ts"),    ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".vivo"),  ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".vob"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wmv"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".xvid"),  ED2KFT_VIDEO));

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bmp"),   ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dcx"),   ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".emf"),   ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".gif"),   ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ico"),   ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".jpeg"),  ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".jpg"),   ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".pct"),   ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".pcx"),   ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".pic"),   ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".pict"),  ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".png"),   ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".psd"),   ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".psp"),   ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".tga"),   ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".tif"),   ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".tiff"),  ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wmf"),   ED2KFT_IMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".xif"),   ED2KFT_IMAGE));

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".7z"),    ED2KFT_ARCHIVE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ace"),   ED2KFT_ARCHIVE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".arj"),   ED2KFT_ARCHIVE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bz2"),   ED2KFT_ARCHIVE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".cab"),   ED2KFT_ARCHIVE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".gz"),    ED2KFT_ARCHIVE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".hqx"),   ED2KFT_ARCHIVE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".lha"),   ED2KFT_ARCHIVE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".msi"),   ED2KFT_ARCHIVE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".rar"),   ED2KFT_ARCHIVE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".sea"),   ED2KFT_ARCHIVE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".sit"),   ED2KFT_ARCHIVE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".tar"),   ED2KFT_ARCHIVE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".tgz"),   ED2KFT_ARCHIVE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".uc2"),   ED2KFT_ARCHIVE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".zip"),   ED2KFT_ARCHIVE));

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bat"),   ED2KFT_PROGRAM));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".cmd"),   ED2KFT_PROGRAM));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".com"),   ED2KFT_PROGRAM));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".exe"),   ED2KFT_PROGRAM));

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bin"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bwa"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bwi"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bws"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bwt"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ccd"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".cue"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dmg"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dmz"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".img"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".iso"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mdf"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mds"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".nrg"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".sub"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".toast"), ED2KFT_CDIMAGE));

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".chm"),   ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".css"),   ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".diz"),   ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".doc"),   ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dot"),   ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".hlp"),   ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".htm"),   ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".html"),  ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".nfo"),   ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".pdf"),   ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".pps"),   ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ppt"),   ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ps"),    ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".rtf"),   ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wri"),   ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".txt"),   ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".xls"),   ED2KFT_DOCUMENT));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".xlt"),   ED2KFT_DOCUMENT));
	}
};


// get the list initialized *before* any code is accessing it
CED2KFileTypes theED2KFileTypes;

EED2KFileType GetED2KFileTypeID(const CPath& fileName)
{
	const wxString ext = fileName.GetExt().Lower();
	if (ext.IsEmpty()) {
		return ED2KFT_ANY;
	}
	
	SED2KFileTypeMap::iterator it = ED2KFileTypesMap.find(wxT(".") + ext);
	if (it != ED2KFileTypesMap.end()) {
		return it->second.GetType();
	} else {
		return ED2KFT_ANY;
	}
}


// Retuns the ed2k file type term which is to be used in server searches
wxString GetED2KFileTypeSearchTerm(EED2KFileType iFileID)
{
	if (iFileID == ED2KFT_AUDIO)		return ED2KFTSTR_AUDIO;
	if (iFileID == ED2KFT_VIDEO)		return ED2KFTSTR_VIDEO;
	if (iFileID == ED2KFT_IMAGE)		return ED2KFTSTR_IMAGE;
	if (iFileID == ED2KFT_DOCUMENT)		return ED2KFTSTR_DOCUMENT;
	if (iFileID == ED2KFT_PROGRAM)		return ED2KFTSTR_PROGRAM;
	// NOTE: Archives and CD-Images are published with file type "Pro"
	if (iFileID == ED2KFT_ARCHIVE)		return ED2KFTSTR_PROGRAM;
	if (iFileID == ED2KFT_CDIMAGE)		return ED2KFTSTR_PROGRAM;

	return wxEmptyString;
}


// Returns a file type which is used eMule internally only, examining the extention of the given filename
wxString GetFileTypeByName(const CPath& fileName)
{
	EED2KFileType iFileType = GetED2KFileTypeID(fileName);
	switch (iFileType) {
		case ED2KFT_AUDIO:	return ED2KFTSTR_AUDIO;
		case ED2KFT_VIDEO:	return ED2KFTSTR_VIDEO;
		case ED2KFT_IMAGE:	return ED2KFTSTR_IMAGE;
		case ED2KFT_DOCUMENT:	return ED2KFTSTR_DOCUMENT;
		case ED2KFT_PROGRAM:	return ED2KFTSTR_PROGRAM;
		case ED2KFT_ARCHIVE:	return ED2KFTSTR_ARCHIVE;
		case ED2KFT_CDIMAGE:	return ED2KFTSTR_CDIMAGE;
		default:		return wxEmptyString;
	}
}


// Retuns the ed2k file type integer ID which is to be used for publishing+searching
EED2KFileType GetED2KFileTypeSearchID(EED2KFileType iFileID)
{
	switch (iFileID) {
		case ED2KFT_AUDIO:	return ED2KFT_AUDIO;
		case ED2KFT_VIDEO:	return ED2KFT_VIDEO;
		case ED2KFT_IMAGE:	return ED2KFT_IMAGE;
		case ED2KFT_DOCUMENT:	return ED2KFT_DOCUMENT;
		case ED2KFT_PROGRAM:	return ED2KFT_PROGRAM;
		// NOTE: Archives and CD-Images are published+searched with file type "Pro"
		// NOTE: If this gets changed, the function 'GetED2KFileTypeSearchTerm' also needs to get updated!
		case ED2KFT_ARCHIVE:	return ED2KFT_PROGRAM;
		case ED2KFT_CDIMAGE:	return ED2KFT_PROGRAM;
		default:		return  ED2KFT_ANY;
	}
}


/**
 * Dumps a buffer to a wxString
 */
wxString DumpMemToStr(const void *buff, int n, const wxString& msg, bool ok)
{
	const unsigned char *p = (const unsigned char *)buff;
	int lines = (n + 15)/ 16;
	
	wxString result;
	// Allocate aproximetly what is needed
	result.Alloc( ( lines + 1 ) * 80 ); 
	if ( !msg.IsEmpty() ) {
		result += msg + wxT(" - ok=") + ( ok ? wxT("true, ") : wxT("false, ") );
	}

	result += wxString::Format( wxT("%d bytes\n"), n );
	for ( int i = 0; i < lines; ++i) {
		// Show address
		result += wxString::Format( wxT("%08x  "), i * 16 );
		
		// Show two columns of hex-values
		for ( int j = 0; j < 2; ++j) {
			for ( int k = 0; k < 8; ++k) {
				int pos = 16 * i + 8 * j + k;
				
				if ( pos < n ) {
					result += wxString::Format( wxT("%02x "), p[pos] );
				} else {
					result += wxT("   ");
				}
			}
			result += wxT(" ");
		}
		result += wxT("|");
		// Show a column of ascii-values
		for ( int k = 0; k < 16; ++k) {
			int pos = 16 * i + k;

			if ( pos < n ) {
				if ( isspace( p[pos] ) ) {
					result += wxT(" ");
				} else if ( !isgraph( p[pos] ) ) {
					result += wxT(".");
				} else {
					result += (wxChar)p[pos];
				}
			} else {
				result += wxT(" ");
			}
		}
		result += wxT("|\n");
	}
	result.Shrink();
	
	return result;
}


/**
 * Dumps a buffer to stdout
 */
void DumpMem(const void *buff, int n, const wxString& msg, bool ok)
{
	printf("%s\n", (const char*)unicode2char(DumpMemToStr( buff, n, msg, ok )) );
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


wxString GetConfigDir()
{
	// Cache the path.
	static wxString configPath;

	if (configPath.IsEmpty()) {
#ifndef EC_REMOTE
		// "Portable aMule" - Use aMule from an external USB drive
		// Check for ./config/amule.conf and use this configuration if found
		const wxString configDir = JoinPaths(wxFileName::GetCwd(), wxT("config"));
		const wxString configFile = JoinPaths(configDir, wxT("amule.conf"));

		if (CPath::DirExists(configDir) && CPath::FileExists(configFile)) {
			AddLogLineM(true, CFormat(wxT("Using configDir: %s")) % configDir);

			configPath = configDir;
		} else {
			configPath = wxStandardPaths::Get().GetUserDataDir();
		}
#else
		configPath = wxStandardPaths::Get().GetUserDataDir();
#endif

		configPath += wxFileName::GetPathSeparator();
	}

	return configPath;
}


void InitCustomLanguages()
{
	wxLanguageInfo CustomLanguage;
	CustomLanguage.Language = wxLANGUAGE_ITALIAN_NAPOLITAN;
	CustomLanguage.CanonicalName = wxT("it_NA");
	CustomLanguage.Description = wxT("sNeo's Custom Napolitan Language");
	wxLocale::AddLanguage(CustomLanguage);

	CustomLanguage.Language = wxLANGUAGE_ASTURIAN;
	CustomLanguage.CanonicalName = wxT("ast_ES");
	CustomLanguage.Description = wxT("Asturian");
	wxLocale::AddLanguage(CustomLanguage);
}


void InitLocale(wxLocale& locale, int language)
{
	int language_flags = 0;
	if ((wxLANGUAGE_CUSTOM != language) && 
		(wxLANGUAGE_ASTURIAN != language) &&
		(wxLANGUAGE_ITALIAN_NAPOLITAN != language)) {
		language_flags = wxLOCALE_LOAD_DEFAULT | wxLOCALE_CONV_ENCODING;
	}
	
	locale.Init(language,language_flags); 
	
	if (language != wxLANGUAGE_CUSTOM) {

#if defined(__WXMAC__) || defined(__WXMSW__)
		locale.AddCatalogLookupPathPrefix(JoinPaths(wxStandardPaths::Get().GetDataDir(), wxT("locale")));
#endif
		locale.AddCatalog(wxT(PACKAGE));

	} else {
		locale.AddCatalogLookupPathPrefix(GetConfigDir());
		locale.AddCatalog(wxT("custom"));
	}
}


int StrLang2wx(const wxString& language)
{
	// get rid of possible encoding and modifier
	wxString lang(language.BeforeFirst('.').BeforeFirst('@'));

	if (!lang.IsEmpty()) {
		const wxLanguageInfo *lng = wxLocale::FindLanguageInfo(lang);
		if (lng) {
			return lng->Language;
		} else {
			return wxLANGUAGE_DEFAULT;
		}
	} else {
		return wxLANGUAGE_DEFAULT;
	}
}


wxString wxLang2Str(const int lang)
{
	if (lang != wxLANGUAGE_DEFAULT) {
		const wxLanguageInfo *lng = wxLocale::GetLanguageInfo(lang);
		if (lng) {
			return lng->CanonicalName;
		} else {
			return wxEmptyString;
		}
	} else {
		return wxEmptyString;
	}
}

wxString GetPassword() {
wxString pass_plain;
CMD4Hash password;
		#ifndef __WXMSW__
			pass_plain = char2unicode(getpass("Enter password for mule connection: "));
		#else
			//#warning This way, pass enter is not hidden on windows. Bad thing.
			char temp_str[512];
			fflush(stdin);
			printf("Enter password for mule connection: \n");
			fflush(stdout);
			fgets(temp_str, 512, stdin);
			temp_str[strlen(temp_str)-1] = '\0';
			pass_plain = char2unicode(temp_str);
		#endif
		wxCHECK2(password.Decode(MD5Sum(pass_plain).GetHash()), /* Do nothing. */ );
		// MD5 hash for an empty string, according to rfc1321.
		if (password.Encode() == wxT("D41D8CD98F00B204E9800998ECF8427E")) {
			printf("No empty password allowed.\n");
			return GetPassword();
		}


return password.Encode();
}

// File_checked_for_headers
