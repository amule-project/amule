//
// This file is part of the aMule Project.
//
// Copyright (c) 2005 Mikkel Schubert ( xaignar@users.sourceforge.net )
// Copyright (c) 2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "MuleDebug.h"
#endif

#include <exception>			// Needed for std::exception
#include <cxxabi.h>				// Needed for __cxxabiv1::
#include <signal.h>				// Needed for raise()

#include <wx/string.h>

#include "MuleDebug.h"			// Interface declaration
#include "StringFunctions.h"	// Needed for unicode2char
#include "OtherFunctions.h"		// Needed for GetFullMuleVersion()

/**
 * This functions displays a verbose description of 
 * any unhandled exceptions that occour and then
 * terminate the program by raising SIGABRT.
 */
void OnUnhandledException()
{
	std::type_info *t = __cxxabiv1::__cxa_current_exception_type();
    if (t) {
		// Note that "name" is the mangled name.
		char const *name = t->name();
		int status = -1;
		char *dem = 0;
	  
		dem = __cxxabiv1::__cxa_demangle(name, 0, 0, &status);
		printf("aMule terminated after throwing an instance of '%s'\n", (status ? name : dem));
		printf("\tVersion: %s\n", (const char*)unicode2char(otherfunctions::GetFullMuleVersion()));
		free(dem);

		try {
			throw;
		} catch (const std::exception& e) {
			printf("\twhat(): %s\n", e.what());
		} catch (const CMuleException& e) {
			printf("\twhat(): %s\n", (const char*)unicode2char(e.what()));
		} catch (const wxString& e) {
			printf("\twhat(): %s\n", (const char*)unicode2char(e));			
		} catch (...) {
			// Unable to retrieve cause of exception
		}
	}

	raise(SIGABRT);
};


void InstallMuleExceptionHandler()
{
	std::set_terminate(OnUnhandledException);
}


