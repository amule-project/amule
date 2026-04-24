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

#include <wx/filename.h>			// Needed for wxFileName
#include <wx/log.h>				// Needed for wxLogNull
#include "config.h"				// Needed for a number of defines

#include <wx/stdpaths.h>			// Do_not_auto_remove
#include <common/StringFunctions.h>
#include <common/ClientVersion.h>
#include <common/MD5Sum.h>
#include <common/Path.h>
#include "Logger.h"
#include "BitVector.h"				// Needed for BitVector

#include "OtherFunctions.h"			// Interface declarations

#include <map>

#ifdef __WXBASE__
	#include <cerrno>
#else
	#include <wx/utils.h>
#endif




// Formats a filesize in bytes to make it suitable for displaying
wxString CastItoXBytes( uint64 count )
{

	if (count < 1024)
		return wxString(CFormat("%u ") % count) + wxPLURAL("byte", "bytes", count) ;
	else if (count < 1048576)
		return wxString(CFormat("%u ") % (count >> 10)) + _("kB") ;
	else if (count < 1073741824)
		return wxString(CFormat("%.2f ") % ((float)(uint32)count/1048576)) + _("MB") ;
	else if (count < 1099511627776LL)
		return wxString(CFormat("%.3f ") % ((float)((uint32)(count/1024))/1048576)) + _("GB") ;
	else
		return wxString(CFormat("%.3f ") % ((float)count/1099511627776LL)) + _("TB") ;
}


wxString CastItoIShort(uint64 count)
{

	if (count < 1000)
		return CFormat("%u") % count;
	else if (count < 1000000)
		return wxString(CFormat("%.0f") % ((float)(uint32)count/1000)) + _("k") ;
	else if (count < 1000000000)
		return wxString(CFormat("%.2f") % ((float)(uint32)count/1000000)) + _("M") ;
	else if (count < 1000000000000LL)
		return wxString(CFormat("%.2f") % ((float)((uint32)(count/1000))/1000000)) + _("G") ;
	else
		return wxString(CFormat("%.2f") % ((float)count/1000000000000LL)) + _("T");
}


wxString CastItoSpeed(uint32 bytes)
{
	if (bytes < 1024)
		return wxString(CFormat("%u ") % bytes) + wxPLURAL("byte/sec", "bytes/sec", bytes);
	else if (bytes < 1048576)
		return wxString(CFormat("%.2f ") % (bytes / 1024.0)) + _("kB/s");
	else
		return wxString(CFormat("%.2f ") % (bytes / 1048576.0)) + _("MB/s");
}


