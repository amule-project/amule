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
	void testFormat(const wxString& str, const char* value) {
		for (int p = -1; p < 5; ++p) {
			wxString format;

			format += "%";

			if (p == -1) {
				format += ".";
			} else {
				format += wxString::Format(".%d", p);
			}

			format += str;

			wxString reference = wxString::Format(format, value);
			wxString actual = CFormat(format) % value;

			ASSERT_EQUALS_M(reference, actual, format + ": '" + reference + "' != '" + actual + "'");
		}
	}
END_DECLARE;


TEST(Format, ConstructorAndGetString)
{
	{
		// Null should be valid
		CFormat fmt1(NULL);
		ASSERT_EQUALS("", fmt1.GetString());
	}

	{
		// Empty string should be valid
		CFormat fmt2("");
		ASSERT_EQUALS("", fmt2.GetString());
	}

	{
		// Simple string with no format fields
		CFormat fmt3("a b c");
		ASSERT_EQUALS("a b c", fmt3.GetString());
	}

	{
		// Simple string with one format field
		CFormat fmt4("a %i c");
		ASSERT_EQUALS("a %i c", fmt4.GetString());
	}
}


TEST(Format, SetStringAndGetString)
{
	CFormat format(NULL);
	ASSERT_EQUALS("", format.GetString());

	// Empty string should be valid
	format = CFormat("");
	ASSERT_EQUALS("", format.GetString());

	// Simple string with no format fields
	format = CFormat("a b c");
	ASSERT_EQUALS("a b c", format.GetString());

	// Simple string with one format field
	format = CFormat("a %i c");
	ASSERT_EQUALS("a %i c", format.GetString());
}



//! Implementation for the Standard type test
#define STANDARD_TEST(cformat, wxformat, value) \
	{ \
		wxString reference	= wxString::Format(wxString("%") + wxformat, value); \
		wxString actual		= CFormat(wxString("%") + cformat) % value; \
		ASSERT_EQUALS_M(reference, actual, wxString("%") << wxformat \
				<< " vs. %" << cformat << ": '" + reference + "' != '" + actual + "'"); \
	}



//! Test the two boundaries and a middle value of the specified format
#define STANDARD_TYPE_TESTS(cformat, wxformat, type) \
	STANDARD_TEST(cformat, wxformat, MIN(type)); \
	STANDARD_TEST(cformat, wxformat, (type)(MAX(type) / 2)); \
	STANDARD_TEST(cformat, wxformat, MAX(type)); \

// In wx >= 2.9 wxChar represents a Unicode code point, thus its maximum value
// is 1114111 (0x10ffff) on platforms with 4-byte wchar_t (Linux, macOS).
// Windows uses 2-byte wchar_t, so the ceiling is 0xffff there — do the min
// at 32-bit width and cast back, so the narrowing from 0x10ffff to wxChar
// happens explicitly rather than as an implicit constant conversion.
TEST(Format, InjectwxChar)
{
	const uint32_t maxCodePoint = std::min<uint32_t>(
		static_cast<uint32_t>(MAX(wxChar)), 0x10ffffu);
	STANDARD_TEST("c", "c", MIN(wxChar));
	STANDARD_TEST("c", "c", static_cast<wxChar>(maxCodePoint / 2));
	STANDARD_TEST("c", "c", static_cast<wxChar>(maxCodePoint));
}


//! All length specifiers are supported and should yield the same result
const char* int_lengths[] =
{
	"h",
	"",
	"l",
	"ll",
	NULL
};

//! All signed types are supported, and should yield the same result
const char* sint_types[] =
{
	"d",
	"i",
	NULL
};


//! All unsigned types are supported, and should yield the same result
const char* uint_types[] =
{
	"u",
	"o",
	"x",
	"X",
	NULL
};


TEST(Format, InjectInteger)
{
	const char** sint_entry = sint_types;

	while (*sint_entry) {
		const char** len_entry = int_lengths;

		while (*len_entry) {
			wxString entry = wxString() << *len_entry << *sint_entry;

			STANDARD_TYPE_TESTS(entry, wxString() << "h" << *sint_entry,	signed short);
			STANDARD_TYPE_TESTS(entry, wxString() << "" << *sint_entry,	signed int);
			STANDARD_TYPE_TESTS(entry, wxString() << "l" << *sint_entry,	signed long);
			STANDARD_TYPE_TESTS(entry, wxString() << WXLONGLONGFMTSPEC << *sint_entry,	signed long long);

			++len_entry;
		}

		++sint_entry;
	}


	const char** uint_entry = uint_types;
	while (*uint_entry) {
		const char** len_entry = int_lengths;

		while (*len_entry) {
			wxString entry = wxString() << *len_entry << *uint_entry;

			STANDARD_TYPE_TESTS(entry, wxString() << "h" << *uint_entry,	unsigned short);
			STANDARD_TYPE_TESTS(entry, wxString() << "" << *uint_entry,	unsigned int);
			STANDARD_TYPE_TESTS(entry, wxString() << "l" << *uint_entry,	unsigned long);
			STANDARD_TYPE_TESTS(entry, wxString() << WXLONGLONGFMTSPEC << *uint_entry,	unsigned long long);

			++len_entry;
		}

		++uint_entry;
	}
}


