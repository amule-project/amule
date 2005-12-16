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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef SAFEFILE_H
#define SAFEFILE_H

#include <wx/filefn.h>			// Needed for wxSeekMode

#include <common/MuleDebug.h>			// Needef for CMuleException
#include <common/StringFunctions.h>	// Needed for the utf8 types.

#include "Types.h"				// Needed for uint* types

namespace Kademlia {
	class CUInt128;
}
using Kademlia::CUInt128;
class CMD4Hash;


/**
 * This class provides a interface for safe file IO.
 *
 * Basic IO operations will either succeed or throw an exception,
 * so that failure cannot be ignored. There are currently 3 types
 * of failures: Read past EOF, errors while reading, and errors 
 * while writing.
 *
 * Beyond basic IO, the interface provides functions for reading
 * and writing a number of simple data-types. These are all written
 * and read as little-endian in order to allow for communication
 * across platforms.
 *
 * Note that when empty areas are created, for instance by seeking
 * past the end, then writing, the value of bytes where no data was
 * explicitly written is not specified.
 */
class CFileDataIO
{
public:
	/**
	 * The Destructor does nothing, but is needed to allow
	 * for safe deletion objects via CFileDataIO pointers.
	 */
	virtual ~CFileDataIO();
	
	
	/**
	 * Must return the current position in the file.
	 */
	virtual uint64 GetPosition() const = 0;
	
	/**
	 * Must return the length of the file-object in bytes.
	 */
	virtual uint64 GetLength() const = 0;

	/**
	 * Returns true when the file-position is past or at the end of the file.
	 */
	virtual bool Eof() const;

	
	/**
	 * Changes the file position.
	 *
	 * Note that seeking to an negative position is an illegal operation.
	 * 
	 * @see wxFile::Seek
	 */
	virtual uint64 Seek(sint64 offset, wxSeekMode from = wxFromStart) const;

 
	/**
	 * Reads 'count' bytes into 'buffer'.
	 *
	 * @param buffer The target buffer.
	 * @param count The number of bytes to read.
	 *
	 * Note that Read will read the specified number of
	 * bytes unless this would read past the end of the
	 * file. In that case, a CEOFException is thrown and
	 * the position and target buffer is left unchanged.
	 *
	 * However, it is also possible that the read will
	 * fail due to IO errors (bad hardware, ect), in which 
	 * case an CIOFailureException will be thrown. 
	 */
	virtual void Read(void* buffer, size_t count) const;

	/**
	 * Write 'count' bytes from 'buffer' into the file.
	 *
	 * @param buffer The source-data buffer.
	 * @param count The number of bytes to write.
	 *
	 * Note that Write will throw a CIOFailureException
	 * if it fails to write the specified number of bytes,
	 * which can be caused by hardware failures, lack of
	 * free space, etc.
	 */
	virtual void Write(const void* buffer, size_t count);

	
	/**	
	 * Reads the given type from the file, stored as little-endian.
	 *
	 * @see CSafeFileIO::Read
	 */
	//@{
	virtual uint8		ReadUInt8() const;
	virtual uint16		ReadUInt16() const;
	virtual uint32		ReadUInt32() const;
	virtual CUInt128	ReadUInt128() const;
	virtual CMD4Hash	ReadHash() const;
	//@}

	/**
	 * Reads a string from the file.
	 * 
	 * @param bOptUTF8 Specifies if the string is UTF8 encoded.
	 * @param lenBytes The number of bytes used to store the string length.
	 * @param SafeRead Avoids throwing CEOFException, see below.
	 * @return The resulting text-string.
	 *
	 * Note that when SafeRead is set to true, CSafeFileIO will crop the length
	 * read from the lenght-field (see lenBytes), so that at most GetLength() -
	 * GetPosition() bytes are read.
	 *
	 * @see CSafeFileIO::Read
	 */
 	virtual wxString	ReadString(bool bOptUTF8, uint8 lenBytes = 2, bool SafeRead = false) const;

	/**
	 * Reads a string from the file, where the length is specified directly.
	 *
	 * @param bOptUTF8 Specifies if the string is UTF8 encoded.
	 * @param length The length of the string.
	 * @return The resulting text-string.
	 *
	 * This function is typically used when the text-fields length is not stored
	 * as an integer-field in front of the text-field.
	 */
	virtual wxString	ReadOnlyString(bool bOptUTF8, uint16 length) const;

	
	/**
	 * Writes a value of the given type to the file, storing it as little-endian.
	 *	
	 * @see CSafeFileIO::Write
	 */
	//@{
	virtual void WriteUInt8(uint8 value);
	virtual void WriteUInt16(uint16 value);
	virtual void WriteUInt32(uint32 value);
	virtual void WriteUInt128(const CUInt128& value);
	virtual void WriteHash(const CMD4Hash& value);
	//@}
	
	/**
	 * Writes a text-string to the file.
	 *
	 * @param str The string to be written.
	 * @param encoding The text-ecoding, see EUtf8Str.
	 * @param lenBytes The number of bytes used to store the string length.
	 *
	 * Valid values for the 'lenBytes' parameters is 0 bytes (no length field),
	 * 2 bytes and 4 bytes.
	 *
	 * @see CSafeFileIO::Write
	 */
	virtual void WriteString(const wxString& str, EUtf8Str encoding = utf8strNone, uint8 lenBytes = 2);

protected:
	/**
	 * The actual read / write function, as implemented by subclasses.
	 *
	 * @param buffer The buffer to read data into / write data from.
	 * @param count The number of bytes to read / written.
	 * @return The number of bytes read / written or -1 in case of errors.
	 *
	 * Note that the return value must be the actual number of bytes 
	 * read or written, with the exception that in case of errors, -1
	 * may be returned. This is because the return value is used to
	 * detect if the operation succeded.
	 *
	 * This function should not throw Either of the CSafeIOExceptions,
	 * this is done by the CSafeFileIO::Read and the CSafeFileIO::Write
	 * functions.
	 */
	//@{
	virtual sint64 doRead(void* buffer, size_t count) const = 0;
	virtual sint64 doWrite(const void* buffer, size_t count) = 0;
	//@}

	/**
	 * The actual seek function, as implemented by subclasses.
	 *
	 * @param offset The absolute offset to seek to.
	 * @return The resulting offset.
	 *
	 * This function should not throw of the CSafeIOExceptions,
	 * this is handled by the CSafeFileIO::Seek. At the moment,
	 * seeks that fail are considered a fatal error.
	 */
	virtual sint64 doSeek(sint64 offset) const = 0;

private:
	/**
	 * Helper-function that does the actual writing of the string.
	 * 
	 * @param str The string to be written.
	 * @param encoding The encoding of the string.
	 * @param lenBytes The number of bytes used to store the string length.
	 *
	 * The 
	 */
	void WriteStringCore(const char* str, EUtf8Str encoding, uint8 lenBytes);
};


/**
 * The base class of IO exceptions used by
 * the CSafeFileIO interface and implementations
 * of the interface.
 */
struct CSafeIOException : public CMuleException
{
	CSafeIOException(const wxString& type, const wxString& desc);
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
	CEOFException(const wxString& desc);	
};


/**
 * This exception reflects a failure in performing
 * basic IO operations read and write. It will be
 * thrown in case a read or a write fails to read
 * or write the specified number of bytes.
 */
struct CIOFailureException : public CSafeIOException {
	CIOFailureException(const wxString& type, const wxString& desc);
	CIOFailureException(const wxString& desc);
};


#endif // SAFEFILE_H
