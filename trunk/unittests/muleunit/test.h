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

#ifndef TEST_H
#define TEST_H

#include <exception>

#include <wx/string.h>
#include <list>
#include <string>


/**
 * MuleUnit namespace.
 * This is the namespace containing all muleunit classes.
 */
namespace muleunit
{

class TestCase;
class BTList;


/** Returns the size of a static array. */
template <typename T, size_t N>
inline size_t ArraySize(T(&)[N])
{
	return N;
}


/** Print wide-char strings. */
inline void Print(const wxString& str)
{
	wxPuts(str.c_str());
}


/** This exception is raised if an ASSERT fails. */
struct CTestFailureException : public std::exception
{
	/** Constructor, takes a snapshot of the current context, and adds the given information. */
	CTestFailureException(const wxString& msg, const wxChar* file, long lineNumber);

	~CTestFailureException() throw();

	/** Prints the context backtrace for the location where the exception was thrown. */
	void PrintBT() const;

	virtual const char* what () const throw ();
private:
	//! Pointer to struct containing a snapshot of the contexts
	//! taken at the time the exception was created.
	struct BTList* m_bt;

	//! The message passed in the constructor.
	std::string m_message;
};

/** This exception is raised if an wxASSERT fails. */
struct CAssertFailureException : public CTestFailureException
{
public:
	CAssertFailureException(const wxString& msg, const wxChar* file, long lineNumber)
		: CTestFailureException(msg, file, lineNumber)
	{
	}
};


/** 
 * This class is used to produce informative backtraces
 *
 * This is done by specifying a "context" for a given scope, using
 * the CONTEXT macro, at which point a description is added to the
 * current list of contexts. At destruction, when the scope is exited,
 * the context is removed from the queue. 
 *
 * The resulting "backtrace" can then be printed by calling the
 * PrintBT() function of an CTestFailureException.
 */
class CContext
{
public:
	/** Adds a context with the specified information and description. */
	CContext(const wxChar* file, int line, const wxString& desc);

	/** Removes the context addded by the constructor. */
	~CContext();
};


//! Used to join the CContext instance name with the line-number.
//! This is done to prevent shadowing.
#define DO_CONTEXT(x, y, z) x y##z

//! Specifies the context of the current scope.
#define CONTEXT(x) CContext wxCONCAT(context,__LINE__)(wxT(__FILE__), __LINE__, x)


/** 
 * This class disables assertions while it is in scope.
 */
class CAssertOff
{
public:
	CAssertOff();
	~CAssertOff();
};


/**
 * Test class containing all macros to do unit testing. 
 * A test object represents a test that will be executed. Once it has been
 * executed, it reports all failures in the testPartResult linked list.
 *
 * A failure occurs when a test fails (condition is false).
 */
class Test
{
public:
	/**
	 * Main Test constructor. Used to create a test that will register itself
	 * with TestRegistry and with its test case.
	 * @param testCaseName Name of the test case this test belongs to
	 * @param testName Name of this test
	 */
	Test(const wxString& testCaseName, const wxString& testName);

	/**
	 * Main Test desctructor
	 * Delete the testPartResult linked list. This is why the user should
	 * only use the macro provided by muleunit to report a test result.
	 */
	virtual ~Test();

	/**
	 * Fixtures that will be called after run().
	 */
	virtual void tearDown();

	/**
	 * Fixtures that will be called before run().
	 */
	virtual void setUp();

	/**
	 * Test code should be in this method.
	 * run() will be called by the Test's TestCase, hence subclasses of Test
	 * should override this method.
	 */
	virtual void run();

	/**
	 * Get the name of the TestCase this test belongs to. The name of the
	 * TestCase is the first parameter of the test declaration. For example,
	 * if a test is declared as TEST(TESTCASE1, TEST1), this method will return
	 * "TESTCASE1".
	 *
	 * @return The TestCase name of this test
	 */
	const wxString& getTestCaseName() const;

	/**
	 * Get the name of this test. The name of the test is the second
	 * parameter of the test declaration. For example,
	 * if a test is declared as TEST(TESTCASE1, TEST1), this method will return
	 * "TEST1".
	 *
	 * @return The name of this test.
	 */
	const wxString& getTestName() const;

