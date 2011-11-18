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


#include "testcase.h"
#include "test.h"

#include <exception>

using namespace muleunit;

TestCase::TestCase(const wxString& name)
		: m_name(name)
{}

TestCase::~TestCase()
{
}

void TestCase::addTest(Test *test)
{
	m_tests.push_back(test);
}

const TestList& TestCase::getTests() const
{
	return m_tests;
}

int TestCase::getTestsCount() const
{
	return m_tests.size();
}


const wxString& TestCase::getName() const
{
	return m_name;
}



bool TestCase::run()
{
	Print(wxT("\nRunning test-collection \"") + m_name + wxString::Format(wxT("\" with %u test-cases:"), m_tests.size()));

	bool failures = false;
	
	TestList::iterator it = m_tests.begin();
	for (; it != m_tests.end(); ++it) {
		Test* test = *it;
		
		Print(wxT("\tTest \"") + test->getTestName() + wxT("\" "));

		bool wasSetup = false;
		try {
			test->setUp();
			wasSetup = true;
		} catch (const CTestFailureException& e) {
			failures = true;
			Print(wxT("\t\tFailure in setUp:\n"));
			e.PrintBT();
		}

		// Only run the test if it was actually setup. Otherwise we
		// are sure to get spurious failures.
		if (wasSetup) {
			try {
				test->run();
			} catch (const CTestFailureException& e) {
				failures = true;
				Print(wxT("\t\tFailure running:"));
				e.PrintBT();
			}
		}
		
		try {
			test->tearDown();
		} catch (const CTestFailureException& e) {
			failures = true;
			Print(wxT("\t\tFailure in tearDown:"));
			e.PrintBT();
		}
	}

	return !failures;
}

