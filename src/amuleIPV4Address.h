// This file is part of the aMule project.
//
// Copyright (c) 2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2002, Merkur <merkur-@users.sourceforge.net>
// Copyright (c) 2004, Carlo Wood <carlo@alinoe.com>
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

#ifndef AMULEIPV4ADDRESS_H
#define AMULEIPV4ADDRESS_H

#ifdef __WXMSW__
	#include <winsock.h>
#else
	#include <sys/socket.h>		// 
	#include <netinet/in.h>		// Those three are for 'struct in_addr' and 'inet_aton'.
	#include <arpa/inet.h>		//
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/object.h>		// Needed by wx/sckaddr.h
#include <wx/sckaddr.h>		// Needed for wxIPV4address

class amuleIPV4Address : public wxIPV4address {
  public:
  amuleIPV4Address(void) { }
#ifndef __WXMSW__  
  	virtual bool Hostname(unsigned long addr) {
		return GAddress_INET_SetHostAddress(m_address,addr)==GSOCK_NOERROR;
	};
	virtual bool Hostname(char* addr) {
		struct in_addr inaddr;
		inet_aton(addr,&inaddr);
		return GAddress_INET_SetHostAddress(m_address,inaddr.s_addr)==GSOCK_NOERROR;
	}
#endif

};

#endif // AMULEIPV4ADDRESS_H
