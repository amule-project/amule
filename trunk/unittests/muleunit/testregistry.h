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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//


#ifndef TESTREGISTRY_H
#define TESTREGISTRY_H

#include "testresult.h"

namespace muleunit
{
	class Test;

/**
 * The TestRegistry is the main class used to register all tests,
 * and create appropriate TestCase. It can then be used to run
 * tests and print results. All methods that should be used by
 * the user are static.
 */
class TestRegistry
{
public:
	TestRegistry();
	~TestRegistry();

	/**
	 * Add a test in the registry. If the previous TestCase was not the same
	 * as the one of the current test, a new TestCase is created.
	 *
	 * @param test Test to be added
	 */
	static void addTest(Test *test);

	/**
	 * Pass all tests in the registry to the TestRunner runner and
	 * return the results of all tests ran. This will also print the results 
	 * using the default test printer (normal level of details and to the 
	 * standard output).
	 *
	 * @param runner The custom runner used to decided which test to run
	 * @return The test results
	 */
	static const TestResult* runAndPrint();

private:
	static TestRegistry& instance();
	void add(Test *test);
	
	const TestResult* runTests();
	TestResult m_testResult;

	TestCaseList m_testCases;
};

} // MuleUnit ns
#endif // TESTREGISTRY_H

