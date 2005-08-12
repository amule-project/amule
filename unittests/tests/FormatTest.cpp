#include <muleunit/test.h>
#include <Format.h>

using namespace muleunit;

// Note: These tests have been written in accordance with the 
//       capabilities of printf found in printf(3).

#define ELEMENTS(x) (sizeof(x)/sizeof(x[0]))
#define MIN(x) std::numeric_limits<x>::min()
#define MAX(x) std::numeric_limits<x>::max()

const wxChar* Flags[] = {
	wxT(""),
	wxT("#"),
	wxT("0"),
	wxT("-"),
	wxT(" "),
	wxT("+")
};



DECLARE(Format)
	template <typename TYPE>
	void testFormat(const wxString& str, TYPE value) {
		for (unsigned int f = 0; f < ELEMENTS(Flags); ++f) {
			for (int w = 0; w < 10; ++w) {
				for (int p = -2; p < 5; ++p) {
					wxString format;
					
					format += wxT("%");
					format += Flags[f];
					
					if (w > 0) {
						format += wxString::Format(wxT("%d"), w);
					}

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
		}
	}
	
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


//! Test the two boundries and a middle value of the specificed format
#define STANDARD_TYPE_TESTS(format, type) \
	testFormat(format, MIN(type)); \
	testFormat(format, MAX(type) / 2); \
	testFormat(format, MIN(type)); \

//! Test that values outside the scope of a type are rejected
#define STANDARD_TYPE_CONSTRAINTS(format, type, next) \
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%") format) % (((next)MIN(type)) - 1)); \
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%") format) % (((next)MAX(type)) + 1)); 



TEST(Format, InjectwxChar)
{
	STANDARD_TYPE_TESTS(wxT("c"), wxChar);
	STANDARD_TYPE_CONSTRAINTS(wxT("c"), wxChar, signed long long);
}


TEST(Format, InjectShort)
{
	STANDARD_TYPE_TESTS(wxT("hi"), signed short);
	STANDARD_TYPE_TESTS(wxT("hu"), unsigned short);

	STANDARD_TYPE_CONSTRAINTS(wxT("hi"), signed short, int);
	STANDARD_TYPE_CONSTRAINTS(wxT("hu"), unsigned short, int);
}


TEST(Format, InjectInt)
{
	STANDARD_TYPE_TESTS(wxT("i"), signed int);
	STANDARD_TYPE_TESTS(wxT("d"), signed int);
	STANDARD_TYPE_TESTS(wxT("o"), unsigned int);
	STANDARD_TYPE_TESTS(wxT("u"), unsigned int);
	STANDARD_TYPE_TESTS(wxT("x"), unsigned int);
	STANDARD_TYPE_TESTS(wxT("X"), unsigned int);
	
	STANDARD_TYPE_CONSTRAINTS(wxT("i"), signed int, long long);
	STANDARD_TYPE_CONSTRAINTS(wxT("d"), signed int, long long);
	STANDARD_TYPE_CONSTRAINTS(wxT("o"), unsigned int, long long);
	STANDARD_TYPE_CONSTRAINTS(wxT("u"), unsigned int, long long);
	STANDARD_TYPE_CONSTRAINTS(wxT("x"), unsigned int, long long);
	STANDARD_TYPE_CONSTRAINTS(wxT("X"), unsigned int, long long);
}


TEST(Format, InjectLong)
{
	STANDARD_TYPE_TESTS(wxT("li"), signed long);
	STANDARD_TYPE_TESTS(wxT("lu"), unsigned long);

	STANDARD_TYPE_CONSTRAINTS(wxT("li"), signed long, long long);
	STANDARD_TYPE_CONSTRAINTS(wxT("lu"), unsigned long, long long);
}


TEST(Format, InjectLongLong)
{
	STANDARD_TYPE_TESTS(wxT("lli"), signed long long);
	STANDARD_TYPE_TESTS(wxT("llu"), unsigned long long);
}


TEST(Format, InjectFloatAndDouble)
{
	STANDARD_TYPE_TESTS(wxT("e"), float);
	STANDARD_TYPE_TESTS(wxT("E"), float);
	STANDARD_TYPE_TESTS(wxT("f"), float);
	STANDARD_TYPE_TESTS(wxT("F"), float);
	STANDARD_TYPE_TESTS(wxT("g"), float);
	STANDARD_TYPE_TESTS(wxT("G"), float);
	
	STANDARD_TYPE_TESTS(wxT("e"), double);
	STANDARD_TYPE_TESTS(wxT("E"), double);
	STANDARD_TYPE_TESTS(wxT("f"), double);
	STANDARD_TYPE_TESTS(wxT("F"), double);
	STANDARD_TYPE_TESTS(wxT("g"), double);
	STANDARD_TYPE_TESTS(wxT("G"), double);
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
		fmt1 % -1 % 2 % -4;
		ASSERT_TRUE(fmt1.IsReady());
		ASSERT_EQUALS(wxT("-1 _ 2 _ -4"), fmt1.GetString());
	}
		
	{	
		CFormat fmt2(wxT("%d _ %u _ %i"));
		ASSERT_FALSE(fmt2.IsReady());
		fmt2 % -1;
		ASSERT_FALSE(fmt2.IsReady());
		fmt2 %  2;
		ASSERT_FALSE(fmt2.IsReady());
		fmt2 % -4;
		ASSERT_TRUE(fmt2.IsReady());
		ASSERT_EQUALS(wxT("-1 _ 2 _ -4"), fmt2.GetString());
	}
	
	{
		// Test grouped fields
		CFormat fmt3(wxT("%d%u%i"));
		fmt3 % -1 % 2 % -4;
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
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%s")) % 1);
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%s")) % 1.0f);
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%s")) % wxT('1'));
	
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%f")) % 1);
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%f")) % wxT('1'));
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%f")) % wxT("1"));

	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%d")) % 1.0f);
	ASSERT_RAISES(CInvalidParamsEx, CFormat(wxT("%d")) % wxT("1"));
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

