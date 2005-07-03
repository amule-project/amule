//
// Copyright (C) 2005 Mikkel Schubert (Xaignar@amule.org)
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include <wx/wx.h>
#include "testregistry.h"

using namespace muleunit;

int g_errCount = 0;

class MyApp : public wxApp
{
public:
    virtual bool OnInit()
	{
		const TestResult* result = TestRegistry::runAndPrint();

		g_errCount = result->getFailures();
		
		// Musn't enter the main loop
		return false;			
	}

	// Needed for console wxLibs
	virtual int OnRun()
	{
		return 0;
	}
};


// We need to override the default main code, since we want
// to return an err only if there were any failures in the tests.
IMPLEMENT_APP_NO_MAIN(MyApp);

int main(int argc, char **argv)
{
	wxEntry(argc, argv);

	return g_errCount;
}
