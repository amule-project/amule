//
// MuleUnit: A minimalistic C++ Unit testing framework based on EasyUnit.
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


#include "testresult.h"
#include "testcase.h"

using namespace muleunit;

TestResult::TestResult()
		: m_successes(0),
		  m_failures(0)
{}


TestResult::~TestResult()
{}


int TestResult::getSuccesses() const
{
	return m_successes;
}

int TestResult::getFailures() const
{
	return m_failures;
}

const TestCaseList& TestResult::getTestCases() const
{
	return m_testCases;
}

void TestResult::setTestCases(const TestCaseList& testCases)
{
	m_testCases = testCases;
}

void TestResult::addResult(TestCase *testCase)
{
	int tcFailures = testCase->getFailuresCount();

	if (tcFailures == 0) {
		m_successes++;
	} else {
		m_failures++;
	}
}


