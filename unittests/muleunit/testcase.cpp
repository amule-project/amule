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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//


#include "testcase.h"
#include "test.h"
#include "testresult.h"

#include "MuleDebug.h"

#include <exception>

using namespace muleunit;

TestCase::TestCase(const wxString& name, TestResult *testResult)
		: m_failuresCount(0),
		  m_successesCount(0),
		  m_name(name),
		  m_testResult(testResult),
		  m_ran(false)
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

int TestCase::getFailuresCount() const
{
	return m_failuresCount;
}

int TestCase::getSuccessesCount() const
{
	return m_successesCount;
}

bool TestCase::ran() const
{
	return m_ran;
}

const wxString& TestCase::getName() const
{
	return m_name;
}

void TestCase::updateCount(Test *test)
{
	if (test->getTestFailures().empty()) {
		m_successesCount++;
	} else {
		m_failuresCount++;
	}
}


void Print(const wxChar *pszFormat, ...)
{
    va_list argptr;
    va_start(argptr, pszFormat);

	wxPuts(wxString::FormatV(pszFormat, argptr).c_str());
}


void TestCase::run()
{
	Print(wxT("\nRunning test-collection \"%s\" with %u test-cases:"),
		m_name.c_str(), m_tests.size());

	TestList::iterator it = m_tests.begin();
	for (; it != m_tests.end(); ++it) {
		Test* test = *it;
		
		Print(wxT("\tTest \"%s\" "), test->getTestName().c_str());

		test->setUp();
		test->run();
		test->tearDown();
		updateCount(test);

		const TestFailureList failures = test->getTestFailures();

		TestFailureList::const_iterator it = failures.begin();
		for (; it != failures.end(); ++it) {
			Print(wxT("\t\tFailure: \"%s\" line %ld in %s"),
					 it->message.c_str(),
					 it->lineNumber,
					 it->fileName.c_str());
		}
	}

	m_ran = true;

	m_testResult->addResult(this);
}