TEST(Format, InjectFloatAndDouble)
{
	STANDARD_TYPE_TESTS("e", "e", float);
	STANDARD_TYPE_TESTS("E", "E", float);
	STANDARD_TYPE_TESTS("f", "f", float);
	STANDARD_TYPE_TESTS("F", "F", float);
	STANDARD_TYPE_TESTS("g", "g", float);
	STANDARD_TYPE_TESTS("G", "G", float);

	STANDARD_TYPE_TESTS("e", "e", double);
	STANDARD_TYPE_TESTS("E", "E", double);
	STANDARD_TYPE_TESTS("f", "f", double);
	STANDARD_TYPE_TESTS("F", "F", double);
	STANDARD_TYPE_TESTS("g", "g", double);
	STANDARD_TYPE_TESTS("G", "G", double);
}


TEST(Format, InjectString)
{
	testFormat("s", "");
	testFormat("s", "abc");
}


TEST(Format, InjectNULLString)
{
	for (int p = -1; p < 5; ++p) {
		wxString format = "%";

		if (p == -1) {
			format += ".";
		} else {
			format += wxString::Format(".%d", p);
		}

		format += "s";

		// cppcheck-suppress zerodiv
		wxString actual = CFormat(format) % (const char*)NULL;

		ASSERT_TRUE_M(actual.IsEmpty(), "Expected empty string, got '" + actual + "'");
	}
}


TEST(Format, MultipleFields)
{
	{
		CFormat fmt1("%d _ %u _ %i");
		fmt1 % -1 % 2u % -4;
		ASSERT_EQUALS("-1 _ 2 _ -4", fmt1.GetString());
	}

	{
		CFormat fmt2("%d _ %u _ %i");
		fmt2 % -1;
		fmt2 %  2u;
		fmt2 % -4;
		ASSERT_EQUALS("-1 _ 2 _ -4", fmt2.GetString());
	}

	{
		// Test grouped fields
		CFormat fmt3("%d%u%i");
		fmt3 % -1 % 2u % -4;
		ASSERT_EQUALS("-12-4", fmt3.GetString());
	}
}


TEST(Format, EscapedPercentageSign)
{
	{
		CFormat fmt1("%%");
		ASSERT_EQUALS("%", fmt1.GetString());
	}

	{
		CFormat fmt2("-- %% --");
		ASSERT_EQUALS("-- % --", fmt2.GetString());
	}

	{
		CFormat fmt3("%d _ %% _ %i");
		fmt3 % 1 % 2;
		ASSERT_EQUALS("1 _ % _ 2", fmt3.GetString());
	}

	{
		CFormat fmt4("%% _ %% _ %%");
		ASSERT_EQUALS("% _ % _ %", fmt4.GetString());
	}
}


///////////////////////////////////////////////////////////
// The following checks for invalid operations

TEST(Format, MalformedFields)
{
#ifdef __WXDEBUG__
	{
		// Incomplete format string
		ASSERT_RAISES(CAssertFailureException, CFormat("%"));
		ASSERT_RAISES(CAssertFailureException, CFormat(" -- %"));

		// Non-existing type
		ASSERT_RAISES(CAssertFailureException, CFormat("%q"));
		ASSERT_RAISES(CAssertFailureException, CFormat(" -- %q"));
		ASSERT_RAISES(CAssertFailureException, CFormat(" -- %q -- "));

		// Partially valid format strings
		ASSERT_RAISES(CAssertFailureException, CFormat("%i%q"));
		ASSERT_RAISES(CAssertFailureException, CFormat("%q%i"));
	}
#endif

	{
		CAssertOff null;

		ASSERT_EQUALS("%", wxString(CFormat("%")));
		ASSERT_EQUALS(" -- %", wxString(CFormat(" -- %")));

		// Non-existing type
		ASSERT_EQUALS("%q", wxString(CFormat("%q") % 1));
		ASSERT_EQUALS(" -- %q", wxString(CFormat(" -- %q") % 1.0f));
		ASSERT_EQUALS(" -- %q -- ", wxString(CFormat(" -- %q -- ") % "1"));

		// Partially valid format strings
		ASSERT_EQUALS("1%q", wxString(CFormat("%i%q") % 1));
		ASSERT_EQUALS("%q1", wxString(CFormat("%q%i") % 1));

		// Wrong and right arguments
		ASSERT_EQUALS("%i -- 17", wxString(CFormat("%i -- %i") % 1.0 % 17));
	}
}


