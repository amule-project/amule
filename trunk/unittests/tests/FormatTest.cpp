#include <muleunit/test.h>
#include <common/Format.h>
#include <limits>

#include <Types.h>

using namespace muleunit;

// Note: These tests have been written in accordance with the 
//       capabilities of printf found in printf(3).
// Note to the above: Except where otherwise stated.

#define ELEMENTS(x) (sizeof(x)/sizeof(x[0]))
#define MIN(x) std::numeric_limits<x>::min()
#define MAX(x) std::numeric_limits<x>::max()


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
		ASSERT_EQUALS(wxT(""), fmt1.GetString());
	}

	{
		// Empty string should be valid
		CFormat fmt2(wxT(""));
		ASSERT_EQUALS(wxT(""), fmt2.GetString());
	}

	{
		// Simple string with no format fields
		CFormat fmt3(wxT("a b c"));
		ASSERT_EQUALS(wxT("a b c"), fmt3.GetString());
	}

	{
		// Simple string with one format field
		CFormat fmt4(wxT("a %i c"));
		ASSERT_EQUALS(wxT("a %i c"), fmt4.GetString());
	}
}


TEST(Format, SetStringAndGetString)
{
	CFormat format(NULL);
	ASSERT_EQUALS(wxT(""), format.GetString());

	// Empty string should be valid
	format = CFormat(wxT(""));
	ASSERT_EQUALS(wxT(""), format.GetString());

	// Simple string with no format fields
	format = CFormat(wxT("a b c"));
	ASSERT_EQUALS(wxT("a b c"), format.GetString());

	// Simple string with one format field
	format = CFormat(wxT("a %i c"));
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



//! Test the two boundaries and a middle value of the specificed format
#define STANDARD_TYPE_TESTS(cformat, wxformat, type) \
	STANDARD_TEST(cformat, wxformat, MIN(type)); \
	STANDARD_TEST(cformat, wxformat, (type)(MAX(type) / 2)); \
	STANDARD_TEST(cformat, wxformat, MAX(type)); \

// In wx >= 2.9 wxChar represents a Unicode code point, thus its maximum value
// is 1114111 (0x10ffff).
TEST(Format, InjectwxChar)
{
	STANDARD_TEST(wxT("c"), wxT("c"), MIN(wxChar));
	STANDARD_TEST(wxT("c"), wxT("c"), (wxChar)(std::min<wxChar>(MAX(wxChar), 0x10ffff) / 2));
	STANDARD_TEST(wxT("c"), wxT("c"), (std::min<wxChar>(MAX(wxChar), 0x10ffff)));
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
			STANDARD_TYPE_TESTS(entry, wxString() << WXLONGLONGFMTSPEC << *sint_entry,	signed long long);

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
			STANDARD_TYPE_TESTS(entry, wxString() << WXLONGLONGFMTSPEC << *uint_entry,	unsigned long long);

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
		ASSERT_EQUALS(wxT("-1 _ 2 _ -4"), fmt1.GetString());
	}

	{	
		CFormat fmt2(wxT("%d _ %u _ %i"));
		fmt2 % -1;
		fmt2 %  2u;
		fmt2 % -4;
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
#ifdef __WXDEBUG__
	{
		// Incomplete format string
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%")));
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT(" -- %")));

		// Non-existing type
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%q")));
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT(" -- %q")));
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT(" -- %q -- ")));

		// Partially valid format strings
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%i%q")));
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%q%i")));
	}
#endif

	{
		CAssertOff null;

		ASSERT_EQUALS(wxT("%"), CFormat(wxT("%")));
		ASSERT_EQUALS(wxT(" -- %"), CFormat(wxT(" -- %")));

		// Non-existing type
		ASSERT_EQUALS(wxT("%q"), CFormat(wxT("%q")) % 1);
		ASSERT_EQUALS(wxT(" -- %q"), CFormat(wxT(" -- %q")) % 1.0f);
		ASSERT_EQUALS(wxT(" -- %q -- "), CFormat(wxT(" -- %q -- ")) % wxT("1"));

		// Partially valid format strings
		ASSERT_EQUALS(wxT("1%q"), CFormat(wxT("%i%q")) % 1);
		ASSERT_EQUALS(wxT("%q1"), CFormat(wxT("%q%i")) % 1);

		// Wrong and right arguments 
		ASSERT_EQUALS(wxT("%i -- 17"), CFormat(wxT("%i -- %i")) % 1.0 % 17);
	}
}


