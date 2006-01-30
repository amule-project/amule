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


#ifndef TESTREGISTRY_H
#define TESTREGISTRY_H

#include <list>

namespace muleunit
{
	class Test;
	class TestCase;

	
typedef std::list<TestCase*> TestCaseList;


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
	 * Runs all added tests and prints the results.
	 *
	 * @return False if there were failures.
	 */
	static bool runAndPrint();

private:
	static TestRegistry& instance();
	void add(Test *test);
	
	bool runTests();
	TestCaseList m_testCases;
};

} // MuleUnit ns
#endif // TESTREGISTRY_H

