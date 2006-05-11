//
// This file is part of the aMule Project.
//
// Copyright (C) 2005-2006Mikkel Schubert ( xaignar@users.sourceforge.net )
// Copyright (C) 2005-2006aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef MULEDEBUG_H
#define MULEDEBUG_H

#include <wx/defs.h>
#include <wx/debug.h>
#include <wx/string.h>

/**
 * Installs an exception handler that can handle CMuleExceptions.
 */
void InstallMuleExceptionHandler();

/**
 *
 */
void OnUnhandledException();


//! Print a backtrace, skipping the first n frames.
void print_backtrace(unsigned n);

//! Returns a backtrace, skipping the first n frames.
wxString get_backtrace(unsigned n);


/**
 * This exception should be used to implement other
 * types of exceptions. It should never be caught, 
 * instead catch the subtypes.
 */
class CMuleException
{
public:
	CMuleException(const wxString& type, const wxString& desc)
		: m_what(type + wxT(": ") + desc)
	{}

	const wxString& what() const {
		return m_what;
	}

private:
	wxString m_what;
};


/**
 * This exception type is used to represent exceptions that are 
 * caused by invalid operations. Exceptions of this type should
 * not be caught as they are the result of bugs.
 */
struct CRunTimeException : public CMuleException
{
	CRunTimeException(const wxString& type, const wxString& desc)
		: CMuleException(wxT("CRunTimeException") + type, desc) {}
};



/**
 * This exception is to be thrown if invalid parameters are passed to a function.
 */
struct CInvalidParamsEx : public CRunTimeException
{
	CInvalidParamsEx(const wxString& desc)
		: CRunTimeException(wxT("CInvalidArgsException"), desc) {}
};


/**
 * This exception is to be thrown if an object is used in an invalid state.
 */
struct CInvalidStateEx : public CRunTimeException
{
	CInvalidStateEx(const wxString& desc)
		: CRunTimeException(wxT("CInvalidStateException"), desc) {}
};

/**
 * This exception is thrown on wrong packets or tags.
 */
struct CInvalidPacket : public CMuleException
{
	CInvalidPacket(const wxString& desc)
		: CMuleException(wxT("CInvalidPacket"), desc) {}
};


// This ifdef ensures that we wont get assertions while 
// unittesting, which would otherwise impede the tests.
#ifdef MULEUNIT
	#define _MULE_THROW(cond, cls, msg) \
		do { \
			if (!(cond)) { \
				throw cls(msg); \
			} \
		} while (false)
#else
	#define _MULE_THROW(cond, cls, msg) \
		do { \
			if (!(cond)) { \
				wxFAIL_MSG(wxT(#cond)); \
				throw cls(msg); \
			} \
		} while (false)
#endif



#define MULE_CHECK_THROW(cond, cls, msg) \
	_MULE_THROW((cond), cls, (msg))


#define MULE_VALIDATE_STATE(cond, msg) \
	MULE_CHECK_THROW((cond), CInvalidStateEx, (msg))

#define MULE_VALIDATE_PARAMS(cond, msg) \
	MULE_CHECK_THROW((cond), CInvalidParamsEx, (msg))


#define MULE_ASSERT(cond)               wxASSERT((cond))
#define MULE_ASSERT_MSG(cond, msg)      wxASSERT_MSG((cond), msg)
#define MULE_FAIL()                     wxFAIL()
#define MULE_FAIL_MSG(msg)              wxFAIL_MSG(msg)
#define MULE_CHECK(cond, retValue)      wxCHECK((cond), (retValue))
#define MULE_CHECK_MSG(cond, ret, msg)  wxCHECK_MSG((cond), (ret), (msg))
#define MULE_CHECK_RET(cond, msg)       wxCHECK_RET((cond), (msg))

#endif
