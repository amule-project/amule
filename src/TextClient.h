//
// This file is part of the aMule Project.
// 
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2007 Angel Vidal (Kry) ( kry@amule.org / http://www.amule.org )
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

#ifndef TEXTCLIENT_H
#define TEXTCLIENT_H

#include "ExternalConnector.h"

#include <map>



class CEC_SearchFile_Tag;

class SearchFile {
	public:
		wxString sFileName;
		unsigned long lFileSize;
		CMD4Hash  nHash;
		wxString  sHash;
		long lSourceCount;
		bool bPresent;
		
		SearchFile(CEC_SearchFile_Tag *);
		
		void ProcessUpdate(CEC_SearchFile_Tag *);
		static class SearchInfo *GetContainerInstance();
		CMD4Hash ID() { return nHash; }
};

typedef std::map<unsigned long int,SearchFile*> CResultMap;

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
	void ShowResults(CResultMap results_map);
	bool m_HasCmdOnCmdLine;
	wxString m_CmdString;
	virtual int OnRun();

	int	m_last_cmd_id;
	CResultMap	m_Results_map;
};

#endif // TEXTCLIENT_H
// File_checked_for_headers
