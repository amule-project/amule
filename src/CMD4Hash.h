// This file is part of the aMule project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
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

#ifndef CMD4HASH_H
#define CMD4HASH_H

class wxString;


#define MD4HASH_LENGTH 16


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
	 * Default contructor.
	 *
	 * The default contructor creates an empty hash of length 16.
	 * Each field of the char array has an initial value of zero.
	 */
	CMD4Hash();
	
	/**
	 * Cast a unsigned char array to a CMD4Hash.
	 * 
	 * @param hash The array to be cast. It MUST be 16 chars long, not including any possible zero-terminator!
	 */
	CMD4Hash(unsigned char hash[]);

	/**
	 * Copy constructor.
	 *
	 * @param hash The hash to be copied.
	 */
	CMD4Hash(const CMD4Hash& hash);

	/**
	 * Cast constructor from wxString.
	 * 
	 * @param hash The hexadecimal representation to convert. Length MUST be 32!
	 *
	 * Casts the hexadecimal representation of a MD4 hash to a CMD4Hash.
	 * It is estientially the same as calling the decode function.
	 */
	CMD4Hash(const wxString& hash);
	
	
	/**
	 * Equality operator.
	 *
	 * Returns true if all fields of both hashes are the same.
	 */
	bool operator == (const CMD4Hash& other_hash) const;
	/**
	 * Non-equality operator
	 *
	 * Returns true if there is any difference between the two hashes.
	 */
	bool operator != (const CMD4Hash& other_hash) const;

	
	/**
	 * Returns true if the hash is empty.
	 *
	 * @return True if all fields are zero, false otherwise.
	 *
	 * This functions checks the contents of the hash and returns true
	 * only if each field of the array contains the value zero.
	 * To achive an empty hash, the function Clear() can be used.
	 */
	bool IsEmpty() const;
	
	/** 
	 * Resets the contents of the hash.
	 *
	 * This functions sets the value of each field of the hash to zero.
	 * IsEmpty() will return true after a call to this function.
	 */
	void Clear();

	
	/**
	 * Decodes a 32 char long hexadecimal representation of a MD4 hash.
	 *
	 * @param hash The hash representation to be converted. Length must be 32.
	 *
	 * Based on functions from the Gnucleus project [found by Tarod].
	 * This function converts a hexadecimal representation of a MD4
	 * hash and stores it in the m_hash data-member.
	 */ 
	void Decode(const wxString& hash);
	
	/** 
	 * Creates a 32 char long hexadecimal representation of a MD4 hash.
	 *
	 * @return Hexadecimal representation of the m_hash data-member.
	 *
	 * Based on functions from the Gnucleus project [found by Tarod].
	 * This function creates a hexadecimal representation of the MD4 
	 * hash stored in the m_hash data-member and returns it.
	 */
	wxString Encode() const;

	
	/**
	 * Explicitly set the hash-array to the contents of a unsigned char array.
	 *
	 * @param hash The array to be assigned. Must be of length 16.
	 */
	inline void SetHash(unsigned char hash[]) {
		(*this) = hash;
	}
	
	/**
	 * Explicit access to the hash-array.
	 *
	 * @return Pointer to the hash array.
	 */
	inline unsigned char* GetHash() {
		return m_hash;
	}
	inline const unsigned char* GetHash() const {
		return m_hash;
	}
	
	/**
	 * Implicit access to the array.
	 *
	 * @return Pointer to the hash array.
	 *
	 * The purpose of this operators is to maintain backwards
	 * compatibility with code that expects the hash to be a 
	 * unsigned char array. Please use the GetHash function
	 * rather than this operator.
	 */
	inline operator unsigned char*() { 
		return m_hash;
	}
	inline operator const unsigned char*() const {
		return m_hash;
	}
	
	
private:
	//! The raw MD4-hash.
	//!
	//! The raw representation of the MD4-hash. In most cases, you should
	//! try to avoid direct access and instead use the member functions.
	unsigned char m_hash[MD4HASH_LENGTH];
};

#endif
