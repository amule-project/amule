// This file is part of the aMule Project.
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

#ifndef MFC_H
#define MFC_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/dynarray.h>	// Needed for WX_DEFINE_ARRAY

#include "types.h"		// Needed for uint16
#include "position.h"		// Needed for VoidList, VoidHash and _POSITION

typedef char TCHAR;

int stricmp( const char* s1, const char* s2 );

/*
This is moved to "CTypedPtrList.h"
template<class BASE_CLASS, class TYPE>
  class CTypedPtrList : public CList<TYPE,TYPE>
*/


// default hasher
template<class ARG_KEY>

unsigned int HashKey(ARG_KEY key) {
  return ((unsigned int)(void*)(unsigned long)key);
}

template<class KEY,class ARG_KEY,class VALUE,class ARG_VALUE>
class CMap : public wxObject 
{
protected:
	VoidHash internalHash;

public:
	CMap(int blocks=0) {};
	~CMap() {internalHash.clear();}

	int GetCount() const {
		return internalHash.size();
	}
	
	bool IsEmpty() const {
		return internalHash.empty();
	}

	bool Lookup(ARG_KEY key,VALUE& value) {
		VoidHash::iterator it=internalHash.find(HashKey(key));
		if(it!=internalHash.end()) {
			value=(VALUE)((unsigned long)it->second);
			return true;
		} else {
			return false;
		}
	};
	
	void SetAt(ARG_KEY key,ARG_VALUE newValue) {
		internalHash[HashKey(key)]=(void*)((unsigned long)newValue);
	};
	
	VALUE& operator[](ARG_KEY key) {
		return (VALUE&)internalHash[HashKey(key)];
	};
	
	bool RemoveKey(ARG_KEY key) {
		int x=internalHash.erase(HashKey(key));
		if(x) { 
			return true;
		} else {
			return false;
		}
	};

	void RemoveAll() {
		internalHash.clear();
	};
	
	POSITION GetStartPosition() /*const*/ {
		if(internalHash.empty()) {
			return 0;
		} else {
			positioner.it=internalHash.begin();
			return (POSITION)&positioner;
		}
	};

	void GetNextAssoc(POSITION& nextPos,VALUE& val) {
		VoidHash::iterator it=nextPos->it;
		val=(VALUE)it->second;
		it++;
		if(it!=internalHash.end()) {
			positioner.it=it;
			nextPos=(POSITION)&positioner;
		} else {
			nextPos=0;
		}
	}

	void GetNextAssoc(POSITION& nextPos,KEY& key,VALUE& val) {
		VoidHash::iterator it=nextPos->it;
		key=(KEY)it->first;
		val=(VALUE)it->second;
		it++;
		if(it!=internalHash.end()) {
			positioner.it=it;
			nextPos=(POSITION)&positioner;
		} else {
			nextPos=0;
		}
	};

	unsigned int GetHashTableSize() const {
		return 1024;
	};
	
	void InitHashTable(unsigned int size,bool allocnow=true) {
		// nothing here
	};

private:
	VoidHash::const_iterator myit;
	_POSITION positioner;

// END CMAP
};

char *itoa(int i, char *a, int r);

#endif // MFC_H
