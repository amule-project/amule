//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef MAPKEY_H
#define MAPKEY_H

#include <wx/object.h>		// Needed for wxObject

#include "types.h"		// Needed for uint32
#include "mfc.h"		// Needed for HashKey

class CCKey : public wxObject {
public:
	CCKey(const uchar* key = 0)		{m_key = key;}
	CCKey(const unsigned int key)		{m_key = (uchar*)key;}
	CCKey(const CCKey& k1)		{m_key = k1.m_key;}

	friend bool operator<(const CCKey& k1,const CCKey&k2)
	{
		return memcmp(k1.m_key, k2.m_key, 16) < 0;
	}

	CCKey& operator=(const CCKey& k1)						{m_key = k1.m_key; return *this; }
	friend bool operator==(const CCKey& k1,const CCKey& k2)	{return !memcmp(k1.m_key,k2.m_key,16);}
	
	const uchar* m_key;
};

template<> inline unsigned int HashKey(CCKey key){
   uint32 hash = 1;
   for (int i = 0;i != 16;i++)
	   hash += (key.m_key[i]+1)*((i*i)+1);
   return hash;
};

#endif // MAPKEY_H
