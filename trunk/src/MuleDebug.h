#ifndef DEBUG_H
#define DEBUG_H

#include <wx/debug.h>

class CBaseException
{
public:
	CBaseException(const wxString& what)
		: m_what(what) {}

	const wxString& what() const {
		return m_what;
	}

private:
	wxString m_what;
};


/**
 * This exception is to be thrown if invalid parameters are passed to a function.
 */
class CInvalidArgsException : public CBaseException
{
public:
	CInvalidArgsException(const wxString& what) : CBaseException(what) {}
};


/**
 * This exception is to be thrown if an object is used in an invalid state.
 */
class CInvalidStateException : public CBaseException
{
public:
	CInvalidStateException(const wxString& what) : CBaseException(what) {}
};


#ifdef MULEUNIT
	#define _MULE_THROW(cls, cond, msg) \
		if (!(cond)) { throw cls(msg); }
#else
	#define _MULE_THROW(cls, cond, msg) \
		if (!(cond)) { wxASSERT(cond); throw cls(msg); }	
#endif


#define MULE_VALIDATE_STATE(cond, msg) \
	_MULE_THROW(CInvalidStateException, (cond), (msg))

#define MULE_VALIDATE_PARAMS(cond, msg) \
	_MULE_THROW(CInvalidArgsException, (cond), (msg))



#define MULE_ASSERT(cond)               wxASSERT((cond))
#define MULE_ASSERT_MSG(cond, msg)      wxASSERT_MSG((cond), msg)
#define MULE_FAIL()                     wxFAIL()
#define MULE_FAIL_MSG(msg)              wxFAIL_MSG(msg)
#define MULE_CHECK(cond, retValue)      wxCHECK((cond), (retValue))
#define MULE_CHECK_MSG(cond, ret, msg)  wxCHECK_MSG((cond), (ret), (msg))
#define MULE_CHECK_RET(cond, msg)       wxCHECK_RET((cond), (msg))

#endif
