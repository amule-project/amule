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


#include "testregistry.h"
#include "testcase.h"
#include "test.h"

using namespace muleunit;


TestRegistry::TestRegistry()
{
}


TestRegistry::~TestRegistry()
{
	TestCaseList::iterator it = m_testCases.begin();
	for (; it != m_testCases.end(); ++it) {
		delete *it;
	}
}


void TestRegistry::addTest(Test *test)
{
	instance().add(test);
}


bool TestRegistry::runAndPrint()
{
	return instance().runTests();
}


TestRegistry& TestRegistry::instance()
{
	static TestRegistry registry;
	return registry;
}


void TestRegistry::add(Test *test)
{
	const wxString tcName = test->getTestCaseName();
	const wxString tName = test->getTestName();


	if (m_testCases.empty() || m_testCases.back()->getName() != tcName) {
		m_testCases.push_back(new TestCase(tcName));
	}

	m_testCases.back()->addTest(test);
}


bool TestRegistry::runTests()
{
	bool success = true;

	TestCaseList::iterator it = m_testCases.begin();
	for (; it != m_testCases.end(); ++it) {
		success &= (*it)->run();
	}

	return success;
}

