/*
 * Copyright (C) 2004 aMule Team ( http://www.amule.org )
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#if !defined(__GSOCKET_FIX_H__)

#define __GSOCKET_FIX_H__

#if !defined(wxSUBRELEASE_NUMBER)
	#define wxSUBRELEASE_NUMBER 0
#endif

#if !defined(wxCHECK_VERSION_FULL)

	#define wxCHECK_VERSION_FULL(major,minor,release,subrel) \
		(wxMAJOR_VERSION > (major) || \
		(wxMAJOR_VERSION == (major) && wxMINOR_VERSION > (minor)) || \
		(wxMAJOR_VERSION == (major) && wxMINOR_VERSION == (minor) && \
		 	wxRELEASE_NUMBER > (release)) || \
		 wxMAJOR_VERSION == (major) && wxMINOR_VERSION == (minor) && \
		 	wxRELEASE_NUMBER == (release) && wxSUBRELEASE_NUMBER >= (subrel))

#endif

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

