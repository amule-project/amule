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
	#include <netinet/in.h>		// Those three are for htonl
	#include <arpa/inet.h>		//
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/object.h>		// Needed by wx/sckaddr.h
#include <wx/sckaddr.h>		// Needed for wxIPV4address
#include <wx/log.h>		// Needed for wxLogWarning

#include "otherfunctions.h"	// Needed for unicode2char


// This is fscking hard to maintain. wxWidgets 2.5.2 has changed internal
// ipaddress structs.
// prevent fscking dns queries
class amuleIPV4Address : public wxIPV4address {
  public:
  amuleIPV4Address(void) { }
#ifndef __WXMSW__  

	virtual bool Hostname(const wxString& name) {
		// Some people are sometimes fool.
		if (name.IsEmpty()) {
			wxLogWarning(wxT("Trying to set a NULL host: giving up") );
			return FALSE;
		}
		return Hostname(StringIPtoUint32(name));
	}

	virtual bool Hostname(uint32 ip) {
		// Some people are sometimes fool.
		if (!ip) {
			wxLogWarning(wxT("Trying to set a wrong ip: giving up") );
			return FALSE;
		}
		// We have to take care that wxIPV4address's internals changed on 2.5.2
		#if wxCHECK_VERSION(2,5,2)
			return GAddress_INET_SetHostAddress(m_address,ntohl(ip))==GSOCK_NOERROR;
		#else
			return GAddress_INET_SetHostAddress(m_address,ip)==GSOCK_NOERROR;
		#endif
	}
	
#endif

};

#endif // AMULEIPV4ADDRESS_H
