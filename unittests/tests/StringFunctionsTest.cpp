#include <muleunit/test.h>
#include <common/StringFunctions.h>

using namespace muleunit;

DECLARE_SIMPLE(StringFunctions)


///////////////////////////////////////////////////////////
// Tests for the FuzzyStrCmp function

//! Returns the number of items in an array.
#define itemsof(x) (sizeof(x)/sizeof(x[0]))

TEST(StringFunctions, FuzzyStrCmp)
{
	struct FuzzyTest {
		const wxChar* a;
		const wxChar* b;
		int expected;
	};

	FuzzyTest checks[] = {
		{ wxT("a (10)"),	wxT("a (2)"),		 1},
		{ wxT("a (10)"),	wxT("b (2)"),		-1},
		{ wxT("c3 (7)"),	wxT("c3 (12)"),		-1},
		{ wxT("c3 (12)"),	wxT("c3 (7)"),		 1},
		{ wxT("c3 12d"),	wxT("c3 12d"),		 0},
		{ wxT("a (10)  "),	wxT("a (2)  "),		 1},
		{ wxT("a (10)  "),	wxT("b (2)  "),		-1},
		{ wxT("  c3 (7)"),	wxT("  c3 (12)"),	-1},
		{ wxT("  c3 (12)"),	wxT("  c3 (7)"),	 1},
		{ wxT("c3 12d"),	wxT("c3 12d"),		 0},
		{ wxT(""),		wxT(""),		 0},
		{ wxT(""),		wxT("c3 12d"),		-1},
		{ wxT("c3 12d"),	wxT(""),		 1},
		{ wxT(" "),		wxT("c3 12d"),		-1},
		{ wxT("c3 12d"),	wxT(" "),		 1},
		{ wxT("17.10"),		wxT("17.2"),		 1},
		{ wxT("  c3 (a)"),	wxT("  c3 (12)"),	 1},
		{ wxT("  c3 (12)"),	wxT("  c3 (a)"),	-1},
	};

	for (size_t i = 0; i < itemsof(checks); ++i) {
		wxString a = checks[i].a;
		wxString b = checks[i].b;

		ASSERT_EQUALS(checks[i].expected, FuzzyStrCmp(a, b));
	}
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

