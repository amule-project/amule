//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "BitVector.h"		// Needed for BitVector

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
	
	ver += wxT(" compiled with ");

	
	// Figure out the wx build-type
	#if   defined(__WXGTK20__)
		ver += wxT("wxGTK2");
	#elif defined(__WXGTK__)
		ver += wxT("wxGTK1");
	// 2.9 has different builds for OSX: Carbon and Cocoa
	#elif defined(__WXOSX_CARBON__)
		ver += wxT("wxOSX Carbon");
	#elif defined(__WXOSX_COCOA__)
		ver += wxT("wxOSX Cocoa");
	// different Cocoa port, "not been updated very actively since beginning 2008"
	#elif defined(__WXCOCOA__)
		ver += wxT("wxCocoa");
	// 2.8 Mac
	#elif defined(__WXMAC__)
		ver += wxT("wxMac");
	#elif defined(__WXMSW__) && defined(__VISUALC__)
		ver += wxT("wxMSW VC");
	#elif defined(__WXMSW__)
		ver += wxT("wxMSW");
	#endif

	ver += CFormat(wxT(" v%d.%d.%d")) % wxMAJOR_VERSION % wxMINOR_VERSION % wxRELEASE_NUMBER;

#ifdef __WXDEBUG__
	ver += wxT(" (Debugging)");
#endif
	
#ifdef SVNDATE
	ver += CFormat(wxT(" (Snapshot: %s)")) % wxT(SVNDATE);
#endif
	
	return ver;
}


// Formats a filesize in bytes to make it suitable for displaying
wxString CastItoXBytes( uint64 count )
{

	if (count < 1024)
		return CFormat(wxT("%u ")) % count + wxPLURAL("byte", "bytes", count) ;
	else if (count < 1048576)
		return CFormat(wxT("%u ")) % (count >> 10) + _("kB") ;
	else if (count < 1073741824)
		return CFormat(wxT("%.2f ")) % ((float)(uint32)count/1048576) + _("MB") ;
	else if (count < 1099511627776LL)
		return CFormat(wxT("%.3f ")) % ((float)((uint32)(count/1024))/1048576) + _("GB") ;
	else
		return CFormat(wxT("%.3f ")) % ((float)count/1099511627776LL) + _("TB") ;
}


wxString CastItoIShort(uint64 count)
{

	if (count < 1000)
		return CFormat(wxT("%u")) % count;
	else if (count < 1000000)
		return CFormat(wxT("%.0f")) % ((float)(uint32)count/1000) + _("k") ;
	else if (count < 1000000000)
		return CFormat(wxT("%.2f")) % ((float)(uint32)count/1000000) + _("M") ;
	else if (count < 1000000000000LL)
		return CFormat(wxT("%.2f")) % ((float)((uint32)(count/1000))/1000000) + _("G") ;
	else
		return CFormat(wxT("%.2f")) % ((float)count/1000000000000LL) + _("T");
}


wxString CastItoSpeed(uint32 bytes)
{
	if (bytes < 1024)
		return CFormat(wxT("%u ")) % bytes + wxPLURAL("byte/sec", "bytes/sec", bytes);
	else if (bytes < 1048576)
		return CFormat(wxT("%.2f ")) % (bytes / 1024.0) + _("kB/s");
	else
		return CFormat(wxT("%.2f ")) % (bytes / 1048576.0) + _("MB/s");
}


