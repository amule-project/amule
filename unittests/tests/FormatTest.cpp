#include <muleunit/test.h>
#include <common/Format.h>
#include <limits.h>

#include <Types.h>

using namespace muleunit;

// Note: These tests have been written in accordance with the 
//       capabilities of printf found in printf(3).

#define ELEMENTS(x) (sizeof(x)/sizeof(x[0]))
#define MIN(x) std::numeric_limits<x>::min()
#define MAX(x) std::numeric_limits<x>::max()

// Needs reentrant wxOnAssert, which is missing in pre-2.8.8.
#if wxCHECK_VERSION(2, 8, 8)

DECLARE(Format)
	// Less is valid for the string type, so we need a cut
	// down test for that.
	void testFormat(const wxString& str, const wxChar* value) {
		for (int p = -1; p < 5; ++p) {
			wxString format;
				
			format += wxT("%");
			
			if (p == -1) {
				format += wxT(".");
			} else {
				format += wxString::Format(wxT(".%d"), p);
			}
				
			format += str;

			wxString reference = wxString::Format(format, value);
			wxString actual = CFormat(format) % value;
		
			ASSERT_EQUALS_M(reference, actual, format + wxT(": '") + reference + wxT("' != '") + actual + wxT("'"));
		}
	}
END_DECLARE;


TEST(Format, ConstructorAndGetString)
{
	{
		// Null should be valid
		CFormat fmt1(NULL);
		ASSERT_TRUE(fmt1.IsReady());
		ASSERT_EQUALS(wxT(""), fmt1.GetString());
	}

	{
		// Empty string should be valid
		CFormat fmt2(wxT(""));
		ASSERT_TRUE(fmt2.IsReady());
		ASSERT_EQUALS(wxT(""), fmt2.GetString());
	}

	{
		// Simple string with no format fields
		CFormat fmt3(wxT("a b c"));
		ASSERT_TRUE(fmt3.IsReady());
		ASSERT_EQUALS(wxT("a b c"), fmt3.GetString());
	}

	{
		// Simple string with one format field
		CFormat fmt4(wxT("a %i c"));
		ASSERT_FALSE(fmt4.IsReady());
		ASSERT_RAISES(CAssertFailureException, fmt4.GetString());

		CAssertOff null;
		ASSERT_EQUALS(wxT("a %i c"), fmt4.GetString());
	}
}


TEST(Format, SetStringAndGetString)
{
	CFormat format(NULL);
	ASSERT_TRUE(format.IsReady());
	ASSERT_EQUALS(wxT(""), format.GetString());

	// Empty string should be valid
	format = CFormat(wxT(""));
	ASSERT_TRUE(format.IsReady());
	ASSERT_EQUALS(wxT(""), format.GetString());

	// Simple string with no format fields
	format = CFormat(wxT("a b c"));
	ASSERT_TRUE(format.IsReady());
	ASSERT_EQUALS(wxT("a b c"), format.GetString());

	// Simple string with one format field
	format = CFormat(wxT("a %i c"));
	ASSERT_FALSE(format.IsReady());
	ASSERT_RAISES(CAssertFailureException, format.GetString());

	CAssertOff null;
	ASSERT_EQUALS(wxT("a %i c"), format.GetString());
}



//! Implementation for the Standard type test
#define STANDARD_TEST(cformat, wxformat, value) \
	{ \
		wxString reference	= wxString::Format(wxString(wxT("%")) + wxformat, value); \
		wxString actual		= CFormat(wxString(wxT("%")) + cformat) % value; \
 		ASSERT_EQUALS_M(reference, actual, wxString(wxT("%")) << wxformat \
				<< wxT(" vs. %") << cformat << wxT(": '") + reference + wxT("' != '") + actual + wxT("'")); \
	}
	


//! Test the two boundries and a middle value of the specificed format
#define STANDARD_TYPE_TESTS(cformat, wxformat, type) \
	STANDARD_TEST(cformat, wxformat, MIN(type)); \
	STANDARD_TEST(cformat, wxformat, (type)(MAX(type) / 2)); \
	STANDARD_TEST(cformat, wxformat, MAX(type)); \

