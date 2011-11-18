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

#include "test.h"
#include "testregistry.h"
#include <list>

using namespace muleunit;

/** Entry for a context. */
struct BTEntry
{
	BTEntry(const wxChar* f, int l, const wxString& m)
		: file(f)
		, line(l)
		, msg(m)
	{ 
	}

	wxString file;
	int line;
	wxString msg;
};


static std::list<BTEntry> g_backtrace;

namespace muleunit {
	/** Structure used to contain a snapshot of the contexts. */
	struct BTList
	{
		BTList(const std::list<BTEntry>& lst)
			: snapshot(lst)
		{
		}

		std::list<BTEntry> snapshot;	
	};
}


CTestFailureException::CTestFailureException(const wxString& msg, const wxChar* file, long lineNumber)
	: m_bt(new BTList(g_backtrace)), m_message(msg.ToAscii())
{
	m_bt->snapshot.push_back(BTEntry(file, lineNumber, msg));
}


CTestFailureException::~CTestFailureException() throw()
{
	delete m_bt;
}


void CTestFailureException::PrintBT() const
{
	wxString indent = wxT("\t\t");
	std::list<BTEntry>::const_iterator it = m_bt->snapshot.begin();
	for (; it != m_bt->snapshot.end(); ++it) {
		indent += ' ';

		Print(indent + it->file + wxString::Format(wxT(":%i -- "), it->line) + it->msg);
	}
}


const char* CTestFailureException::what () const throw ()
{
	return m_message.c_str();
}


CContext::CContext(const wxChar* file, int line, const wxString& msg)
{
	g_backtrace.push_back(BTEntry(file, line, msg));
}


CContext::~CContext()
{
	g_backtrace.pop_back();
}

extern unsigned s_disableAssertions;


CAssertOff::CAssertOff()
{
	s_disableAssertions++;
}

CAssertOff::~CAssertOff()
{
	s_disableAssertions--;
}



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

