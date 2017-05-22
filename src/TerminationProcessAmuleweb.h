//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Marcelo Roberto Jimenez ( phoenix@amule.org )
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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


#ifndef TERMINATIONPROCESSAMULEWEB_H
#define TERMINATIONPROCESSAMULEWEB_H


#include "TerminationProcess.h"	// for CTerminationProcess


// This is the handler for process termination events.
// Used on some calls to wxExecute().
class CTerminationProcessAmuleweb : public CTerminationProcess
{
private:
	long *m_webserver_pid;

public:
	CTerminationProcessAmuleweb(const wxString &cmd, long *webserver_pid);
	virtual void OnTerminate(int pid, int status);
};


#endif // TERMINATIONPROCESSAMULEWEB_H