	template <typename A, typename B>
	static void DoAssertEquals(const wxString& file, unsigned line, const A& a, const B& b)
	{
		if (!(a == b)) {
			wxString message = wxT("Expected '") + StringFrom(a) + 
								wxT("' but got '") + StringFrom(b) + wxT("'");

			throw CTestFailureException(message, file, line);
		}
	}

protected:
	wxString m_testCaseName;
	wxString m_testName;
	TestCase* m_testCase;
};


/**
 * Helperfunction that converts basic types to strings.
 */
template <typename TYPE>
wxString StringFrom(const TYPE& value)
{
	return wxString() << value;
}

inline wxString StringFrom(unsigned long long value)
{
	return wxString::Format(wxT("%") wxLongLongFmtSpec wxT("u"), value);
}

inline wxString StringFrom(signed long long value)
{
	return wxString::Format(wxT("%") wxLongLongFmtSpec wxT("i"), value);
}


#define THROW_TEST_FAILURE(message) \
	throw CTestFailureException(message, wxT(__FILE__), __LINE__)


/**
 * Asserts that a condition is true.
 * If the condition is not true, a failure is generated.
 * @param condition Condition to fullfill for the assertion to pass
 * @param message Message that will be displayed if this assertion fails
 */
#define ASSERT_TRUE_M(condition, message) \
{ \
	if (!(condition)) { \
		THROW_TEST_FAILURE(message); \
	} \
}


/**
 * Same as ASSERT_TRUE, but without an explicit message.
 */
#define ASSERT_TRUE(condition) \
	ASSERT_TRUE_M(condition, wxString(wxT("Not true: ")) + wxT(#condition));


/**
 * Same as ASSERT_TRUE, but without an explicit message and condition must be false.
 */
#define ASSERT_FALSE(condition) \
	ASSERT_TRUE_M(!(condition), wxString(wxT("Not false: ")) + wxT(#condition));


/**
 * Asserts that the two parameters are equals. Operator == must be defined.
 * If the two parameters are not equals, a failure is generated.
 * @param expected Expected value
 * @param actual Actual value to be compared
 * @param message Message that will be displayed if this assertion fails
 */
#define ASSERT_EQUALS_M(expected,actual,message)\
{ \
	if (!(expected == actual)) { \
		THROW_TEST_FAILURE(message); \
	} \
}


/**
 * Same as ASSERT_EQUALS_M, but without an explicit message.
 */
#define ASSERT_EQUALS(expected, actual) \
	Test::DoAssertEquals(wxT(__FILE__), __LINE__, expected, actual)


/**
 * Make a test fails with the given message.
 * @param text Failure message
 */
#define FAIL_M(text) \
	THROW_TEST_FAILURE(text)

/**
 * Same as FAIL_M, but without an explicit message.
 */
#define FAIL() FAIL_M(wxT("Test failed."))


/**
 * Requires that an exception of a certain type is raised.
 */
#define ASSERT_RAISES_M(type, call, message) \
	try { \
		{ call; }\
		THROW_TEST_FAILURE(message); \
	} catch (const type&) { \
	} catch (const std::exception& e) { \
		THROW_TEST_FAILURE(wxString::FromAscii(e.what())); \
	}
	


/**
 * Same as ASSERT_RAISES, but without an explicit message.
 */
#define ASSERT_RAISES(type, call) \
	ASSERT_RAISES_M(type, (call), wxT("Exception of type ") wxT(#type) wxT(" not raised."))



/**
 * Define a test in a TestCase using test fixtures.
 * User should put his test code between brackets after using this macro.
 *
 * This macro should only be used if test fixtures were declared earlier in
 * this order: DECLARE, SETUP, TEARDOWN.
 * @param testCaseName TestCase name where the test belongs to. Should be
 * the same name of DECLARE, SETUP and TEARDOWN.
 * @param testName Unique test name.
 * @param testDisplayName This will be displayed when running the test.
 */
#define TEST_M(testCaseName, testName, testDisplayName)                        \
    class testCaseName##testName##Test : public testCaseName##Declare##Test    \
    {                                                                          \
    public:                                                                    \
        testCaseName##testName##Test()                                         \
            : testCaseName##Declare##Test(wxT(#testCaseName), testDisplayName) \
        {                                                                      \
        }                                                                      \
                                                                               \
        void run();                                                            \
    } testCaseName##testName##Instance;                                        \
                                                                               \
    void testCaseName##testName##Test::run()

/**
 * Define a test in a TestCase using test fixtures.
 * User should put his test code between brackets after using this macro.
 *
 * This macro should only be used if test fixtures were declared earlier in
 * this order: DECLARE, SETUP, TEARDOWN.
 * @param testCaseName TestCase name where the test belongs to. Should be
 * the same name of DECLARE, SETUP and TEARDOWN.
 * @param testName Unique test name.
 */
#define TEST(testCaseName, testName)	TEST_M(testCaseName, testName, wxT(#testName))

/**
 * Location to declare variables and objets.
 * This is where user should declare members accessible by TESTF,
 * SETUP and TEARDOWN.
 *
 * User should not use brackets after using this macro. User should
 * not initialize any members here.
 *
 * @param testCaseName TestCase name of the fixtures
 * @see END_DECLARE for more information.
 */
#define DECLARE(testCaseName)\
	class testCaseName##Declare##Test : public Test \
	{ \
	public: \
		testCaseName##Declare##Test(const wxString& testCaseName, const wxString& testName) \
			: Test (testCaseName, testName) {} \
		virtual void run() = 0; \
		
/**
 * Ending macro used after DECLARE.
 *
 * User should use this macro after declaring members with
 * DECLARE macro.
 */
#define END_DECLARE };


/**
 * Macro for creating a fixture with no setup/teardown or member variables.
 */
#define DECLARE_SIMPLE(testCaseName) \
	DECLARE(testCaseName) \
	END_DECLARE;

} // MuleUnit ns
#endif // TEST_H

