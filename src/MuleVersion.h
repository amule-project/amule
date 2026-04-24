//							-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2016 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef MULEVERSION_H
#define MULEVERSION_H

#include "config.h"		// Needed for VERSION and ASIO_SOCKETS

#ifdef ASIO_SOCKETS
#	define MULEVERSION_RETVAL_BEGIN		wxString ver(
#	define MULEVERSION_RETVAL_END		);
#	define MULEVERSION_BOOST_VERSION	ver += " and Boost " + MuleBoostVersion;
#	define MULEVERSION_ADD_BEGIN		ver +=
#	define MULEVERSION_ADD_END		;
#	define MULEVERSION_RETURN_RESULT	return ver;

/**
 * Version of Boost aMule is compiled with.
 *
 * This variable exists only if aMule is compiled with Boost.Asio networking.
 * Defined in LibSocketAsio.cpp.
 */
extern wxString MuleBoostVersion;

#else
#	define MULEVERSION_RETVAL_BEGIN		return wxString(
#	define MULEVERSION_RETVAL_END
#	define MULEVERSION_BOOST_VERSION
#	define MULEVERSION_ADD_BEGIN
#	define MULEVERSION_ADD_END
#	define MULEVERSION_RETURN_RESULT	);
#endif


/**
 * Returns a description of the version of aMule being used.
 *
 * @return A detailed description of the aMule version, including wx information.
 *
 * Use this rather than just using the VERSION or CURRENT_VERSION_LONG
 * constants, when displaying information to the user. The purpose is to
 * help with debugging.
 */
inline wxString GetMuleVersion()
{
	MULEVERSION_RETVAL_BEGIN
		VERSION
		" compiled with "

// Figure out the toolkit used by wxWidgets...
#if defined(__WXGTK__)
#	if defined(__WXGTK3__)
#		define MULEVERSION_WXTOOLKIT	"GTK3"
#	elif defined(__WXGTK2__)
#		define MULEVERSION_WXTOOLKIT	"GTK2"
#	else
#		define MULEVERSION_WXTOOLKIT	"GTK"
#	endif
#elif defined(__WXOSX__)
#	if defined(__WXOSX_CARBON__)
#		define MULEVERSION_WXTOOLKIT	"OSX Carbon"
#	elif defined(__WXOSX_COCOA__)
#		define MULEVERSION_WXTOOLKIT	"OSX Cocoa"
#	elif defined(__WXOSX_IPHONE__)
#		define MULEVERSION_WXTOOLKIT	"OSX iPhone"
#	else
#		define MULEVERSION_WXTOOLKIT	"OSX"
#	endif
#elif defined(__WXCOCOA__)
#		define MULEVERSION_WXTOOLKIT	"Cocoa"
#elif defined(__WXMAC__)
#		define MULEVERSION_WXTOOLKIT	"Mac"
#elif defined(__WXMSW__) || defined(__WINDOWS__)
#	if defined(__VISUALC__)
#		define MULEVERSION_WXTOOLKIT	"MSW VC"
#	elif defined(__WXMICROWIN__)
#		define MULEVERSION_WXTOOLKIT	"MicroWin"
#	else
#		define MULEVERSION_WXTOOLKIT	"MSW"
#	endif
#elif defined(__NANOX__)
#		define MULEVERSION_WXTOOLKIT	"NanoX"
#elif defined(__WXMOTIF__)
#		define MULEVERSION_WXTOOLKIT	"Motif"
#elif defined(__WXMGL__)
#		define MULEVERSION_WXTOOLKIT	"MGL"
#elif defined(__WXPM__)
#		define MULEVERSION_WXTOOLKIT	"PM"
#elif defined(__WXDFB__)
#		define MULEVERSION_WXTOOLKIT	"DirectFB"
#elif defined(__WXX11__)
#		define MULEVERSION_WXTOOLKIT	"X11"
#endif

// ...and describe it.
#if defined(__WXBASE__)
		"wxBase"
#	ifdef MULEVERSION_WXTOOLKIT
		"(" MULEVERSION_WXTOOLKIT ")"
#	endif
#elif defined(__WXUNIVERSAL__)
		"wxUniversal"
#	ifdef MULEVERSION_WXTOOLKIT
		"(" MULEVERSION_WXTOOLKIT ")"
#	endif
#else
#	ifdef MULEVERSION_WXTOOLKIT
		"wx" MULEVERSION_WXTOOLKIT
#	else
		"wxWidgets"
#	endif
#endif

		// wxWidgets version
		" v" wxSTRINGIZE_T(wxMAJOR_VERSION) "." wxSTRINGIZE_T(wxMINOR_VERSION) "." wxSTRINGIZE_T(wxRELEASE_NUMBER)
	MULEVERSION_RETVAL_END

	// Describe Boost version, if compiled with Boost.Asio
	MULEVERSION_BOOST_VERSION

#if defined(__DEBUG__) || defined(SVNDATE)
	MULEVERSION_ADD_BEGIN
#	ifdef __DEBUG__
		" (Debugging)"
#	endif
#	ifdef SVNDATE
		" (Snapshot: " SVNDATE ")"
#	endif
	MULEVERSION_ADD_END
#endif

	MULEVERSION_RETURN_RESULT
}

#undef MULEVERSION_RETVAL_BEGIN
#undef MULEVERSION_RETVAL_END
#undef MULEVERSION_BOOST_VERSION
#undef MULEVERSION_ADD_BEGIN
#undef MULEVERSION_ADD_END
#undef MULEVERSION_RETURN_RESULT
#undef MULEVERSION_WXTOOLKIT

#endif /* MULEVERSION_H */