TEST(Format, InjectwxChar)
{
	STANDARD_TYPE_TESTS(wxT("c"), wxT("c"), wxChar);
}


//! All length specifiers are supported and should yield the same result
const wxChar* int_lengths[] =
{
	wxT("h"),
	wxT(""),
	wxT("l"),
	wxT("ll"),
	NULL
};

//! All signed types are supported, and should yield the same result
const wxChar* sint_types[] =
{
	wxT("d"),
	wxT("i"),
	NULL
};


//! All unsigned types are supported, and should yield the same result
const wxChar* uint_types[] =
{
	wxT("u"),
	wxT("o"),
	wxT("x"),
	wxT("X"),
	NULL
};


TEST(Format, InjectInteger)
{
	const wxChar** sint_entry = sint_types;
	
	while (*sint_entry) {
		const wxChar** len_entry = int_lengths;
		
		while (*len_entry) {
			wxString entry = wxString() << *len_entry << *sint_entry;
			
			STANDARD_TYPE_TESTS(entry, wxString() << wxT("h") << *sint_entry,	signed short);
			STANDARD_TYPE_TESTS(entry, wxString() << wxT("") << *sint_entry,	signed int);
			STANDARD_TYPE_TESTS(entry, wxString() << wxT("l") << *sint_entry,	signed long);
			STANDARD_TYPE_TESTS(entry, wxString() << wxLongLongFmtSpec << *sint_entry,	signed long long);
			
			++len_entry;
		}

		++sint_entry;
	}

	
	const wxChar** uint_entry = uint_types;
	while (*uint_entry) {
		const wxChar** len_entry = int_lengths;

		while (*len_entry) {
			wxString entry = wxString() << *len_entry << *uint_entry;
			
			STANDARD_TYPE_TESTS(entry, wxString() << wxT("h") << *uint_entry,	unsigned short);
			STANDARD_TYPE_TESTS(entry, wxString() << wxT("") << *uint_entry,	unsigned int);
			STANDARD_TYPE_TESTS(entry, wxString() << wxT("l") << *uint_entry,	unsigned long);
			STANDARD_TYPE_TESTS(entry, wxString() << wxLongLongFmtSpec << *uint_entry,	unsigned long long);
			
			++len_entry;
		}

		++uint_entry;
	}
}


TEST(Format, InjectFloatAndDouble)
{
	STANDARD_TYPE_TESTS(wxT("e"), wxT("e"), float);
	STANDARD_TYPE_TESTS(wxT("E"), wxT("E"), float);
	STANDARD_TYPE_TESTS(wxT("f"), wxT("f"), float);
	STANDARD_TYPE_TESTS(wxT("F"), wxT("F"), float);
	STANDARD_TYPE_TESTS(wxT("g"), wxT("g"), float);
	STANDARD_TYPE_TESTS(wxT("G"), wxT("G"), float);
	
	STANDARD_TYPE_TESTS(wxT("e"), wxT("e"), double);
	STANDARD_TYPE_TESTS(wxT("E"), wxT("E"), double);
	STANDARD_TYPE_TESTS(wxT("f"), wxT("f"), double);
	STANDARD_TYPE_TESTS(wxT("F"), wxT("F"), double);
	STANDARD_TYPE_TESTS(wxT("g"), wxT("g"), double);
	STANDARD_TYPE_TESTS(wxT("G"), wxT("G"), double);
}


TEST(Format, InjectString)
{
	testFormat(wxT("s"), wxT(""));
	testFormat(wxT("s"), wxT("abc"));
}


TEST(Format, InjectNULLString)
{
	for (int p = -1; p < 5; ++p) {
		wxString format = wxT("%");
			
		if (p == -1) {
			format += wxT(".");
		} else {
			format += wxString::Format(wxT(".%d"), p);
		}
				
		format += wxT("s");
		
		wxString actual = CFormat(format) % (const wxChar*)NULL;
		
		ASSERT_TRUE_M(actual.IsEmpty(), wxT("Expected empty string, got '") + actual + wxT("'"));
	}
}


