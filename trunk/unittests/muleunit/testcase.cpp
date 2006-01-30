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


#include "testcase.h"
#include "test.h"

#include "MuleDebug.h"

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


void Print(const wxChar *pszFormat, ...)
{
    va_list argptr;
    va_start(argptr, pszFormat);

	wxPuts(wxString::FormatV(pszFormat, argptr).c_str());

	va_end(argptr);
}


bool TestCase::run()
{
	Print(wxT("\nRunning test-collection \"%s\" with %u test-cases:"),
		m_name.c_str(), m_tests.size());

	bool failures = false;
	
	TestList::iterator it = m_tests.begin();
	for (; it != m_tests.end(); ++it) {
		Test* test = *it;
		
		Print(wxT("\tTest \"%s\" "), test->getTestName().c_str());

		bool wasSetup = false;
		try {
			test->setUp();
			wasSetup = true;
		} catch (const CTestFailureException& e) {
			failures = true;
			Print(wxT("\t\tFailure in setUp: \"%s\" line %ld in %s"),
					 e.m_msg.c_str(), e.m_line, e.m_file.c_str());
		}

		// Only run the test if it was actually setup. Otherwise we
		// are sure to get spurious failures.
		if (wasSetup) {
			try {
				test->run();
			} catch (const CTestFailureException& e) {
				failures = true;
				Print(wxT("\t\tFailure running: \"%s\" line %ld in %s"),
					 e.m_msg.c_str(), e.m_line, e.m_file.c_str());
			}
		}
		
		try {
			test->tearDown();
		} catch (const CTestFailureException& e) {
			failures = true;
			Print(wxT("\t\tFailure in tearDown: \"%s\" line %ld in %s"),
					 e.m_msg.c_str(), e.m_line, e.m_file.c_str());
		}
	}

	return not failures;
}

