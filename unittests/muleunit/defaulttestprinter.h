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

#ifndef DEFAULTTESTPRINTER_H
#define DEFAULTTESTPRINTER_H

#include "testprinter.h"
#include "testcase.h"
#include "test.h"
#include "testresult.h"
#include <cstdio>

namespace muleunit
{

/**
 * This is the default testprinter used by muleunit testregistry
 * when the user calls the runAndPrint() method without specifying
 * a testprinter.
 *
 * This testprinter writes plain text result to any supplied file.
 * The default file is the standard output.
 *
 * You may customize the outpur format by specifying the header level
 * and if you wish the testprinter to print details about each success.
 *
 * The default header level is normal and by default, the testprinter
 * does not print details about each success.
 */
class DefaultTestPrinter : public TestPrinter
{
public:

	/**
	 * Default constructor that sets the header level
	 * to normal and the output source to the standard
	 * output.
	 */
	DefaultTestPrinter();

	/**
	 * Prints a header depending of the header level and
	 * details about each test to the output_.
	 *
	 * @param testResult Results of all tests that were ran.
	 */
	virtual void print(const TestResult *testResult);

protected:
	virtual void printTests(TestCase *testCase);
	virtual void printResults(Test *test);
	int m_testsTotal;
	int m_testFailuresTotal;
	int m_failuresTotal;
};

} // MuleUnit ns
#endif // DEFAULTTESTPRINTER_H
