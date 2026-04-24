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
		const char* a;
		const char* b;
		int expected;
	};

	FuzzyTest checks[] = {
		{ "a (10)",	"a (2)",		 1},
		{ "a (10)",	"b (2)",		-1},
		{ "c3 (7)",	"c3 (12)",		-1},
		{ "c3 (12)",	"c3 (7)",		 1},
		{ "c3 12d",	"c3 12d",		 0},
		{ "a (10)  ",	"a (2)  ",		 1},
		{ "a (10)  ",	"b (2)  ",		-1},
		{ "  c3 (7)",	"  c3 (12)",	-1},
		{ "  c3 (12)",	"  c3 (7)",	 1},
		{ "c3 12d",	"c3 12d",		 0},
		{ "",		"",		 0},
		{ "",		"c3 12d",		-1},
		{ "c3 12d",	"",		 1},
		{ " ",		"c3 12d",		-1},
		{ "c3 12d",	" ",		 1},
		{ "17.10",		"17.2",		 1},
		{ "  c3 (a)",	"  c3 (12)",	 1},
		{ "  c3 (12)",	"  c3 (a)",	-1},
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
		CSimpleTokenizer tkz1("", '-');
		ASSERT_EQUALS("", tkz1.remaining());
		ASSERT_EQUALS("", tkz1.next());
		ASSERT_EQUALS("", tkz1.remaining());
		ASSERT_EQUALS("", tkz1.next());
	}

	// String with no tokens should be return immediately
	{
		CSimpleTokenizer tkz2(" abc ", '-');
		ASSERT_EQUALS(" abc ", tkz2.remaining());
		ASSERT_EQUALS(" abc ", tkz2.next());
		ASSERT_EQUALS("", tkz2.next());
		ASSERT_EQUALS("", tkz2.next());
	}
}


TEST(SimpleParser, EmptyTokens)
{
	{
		CSimpleTokenizer tkz1(" a", ' ');
		ASSERT_EQUALS(" a", tkz1.remaining());
		ASSERT_EQUALS(0u, tkz1.tokenCount());

		ASSERT_EQUALS("", tkz1.next());
		ASSERT_EQUALS("a", tkz1.remaining());
		ASSERT_EQUALS(1u, tkz1.tokenCount());

		ASSERT_EQUALS("a", tkz1.next());
		ASSERT_EQUALS("", tkz1.remaining());
		ASSERT_EQUALS(1u, tkz1.tokenCount());

		ASSERT_EQUALS("", tkz1.next());
		ASSERT_EQUALS("", tkz1.remaining());
		ASSERT_EQUALS(1u, tkz1.tokenCount());
	}

	{
		CSimpleTokenizer tkz2("c ", ' ');
		ASSERT_EQUALS("c ", tkz2.remaining());
		ASSERT_EQUALS(0u, tkz2.tokenCount());

		ASSERT_EQUALS("c", tkz2.next());
		ASSERT_EQUALS("", tkz2.remaining());
		ASSERT_EQUALS(1u, tkz2.tokenCount());

		ASSERT_EQUALS("", tkz2.next());
		ASSERT_EQUALS("", tkz2.remaining());
		ASSERT_EQUALS(1u, tkz2.tokenCount());

		ASSERT_EQUALS("", tkz2.next());
		ASSERT_EQUALS("", tkz2.remaining());
		ASSERT_EQUALS(1u, tkz2.tokenCount());
	}

	{
		CSimpleTokenizer tkz3(" a c ", ' ');
		ASSERT_EQUALS(" a c ", tkz3.remaining());
		ASSERT_EQUALS(0u, tkz3.tokenCount());

		ASSERT_EQUALS("", tkz3.next());
		ASSERT_EQUALS("a c ", tkz3.remaining());
		ASSERT_EQUALS(1u, tkz3.tokenCount());

		ASSERT_EQUALS("a", tkz3.next());
		ASSERT_EQUALS("c ", tkz3.remaining());
		ASSERT_EQUALS(2u, tkz3.tokenCount());

		ASSERT_EQUALS("c", tkz3.next());
		ASSERT_EQUALS("", tkz3.remaining());
		ASSERT_EQUALS(3u, tkz3.tokenCount());

		ASSERT_EQUALS("", tkz3.next());
		ASSERT_EQUALS("", tkz3.remaining());
		ASSERT_EQUALS(3u, tkz3.tokenCount());

		ASSERT_EQUALS("", tkz3.next());
		ASSERT_EQUALS("", tkz3.remaining());
		ASSERT_EQUALS(3u, tkz3.tokenCount());
	}
}


TEST(SimpleParser, NormalTokens)
{
	CSimpleTokenizer tkz("a c", ' ');
	ASSERT_EQUALS("a c", tkz.remaining());
	ASSERT_EQUALS(0u, tkz.tokenCount());

	ASSERT_EQUALS("a", tkz.next());
	ASSERT_EQUALS("c", tkz.remaining());
	ASSERT_EQUALS(1u, tkz.tokenCount());

	ASSERT_EQUALS("c", tkz.next());
	ASSERT_EQUALS("", tkz.remaining());
	ASSERT_EQUALS(1u, tkz.tokenCount());

	ASSERT_EQUALS("", tkz.next());
	ASSERT_EQUALS("", tkz.remaining());
	ASSERT_EQUALS(1u, tkz.tokenCount());

	ASSERT_EQUALS("", tkz.next());
	ASSERT_EQUALS("", tkz.remaining());
	ASSERT_EQUALS(1u, tkz.tokenCount());
}

