//
// MuleUnit: A minimalistic C++ Unit testing framework based on EasyUnit.
//
// Copyright (c) 2005-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2011 Barthelemy Dagenais ( barthelemy@prologique.com )
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <wx/wx.h>
#include "testregistry.h"
#include "test.h"
#include <common/MuleDebug.h>

using namespace muleunit;

wxString GetFullMuleVersion()
{
	return wxT("UnitTest");
}


unsigned s_disableAssertions = 0;


class UnitTestApp : public wxAppConsole
{
public:
	int OnRun() {
		return (TestRegistry::runAndPrint() ? 0 : 1);
	}

	void OnAssertFailure(const wxChar* file, int line,  const wxChar* func, const wxChar* cond, const wxChar* msg)
	{
		if (s_disableAssertions) {
			return;
		}

		wxString desc;
		if (cond && msg) {
			desc << cond << wxT(" -- ") << msg;
		} else if (cond) {
			desc << wxT("Assertion: ") << cond;
		} else {
			desc << msg;
		}

		throw CAssertFailureException(desc, file, line);
	}

#ifndef __WXMSW__
	void OnUnhandledException() {
		::OnUnhandledException();
	}
#endif

};


IMPLEMENT_APP_CONSOLE(UnitTestApp);
