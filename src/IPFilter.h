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

#ifndef IPFILTER_H
#define IPFILTER_H

#include "types.h"		// Needed for uint8, uint16 and uint32
#include "CString.h"		// Needed for CString
#include "CArray.h"		// Needed for CArray
#include <wx/thread.h>

struct IPRange_Struct {
   uint32           IPstart;
   uint32           IPend;
   uint8			filter;
   CString			description;
   ~IPRange_Struct() {  }
};

static wxMutex s_IPfilter_Data_mutex;

class CIPFilter
{
	
public:
	CIPFilter();
	~CIPFilter();
	void	AddBannedIPRange(uint32 IPfrom,uint32 IPto,uint8 filter, CString desc);
	void	RemoveAllIPs();
	int		LoadFromFile();
	void	SaveToFile();
	bool	IsFiltered(uint32 IP2test);
	CString GetLastHit()				{ return lasthit;}
	uint16	BanCount()					{ return iplist.GetCount(); }
	void 	Reload();
private: 
	CString lasthit;
	CArray<IPRange_Struct*,IPRange_Struct*> iplist;
};

#endif // IPFILTER_H