TEST(Format, MultipleFields)
{
	{
		CFormat fmt1(wxT("%d _ %u _ %i"));
		fmt1 % -1 % 2u % -4;
		ASSERT_TRUE(fmt1.IsReady());
		ASSERT_EQUALS(wxT("-1 _ 2 _ -4"), fmt1.GetString());
	}
		
	{	
		CFormat fmt2(wxT("%d _ %u _ %i"));
		ASSERT_FALSE(fmt2.IsReady());
		fmt2 % -1;
		ASSERT_FALSE(fmt2.IsReady());
		fmt2 %  2u;
		ASSERT_FALSE(fmt2.IsReady());
		fmt2 % -4;
		ASSERT_TRUE(fmt2.IsReady());
		ASSERT_EQUALS(wxT("-1 _ 2 _ -4"), fmt2.GetString());
	}
	
	{
		// Test grouped fields
		CFormat fmt3(wxT("%d%u%i"));
		fmt3 % -1 % 2u % -4;
		ASSERT_EQUALS(wxT("-12-4"), fmt3.GetString());
	}
}


TEST(Format, EscapedPercentageSign) 
{
	{
		CFormat fmt1(wxT("%%"));
		ASSERT_EQUALS(wxT("%"), fmt1.GetString());
	}

	{
		CFormat fmt2(wxT("-- %% --"));
		ASSERT_EQUALS(wxT("-- % --"), fmt2.GetString());
	}
	
	{
		CFormat fmt3(wxT("%d _ %% _ %i"));
		fmt3 % 1 % 2;
		ASSERT_TRUE(fmt3.IsReady());
		ASSERT_EQUALS(wxT("1 _ % _ 2"), fmt3.GetString());
	}

	{
		CFormat fmt4(wxT("%% _ %% _ %%"));
		ASSERT_EQUALS(wxT("% _ % _ %"), fmt4.GetString());	
	}
}


///////////////////////////////////////////////////////////
// The following checks for invalid operations

TEST(Format, MalformedFields)
{
	{
		// Incomplete format string
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%")));
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT(" -- %")));

		// Non-existing type
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%q")) % 1);
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT(" -- %q")) % 1.0f );
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT(" -- %q -- ")) % wxT("1"));

		// Invalid string length
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%.qs")) % wxT(""));
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%.-10s")) % wxT(""));
	}

	{
		CAssertOff null;
		
		ASSERT_EQUALS(wxT("%"), CFormat(wxT("%")));
		ASSERT_EQUALS(wxT(" -- %"), CFormat(wxT(" -- %")));

		// Non-existing type
		ASSERT_EQUALS(wxT("%q"), CFormat(wxT("%q")) % 1);
		ASSERT_EQUALS(wxT(" -- %q"), CFormat(wxT(" -- %q")) % 1.0f );
		ASSERT_EQUALS(wxT(" -- %q -- "), CFormat(wxT(" -- %q -- ")) % wxT("1"));

		// Invalid string length
		ASSERT_EQUALS(wxT("%.qs"), CFormat(wxT("%.qs")) % wxT(""));
		ASSERT_EQUALS(wxT("%.-10s"), CFormat(wxT("%.-10s")) % wxT(""));

		// Wrong and right arguments 
		ASSERT_EQUALS(wxT("%s -- 17"), CFormat(wxT("%s -- %i")) % 1 % 17);
	}	
}


TEST(Format, NotReady)
{
	CFormat fmt(wxT("-- %s - %d"));
	ASSERT_FALSE(fmt.IsReady());
	ASSERT_RAISES(CAssertFailureException, fmt.GetString());

	{
		CAssertOff null;
		ASSERT_EQUALS(wxT("-- %s - %d"), fmt);
	}

	fmt % wxT("foo");

	ASSERT_FALSE(fmt.IsReady());
	ASSERT_RAISES(CAssertFailureException, fmt.GetString());

	{
		CAssertOff null;
		ASSERT_EQUALS(wxT("-- foo - %d"), fmt);
	}

	fmt % 42;

	ASSERT_TRUE(fmt.IsReady());
	ASSERT_EQUALS(wxT("-- foo - 42"), fmt.GetString());
}


