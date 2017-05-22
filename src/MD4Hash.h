//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef CMD4HASH_H
#define CMD4HASH_H


#include "ArchSpecific.h"	// Needed for Raw{Peek,Poke}UInt64()

#include <common/MuleDebug.h>		// Needed for MULE_VALIDATE_PARAMS

#ifdef USE_WX_EXTENSIONS
#include <common/StringFunctions.h>
#endif

#include <string>


const size_t MD4HASH_LENGTH = 16;


/**
 * Container-class for the MD4 hashes used in aMule.
 *
 * This is a safe representation of the MD4 hashes used in aMule. By transparently
 * wrapping the char array used to store the hash, we get the advantages of
 * assigment, equality and non-equality operators, plus other nifty features.
 *
 * Please remember that the hashes are arrays with length 16 WITHOUT a zero-terminator!
 */
class CMD4Hash
{
public:
	/**
	 * Default constructor.
	 *
	 * The default constructor creates an empty hash of length 16.
	 * Each field of the char array has an initial value of zero.
	 */
	CMD4Hash() {
		Clear();
	}

	~CMD4Hash() {
	}

	/**
	 * Cast a unsigned char array to a CMD4Hash.
	 *
	 * @param hash The array to be cast.
	 *
	 * Please note that the array must either be a NULL pointer or be at least
	 * 16 chars long, not including any possible zero-terminator!
	 */
	explicit CMD4Hash(const unsigned char hash[]) {
		SetHash(hash);
	}

	/**
	 * Equality operator.
	 *
	 * Returns true if all fields of both hashes are the same.
	 */
	bool operator == (const CMD4Hash& other_hash) const {
		return (
			( RawPeekUInt64( m_hash      ) == RawPeekUInt64( other_hash.m_hash      ) ) &&
			( RawPeekUInt64( m_hash + 8  ) == RawPeekUInt64( other_hash.m_hash + 8  ) )
		);
	}

	/**
	 * Non-equality operator
	 *
	 * Returns true if there is any difference between the two hashes.
	 */
	bool operator != (const CMD4Hash& other_hash) const {
		return !(*this == other_hash);
	}

	/**
	 * Less than operator.
	 *
	 * @return True if the hash is less than other_hash, false otherwise.
	 *
	 * The purpose of this function is to enable the usage of CMD4Hashes in
	 * sorted STL containers like std::map.
	 */
	bool operator  < (const CMD4Hash& other_hash) const {
		for ( size_t i = 0; i < MD4HASH_LENGTH; ++i ) {
			if ( m_hash[i] < other_hash.m_hash[i] ) {
				return true;
			} else if ( other_hash.m_hash[i] < m_hash[i] ) {
				return false;
			}
		}

		return false;
	}


	/**
	 * Returns true if the hash is empty.
	 *
	 * @return True if all fields are zero, false otherwise.
	 *
	 * This functions checks the contents of the hash and returns true
	 * only if each field of the array contains the value zero.
	 * To achive an empty hash, the function Clear() can be used.
	 */
	bool IsEmpty() const {
		return (
			!RawPeekUInt64( m_hash      ) &&
			!RawPeekUInt64( m_hash + 8  )
		);
	}

	/**
	 * Resets the contents of the hash.
	 *
	 * This functions sets the value of each field of the hash to zero.
	 * IsEmpty() will return true after a call to this function.
	 */
	void Clear() {
		RawPokeUInt64( m_hash,			0 );
		RawPokeUInt64( m_hash + 8,		0 );
	}


	/**
	 * Decodes a 32 char long hexadecimal representation of a MD4 hash.
	 *
	 * @param hash The hash representation to be converted. Length must be 32.
	 * @return Return value specifies if the hash was succesfully decoded.
	 *
	 * This function converts a hexadecimal representation of a MD4
	 * hash and stores it in the m_hash data-member.
	 */

	bool Decode(const std::string& hash) {
		if (hash.length() != MD4HASH_LENGTH * 2) {
			return false;
		}

		for ( size_t i = 0; i < MD4HASH_LENGTH * 2; i++ ) {
			unsigned char word = toupper(hash[i]);

			if ((word >= '0') && (word <= '9')) {
				word -= '0';
			} else if ((word >= 'A') && (word <= 'F')) {
				word -= 'A' - 10;
			} else {
				// Invalid chars
				return false;
			}

			if (i % 2 == 0) {
				m_hash[i/2] = word << 4;
			} else {
				m_hash[i/2] += word;
			}
		}

		return true;
	}

	#ifdef USE_WX_EXTENSIONS
	bool Decode(const wxString& hash) {
		return Decode(std::string(unicode2char(hash)));
	}
	#endif

	/**
	 * Creates a 32 char long hexadecimal representation of a MD4 hash.
	 *
	 * @return Hexadecimal representation of the m_hash data-member.
	 *
	 * This function creates a hexadecimal representation of the MD4
	 * hash stored in the m_hash data-member and returns it.
	 */
	std::string EncodeSTL() const {
		std::string Base16Buff;

		for ( size_t i = 0; i < MD4HASH_LENGTH*2; i++ ) {
			size_t x = ( i % 2 == 0 ) ? ( m_hash[i/2] >> 4 ) : ( m_hash[i/2] & 0xf );

			if ( x <  10 ) {
				Base16Buff += (char)( x + '0' );
			} else {
				Base16Buff += (char)(  x + ( 'A' - 10 ));
			}
		}

		return Base16Buff;
	}

	#ifdef USE_WX_EXTENSIONS
	wxString Encode() const {
		return char2unicode(EncodeSTL().c_str());
	}
	#endif

	/**
	 * Explicitly set the hash-array to the contents of a unsigned char array.
	 *
	 * @param hash The array to be assigned.
	 *
	 * The hash must either be a NULL pointer or be of length 16.
	 */
	void SetHash(const unsigned char hash[]) {
		if ( hash ) {
			RawPokeUInt64( m_hash,		RawPeekUInt64( hash ) );
			RawPokeUInt64( m_hash + 8,	RawPeekUInt64( hash + 8 ) );
		} else {
			Clear();
		}
	}

	/**
	 * Explicit access to the hash-array.
	 *
	 * @return Pointer to the hash array.
	 */
	unsigned char* GetHash() {
		return m_hash;
	}
	const unsigned char* GetHash() const {
		return m_hash;
	}

	/**
	 * Explic access to values in the hash-array.
	 *
	 * @param i An index less than the length of an MD4 hash.
	 * @return The value (or its reference) at the given index.
	 */
	unsigned char operator[](size_t i) const {
		MULE_VALIDATE_PARAMS(i < MD4HASH_LENGTH, wxT("Invalid index in CMD4Hash::operator[]"));
		return m_hash[i];
	}

	unsigned char& operator[](size_t i) {
		MULE_VALIDATE_PARAMS(i < MD4HASH_LENGTH, wxT("Invalid index in CMD4Hash::operator[]"));
		return m_hash[i];
	}

private:
	//! The raw MD4-hash.
	//!
	//! The raw representation of the MD4-hash. In most cases, you should
	//! try to avoid direct access and instead use the member functions.
	unsigned char m_hash[MD4HASH_LENGTH];
};


#endif
// File_checked_for_headers