// Make a time value in seconds suitable for displaying
wxString CastSecondsToHM(uint32 count, uint16 msecs)
{
	if (count < 60) {
		if (!msecs) {
			return CFormat("%02u %s") % count % _("secs");
		} else {
			return CFormat("%.3f %s")
				% (count + ((float)msecs/1000)) % _("secs");
		}
	} else if (count < 3600) {
		return CFormat("%u:%02u %s")
			% (count/60) % (count % 60) % _("mins");
	} else if (count < 86400) {
		return CFormat("%u:%02u %s")
			% (count/3600) % ((count % 3600)/60) % _("hours");
	} else {
		return CFormat("%u %s %02u:%02u %s")
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


// Returns the (translated) description associated with a FileType
wxString GetFiletypeDesc(FileType type, bool translated)
{
	switch ( type ) {
		case ftVideo:
			if (translated) {
				return _("Videos");
			} else {
				return "Videos";
			}
			break;
		case ftAudio:
			if (translated) {
				return _("Audio");
			} else {
				return "Audio";
			}
			break;
		case ftArchive:
			if (translated) {
				return _("Archives");
			} else {
				return "Archives";
			}
			break;
		case ftCDImage:
			if (translated) {
				return _("CD-Images");
			} else {
				return "CD-Images";
			}
			break;
		case ftPicture:
			if (translated) {
				return _("Pictures");
			} else {
				return "Pictures";
			}
			break;
		case ftText:
			if (translated) {
				return _("Texts");
			} else {
				return "Texts";
			}
			break;
		case ftProgram:
			if (translated) {
				return _("Programs");
			} else {
				return "Programs";
			}
			break;
		default:
			if (translated) {
				return _("Any");
			} else {
				return "Any";
			}
			break;
	}
}

// Returns the Typename, examining the extension of the given filename

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
static wxChar base16Chars[17] = L"0123456789ABCDEF";
static wxChar base32Chars[33] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
#define BASE16_LOOKUP_MAX 23
static wxChar base16Lookup[BASE16_LOOKUP_MAX][2] = {
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


// Returns a BASE16 encoded byte array
//
// [In]
//   buffer: Pointer to byte array
//   bufLen: Length of buffer array
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
//   base16BufLen: Length BASE16 coded string's length
//
// [Out]
//   buffer: byte array containing decoded string
unsigned int DecodeBase16(const wxString &base16Buffer, unsigned int base16BufLen, uint8_t *buffer)
{
	if (base16BufLen & 1) {
		return 0;
	}
	unsigned int ret = base16BufLen >> 1;
	memset( buffer, 0,  ret);
	for(unsigned int i = 0; i < base16BufLen; ++i) {
		int lookup = toupper(base16Buffer[i]) - '0';
		// Check to make sure that the given word falls inside a valid range
		uint8_t word = (lookup < 0 || lookup >= BASE16_LOOKUP_MAX) ?
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
//   bufLen: Length of buffer array
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
//   base32BufLen: Length BASE32 coded string's length
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
		if (base32Buffer[i] >= 'A' && base32Buffer[i] <= 'Z') {
			nBits |= ( base32Buffer[i] - 'A' );
		}
		else if (base32Buffer[i] >= 'a' && base32Buffer[i] <= 'z') {
			nBits |= ( base32Buffer[i] - 'a' );
		}
		else if (base32Buffer[i] >= '2' && base32Buffer[i] <= '7') {
			nBits |= ( base32Buffer[i] - '2' + 26 );
		} else {
			return 0;
		}
		nCount += 5;
		if (nCount >= 8)
		{
			*buffer++ = (uint8_t)( nBits >> (nCount - 8) );
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
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");


/* Option variables */
static bool g_fUseCRLF = false;
static unsigned int g_nCharsPerLine = 72;
static wxString strHeaderLine;


wxString EncodeBase64(const char *pbBufferIn, unsigned int bufLen)
{
	wxString pbBufferOut;
	wxString strHeader;

	if( !strHeaderLine.IsEmpty() ) {
		strHeader = "-----BEGIN " + strHeaderLine + "-----";
		if( g_fUseCRLF ) {
			strHeader += "\r";
		}
		strHeader += "\n";
	}

	unsigned long nDiv = ((unsigned long)bufLen) / 3;
	unsigned long nRem = ((unsigned long)bufLen) % 3;
	unsigned int NewLineSize = g_fUseCRLF ? 2 : 1;

	// Allocate enough space in the output buffer to speed up things
	pbBufferOut.Alloc(
		strHeader.Len() * 2 +		// header/footer
		(bufLen * 4) / 3 + 1 +		// Number of codes
		nDiv           * NewLineSize +	// Number of new lines
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
				pbBufferOut += "\r";
			}
			pbBufferOut += "\n";
		}
	}
	switch( nRem ) {
	case 2:
		pbBufferOut += to_b64[ (pIn[0] >> 2) & 0x3f];
		pbBufferOut += to_b64[((pIn[0] << 4) & 0x30) | ((pIn[1] >> 4) & 0xf)];
		pbBufferOut += to_b64[ (pIn[1] << 2) & 0x3c];
		pbBufferOut += "=";
		nChars += 4;
		if( nChars >= g_nCharsPerLine && g_nCharsPerLine != 0 ) {
			nChars = 0;
			if( g_fUseCRLF ) {
				pbBufferOut += "\r";
			}
			pbBufferOut += "\n";
		}
		break;
	case 1:
		pbBufferOut += to_b64[ (pIn[0] >> 2) & 0x3f];
		pbBufferOut += to_b64[ (pIn[0] << 4) & 0x30];
		pbBufferOut += "=";
		pbBufferOut += "=";
		nChars += 4;
		if( nChars >= g_nCharsPerLine && g_nCharsPerLine != 0 ) {
			nChars = 0;
			if( g_fUseCRLF ) {
				pbBufferOut += "\r";
			}
			pbBufferOut += "\n";
		}
		break;
	}

	if( nRem > 0 ) {
		if( nChars > 0 ) {
			if( g_fUseCRLF ) {
				pbBufferOut += "\r";
			}
			pbBufferOut += "\n";
		}
	}

	if( !strHeaderLine.IsEmpty() ) {
		pbBufferOut = "-----END " + strHeaderLine + "-----";
		if( g_fUseCRLF ) {
			pbBufferOut += "\r";
		}
		pbBufferOut += "\n";
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
		wxChar bits = 'z';
		if( z > 0 ) {
			if(c == '\n') {
				z = 0;
			}
		}
		else if(c >= 'A' && c <= 'Z') {
			bits = c - 'A';
		}
		else if(c >= 'a' && c <= 'z') {
			bits = c - 'a' + (wxChar)26;
		}
		else if(c >= '0' && c <= '9') {
			bits = c - '0' + (wxChar)52;
		}
		else if(c == '+') {
			bits = (wxChar)62;
		}
		else if(c == '/') {
			bits = (wxChar)63;
		}
		else if(c == '-') {
			z = 1;
		}
		else if(c == '=') {
			break;
		} else {
			bits = 'y';
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


// Returns the text associated with a category type
wxString GetCatTitle(AllCategoryFilter cat)
{
	switch (cat) {
		case acfAll:		 return _("all");
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
		default: return "?";
	}
}


typedef std::map<wxString, EED2KFileTypeClass> SED2KFileTypeMap;
typedef SED2KFileTypeMap::value_type SED2KFileTypeMapElement;
static SED2KFileTypeMap ED2KFileTypesMap;


class CED2KFileTypes{
public:
	CED2KFileTypes()
	{
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".669",   ED2KFT_AUDIO));		// 8 channel tracker module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".aac",   ED2KFT_AUDIO));		// Advanced Audio Coding File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ac3",   ED2KFT_AUDIO));		// Audio Codec 3 File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".aif",   ED2KFT_AUDIO));		// Audio Interchange File Format
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".aifc",  ED2KFT_AUDIO));		// Audio Interchange File Format
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".aiff",  ED2KFT_AUDIO));		// Audio Interchange File Format
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".amf",   ED2KFT_AUDIO));		// DSMI Advanced Module Format
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".amr",   ED2KFT_AUDIO));		// Adaptive Multi-Rate Codec File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ams",   ED2KFT_AUDIO));		// Extreme Tracker Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ape",   ED2KFT_AUDIO));		// Monkey's Audio Lossless Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".au",    ED2KFT_AUDIO));		// Audio File (Sun, Unix)
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".aud",   ED2KFT_AUDIO));		// General Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".audio", ED2KFT_AUDIO));		// General Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".cda",   ED2KFT_AUDIO));		// CD Audio Track
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".dbm",   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".dmf",   ED2KFT_AUDIO));		// Delusion Digital Music File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".dsm",   ED2KFT_AUDIO));		// Digital Sound Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".dts",   ED2KFT_AUDIO));		// DTS Encoded Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".far",   ED2KFT_AUDIO));		// Farandole Composer Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".flac",  ED2KFT_AUDIO));		// Free Lossless Audio Codec File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".it",    ED2KFT_AUDIO));		// Impulse Tracker Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".m1a",   ED2KFT_AUDIO));		// MPEG-1 Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".m2a",   ED2KFT_AUDIO));		// MPEG-2 Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".m4a",   ED2KFT_AUDIO));		// MPEG-4 Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mdl",   ED2KFT_AUDIO));		// DigiTrakker Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".med",   ED2KFT_AUDIO));		// Amiga MED Sound File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mid",   ED2KFT_AUDIO));		// MIDI File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".midi",  ED2KFT_AUDIO));		// MIDI File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mka",   ED2KFT_AUDIO));		// Matroska Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mod",   ED2KFT_AUDIO));		// Amiga Music Module File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mol",   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mp1",   ED2KFT_AUDIO));		// MPEG-1 Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mp2",   ED2KFT_AUDIO));		// MPEG-2 Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mp3",   ED2KFT_AUDIO));		// MPEG-3 Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mpa",   ED2KFT_AUDIO));		// MPEG Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mpc",   ED2KFT_AUDIO));		// Musepack Compressed Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mpp",   ED2KFT_AUDIO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mtm",   ED2KFT_AUDIO));		// MultiTracker Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".nst",   ED2KFT_AUDIO));		// NoiseTracker
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ogg",   ED2KFT_AUDIO));		// Ogg Vorbis Compressed Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".okt",   ED2KFT_AUDIO));		// Oktalyzer Module (Amiga)
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".psm",   ED2KFT_AUDIO));		// Protracker Studio Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ptm",   ED2KFT_AUDIO));		// PolyTracker Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ra",    ED2KFT_AUDIO));		// Real Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".rmi",   ED2KFT_AUDIO));		// MIDI File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".s3m",   ED2KFT_AUDIO));		// Scream Tracker 3 Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".snd",   ED2KFT_AUDIO));		// Audio File (Sun, Unix)
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".stm",   ED2KFT_AUDIO));		// Scream Tracker 2 Module
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ult",   ED2KFT_AUDIO));		// UltraTracker
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".umx",   ED2KFT_AUDIO));		// Unreal Music Package
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".wav",   ED2KFT_AUDIO));		// WAVE Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".wma",   ED2KFT_AUDIO));		// Windows Media Audio File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".wow",   ED2KFT_AUDIO));		// Grave Composer audio tracker
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".xm",    ED2KFT_AUDIO));		// Fasttracker 2 Extended Module

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".3g2",   ED2KFT_VIDEO));		// 3GPP Multimedia File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".3gp",   ED2KFT_VIDEO));		// 3GPP Multimedia File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".3gp2",  ED2KFT_VIDEO));		// 3GPP Multimedia File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".3gpp",  ED2KFT_VIDEO));		// 3GPP Multimedia File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".asf",   ED2KFT_VIDEO));		// Advanced Systems Format (MS)
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".amv",   ED2KFT_VIDEO));		// Anime Music Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".asf",   ED2KFT_VIDEO));		// Advanced Systems Format File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".avi",   ED2KFT_VIDEO));		// Audio Video Interleave File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".bik",   ED2KFT_VIDEO));		// BINK Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".divx",  ED2KFT_VIDEO));		// DivX-Encoded Movie File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".dvr-ms",ED2KFT_VIDEO));		// Microsoft Digital Video Recording
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".flc",   ED2KFT_VIDEO));		// FLIC Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".fli",   ED2KFT_VIDEO));		// FLIC Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".flic",  ED2KFT_VIDEO));		// FLIC Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".flv",   ED2KFT_VIDEO));		// Flash Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".hdmov", ED2KFT_VIDEO));		// High-Definition QuickTime Movie
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ifo",   ED2KFT_VIDEO));		// DVD-Video Disc Information File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".m1v",   ED2KFT_VIDEO));		// MPEG-1 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".m2t",   ED2KFT_VIDEO));		// MPEG-2 Video Transport Stream
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".m2ts",  ED2KFT_VIDEO));		// MPEG-2 Video Transport Stream
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".m2v",   ED2KFT_VIDEO));		// MPEG-2 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".m4b",   ED2KFT_VIDEO));		// MPEG-4 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".m4v",   ED2KFT_VIDEO));		// MPEG-4 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mkv",   ED2KFT_VIDEO));		// Matroska Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mov",   ED2KFT_VIDEO));		// QuickTime Movie File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".movie", ED2KFT_VIDEO));		// QuickTime Movie File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mp1v",  ED2KFT_VIDEO));		// QuickTime Movie File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mp2v",  ED2KFT_VIDEO));		// MPEG-1 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mp4",   ED2KFT_VIDEO));		// MPEG-2 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mpe",   ED2KFT_VIDEO));		// MPEG-4 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mpeg",  ED2KFT_VIDEO));		// MPEG Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mpg",   ED2KFT_VIDEO));		// MPEG Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mps",   ED2KFT_VIDEO));		// MPEG Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mpv",   ED2KFT_VIDEO));		// MPEG Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mpv1",  ED2KFT_VIDEO));		// MPEG-1 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mpv2",  ED2KFT_VIDEO));		// MPEG-2 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ogm",   ED2KFT_VIDEO));		// Ogg Media File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ogv",   ED2KFT_VIDEO));		// Ogg Theora Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".pva",   ED2KFT_VIDEO));		// MPEG Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".qt",    ED2KFT_VIDEO));		// QuickTime Movie
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ram",   ED2KFT_VIDEO));		// Real Audio Media
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ratdvd",ED2KFT_VIDEO));		// RatDVD Disk Image
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".rm",    ED2KFT_VIDEO));		// Real Media File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".rmm",   ED2KFT_VIDEO));		// Real Media File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".rmvb",  ED2KFT_VIDEO));		// Real Video Variable Bit Rate File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".rv",    ED2KFT_VIDEO));		// Real Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".rv9",   ED2KFT_VIDEO));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".smil",  ED2KFT_VIDEO));		// SMIL Presentation File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".smk",   ED2KFT_VIDEO));		// Smacker Compressed Movie File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".swf",   ED2KFT_VIDEO));		// Macromedia Flash Movie
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".tp",    ED2KFT_VIDEO));		// Video Transport Stream File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ts",    ED2KFT_VIDEO));		// Video Transport Stream File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".vid",   ED2KFT_VIDEO));		// General Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".video", ED2KFT_VIDEO));		// General Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".vivo",  ED2KFT_VIDEO));		// VivoActive Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".vob",   ED2KFT_VIDEO));		// DVD Video Object File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".vp6",   ED2KFT_VIDEO));		// TrueMotion VP6 Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".webm",  ED2KFT_VIDEO));		// WebM Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".wm",    ED2KFT_VIDEO));		// Windows Media Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".wmv",   ED2KFT_VIDEO));		// Windows Media Video File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".xvid",  ED2KFT_VIDEO));		// Xvid-Encoded Video File

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".bmp",   ED2KFT_IMAGE));		// Bitmap Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".dcx",   ED2KFT_IMAGE));		// FAXserve Fax Document
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".emf",   ED2KFT_IMAGE));		// Enhanced Windows Metafile
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".gif",   ED2KFT_IMAGE));		// Graphical Interchange Format File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ico",   ED2KFT_IMAGE));		// Icon File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".jfif",  ED2KFT_IMAGE));		// JPEG File Interchange Format
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".jpe",   ED2KFT_IMAGE));		// JPEG Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".jpeg",  ED2KFT_IMAGE));		// JPEG Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".jpg",   ED2KFT_IMAGE));		// JPEG Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".pct",   ED2KFT_IMAGE));		// PICT Picture File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".pcx",   ED2KFT_IMAGE));		// Paintbrush Bitmap Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".pic",   ED2KFT_IMAGE));		// PICT Picture File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".pict",  ED2KFT_IMAGE));		// PICT Picture File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".png",   ED2KFT_IMAGE));		// Portable Network Graphic
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".psd",   ED2KFT_IMAGE));		// Photoshop Document
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".psp",   ED2KFT_IMAGE));		// Paint Shop Pro Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".tga",   ED2KFT_IMAGE));		// Targa Graphic
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".tif",   ED2KFT_IMAGE));		// Tagged Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".tiff",  ED2KFT_IMAGE));		// Tagged Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".wbmp",  ED2KFT_IMAGE));		// Wireless Application Protocol Bitmap Format
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".webp",  ED2KFT_IMAGE));		// Weppy Photo File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".wmf",   ED2KFT_IMAGE));		// Windows Metafile
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".wmp",   ED2KFT_IMAGE));		// Windows Media Photo File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".xif",   ED2KFT_IMAGE));		// ScanSoft Pagis Extended Image Format File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".xpm",   ED2KFT_IMAGE));		// X-Windows Pixmap

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".7z",    ED2KFT_ARCHIVE));	// 7-Zip Compressed File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ace",   ED2KFT_ARCHIVE));	// WinAce Compressed File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".alz",   ED2KFT_ARCHIVE));	// ALZip Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".arc",   ED2KFT_ARCHIVE));	// Compressed File Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".arj",   ED2KFT_ARCHIVE));	// ARJ Compressed File Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".bz2",   ED2KFT_ARCHIVE));	// Bzip Compressed File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".cab",   ED2KFT_ARCHIVE));	// Cabinet File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".cbr",   ED2KFT_ARCHIVE));	// Comic Book RAR Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".cbt",   ED2KFT_ARCHIVE));	// Comic Book Tarball
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".cbz",   ED2KFT_ARCHIVE));	// Comic Book ZIP Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".gz",    ED2KFT_ARCHIVE));	// Gnu Zipped File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".hqx",   ED2KFT_ARCHIVE));	// BinHex 4.0 Encoded File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".lha",   ED2KFT_ARCHIVE));	// LHARC Compressed Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".lzh",   ED2KFT_ARCHIVE));	// LZH Compressed File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".msi",   ED2KFT_ARCHIVE));	// Microsoft Installer File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".pak",   ED2KFT_ARCHIVE));	// PAK (Packed) File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".par",   ED2KFT_ARCHIVE));	// Parchive Index File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".par2",  ED2KFT_ARCHIVE));	// Parchive 2 Index File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".rar",   ED2KFT_ARCHIVE));	// WinRAR Compressed Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".sea",   ED2KFT_ARCHIVE));	// Self-Extracting Archive (Mac)
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".sit",   ED2KFT_ARCHIVE));	// Stuffit Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".sitx",  ED2KFT_ARCHIVE));	// Stuffit X Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".tar",   ED2KFT_ARCHIVE));	// Consolidated Unix File Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".tbz2",  ED2KFT_ARCHIVE));	// Tar BZip 2 Compressed File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".tgz",   ED2KFT_ARCHIVE));	// Gzipped Tar File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".uc2",   ED2KFT_ARCHIVE));	// UltraCompressor 2 Archive
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".xpi",   ED2KFT_ARCHIVE));	// Mozilla Installer Package
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".z",     ED2KFT_ARCHIVE));	// Unix Compressed File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".zip",   ED2KFT_ARCHIVE));	// Zipped File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".zoo",   ED2KFT_ARCHIVE));	// Zoo Archive

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".bat",   ED2KFT_PROGRAM));	// Batch File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".cmd",   ED2KFT_PROGRAM));	// Command File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".com",   ED2KFT_PROGRAM));	// COM File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".exe",   ED2KFT_PROGRAM));	// Executable File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".hta",   ED2KFT_PROGRAM));	// HTML Application
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".js",    ED2KFT_PROGRAM));	// Java Script
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".jse",   ED2KFT_PROGRAM));	// Encoded  Java Script
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".msc",   ED2KFT_PROGRAM));	// Microsoft Common Console File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".vbe",   ED2KFT_PROGRAM));	// Encoded Visual Basic Script File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".vbs",   ED2KFT_PROGRAM));	// Visual Basic Script File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".wsf",   ED2KFT_PROGRAM));	// Windows Script File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".wsh",   ED2KFT_PROGRAM));	// Windows Scripting Host File

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".bin",   ED2KFT_CDIMAGE));	// CD Image
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".bwa",   ED2KFT_CDIMAGE));	// BlindWrite Disk Information File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".bwi",   ED2KFT_CDIMAGE));	// BlindWrite CD/DVD Disc Image
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".bws",   ED2KFT_CDIMAGE));	// BlindWrite Sub Code File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".bwt",   ED2KFT_CDIMAGE));	// BlindWrite 4 Disk Image
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ccd",   ED2KFT_CDIMAGE));	// CloneCD Disk Image
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".cue",   ED2KFT_CDIMAGE));	// Cue Sheet File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".dmg",   ED2KFT_CDIMAGE));	// Mac OS X Disk Image
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".dmz",   ED2KFT_CDIMAGE));
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".img",   ED2KFT_CDIMAGE));	// Disk Image Data File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".iso",   ED2KFT_CDIMAGE));	// Disc Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mdf",   ED2KFT_CDIMAGE));	// Media Disc Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mds",   ED2KFT_CDIMAGE));	// Media Descriptor File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".nrg",   ED2KFT_CDIMAGE));	// Nero CD/DVD Image File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".sub",   ED2KFT_CDIMAGE));	// Subtitle File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".toast", ED2KFT_CDIMAGE));	// Toast Disc Image

		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".azw",   ED2KFT_DOCUMENT));	// EBook File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".chm",   ED2KFT_DOCUMENT));	// Compiled HTML Help File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".css",   ED2KFT_DOCUMENT));	// Cascading Style Sheet
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".diz",   ED2KFT_DOCUMENT));	// Description in Zip File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".doc",   ED2KFT_DOCUMENT));	// Document File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".dot",   ED2KFT_DOCUMENT));	// Document Template File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".epub",  ED2KFT_DOCUMENT));	// EBook File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".hlp",   ED2KFT_DOCUMENT));	// Help File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".htm",   ED2KFT_DOCUMENT));	// HTML File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".html",  ED2KFT_DOCUMENT));	// HTML File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".mobi",  ED2KFT_DOCUMENT));	// EBook File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".nfo",   ED2KFT_DOCUMENT));	// Warez Information File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".odp",   ED2KFT_DOCUMENT));	// OpenDocument Presentation
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ods",   ED2KFT_DOCUMENT));	// OpenDocument Spreadsheet
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".odt",   ED2KFT_DOCUMENT));	// OpenDocument File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".otp",   ED2KFT_DOCUMENT));	// OpenDocument Presentation Template
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ott",   ED2KFT_DOCUMENT));	// OpenDocument Template File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ots",   ED2KFT_DOCUMENT));	// OpenDocument Spreadsheet Template
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".pdf",   ED2KFT_DOCUMENT));	// Portable Document Format File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".pps",   ED2KFT_DOCUMENT));	// PowerPoint Slide Show
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ppt",   ED2KFT_DOCUMENT));	// PowerPoint Presentation
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".ps",    ED2KFT_DOCUMENT));	// PostScript File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".rtf",   ED2KFT_DOCUMENT));	// Rich Text Format File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".stc",   ED2KFT_DOCUMENT));	// OpenOffice.org 1.0 Spreadsheet Template
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".sti",   ED2KFT_DOCUMENT));	// OpenOffice.org 1.0 Presentation Template
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".stw",   ED2KFT_DOCUMENT));	// OpenOffice.org 1.0 Document Template File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".sxc",   ED2KFT_DOCUMENT));	// OpenOffice.org 1.0 Spreadsheet
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".sxi",   ED2KFT_DOCUMENT));	// OpenOffice.org 1.0 Presentation
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".sxw",   ED2KFT_DOCUMENT));	// OpenOffice.org 1.0 Document File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".text",  ED2KFT_DOCUMENT));	// General Text File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".txt",   ED2KFT_DOCUMENT));	// Text File
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".wri",   ED2KFT_DOCUMENT));	// Windows Write Document
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".xls",   ED2KFT_DOCUMENT));	// Microsoft Excel Spreadsheet
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".xlt",   ED2KFT_DOCUMENT));	// Microsoft Excel Template
		ED2KFileTypesMap.insert(SED2KFileTypeMapElement(".xml",   ED2KFT_DOCUMENT));	// XML File
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

	SED2KFileTypeMap::iterator it = ED2KFileTypesMap.find("." + ext);
	if (it != ED2KFileTypesMap.end()) {
		return it->second.GetType();
	} else {
		return ED2KFT_ANY;
	}
}


