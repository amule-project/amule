//
// This file is part of the aMule Project.
//
// Copyright (c) 2003 aMule Team ( http://www.amule-project.net )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef __GSOCKET_FIX_H__

#define __GSOCKET_FIX_H__

#include "Types.h"	// For wxCHECK_VERSION_FULL

// This flag is only defined from version 2.5.2.3 and up.
#if !wxCHECK_VERSION_FULL(2,5,2,3)
	// If wx < 2.5.3
	#if wxCHECK_VERSION_FULL(2,5,0,0)
		// If 2.5.0.0 <= wx < 2.5.2.3, we use our gsocket-2.5.c
		#define wxSOCKET_REUSEADDR 8
		#define GSOCK_OPTERR 10
	#else
		// If wx < 2.5.0 (i.e. 2.4.2), we use our gsocket.c
		#define wxSOCKET_REUSEADDR 0
	#endif
#else
	// If wx >= 2.5.2.3, there is nothing to do, wx socket.c is fine now.
#endif

#endif // __GSOCKET_FIX_H__

