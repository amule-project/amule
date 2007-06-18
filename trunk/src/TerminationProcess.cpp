//
// This file is part of the aMule Project.
//
// Copyright (c) 2006-2007 Marcelo Roberto Jimenez ( phoenix@amule.org )
// Copyright (c) 2006-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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


#include "TerminationProcess.h"


#include <wx/intl.h>		// Needed for _()


#include <common/Format.h>	// Needed for CFormat
#include "Logger.h"		// Needed for AddLogLineM


CTerminationProcess::CTerminationProcess(const wxString &cmd)
:
wxProcess(),
m_cmd(cmd)
{
}


void CTerminationProcess::OnTerminate(int pid, int status)
{
	AddLogLineM(false,
		CFormat(_("Command `%s' with pid `%d' has finished with status code `%d'.")) %
			m_cmd % pid % status);
	delete this;
}