TEST(Format, NotReady)
{
	CFormat fmt("-- %s - %d");
	ASSERT_EQUALS("-- %s - %d", fmt.GetString());

	fmt % "foo";
	ASSERT_EQUALS("-- foo - %d", fmt.GetString());

	fmt % 42;
	ASSERT_EQUALS("-- foo - 42", fmt.GetString());
}


TEST(Format, WrongTypes)
{
#ifdef __WXDEBUG__
	{
		// Entirely wrong types
		ASSERT_RAISES(CAssertFailureException, CFormat("%c") % "1");
		ASSERT_RAISES(CAssertFailureException, CFormat("%f") % "1");
		ASSERT_RAISES(CAssertFailureException, CFormat("%d") % 1.0f);
		ASSERT_RAISES(CAssertFailureException, CFormat("%d") % "1");
	}
#endif

	{
		CAssertOff null;

		ASSERT_EQUALS("-- %d -- 42 --", wxString(CFormat("-- %d -- %u --") % 1.0f % 42));
	}
}


TEST(Format, NotSupported)
{
#ifdef __WXDEBUG__
	{
		ASSERT_RAISES(CAssertFailureException, CFormat("%*d") % 1);
		ASSERT_RAISES(CAssertFailureException, CFormat("%*s") % "");
		ASSERT_RAISES(CAssertFailureException, CFormat("%n") % "");
	}
#endif

	{
		CAssertOff null;

		ASSERT_EQUALS("%*d", wxString(CFormat("%*d") % 1));
		ASSERT_EQUALS("%*s", wxString(CFormat("%*s") % ""));
		ASSERT_EQUALS("%n", wxString(CFormat("%n") % ""));
	}
}


TEST(Format, Overfeeding)
{
	CFormat fmt("%d - %d");
	fmt % 1 % 2;
	ASSERT_EQUALS("1 - 2", fmt.GetString());

	fmt % 1;
	ASSERT_EQUALS("1 - 2", fmt.GetString());

	fmt % 1.0;
	ASSERT_EQUALS("1 - 2", fmt.GetString());

	fmt % "x";
	ASSERT_EQUALS("1 - 2", fmt.GetString());
}


TEST(Format, 64bValues)
{
	{
		CFormat fmt("%lli - %lli");
		fmt % MIN(sint64) % MAX(sint64);
		ASSERT_EQUALS("-9223372036854775808 - 9223372036854775807", fmt.GetString());
	}

	{
		CFormat fmt("%llu - %llu");
		fmt % MIN(uint64) % MAX(uint64);
		ASSERT_EQUALS("0 - 18446744073709551615", fmt.GetString());
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
		return wxString::Format("%i", m_value);
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
		CFormat fmt("%s");
		fmt % CTestPrintable(10);
		ASSERT_EQUALS("10", fmt.GetString());
	}

	{
		CFormat fmt("%s");
		fmt % CTestPrintable(-10);
		ASSERT_EQUALS("-10", fmt.GetString());
	}
}

TEST(Format, ReorderedArguments)
{
	// Swapping two arguments of the same type
	{
		CFormat fmt("%2$i,%1$i");
		fmt % 1 % 2;
		ASSERT_EQUALS("2,1", fmt.GetString());
	}

	// Swapping arguments of different type
	{
		CFormat fmt("%2$i,%1$c");
		fmt % 'a' % 2;
		ASSERT_EQUALS("2,a", fmt.GetString());
	}

	// Using the same argument multiple times
	{
		CFormat fmt("%1$i,%1$i");
		fmt % 3;
		ASSERT_EQUALS("3,3", fmt.GetString());
	}

	// Leaving gaps (printf doesn't allow this!)
	{
		CFormat fmt("%1$i,%3$i");
		fmt % 1 % 2 % 3;
		ASSERT_EQUALS("1,3", fmt.GetString());
	}

	// Mixing positional and indexed arguments (printf doesn't allow this!)
	{
		CFormat fmt("%3$i,%i,%1$i");
		fmt % 1 % 2 % 3;
		ASSERT_EQUALS("3,2,1", fmt.GetString());
	}
}

// Tests for non-standard functionality.
// These are extensions to the standard printf functionality.
TEST(Format, DifferentArguments)
{
	// Tests for default conversion with the 's' conversion type
	ASSERT_EQUALS("a", wxString(CFormat("%s") % 'a'));
	ASSERT_EQUALS("1", wxString(CFormat("%s") % 1u));
	ASSERT_EQUALS("-1", wxString(CFormat("%s") % -1));
	ASSERT_EQUALS(wxString::Format("%g", 1.2), wxString(CFormat("%s") % 1.2));

	// Test for changing the conversion type based on the argument type
	ASSERT_EQUALS("-1", wxString(CFormat("%u") % -1));

	// Tests for accepting mismatching argument type
	ASSERT_EQUALS("C", wxString(CFormat("%c") % 67));
	ASSERT_EQUALS("69", wxString(CFormat("%i") % 'E'));
	ASSERT_EQUALS(wxString::Format("%.e", 1.0), wxString(CFormat("%.e") % 1u));
}
