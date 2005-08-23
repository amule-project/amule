#include <muleunit/test.h>
#include <Format.h>
#include <limits.h>

using namespace muleunit;

// Note: These tests have been written in accordance with the 
//       capabilities of printf found in printf(3).

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
		ASSERT_RAISES(CInvalidStateEx, fmt4.GetString());
	}
}


TEST(Format, SetStringAndGetString)
{
	CFormat format(NULL);
	ASSERT_TRUE(format.IsReady());
	ASSERT_EQUALS(wxT(""), format.GetString());

	// Empty string should be valid
	format.SetString(wxT(""));
	ASSERT_TRUE(format.IsReady());
	ASSERT_EQUALS(wxT(""), format.GetString());

	// Simple string with no format fields
	format.SetString(wxT("a b c"));
	ASSERT_TRUE(format.IsReady());
	ASSERT_EQUALS(wxT("a b c"), format.GetString());

	// Simple string with one format field
	format.SetString(wxT("a %i c"));
	ASSERT_FALSE(format.IsReady());
	ASSERT_RAISES(CInvalidStateEx, format.GetString());
}



//! Implementation for the Standard type test
#define STANDARD_TEST(cformat, wxformat, value) \
	{ \
		wxString reference	= wxString::Format(wxString(wxT("%")) + wxformat, value); \
		wxString actual		= CFormat(wxString(wxT("%")) + cformat) % value; \
 		ASSERT_EQUALS_M(reference, actual, wxString(wxT("%")) << wxformat << wxT(": '") + reference + wxT("' != '") + actual + wxT("'")); \
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
			STANDARD_TYPE_TESTS(entry, wxString() << wxT("ll") << *sint_entry,	signed long long);
			
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
			STANDARD_TYPE_TESTS(entry, wxString() << wxT("ll") << *uint_entry,	unsigned long long);
			
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


TEST(Format, ResetString)
{
	CFormat fmt(wxT("- %d -"));
	fmt % 1;

	ASSERT_TRUE(fmt.IsReady());
	ASSERT_EQUALS(wxT("- 1 -"), fmt.GetString());

	fmt.ResetString();
	ASSERT_FALSE(fmt.IsReady());
	fmt % 2;
	
	ASSERT_EQUALS(wxT("- 2 -"), fmt.GetString());
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
	// Incomplete format string
	ASSERT_RAISES(CInvalidStateEx, CFormat(wxT("%")));
	ASSERT_RAISES(CInvalidStateEx, CFormat(wxT(" -- %")));

	// Non-existing type
	ASSERT_RAISES(CInvalidStateEx, CFormat(wxT("%q")) % 1);
	ASSERT_RAISES(CInvalidStateEx, CFormat(wxT(" -- %q")) % 1.0f );
	ASSERT_RAISES(CInvalidStateEx, CFormat(wxT(" -- %q -- ")) % wxT("1"));

	// Invalid string length
	ASSERT_RAISES(CInvalidStateEx, CFormat(wxT("%.qs")) % wxT(""));
	ASSERT_RAISES(CInvalidStateEx, CFormat(wxT("%.-10s")) % wxT(""));
}


TEST(Format, NotReady)
{
	CFormat fmt(wxT("-- %s - %d"));
	ASSERT_FALSE(fmt.IsReady());
	ASSERT_RAISES(CInvalidStateEx, fmt.GetString());

	fmt % wxT("foo");

	ASSERT_FALSE(fmt.IsReady());
	ASSERT_RAISES(CInvalidStateEx, fmt.GetString());

	fmt % 42;

	ASSERT_TRUE(fmt.IsReady());
	ASSERT_EQUALS(wxT("-- foo - 42"), fmt.GetString());
}


TEST(Format, WrongTypes)
{
	// Entirely wrong types:
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%s")) % 1);
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%s")) % 1.0f);
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%s")) % wxT('1'));
	
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%f")) % 1);
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%f")) % wxT('1'));
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%f")) % wxT("1"));

	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%d")) % 1.0f);
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%d")) % wxT("1"));

	
	// Type may not contain a valid value for the format:
}


TEST(Format, NotSupported)
{
	ASSERT_RAISES(CInvalidStateEx, CFormat(wxT("%*d")) % 1);
	ASSERT_RAISES(CInvalidStateEx, CFormat(wxT("%*s")) % wxT(""));
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%p")) % wxT(""));
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%n")) % wxT(""));
}


TEST(Format, Overfeeding)
{
	CFormat fmt(wxT("%d - %d"));
	ASSERT_FALSE(fmt.IsReady());
	fmt % 1 % 2;
	ASSERT_TRUE(fmt.IsReady());
	ASSERT_EQUALS(wxT("1 - 2"), fmt.GetString());
		
	ASSERT_RAISES(CInvalidStateEx, fmt % 1);
	ASSERT_RAISES(CInvalidStateEx, fmt % 1.0f);
	ASSERT_RAISES(CInvalidStateEx, fmt % wxT("1"));

	ASSERT_TRUE(fmt.IsReady());
	ASSERT_EQUALS(wxT("1 - 2"), fmt.GetString());
}

