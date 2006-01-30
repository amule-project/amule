//
// Copyright (C) 2005-2006Mikkel Schubert (Xaignar@amule.org)
// Copyright (C) 2004 Barthelemy Dagenais (barthelemy@prologique.com)
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
#include "MuleDebug.h"

using namespace muleunit;

wxString GetFullMuleVersion()
{
	return wxT("UnitTest");
}


class UnitTestApp : public wxAppConsole
{
public:
	int OnRun() {
		return (TestRegistry::runAndPrint() ? 0 : 1);
	}

	void OnUnhandledException() {
		::OnUnhandledException();
	}
};


DECLARE_APP(UnitTestApp);
IMPLEMENT_APP(UnitTestApp);

