// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

#include <map>			// Needed for std::map

struct IPRange_Struct {
	uint32		IPStart;
	uint32		IPEnd;
	uint8		AccessLevel;
	wxString	Description;
};

class CIPFilter
{
	
public:
	CIPFilter();
	~CIPFilter();
	void	AddBannedIPRange(uint32 IPstart, uint32 IPend, uint8 AccessLevel, const wxString& Description);
	void	RemoveAllIPs();
	bool	ProcessLineOk(wxString *sLine, unsigned long linecounter);
	int	LoadFromFile();
	void	SaveToFile();
	bool	IsFiltered(uint32 IP2test);
	const wxString& GetLastHit()		{ return lasthit;}
	uint16	BanCount()			{ return iplist.size(); }
	void 	Reload();

private:
	bool m_inet_atoh(wxString &s, uint32 *ip);
	wxString lasthit;
	typedef std::map<uint32, IPRange_Struct *> IPListMap;
	IPListMap iplist;
};

#endif // IPFILTER_H

