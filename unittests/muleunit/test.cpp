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

#include "test.h"
#include "testregistry.h"

using namespace muleunit;

Test::Test(const wxString& testCaseName, const wxString& testName)
		: m_testCaseName(testCaseName),
		  m_testName(testName),
		  m_failed(false)
{
	TestRegistry::addTest(this);
}


Test::~Test()
{
	TestPartResultList::iterator it = m_testPartResults.begin();
	for (; it != m_testPartResults.end(); ++it) {
		delete *it;
	}
}


void Test::setUp()
{
}


void Test::tearDown()
{
}


void Test::run()
{
}


void Test::addTestPartResult(TestPartResult *testPartResult)
{
	m_testPartResults.push_back(testPartResult);
	
	m_failed |= (testPartResult->getType() != success);
}


const TestPartResultList& Test::getTestPartResult() const
{
	return m_testPartResults;
}


bool Test::failed() const
{
	return m_failed;
}


const wxString& Test::getTestName() const
{
	return m_testName;
}


const wxString& Test::getTestCaseName() const
{
	return m_testCaseName;
}

