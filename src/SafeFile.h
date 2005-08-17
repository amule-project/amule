//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef SAFEFILE_H
#define SAFEFILE_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "SafeFile.h"
#endif

#include <wx/filefn.h>			// Needed for wxSeekMode

#include "Types.h"				// Needed for LPCSTR
#include "StringFunctions.h"	// Needed for the utf8 types.
#include "MuleDebug.h"			// Needed for CMuleException

namespace Kademlia {
	class CUInt128;
}
using Kademlia::CUInt128;
class CMD4Hash;


///////////////////////////////////////////////////////////////////////////////
class CFileDataIO
{
public:
	virtual void Read(void *pBuf, off_t nCount) const;
	virtual void Write(const void *pBuf, size_t nCount);
	virtual off_t GetPosition() const = 0;
	virtual off_t GetLength() const = 0;

	virtual off_t Seek(off_t offset, wxSeekMode from = wxFromStart) const;

	virtual uint8		ReadUInt8() const;
	virtual uint16		ReadUInt16() const;
	virtual uint32		ReadUInt32() const;
	#if defined(__COMPILE_KAD__) && !defined(CLIENT_GUI)	
	virtual CUInt128	ReadUInt128() const;
	#endif
	virtual CMD4Hash	ReadHash() const;
 	virtual wxString	ReadString(bool bOptUTF8, uint8 SizeLen = 2 /* bytes */, bool SafeRead = false) const;
	virtual wxString	ReadOnlyString(bool bOptUTF8, uint16 raw_len) const;

	virtual void WriteUInt8(uint8 nVal);
	virtual void WriteUInt16(uint16 nVal);
	virtual void WriteUInt32(uint32 nVal);
	#if defined(__COMPILE_KAD__) && !defined(CLIENT_GUI)
	virtual void WriteUInt128(const CUInt128& pVal);
	#endif
	virtual void WriteHash(const CMD4Hash& hash);
	virtual void WriteString(const wxString& rstr, EUtf8Str eEncode = utf8strNone, uint8 SizeLen = 2 /* bytes */);

protected:
	virtual ~CFileDataIO();
	virtual off_t doRead(void *pBuf, off_t nCount) const = 0;
	virtual size_t doWrite(const void *pBuf, size_t nCount) = 0;
	virtual off_t doSeek(off_t offset) const = 0;

private:
	void WriteStringCore(const char *s, EUtf8Str eEncode, uint8 SizeLen);
};
 

/**
 * The base class of IO exceptions used by
 * the CSafeFileIO interface and implementations
 * of the interface.
 */
struct CSafeIOException : public CMuleException
{
	CSafeIOException(const wxString& what);
};


/**
 * This exception is thrown when attempts are 
 * made at reading past the end of the file.
 *
 * This typically happens when a invalid packet
 * is received that is shorter than expected and
 * is not fatal.
 */
struct CEOFException : public CSafeIOException {
	CEOFException(const wxString& what);	
};


/**
 * This exception reflects a failure in performing
 * basic IO operations read and write. It will be
 * thrown in case a read or a write fails to read
 * or write the specified number of bytes.
 */
struct CIOFailureException : public CSafeIOException {
	CIOFailureException(const wxString& what);
};


#endif // SAFEFILE_H