// Returns the ed2k file type term which is to be used in server searches
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

	return "";
}


// Returns a file type which is used eMule internally only, examining the extension of the given filename
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
		default:		return "";
	}
}


// Returns the ed2k file type integer ID which is to be used for publishing+searching
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
		result += msg + " - ok=" + ( ok ? "true, " : "false, " );
	}

	result += CFormat("%d bytes\n") % n;
	for ( int i = 0; i < lines; ++i) {
		// Show address
		result += CFormat("%08x  ") % (i * 16);

		// Show two columns of hex-values
		for ( int j = 0; j < 2; ++j) {
			for ( int k = 0; k < 8; ++k) {
				int pos = 16 * i + 8 * j + k;

				if ( pos < n ) {
					result += CFormat("%02x ") % p[pos];
				} else {
					result += "   ";
				}
			}
			result += " ";
		}
		result += "|";
		// Show a column of ascii-values
		for ( int k = 0; k < 16; ++k) {
			int pos = 16 * i + k;

			if ( pos < n ) {
				if ( isspace( p[pos] ) ) {
					result += " ";
				} else if ( !isgraph( p[pos] ) ) {
					result += ".";
				} else {
					result += (wxChar)p[pos];
				}
			} else {
				result += " ";
			}
		}
		result += "|\n";
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
	wxString configPath;

	// "Portable aMule" - Use aMule from an external USB drive
	// Check for ./config/amule.conf (or whatever gets passed as configFile)
	// and use this configuration if found
	const wxString configDir = JoinPaths(wxFileName::GetCwd(), "config");
	const wxString configFile = JoinPaths(configDir, configFileBase);

	if (CPath::DirExists(configDir) && CPath::FileExists(configFile)) {
		AddLogLineN(CFormat(_("Using config dir: %s")) % configDir);

		configPath = configDir;
	} else {
		configPath = wxStandardPaths::Get().GetUserDataDir();
	}

	configPath += wxFileName::GetPathSeparator();

	return configPath;
}


