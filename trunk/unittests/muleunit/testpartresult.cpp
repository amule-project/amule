//
// Copyright (C) 2005 Mikkel Schubert (Xaignar@amule.org)
// Copyright (C) 2004 Barthelemy Dagenais (barthelemy@prologique.com)
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//


#include "testpartresult.h"

using namespace muleunit;

TestPartResult::TestPartResult(const wxString& fileName,
                                long lineNumber,
                                const wxString&	message,
                                testType type)
		: m_message (message),
		m_fileName (fileName),
		m_lineNumber (lineNumber),
		m_type (type)
{}

testType TestPartResult::getType() const
{
	return m_type;
}

const wxString& TestPartResult::getMessage() const
{
	return m_message;
}

const wxString& TestPartResult::getFileName() const
{
	return m_fileName;
}

long TestPartResult::getLineNumber() const
{
	return m_lineNumber;
}
