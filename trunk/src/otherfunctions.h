//
// This file is part of the aMule Project
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

#ifndef OTHERFUNCTIONS_H
#define OTHERFUNCTIONS_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "otherfunctions.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/string.h>		// Needed for wxString
#include <wx/dynarray.h>

#ifdef __WXBASE__
	#include <time.h>
	#include <errno.h>
#else
	#include <wx/utils.h>
#endif	

#include "types.h"		// Needed for uint16, uint32 and uint64
#include "endianfix.h"
#include "otherstructs.h" // for Gap_Struct

namespace otherfunctions {
	
/**
 * Helper function.
 *
 * @param ArgA The base value.
 * @param ArgB The value to compare ArgA against.
 * @return See below.
 *
 * Use this function to safely compare two arguments of a type that supports 
 * the "<" operator. It works like strcmp and returns a negative value if ArgA
 * is less than ArgB, zero if ArgA is equal to ArgB and a positive value if 
 * ArgA is greater than ArgB.
 */
template <class TYPE>
int CmpAny(const TYPE& ArgA, const TYPE& ArgB)
{
	if ( ArgA < ArgB ) {
		return -1;
	} else if ( ArgB < ArgA ) {
		return  1;
	} else {
		return  0;
	}
}

//! Overloaded version of CmpAny for use with wxStrings.
inline int CmpAny(const wxString& ArgA, const wxString& ArgB)
{
	return ArgA.CmpNoCase( ArgB );
}

//! Overloaded version of CmpAny for use with C-Strings (Unicoded).
inline int CmpAny(const wxChar* ArgA, const wxChar* ArgB)
{
	return wxString( ArgA ).CmpNoCase( ArgB );
}


/**
 * Returns a description of the version of aMule being used.
 *
 * @return A detailed description of the aMule version, including wx information.
 *
 * Use this rather than just using the VERSION or CURRENT_VERSION_LONG
 * constants, when displaying information to the user. The purpose is to
 * help with debugging.
 */
wxString GetMuleVersion();



/**
 * Helperfunction for accessing a child of the calling widget.
 *
 * @param IdOrName The ID or the Name of the widget to find.
 * @param type The widget-type to cast the found widget to.
 *
 * Use this function as a replacement for the following constructs:
 *  - wxStaticCast( FindWindow( <IdOrName> ), <type> )
 *  - (<type>*)FindWindow( <IdOrName> )
 *
 * It has the advantage of validating the cast in debug builds and being much
 * shorter than than manually typing wxStaticCast + FindWindow. This mean that
 * we will be alerted in case of widget changing type, instead of getting just 
 * getting bad mojo due to casting a pointer to the wrong type.
 */
#define CastChild( IdOrName, type )			dynamic_cast<type*>( FindWindow( IdOrName ) )


/**
 * Helperfunction for accessing the child of a any widget by ID.
 *
 * @param ID The ID of the widget to find.
 * @param parent The parent of the widget to find, or NULL to search from the top.
 * @param type The type to cast the widget to.
 *
 * @see CastChild()
 */
#define CastByID( ID, parent, type )		dynamic_cast<type*>( wxWindow::FindWindowById( (ID), (parent) ) )


/**
 * Helperfunction for accessing the child of a any widget by Name.
 *
 * @param Name The Name of the widget to find.
 * @param parent The parent of the widget to find, or NULL to search from the top.
 * @param type The type to cast the widget to.
 *
 * @see CastChild()
 */
#define CastByName( Name, parent, type )	dynamic_cast<type*>( wxWindow::FindWindowByName( (Name), (parent) ) )


// From Gnucleus project [found by Tarod]
// Base16/Base32/Base64 Encode/Decode functions
wxString EncodeBase16(const unsigned char* buffer, unsigned int bufLen);
void DecodeBase16(const char *base16Buffer, unsigned int base16BufLen, unsigned char *buffer);
wxString EncodeBase32(const unsigned char* buffer, unsigned int bufLen);
unsigned int DecodeBase32(const char *base32Buffer, unsigned int base32BufLen, unsigned char *buffer);
wxString EncodeBase64(const char* buffer, unsigned int bufLen);
unsigned int DecodeBase64(const char *base32Buffer, unsigned int base32BufLen, unsigned char *buffer);

// Converts the number of bytes to human readable form.
wxString CastItoXBytes(uint64 count);
// Converts the number to human readable form, abbreviating when nessecary.
wxString CastItoIShort(uint64 number);
// Converts an ammount of seconds to human readable time.
wxString CastSecondsToHM(sint32 seconds);
// Returns the smount of Bytes the provided size-type represents
uint32 GetTypeSize(uint8 type);
// Returns the string assosiated with a file-rating value.
wxString GetRateString(uint16 rate);


// The following functions are used to identify and/or name the type of a file
enum FileType { ftAny, ftVideo, ftAudio, ftArchive, ftCDImage, ftPicture, ftText, ftProgram };
// Examins a filename and returns the enumerated value assosiated with it, or ftAny if unknown extension
FileType GetFiletype(const wxString& filename);
// Returns the description of a filetype: Movies, Audio, Pictures and so on...
wxString GetFiletypeDesc(FileType type, bool translated = true);
// Shorthand for GetFiletypeDesc(GetFiletype(filename))
wxString GetFiletypeByName(const wxString& filename, bool translated = true);
// Reports if the file has contents or not (no need for the file to exist)
bool IsEmptyFile(const wxString& filename);


// Returns the max number of connections the current OS can handle.
// Currently anything but windows will return the default value (-1);
int GetMaxConnections();
// Returns the name assosiated with a category value.
wxString GetCatTitle(int catid);

// Tests if a ID is low (behind firewall/router/...)
#define HIGHEST_LOWID_HYBRID	16777216
#define HIGHEST_LOWID_ED2K		16777216
inline bool IsLowIDHybrid(uint32 id){
	return (id < HIGHEST_LOWID_HYBRID);
}
inline bool IsLowIDED2K(uint32 id){
	return (id < HIGHEST_LOWID_ED2K); //Need to verify what the highest LowID can be returned by the server.
}


/* Other */

// Compares first and second. For uint16 arrays sorting.
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

void DumpMem_DW(const uint32 *ptr, int count);

/*!
 * General purpose RLE implementation. Just encode or create
 * differential data with previous
 */
class RLE_Data {
		unsigned char *m_buff, *m_enc_buff;
		bool m_use_diff;
		int m_len, m_enc_len;
		