/*************************** Locale specific stuff ***************************/

#ifndef __WINDOWS__
#	define	SETWINLANG(LANG, SUBLANG)
#else
#	define	SETWINLANG(LANG, SUBLANG) \
	info.WinLang = LANG; \
	info.WinSublang = SUBLANG;
#endif

#define CUSTOMLANGUAGE(wxid, iso, winlang, winsublang, dir, desc) \
	info.Language = wxid;		\
	info.CanonicalName = iso;	\
	info.LayoutDirection = dir;	\
	info.Description = desc;	\
	SETWINLANG(winlang, winsublang)	\
	wxLocale::AddLanguage(info);

void InitCustomLanguages()
{
	wxLanguageInfo info;
}


void InitLocale(wxLocale& locale, int language)
{
	locale.Init(language, wxLOCALE_LOAD_DEFAULT);

#if defined(__WXMAC__) || defined(__WINDOWS__)
	locale.AddCatalogLookupPathPrefix(JoinPaths(wxStandardPaths::Get().GetDataDir(), "locale"));
#endif /* (!)(defined(__WXMAC__) || defined(__WINDOWS__)) */

	locale.AddCatalog(PACKAGE);
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
			return "";
		}
	} else {
		return "";
	}
}

/*****************************************************************************/

CMD4Hash GetPassword(bool allowEmptyPassword)
{
	wxString pass_plain;
	CMD4Hash password;
#ifndef __WINDOWS__
	pass_plain = char2unicode(getpass("Enter password for mule connection: "));
#else
	//#warning This way, pass enter is not hidden on windows. Bad thing.
	char temp_str[512];
	// Though fflush() on an input stream is undefined behaviour by the standard,
	// the MSVCRT version does seem to clear the input buffers.
	// cppcheck-suppress fflushOnInputStream
	fflush(stdin);
	printf("Enter password for mule connection: \n");
	fflush(stdout);
	fgets(temp_str, 512, stdin);
	temp_str[strlen(temp_str)-1] = '\0';
	pass_plain = char2unicode(temp_str);
#endif
	wxCHECK2(password.Decode(MD5Sum(pass_plain).GetHash()), /* Do nothing. */ );
	if (!allowEmptyPassword) {
		// MD5 hash for an empty string, according to rfc1321.
		if (password.Encode() == "D41D8CD98F00B204E9800998ECF8427E") {
			printf("No empty password allowed.\n");
			return GetPassword(false);
		}
	}

	return password;
}


const uint8 BitVector::s_posMask[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
const uint8 BitVector::s_negMask[] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};

// File_checked_for_headers
