//
// MuleUnit: A minimalistic C++ Unit testing framework based on EasyUnit.
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


#include "defaulttestprinter.h"
#include "testpartresult.h"

using namespace muleunit;


void Print(const wxChar *pszFormat, ...)
{
    va_list argptr;
    va_start(argptr, pszFormat);

	wxPuts(wxString::FormatV(pszFormat, argptr).c_str());
}


DefaultTestPrinter::DefaultTestPrinter()
		: m_testsTotal(0),
		  m_testFailuresTotal(0),
		  m_failuresTotal(0)
{
}


void DefaultTestPrinter::print(const TestResult *testResult)
{
	int failures = 0;
	int successes = 0;
	wxString state;
	wxString name;
	const TestCaseList& testCases = testResult->getTestCases();

	if (testCases.empty()) {
		Print(wxT("\nNo test ran"));
	} else {
		TestCaseList::const_iterator it = testCases.begin();
		for (; it != testCases.end(); ++it) {
			TestCase* testCase = *it;
			
			if (testCase->ran()) {
				name = testCase->getName();
				failures = testCase->getFailuresCount();
				successes = testCase->getSuccessesCount();

				if (failures) {
					state = wxT("FAILED");
				} else {
					state = wxT("SUCCEEDED");
				}

				Print(wxT("Test case \"%s\" %s with %d failure(s) and %d success(es):"),
						name.c_str(),
						state.c_str(),
						failures,
						successes);

				printTests(testCase);
			}
		}
	}

	Print(wxEmptyString);
}


void DefaultTestPrinter::printTests(TestCase *testCase)
{
	TestList tests = testCase->getTests();
	TestList::iterator it = tests.begin();

	for (; it != tests.end(); ++it) {
		Test* test = *it;
		
		wxString state;
		if (test->failed()) {
			state = wxT("FAILED :");
		} else {
			state = wxT("SUCCEEDED!");
		}

		Print(wxT("  Test \"%s\" %s"), test->getTestName().c_str(), state.c_str());
		printResults(test);
	}
}


void DefaultTestPrinter::printResults(Test *test)
{
	const TestPartResultList testPRs = test->getTestPartResult();

	TestPartResultList::const_iterator it = testPRs.begin();
	for (; it != testPRs.end(); ++it) {
		TestPartResult* testPR = *it;
		int type = testPR->getType();

		if (type == failure) {
			Print(wxT("    Failure: \"%s\" line %ld in %s\n"),
			         testPR->getMessage().c_str(),
			         testPR->getLineNumber(),
			         testPR->getFileName().c_str());
		} else if (type == error) {
			Print(wxT("    Error in %s: \"%s\"\n"),
			         test->getTestName().c_str(),
			         testPR->getMessage().c_str());
		}
	}
}