		// data is bounded by srclen. everything above considered == 0
		template <class T> const unsigned char *EncodeT(T &buff, int srclen, int &outlen)
		{
			//
			// calculate difference from prev
			//
			if ( m_use_diff ) {
				for (int i = 0; i < m_len; i++) {
					m_buff[i] ^= (i < srclen ) ? ((unsigned char)buff[i]) : 0;
				}
			} else {
				//
				// can't use memcpy - in case of generic class T this
				// will rely on "operator []" implementation
				for(int i = 0; i < m_len;i++) {
					m_buff[i] = (i < srclen ) ? ((unsigned char)buff[i]) : 0;
				}
			}
			
			//
			// now RLE
			//
			int i = 0, j = 0;
			while ( i != m_len ) {
				unsigned char curr_val = m_buff[i];
				int seq_start = i;
				while ( (i != m_len) && (curr_val == m_buff[i]) && ((i - seq_start) < 0xff)) {
					i++;
				}
				if (i - seq_start > 1) {
					// if there's 2 or more equal vals - put it twice in stream
					m_enc_buff[j++] = curr_val;
					m_enc_buff[j++] = curr_val;
					m_enc_buff[j++] = i - seq_start;
				} else {
					// single value - put it as is
					m_enc_buff[j++] = curr_val;
				}
			}
	
			outlen = j;
			
			//
			// If using differential encoder, remember current data for
			// later use
			if ( m_use_diff ) {
				//
				// can't use memcpy - in case of generic class T this
				// will rely on "operator []" implementation
				for(int i = 0; i < m_len;i++) {
					m_buff[i] = (i < srclen ) ? ((unsigned char)buff[i]) : 0;
				}
			}
			
			return m_enc_buff;
		}

	public:
		RLE_Data(int len, bool use_diff);
		
		// those constructors are for stl containers
		RLE_Data();
		RLE_Data(const RLE_Data &);
		RLE_Data &operator=(const RLE_Data &);
		
		~RLE_Data();
		
		const unsigned char *Encode(unsigned char *data, int &outlen)
		{
			return EncodeT<unsigned char *>(data, m_len, outlen);
		}
		const unsigned char *Encode(ArrayOfUInts16 &data, int &outlen)
		{
			return EncodeT<ArrayOfUInts16>(data, data.GetCount(), outlen);
		}
		
		const unsigned char *Decode(const unsigned char *data, int len);	
		
		void ResetEncoder()
		{
			memset(m_buff, 0, m_len);
		}

		// change size of internal buffers
		void Realloc(int size);
		
		// decoder will need access to data
		const unsigned char *Buffer() { return m_buff; }
		int Size() { return m_len; }
};

/*!
 * Data difference is different for each EC client
 */
class PartFileEncoderData {
public:
	RLE_Data m_part_status;
	RLE_Data m_gap_status;
	
	//
	// Encoder may reset history if full info requested
	void ResetEncoder()
	{
		m_part_status.ResetEncoder();
		m_gap_status.ResetEncoder();
	}
	
	//
	// decoder side - can be used everywhere
	void Decode(unsigned char *gapdata, int gaplen, unsigned char *partdata, int partlen);
	
	PartFileEncoderData() { }
	PartFileEncoderData(int part_count, int gap_count) :
		m_part_status(part_count, true), m_gap_status(gap_count*sizeof(uint32), true)
	{
	}
		
	// for stl
	PartFileEncoderData(const PartFileEncoderData &obj) :
		m_part_status(obj.m_part_status), m_gap_status(obj.m_gap_status)
	{
	}
	PartFileEncoderData &operator=(const PartFileEncoderData &obj)
	{
		m_part_status = obj.m_part_status;
		m_gap_status = obj.m_gap_status;
		return *this;
	}
};

inline void MilliSleep(uint32 msecs) {
	#ifdef __WXBASE__
		struct timespec waittime;
			waittime.tv_sec = 0;
			waittime.tv_nsec = msecs * 1000 /*micro*/* 1000 /*nano*/;
		struct timespec remtime;
		while ((nanosleep(&waittime,&remtime)==-1) && (errno == EINTR)) {
			memcpy(&waittime,&remtime,sizeof(struct timespec));
		}
	#else
		#if wxCHECK_VERSION(2, 5, 3)
			wxMilliSleep(msecs);
		#else
			wxUsleep(msecs);
		#endif
	#endif
}

} // End namespace

#endif // OTHERFUNCTIONS_H
