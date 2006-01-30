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

#include "test.h"
#include "testregistry.h"

using namespace muleunit;

Test::Test(const wxString& testCaseName, const wxString& testName)
		: m_testCaseName(testCaseName),
		  m_testName(testName)
{
	TestRegistry::addTest(this);
}


Test::~Test()
{
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


const wxString& Test::getTestName() const
{
	return m_testName;
}


const wxString& Test::getTestCaseName() const
{
	return m_testCaseName;
}

