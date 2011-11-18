//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Marcelo Roberto Jimenez ( phoenix@amule.org )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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


#include <sstream>
#include <common/StringFunctions.h>	// for char2unicode()

#include "Logger.h"				// for Add(Debug)LogLineM()


#ifdef __DEBUG__
bool CLogger::IsEnabled(DebugType /*type*/) const
{
	return true;
}
#endif


void CLogger::AddLogLine(
	const wxString &file,
	int line,
	bool critical,
	const std::ostringstream &msg)
{
	AddLogLine(file, line, critical, static_cast<DebugType>(0), msg);
}


void CLogger::AddLogLine(
	const wxString &file,
	int line,
	bool critical,
	DebugType /* type */,
	const wxString &str,
	bool /*  toStdout */,
	bool /* toGUI */)
{
	std::string s;
	if (critical) {
		s = "Critical message: ";
	}
	printf("%s(%d): %s%s\n",
		(const char *)unicode2char(file),
		line, s.c_str(), (const char *)unicode2char(str));
}


void CLogger::AddLogLine(
	const wxString &file,
	int line,
	bool critical,
	DebugType /*type*/,
	const std::ostringstream &msg)
{
	std::string s;
	if (critical) {
		s = "Critical message: ";
	}
	printf("%s(%d): %s%s\n",
		(const char *)unicode2char(file),
		line, s.c_str(), msg.str().c_str());
}

CLogger theLogger;

BEGIN_EVENT_TABLE(CLogger, wxEvtHandler)
END_EVENT_TABLE()