TEST(Format, NotReady)
{
	CFormat fmt(wxT("-- %s - %d"));
	ASSERT_EQUALS(wxT("-- %s - %d"), fmt.GetString());

	fmt % wxT("foo");
	ASSERT_EQUALS(wxT("-- foo - %d"), fmt.GetString());

	fmt % 42;
	ASSERT_EQUALS(wxT("-- foo - 42"), fmt.GetString());
}


TEST(Format, WrongTypes)
{
#ifdef __WXDEBUG__
	{
		// Entirely wrong types
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%c")) % wxT("1"));
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%f")) % wxT("1"));
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%d")) % 1.0f);
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%d")) % wxT("1"));
	}
#endif

	{
		CAssertOff null;

		ASSERT_EQUALS(wxT("-- %d -- 42 --"), CFormat(wxT("-- %d -- %u --")) % 1.0f % 42);
	}
}


TEST(Format, NotSupported)
{
#ifdef __WXDEBUG__
	{
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%*d")) % 1);
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%*s")) % wxT(""));
		ASSERT_RAISES(CAssertFailureException, CFormat(wxT("%n")) % wxT(""));
	}
#endif

	{		
		CAssertOff null;

		ASSERT_EQUALS(wxT("%*d"), CFormat(wxT("%*d")) % 1);
		ASSERT_EQUALS(wxT("%*s"), CFormat(wxT("%*s")) % wxT(""));
		ASSERT_EQUALS(wxT("%n"), CFormat(wxT("%n")) % wxT(""));
	}
}


TEST(Format, Overfeeding)
{
	CFormat fmt(wxT("%d - %d"));
	fmt % 1 % 2;
	ASSERT_EQUALS(wxT("1 - 2"), fmt.GetString());

	fmt % 1;
	ASSERT_EQUALS(wxT("1 - 2"), fmt.GetString());

	fmt % 1.0;
	ASSERT_EQUALS(wxT("1 - 2"), fmt.GetString());

	fmt % wxT("x");
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


class CTestPrintable
{
public:
	CTestPrintable(int value)
		: m_value(value)
	{}

	wxString GetPrintableString() const
	{
		return wxString::Format(wxT("%i"), m_value);
	}

private:
	int m_value;
};

template<>
inline CFormat& CFormat::operator%(CTestPrintable value)
{
	return this->operator%<const wxString&>(value.GetPrintableString());
}

TEST(Format, UserDefined)
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

TEST(Format, ReorderedArguments)
{
	// Swapping two arguments of the same type
	{
		CFormat fmt(wxT("%2$i,%1$i"));
		fmt % 1 % 2;
		ASSERT_EQUALS(wxT("2,1"), fmt.GetString());
	}

	// Swapping arguments of different type
	{
		CFormat fmt(wxT("%2$i,%1$c"));
		fmt % wxT('a') % 2;
		ASSERT_EQUALS(wxT("2,a"), fmt.GetString());
	}

	// Using the same argument multiple times
	{
		CFormat fmt(wxT("%1$i,%1$i"));
		fmt % 3;
		ASSERT_EQUALS(wxT("3,3"), fmt.GetString());
	}

	// Leaving gaps (printf doesn't allow this!)
	{
		CFormat fmt(wxT("%1$i,%3$i"));
		fmt % 1 % 2 % 3;
		ASSERT_EQUALS(wxT("1,3"), fmt.GetString());
	}

	// Mixing positional and indexed arguments (printf doesn't allow this!)
	{
		CFormat fmt(wxT("%3$i,%i,%1$i"));
		fmt % 1 % 2 % 3;
		ASSERT_EQUALS(wxT("3,2,1"), fmt.GetString());
	}
}

// Tests for non-standard functionality.
// These are extensions to the standard printf functionality.
TEST(Format, DifferentArguments)
{
	// Tests for default conversion with the 's' conversion type
	ASSERT_EQUALS(wxT("a"), CFormat(wxT("%s")) % wxT('a'));
	ASSERT_EQUALS(wxT("1"), CFormat(wxT("%s")) % 1u);
	ASSERT_EQUALS(wxT("-1"), CFormat(wxT("%s")) % -1);
	ASSERT_EQUALS(wxString::Format(wxT("%g"), 1.2), CFormat(wxT("%s")) % 1.2);

	// Test for changing the conversion type based on the argument type
	ASSERT_EQUALS(wxT("-1"), CFormat(wxT("%u")) % -1);

	// Tests for accepting mismatching argument type
	ASSERT_EQUALS(wxT("C"), CFormat(wxT("%c")) % 67);
	ASSERT_EQUALS(wxT("69"), CFormat(wxT("%i")) % wxT('E'));
	ASSERT_EQUALS(wxT("1e+00"), CFormat(wxT("%.e")) % 1u);
}
