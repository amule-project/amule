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

#ifndef testresult_H
#define testresult_H

#include <list>

namespace muleunit
{

class TestCase;

typedef std::list<TestCase*> TestCaseList;
	

class TestResult
{
public:
	TestResult();
	virtual ~TestResult();


	/**
	 * Get the number of testcases ran that succeeded.
	 *
	 * @return The number of testcases ran that succeeded.
	 */
	int getSuccesses() const;

	/**
	 * Get the number of testcases ran that failed.
	 *
	 * @return The number of testcases ran that failed.
	 */
	int getFailures() const;

	/**
	 * Get the TestCase list. This list contains all TestCase registered and
	 * not only those that were ran.
	 *
	 * @return The TestCase list
	 */
	const TestCaseList& getTestCases() const;

	/**
	 * Set the TestCase list and the size of the list.
	 *
	 * @param testCases TestCase list
	 * @param testCaseCount size of the TestCase list
	 */
	void setTestCases(const TestCaseList& testCases);

	/**
	 * Add a TestCase result. This is used by a TestCase after it has
	 * completed.
	 *
	 * @param testCase TestCase that ran and contains results to add to
	 * global results
	 */
	virtual void addResult(TestCase *testCase);

protected:
	int m_successes;
	int m_failures;

	TestCaseList m_testCases;
};

} // MuleUnit ns

#endif	// testresult_H