TEST(Format, WrongTypes)
{
	{
		// Entirely wrong types:
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%s")) % 1);
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%s")) % 1.0f);
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%s")) % wxT('1'));
		
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%f")) % 1);
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%f")) % wxT('1'));
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%f")) % wxT("1"));

		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%d")) % 1.0f);
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%d")) % wxT("1"));
	}

	{
		CAssertOff null;

		ASSERT_EQUALS(wxT("-- %s -- 42 --"), CFormat(wxT("-- %s -- %u --")) % 1 % 42);
		ASSERT_EQUALS(wxT("-- %f -- 42 --"), CFormat(wxT("-- %f -- %u --")) % 1 % 42);
		ASSERT_EQUALS(wxT("-- %d -- 42 --"), CFormat(wxT("-- %d -- %u --")) % 1.0f % 42);
	}
}


TEST(Format, NotSupported)
{
	{
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%*d")) % 1);
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%*s")) % wxT(""));
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%p")) % wxT(""));
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%n")) % wxT(""));
	}

	{		
		CAssertOff null;

		ASSERT_EQUALS(wxT("%*d"), CFormat(wxT("%*d")) % 1);
		ASSERT_EQUALS(wxT("%*s"), CFormat(wxT("%*s")) % wxT(""));
		ASSERT_EQUALS(wxT("%p"), CFormat(wxT("%p")) % wxT(""));
		ASSERT_EQUALS(wxT("%n"), CFormat(wxT("%n")) % wxT(""));
	}
}


TEST(Format, Overfeeding)
{
	CFormat fmt(wxT("%d - %d"));
	ASSERT_FALSE(fmt.IsReady());
	fmt % 1 % 2;
	ASSERT_TRUE(fmt.IsReady());
	ASSERT_EQUALS(wxT("1 - 2"), fmt.GetString());
		
	ASSERT_RAISES(CAssertFailureException, fmt % 1);
	ASSERT_TRUE(fmt.IsReady());
	ASSERT_EQUALS(wxT("1 - 2"), fmt.GetString());

	ASSERT_RAISES(CAssertFailureException, fmt % 1.0f);
	ASSERT_TRUE(fmt.IsReady());
	ASSERT_EQUALS(wxT("1 - 2"), fmt.GetString());

	ASSERT_RAISES(CAssertFailureException, fmt % wxT("1"));
	ASSERT_TRUE(fmt.IsReady());
	ASSERT_EQUALS(wxT("1 - 2"), fmt.GetString());
}


TEST(Format, 64bValues)
{	
	{
		CFormat fmt(wxT("%lli - %lli"));
	   	fmt % MIN(sint64) % MAX(sint64);
		ASSERT_EQUALS(wxT("-9223372036854775808 - 9223372036854775807"), fmt.GetString());
	}

	{
		CFormat fmt(wxT("%llu - %llu"));
	   	fmt % MIN(uint64) % MAX(uint64);
		ASSERT_EQUALS(wxT("0 - 18446744073709551615"), fmt.GetString());
	}	
}


class CTestPrintable : public CPrintable
{
public:
	CTestPrintable(int value)
		: m_value(value)
	{
	}

	virtual wxString GetPrintableString() const
	{
		return wxString::Format(wxT("%i"), m_value);
	}

private:
	int m_value;
};

TEST(Format, Printable)
{
	{
		CFormat fmt(wxT("%s"));
		fmt % CTestPrintable(10);
		ASSERT_EQUALS(wxT("10"), fmt.GetString());
	}

	{
		CFormat fmt(wxT("%s"));
		fmt % CTestPrintable(-10);
		ASSERT_EQUALS(wxT("-10"), fmt.GetString());
	}
}

#endif
