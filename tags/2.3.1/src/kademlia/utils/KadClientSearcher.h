//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 Dévai Tamás ( gonosztopi@amule.org )
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef KADEMLIA_UTILS_KADCLIENTSEARCHER_H
#define KADEMLIA_UTILS_KADCLIENTSEARCHER_H

// Interface class for non-kad classes which want to do clientsearches
namespace Kademlia
{

enum EKadClientSearchRes
{
	KCSR_SUCCEEDED,
	KCSR_NOTFOUND,
	KCSR_TIMEOUT
};

class CKadClientSearcher
{
      public:
	virtual	void KadSearchNodeIDByIPResult(EKadClientSearchRes status, const unsigned char* nodeID) = 0;
	virtual	void KadSearchIPByNodeIDResult(EKadClientSearchRes status, uint32_t ip, uint16_t port) = 0;
};

}

#endif /* KADEMLIA_UTILS_KADCLIENTSEARCHER_H */
