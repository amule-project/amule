//
// This file is part of the aMule Project.
// 
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2005 Angel Vidal (Kry) ( kry@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef TEXTCLIENT_H
#define TEXTCLIENT_H

#include "ExternalConnector.h"
#include <wx/intl.h>

wxString ECv2_Response2String(CECPacket *response);

class CamulecmdApp : public CaMuleExternalConnector
{
public:
	const wxString GetGreetingTitle() { return _("aMule text client"); }
	int ProcessCommand(int ID);
	void Process_Answer_v2(const CECPacket *reply);
	void OnInitCommandSet();

private:
	// other command line switches
	void OnInitCmdLine(wxCmdLineParser& amuleweb_parser);
	bool OnCmdLineParsed(wxCmdLineParser& parser);
	void TextShell(const wxString& prompt);
	bool m_HasCmdOnCmdLine;
	wxString m_CmdString;
	virtual int OnRun();

	int	m_last_cmd_id;
};

#endif // TEXTCLIENT_H
