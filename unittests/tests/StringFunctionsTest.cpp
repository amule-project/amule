#include <muleunit/test.h>
#include <common/StringFunctions.h>

#include <wx/filename.h>

using namespace muleunit;

DECLARE_SIMPLE(StringFunctions)

TEST(StringFunctions, JoinPaths)
{
	const wxString sep = wxFileName::GetPathSeparator();

	ASSERT_EQUALS(wxT("a") + sep + wxT("b"), JoinPaths(wxT("a"), wxT("b")));
	ASSERT_EQUALS(wxT("a") + sep + wxT("b"), JoinPaths(wxT("a") + sep, wxT("b")));
	ASSERT_EQUALS(wxT("a") + sep + wxT("b"), JoinPaths(wxT("a"), sep + wxT("b")));
	ASSERT_EQUALS(wxT("a") + sep + wxT("b"), JoinPaths(wxT("a") + sep, sep + wxT("b")));
	ASSERT_EQUALS(wxT("a"), JoinPaths(wxT("a"), wxEmptyString));
	ASSERT_EQUALS(wxT("b"), JoinPaths(wxEmptyString, wxT("b")));
	ASSERT_EQUALS(wxEmptyString, JoinPaths(wxEmptyString, wxEmptyString));
}

///////////////////////////////////////////////////////////
// Tests for the CSimpleParser class

DECLARE_SIMPLE(SimpleParser)

	
TEST(SimpleParser, Constructor)
{
	// Empty strings are acceptable and should just return an empty string
	{
		CSimpleTokenizer tkz1(wxEmptyString, wxT('-'));
		ASSERT_EQUALS(wxT(""), tkz1.remaining());
		ASSERT_EQUALS(wxEmptyString, tkz1.next());
		ASSERT_EQUALS(wxT(""), tkz1.remaining());
		ASSERT_EQUALS(wxEmptyString, tkz1.next());
	}

	// String with no tokens should be return immediatly
	{
		CSimpleTokenizer tkz2(wxT(" abc "), wxT('-'));
		ASSERT_EQUALS(wxT(" abc "), tkz2.remaining());
		ASSERT_EQUALS(wxT(" abc "), tkz2.next());
		ASSERT_EQUALS(wxEmptyString, tkz2.next());
		ASSERT_EQUALS(wxEmptyString, tkz2.next());
	}
}


TEST(SimpleParser, EmptyTokens)
{
	{
		CSimpleTokenizer tkz1(wxT(" a"), wxT(' '));
		ASSERT_EQUALS(wxT(" a"), tkz1.remaining());
		ASSERT_EQUALS(0u, tkz1.tokenCount());
		
		ASSERT_EQUALS(wxEmptyString, tkz1.next());
		ASSERT_EQUALS(wxT("a"), tkz1.remaining());
		ASSERT_EQUALS(1u, tkz1.tokenCount());
		
		ASSERT_EQUALS(wxT("a"), tkz1.next());
		ASSERT_EQUALS(wxT(""), tkz1.remaining());
		ASSERT_EQUALS(1u, tkz1.tokenCount());
		
		ASSERT_EQUALS(wxEmptyString, tkz1.next());
		ASSERT_EQUALS(wxT(""), tkz1.remaining());
		ASSERT_EQUALS(1u, tkz1.tokenCount());
	}
	
	{	
		CSimpleTokenizer tkz2(wxT("c "), wxT(' '));
		ASSERT_EQUALS(wxT("c "), tkz2.remaining());
		ASSERT_EQUALS(0u, tkz2.tokenCount());
		
		ASSERT_EQUALS(wxT("c"), tkz2.next());
		ASSERT_EQUALS(wxT(""), tkz2.remaining());
		ASSERT_EQUALS(1u, tkz2.tokenCount());
		
		ASSERT_EQUALS(wxEmptyString, tkz2.next());
		ASSERT_EQUALS(wxT(""), tkz2.remaining());
		ASSERT_EQUALS(1u, tkz2.tokenCount());
		
		ASSERT_EQUALS(wxEmptyString, tkz2.next());
		ASSERT_EQUALS(wxT(""), tkz2.remaining());
		ASSERT_EQUALS(1u, tkz2.tokenCount());
	}

	{
		CSimpleTokenizer tkz3(wxT(" a c "), wxT(' '));
		ASSERT_EQUALS(wxT(" a c "), tkz3.remaining());
		ASSERT_EQUALS(0u, tkz3.tokenCount());

		ASSERT_EQUALS(wxEmptyString, tkz3.next());
		ASSERT_EQUALS(wxT("a c "), tkz3.remaining());
		ASSERT_EQUALS(1u, tkz3.tokenCount());

		ASSERT_EQUALS(wxT("a"), tkz3.next());
		ASSERT_EQUALS(wxT("c "), tkz3.remaining());
		ASSERT_EQUALS(2u, tkz3.tokenCount());

		ASSERT_EQUALS(wxT("c"), tkz3.next());
		ASSERT_EQUALS(wxT(""), tkz3.remaining());
		ASSERT_EQUALS(3u, tkz3.tokenCount());

		ASSERT_EQUALS(wxEmptyString, tkz3.next());
		ASSERT_EQUALS(wxT(""), tkz3.remaining());
		ASSERT_EQUALS(3u, tkz3.tokenCount());

		ASSERT_EQUALS(wxEmptyString, tkz3.next());
		ASSERT_EQUALS(wxT(""), tkz3.remaining());
		ASSERT_EQUALS(3u, tkz3.tokenCount());
	}
}


TEST(SimpleParser, NormalTokens)
{
	CSimpleTokenizer tkz(wxT("a c"), wxT(' '));
	ASSERT_EQUALS(wxT("a c"), tkz.remaining());
	ASSERT_EQUALS(0u, tkz.tokenCount());
	
	ASSERT_EQUALS(wxT("a"), tkz.next());
	ASSERT_EQUALS(wxT("c"), tkz.remaining());
	ASSERT_EQUALS(1u, tkz.tokenCount());
	
	ASSERT_EQUALS(wxT("c"), tkz.next());
	ASSERT_EQUALS(wxT(""), tkz.remaining());
	ASSERT_EQUALS(1u, tkz.tokenCount());
	
	ASSERT_EQUALS(wxEmptyString, tkz.next());
	ASSERT_EQUALS(wxT(""), tkz.remaining());
	ASSERT_EQUALS(1u, tkz.tokenCount());
	
	ASSERT_EQUALS(wxEmptyString, tkz.next());
	ASSERT_EQUALS(wxT(""), tkz.remaining());
	ASSERT_EQUALS(1u, tkz.tokenCount());
}