// Make a time value in seconds suitable for displaying
wxString CastSecondsToHM(uint32 count, uint16 msecs)
{
	if (count < 60) {
		if (!msecs) {
			return CFormat(wxT("%02u %s")) % count % _("secs");
		} else {
			return CFormat(wxT("%.3f %s"))
				% (count + ((float)msecs/1000)) % _("secs");
		}
	} else if (count < 3600) {
		return CFormat(wxT("%u:%02u %s")) 
			% (count/60) % (count % 60) % _("mins");
	} else if (count < 86400) {
		return CFormat(wxT("%u:%02u %s"))
			% (count/3600) % ((count % 3600)/60) % _("hours");
	} else {
		return CFormat(wxT("%u %s %02u:%02u %s"))
			% (count/86400) % _("Days")
			% ((count % 86400)/3600) % ((count % 3600)/60) % _("hours");
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
 * Copyright (c) 2002-2011 Matthias Gaertner
 * Adapted to use wxWidgets by
 * Copyright (c) 2005-2011 Marcelo Roberto Jimenez ( phoenix@amule.org )
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
wxString GetCatTitle(AllCategoryFilter cat)
{
	switch (cat) {
		case acfAll:	 	 return _("all");
		case acfAllOthers:   return _("all others");
		case acfIncomplete:	 return _("Incomplete");
		case acfCompleted:	 return _("Completed");
		case acfWaiting:	 return _("Waiting");
		case acfDownloading: return _("Downloading");
		case acfErroneous:	 return _("Erroneous");
		case acfPaused:		 return _("Paused");
		case acfStopped:	 return _("Stopped");		
		case acfVideo:		 return _("Video");
		case acfAudio:		 return _("Audio");
		case acfArchive:	 return _("Archive");
		case acfCDImages:	 return _("CD-Images");
		case acfPictures:	 return _("Pictures");
		case acfText:		 return _("Text");
		case acfActive:		 return _("Active");		
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
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".669"),   ED2KFT_AUDIO));		// 8 channel tracker module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".aac"),   ED2KFT_AUDIO));		// Advanced Audio Coding File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ac3"),   ED2KFT_AUDIO));		// Audio Codec 3 File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".aif"),   ED2KFT_AUDIO));		// Audio Interchange File Format
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".aifc"),  ED2KFT_AUDIO));		// Audio Interchange File Format
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".aiff"),  ED2KFT_AUDIO));		// Audio Interchange File Format
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".amf"),   ED2KFT_AUDIO));		// DSMI Advanced Module Format
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".amr"),   ED2KFT_AUDIO));		// Adaptive Multi-Rate Codec File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ams"),   ED2KFT_AUDIO));		// Extreme Tracker Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ape"),   ED2KFT_AUDIO));		// Monkey's Audio Lossless Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".au"),    ED2KFT_AUDIO));		// Audio File (Sun, Unix)
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".aud"),   ED2KFT_AUDIO));		// General Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".audio"), ED2KFT_AUDIO));		// General Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".cda"),   ED2KFT_AUDIO));		// CD Audio Track
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dbm"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dmf"),   ED2KFT_AUDIO));		// Delusion Digital Music File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dsm"),   ED2KFT_AUDIO));		// Digital Sound Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dts"),   ED2KFT_AUDIO));		// DTS Encoded Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".far"),   ED2KFT_AUDIO));		// Farandole Composer Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".flac"),  ED2KFT_AUDIO));		// Free Lossless Audio Codec File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".it"),    ED2KFT_AUDIO));		// Impulse Tracker Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".m1a"),   ED2KFT_AUDIO));		// MPEG-1 Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".m2a"),   ED2KFT_AUDIO));		// MPEG-2 Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".m4a"),   ED2KFT_AUDIO));		// MPEG-4 Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mdl"),   ED2KFT_AUDIO));		// DigiTrakker Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".med"),   ED2KFT_AUDIO));		// Amiga MED Sound File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mid"),   ED2KFT_AUDIO));		// MIDI File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".midi"),  ED2KFT_AUDIO));		// MIDI File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mka"),   ED2KFT_AUDIO));		// Matroska Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mod"),   ED2KFT_AUDIO));		// Amiga Music Module File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mol"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mp1"),   ED2KFT_AUDIO));		// MPEG-1 Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mp2"),   ED2KFT_AUDIO));		// MPEG-2 Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mp3"),   ED2KFT_AUDIO));		// MPEG-3 Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpa"),   ED2KFT_AUDIO));		// MPEG Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpc"),   ED2KFT_AUDIO));		// Musepack Compressed Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpp"),   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mtm"),   ED2KFT_AUDIO));		// MultiTracker Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".nst"),   ED2KFT_AUDIO));		// NoiseTracker
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ogg"),   ED2KFT_AUDIO));		// Ogg Vorbis Compressed Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".okt"),   ED2KFT_AUDIO));		// Oktalyzer Module (Amiga)
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".psm"),   ED2KFT_AUDIO));		// Protracker Studio Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ptm"),   ED2KFT_AUDIO));		// PolyTracker Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ra"),    ED2KFT_AUDIO));		// Real Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".rmi"),   ED2KFT_AUDIO));		// MIDI File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".s3m"),   ED2KFT_AUDIO));		// Scream Tracker 3 Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".snd"),   ED2KFT_AUDIO));		// Audio File (Sun, Unix)
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".stm"),   ED2KFT_AUDIO));		// Scream Tracker 2 Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ult"),   ED2KFT_AUDIO));		// UltraTracker
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".umx"),   ED2KFT_AUDIO));		// Unreal Music Package
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wav"),   ED2KFT_AUDIO));		// WAVE Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wma"),   ED2KFT_AUDIO));		// Windows Media Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wow"),   ED2KFT_AUDIO));		// Grave Composer audio tracker
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".xm"),    ED2KFT_AUDIO));		// Fasttracker 2 Extended Module

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".3g2"),   ED2KFT_VIDEO));		// 3GPP Multimedia File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".3gp"),   ED2KFT_VIDEO));		// 3GPP Multimedia File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".3gp2"),  ED2KFT_VIDEO));		// 3GPP Multimedia File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".3gpp"),  ED2KFT_VIDEO));		// 3GPP Multimedia File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".asf"),   ED2KFT_VIDEO));		// Advanced Systems Format (MS)
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".amv"),   ED2KFT_VIDEO));		// Anime Music Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".asf"),   ED2KFT_VIDEO));		// Advanced Systems Format File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".avi"),   ED2KFT_VIDEO));		// Audio Video Interleave File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bik"),   ED2KFT_VIDEO));		// BINK Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".divx"),  ED2KFT_VIDEO));		// DivX-Encoded Movie File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dvr-ms"),ED2KFT_VIDEO));		// Microsoft Digital Video Recording
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".flc"),   ED2KFT_VIDEO));		// FLIC Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".fli"),   ED2KFT_VIDEO));		// FLIC Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".flic"),  ED2KFT_VIDEO));		// FLIC Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".flv"),   ED2KFT_VIDEO));		// Flash Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".hdmov"), ED2KFT_VIDEO));		// High-Definition QuickTime Movie
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ifo"),   ED2KFT_VIDEO));		// DVD-Video Disc Information File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".m1v"),   ED2KFT_VIDEO));		// MPEG-1 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".m2t"),   ED2KFT_VIDEO));		// MPEG-2 Video Transport Stream
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".m2ts"),  ED2KFT_VIDEO));		// MPEG-2 Video Transport Stream
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".m2v"),   ED2KFT_VIDEO));		// MPEG-2 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".m4b"),   ED2KFT_VIDEO));		// MPEG-4 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".m4v"),   ED2KFT_VIDEO));		// MPEG-4 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mkv"),   ED2KFT_VIDEO));		// Matroska Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mov"),   ED2KFT_VIDEO));		// QuickTime Movie File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".movie"), ED2KFT_VIDEO));		// QuickTime Movie File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mp1v"),  ED2KFT_VIDEO));		// QuickTime Movie File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mp2v"),  ED2KFT_VIDEO));		// MPEG-1 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mp4"),   ED2KFT_VIDEO));		// MPEG-2 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpe"),   ED2KFT_VIDEO));		// MPEG-4 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpeg"),  ED2KFT_VIDEO));		// MPEG Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpg"),   ED2KFT_VIDEO));		// MPEG Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mps"),   ED2KFT_VIDEO));		// MPEG Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpv"),   ED2KFT_VIDEO));		// MPEG Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpv1"),  ED2KFT_VIDEO));		// MPEG-1 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mpv2"),  ED2KFT_VIDEO));		// MPEG-2 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ogm"),   ED2KFT_VIDEO));		// Ogg Media File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".pva"),   ED2KFT_VIDEO));		// MPEG Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".qt"),    ED2KFT_VIDEO));		// QuickTime Movie
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ram"),   ED2KFT_VIDEO));		// Real Audio Media
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ratdvd"),ED2KFT_VIDEO));		// RatDVD Disk Image
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".rm"),    ED2KFT_VIDEO));		// Real Media File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".rmm"),   ED2KFT_VIDEO));		// Real Media File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".rmvb"),  ED2KFT_VIDEO));		// Real Video Variable Bit Rate File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".rv"),    ED2KFT_VIDEO));		// Real Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".rv9"),   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".smil"),  ED2KFT_VIDEO));		// SMIL Presentation File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".smk"),   ED2KFT_VIDEO));		// Smacker Compressed Movie File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".swf"),   ED2KFT_VIDEO));		// Macromedia Flash Movie
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".tp"),    ED2KFT_VIDEO));		// Video Transport Stream File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ts"),    ED2KFT_VIDEO));		// Video Transport Stream File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".vid"),   ED2KFT_VIDEO));		// General Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".video"), ED2KFT_VIDEO));		// General Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".vivo"),  ED2KFT_VIDEO));		// VivoActive Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".vob"),   ED2KFT_VIDEO));		// DVD Video Object File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".vp6"),   ED2KFT_VIDEO));		// TrueMotion VP6 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wm"),    ED2KFT_VIDEO));		// Windows Media Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wmv"),   ED2KFT_VIDEO));		// Windows Media Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".xvid"),  ED2KFT_VIDEO));		// Xvid-Encoded Video File

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bmp"),   ED2KFT_IMAGE));		// Bitmap Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dcx"),   ED2KFT_IMAGE));		// FAXserve Fax Document
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".emf"),   ED2KFT_IMAGE));		// Enhanced Windows Metafile
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".gif"),   ED2KFT_IMAGE));		// Graphical Interchange Format File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ico"),   ED2KFT_IMAGE));		// Icon File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".jfif"),  ED2KFT_IMAGE));		// JPEG File Interchange Format
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".jpe"),   ED2KFT_IMAGE));		// JPEG Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".jpeg"),  ED2KFT_IMAGE));		// JPEG Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".jpg"),   ED2KFT_IMAGE));		// JPEG Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".pct"),   ED2KFT_IMAGE));		// PICT Picture File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".pcx"),   ED2KFT_IMAGE));		// Paintbrush Bitmap Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".pic"),   ED2KFT_IMAGE));		// PICT Picture File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".pict"),  ED2KFT_IMAGE));		// PICT Picture File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".png"),   ED2KFT_IMAGE));		// Portable Network Graphic
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".psd"),   ED2KFT_IMAGE));		// Photoshop Document
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".psp"),   ED2KFT_IMAGE));		// Paint Shop Pro Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".tga"),   ED2KFT_IMAGE));		// Targa Graphic
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".tif"),   ED2KFT_IMAGE));		// Tagged Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".tiff"),  ED2KFT_IMAGE));		// Tagged Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wmf"),   ED2KFT_IMAGE));		// Windows Metafile
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wmp"),   ED2KFT_IMAGE));		// Windows Media Photo File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".xif"),   ED2KFT_IMAGE));		// ScanSoft Pagis Extended Image Format File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".xpm"),   ED2KFT_IMAGE));		// X-Windows Pixmap

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".7z"),    ED2KFT_ARCHIVE));	// 7-Zip Compressed File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ace"),   ED2KFT_ARCHIVE));	// WinAce Compressed File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".alz"),   ED2KFT_ARCHIVE));	// ALZip Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".arc"),   ED2KFT_ARCHIVE));	// Compressed File Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".arj"),   ED2KFT_ARCHIVE));	// ARJ Compressed File Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bz2"),   ED2KFT_ARCHIVE));	// Bzip Compressed File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".cab"),   ED2KFT_ARCHIVE));	// Cabinet File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".cbr"),   ED2KFT_ARCHIVE));	// Comic Book RAR Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".cbz"),   ED2KFT_ARCHIVE));	// Comic Book ZIP Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".gz"),    ED2KFT_ARCHIVE));	// Gnu Zipped File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".hqx"),   ED2KFT_ARCHIVE));	// BinHex 4.0 Encoded File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".lha"),   ED2KFT_ARCHIVE));	// LHARC Compressed Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".lzh"),   ED2KFT_ARCHIVE));	// LZH Compressed File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".msi"),   ED2KFT_ARCHIVE));	// Microsoft Installer File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".pak"),   ED2KFT_ARCHIVE));	// PAK (Packed) File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".par"),   ED2KFT_ARCHIVE));	// Parchive Index File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".par2"),  ED2KFT_ARCHIVE));	// Parchive 2 Index File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".rar"),   ED2KFT_ARCHIVE));	// WinRAR Compressed Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".sea"),   ED2KFT_ARCHIVE));	// Self-Extracting Archive (Mac)
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".sit"),   ED2KFT_ARCHIVE));	// Stuffit Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".sitx"),  ED2KFT_ARCHIVE));	// Stuffit X Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".tar"),   ED2KFT_ARCHIVE));	// Consolidated Unix File Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".tbz2"),  ED2KFT_ARCHIVE));	// Tar BZip 2 Compressed File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".tgz"),   ED2KFT_ARCHIVE));	// Gzipped Tar File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".uc2"),   ED2KFT_ARCHIVE));	// UltraCompressor 2 Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".xpi"),   ED2KFT_ARCHIVE));	// Mozilla Installer Package
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".z"),     ED2KFT_ARCHIVE));	// Unix Compressed File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".zip"),   ED2KFT_ARCHIVE));	// Zipped File

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bat"),   ED2KFT_PROGRAM));	// Batch File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".cmd"),   ED2KFT_PROGRAM));	// Command File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".com"),   ED2KFT_PROGRAM));	// COM File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".exe"),   ED2KFT_PROGRAM));	// Executable File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".hta"),   ED2KFT_PROGRAM));	// HTML Application
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".js"),    ED2KFT_PROGRAM));	// Java Script
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".jse"),   ED2KFT_PROGRAM));	// Encoded  Java Script
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".msc"),   ED2KFT_PROGRAM));	// Microsoft Common Console File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".vbe"),   ED2KFT_PROGRAM));	// Encoded Visual Basic Script File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".vbs"),   ED2KFT_PROGRAM));	// Visual Basic Script File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wsf"),   ED2KFT_PROGRAM));	// Windows Script File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wsh"),   ED2KFT_PROGRAM));	// Windows Scripting Host File

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bin"),   ED2KFT_CDIMAGE));	// CD Image
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bwa"),   ED2KFT_CDIMAGE));	// BlindWrite Disk Information File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bwi"),   ED2KFT_CDIMAGE));	// BlindWrite CD/DVD Disc Image
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bws"),   ED2KFT_CDIMAGE));	// BlindWrite Sub Code File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".bwt"),   ED2KFT_CDIMAGE));	// BlindWrite 4 Disk Image
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ccd"),   ED2KFT_CDIMAGE));	// CloneCD Disk Image
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".cue"),   ED2KFT_CDIMAGE));	// Cue Sheet File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dmg"),   ED2KFT_CDIMAGE));	// Mac OS X Disk Image
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dmz"),   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".img"),   ED2KFT_CDIMAGE));	// Disk Image Data File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".iso"),   ED2KFT_CDIMAGE));	// Disc Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mdf"),   ED2KFT_CDIMAGE));	// Media Disc Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".mds"),   ED2KFT_CDIMAGE));	// Media Descriptor File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".nrg"),   ED2KFT_CDIMAGE));	// Nero CD/DVD Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".sub"),   ED2KFT_CDIMAGE));	// Subtitle File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".toast"), ED2KFT_CDIMAGE));	// Toast Disc Image

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".chm"),   ED2KFT_DOCUMENT));	// Compiled HTML Help File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".css"),   ED2KFT_DOCUMENT));	// Cascading Style Sheet
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".diz"),   ED2KFT_DOCUMENT));	// Description in Zip File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".doc"),   ED2KFT_DOCUMENT));	// Document File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".dot"),   ED2KFT_DOCUMENT));	// Document Template File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".hlp"),   ED2KFT_DOCUMENT));	// Help File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".htm"),   ED2KFT_DOCUMENT));	// HTML File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".html"),  ED2KFT_DOCUMENT));	// HTML File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".nfo"),   ED2KFT_DOCUMENT));	// Warez Information File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".pdf"),   ED2KFT_DOCUMENT));	// Portable Document Format File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".pps"),   ED2KFT_DOCUMENT));	// PowerPoint Slide Show
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ppt"),   ED2KFT_DOCUMENT));	// PowerPoint Presentation
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".ps"),    ED2KFT_DOCUMENT));	// PostScript File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".rtf"),   ED2KFT_DOCUMENT));	// Rich Text Format File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".text"),  ED2KFT_DOCUMENT));	// General Text File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".txt"),   ED2KFT_DOCUMENT));	// Text File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".wri"),   ED2KFT_DOCUMENT));	// Windows Write Document
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".xls"),   ED2KFT_DOCUMENT));	// Microsoft Excel Spreadsheet
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".xlt"),   ED2KFT_DOCUMENT));	// Microsoft Excel Template
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(wxT(".xml"),   ED2KFT_DOCUMENT));	// XML File
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

	result += CFormat(wxT("%d bytes\n")) % n;
	for ( int i = 0; i < lines; ++i) {
		// Show address
		result += CFormat(wxT("%08x  ")) % (i * 16);
		
		// Show two columns of hex-values
		for ( int j = 0; j < 2; ++j) {
			for ( int k = 0; k < 8; ++k) {
				int pos = 16 * i + 8 * j + k;
				
				if ( pos < n ) {
					result += CFormat(wxT("%02x ")) % p[pos];
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


wxString GetConfigDir(const wxString &configFileBase)
{
	// Cache the path.
	static wxString configPath;

	if (configPath.IsEmpty()) {
		// "Portable aMule" - Use aMule from an external USB drive
		// Check for ./config/amule.conf (or whatever gets passed as configFile)
		// and use this configuration if found
		const wxString configDir = JoinPaths(wxFileName::GetCwd(), wxT("config"));
		const wxString configFile = JoinPaths(configDir, configFileBase);

		if (CPath::DirExists(configDir) && CPath::FileExists(configFile)) {
			AddLogLineN(CFormat(_("Using config dir: %s")) % configDir);

			configPath = configDir;
		} else {
			configPath = wxStandardPaths::Get().GetUserDataDir();
		}

		configPath += wxFileName::GetPathSeparator();
	}

	return configPath;
}


/*************************** Locale specific stuff ***************************/

#ifndef __WXMSW__
#	define	SETWINLANG(LANG, SUBLANG)
#else
#	define	SETWINLANG(LANG, SUBLANG) \
	info.WinLang = LANG; \
	info.WinSublang = SUBLANG;
#endif

#define CUSTOMLANGUAGE(wxid, iso, winlang, winsublang, dir, desc) \
	info.Language = wxid;		\
	info.CanonicalName = wxT(iso);	\
	info.LayoutDirection = dir;	\
	info.Description = wxT(desc);	\
	SETWINLANG(winlang, winsublang)	\
	wxLocale::AddLanguage(info);

void InitCustomLanguages()
{
	wxLanguageInfo info;

#if !wxCHECK_VERSION(2, 9, 0)
	CUSTOMLANGUAGE(wxLANGUAGE_ASTURIAN,	"ast",	0,	0,	wxLayout_LeftToRight,	"Asturian");
#endif
}


void InitLocale(wxLocale& locale, int language)
{
	locale.Init(language, wxLOCALE_LOAD_DEFAULT); 
	
#if defined(__WXMAC__) || defined(__WXMSW__)
	locale.AddCatalogLookupPathPrefix(JoinPaths(wxStandardPaths::Get().GetDataDir(), wxT("locale")));
#endif
	locale.AddCatalog(wxT(PACKAGE));
}


int StrLang2wx(const wxString& language)
{
	// get rid of possible encoding and modifier
	wxString lang(language.BeforeFirst('.').BeforeFirst('@'));

	if (!lang.IsEmpty()) {
		const wxLanguageInfo *lng = wxLocale::FindLanguageInfo(lang);
		if (lng) {
			int langID = lng->Language;
			// Traditional Chinese: original Chinese, used in Taiwan, Hong Kong and Macau.
			// Simplified Chinese: simplified Chinese characters used in Mainland China since 1950s, and in some other places such as Singapore and Malaysia.
			//
			// Chinese (Traditional) contains zh_TW, zh_HK and zh_MO (but there are differences in some words). 
			// Because of most Traditional Chinese user are in Taiwan, zh_TW becomes the representation of Traditional Chinese.
			// Chinese (Simplified) contains zh_CN, zh_SG and zh_MY. In the same reason, zh_CN becomes the representation of Simplified Chinese.
			// (see http://forum.amule.org/index.php?topic=13208.msg98043#msg98043 )
			//
			// wx maps "Traditional Chinese" to "Chinese" however. This must me corrected:
			if (langID == wxLANGUAGE_CHINESE) {
				langID = wxLANGUAGE_CHINESE_TRADITIONAL;
			}
			return langID;
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

/*****************************************************************************/

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


const uint8 BitVector::s_posMask[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
const uint8 BitVector::s_negMask[] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};

// File_checked_for_headers
