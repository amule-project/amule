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

// The length of a MD4 hash
#define MD4HASH_LENGTH 16


/* Container-class for the MD4 hashes used in amule. Please remember that the 
   hashes are arrays with length 16, and arn't a zero-terminated! */
class CMD4Hash
{
public:
	// Default contructor, creates an empty hash
	CMD4Hash();
	// Casts a unsigned char array to a CMD4 hash.
	// The array MUST be 16 chars long and not zero-terminated!
	CMD4Hash(unsigned char hash[]);
	// Copy constructor
	CMD4Hash(const CMD4Hash& hash);
	// Casts a hexadecimal representation of a MD4 hash to a MD4 hash
	// Same as calling the decode function. Length MUST be 32!
	CMD4Hash(const wxString& hash);
	
	
	// Equality and non equality operators
	bool operator == (const CMD4Hash& other_hash) const;
	bool operator != (const CMD4Hash& other_hash) const;

	
	// Returns true if the hash is empty (all fields == 0)
	bool IsEmpty() const;
	// Clears the hash
	void Clear();

	
	// Based on functions from the Gnucleus project [found by Tarod]
	// Decodes a 32 char long hexadecimal representation of a MD4 hash
	void Decode(const wxString& hash);
	// Creates a 32 char long hexadecimal representation of a MD4 hash
	wxString Encode() const;

	
	// Explicitly set the hash-array
	inline void SetHash(unsigned char hash[]) {
		(*this) = hash;
	}
	
	// Explicitly access the hash-array
	inline unsigned char* GetHash() {
		return m_hash;
	}
	
	// Implicit access the array, to maintain backwards compatibility
	// with code that expects the hash to be a unsigned char array
	inline operator unsigned char*() { 
		return m_hash;
	}
	
	
private:
	unsigned char m_hash[MD4HASH_LENGTH];
};

#endif
