/*
 * Copyright (C) 2004 aMule Team (http://www.amule.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
 * This file must be included with wxUSE_GUI defined to zero or one.
 * Usually on console applications, this will be taken care of in
 * configure time. This is because wx classes will be compiled 
 * differently in each case.
 * 
 */

#ifndef __EXTERNALCONNECTOR_H__

#define __EXTERNALCONNECTOR_H__

#include <wx/app.h>		// For wxApp and mainly, for wxUSE_GUI
#include <wx/cmdline.h>		// For wxCmdLineEntryDesc
#include <wx/string.h>		// For wxString

#if wxUSE_GUI
	#include <list>		// For std::list
#endif

#include "ECSocket.h"

#define CMD_ID_QUIT		-1
#define CMD_ID_SYNTAX_ERROR	-2

#if wxUSE_GUI
	#define UNUSED_IN_GUI(x)
#else
	#define UNUSED_IN_GUI(x) x
#endif

typedef struct s_CmdId {
	const wxString cmd;
	int id;
} CmdId;

class CaMuleExternalConnector : public wxApp
{
public:
	//
	// Constructor
	// 
	CaMuleExternalConnector();

	//
	// Virtual functions
	//
	virtual void LocalShow(const wxString &) {};
	virtual void ShowGreet() {}
	virtual void Pre_Shell() {}
	virtual void Post_Shell() {}
	virtual int ProcessCommand(int) { return -1; }

	//
	// Other functions
	// 
	void Show(const wxString &s);
	void MainThreadIdleNow();
	const wxString& GetCmdArgs() const { return m_cmdargs; }
	int GetIDFromString(wxString &buffer, CmdId *commands);
	void Process_Answer(const wxString& answer);
	bool Parse_Command(wxString &buffer, CmdId *commands);
	void GetCommand(const wxString &prompt, char* buffer, size_t buffer_size);
	void TextShell(const wxString &prompt, CmdId *commands);
	wxString SendRecvMsg(const wxChar *msg);
	void ConnectAndRun(const wxString &ProgName, CmdId *commands);
	//
	// Command line processing
	// 
	void OnInitCmdLine(wxCmdLineParser& amuleweb_parser);
	bool OnCmdLineParsed(wxCmdLineParser& parser);

private:
	static const wxCmdLineEntryDesc cmdLineDesc[7];
	
	wxString	m_cmdargs;
	ECSocket* 	m_ECClient;
	bool 		m_isConnected;
	bool 		m_HasCommandLinePassword;
	wxString	m_CommandLinePassword;
	bool		m_HasConfigFromFile;
	bool		m_KeepQuiet;
	wxString 	m_sPort;
	wxString 	m_sHostName;	

#if wxUSE_GUI
private:
	typedef std::list<wxString> StrList;
	StrList m_printlist;
	wxMutex m_mutex_printlist;
#endif
};

#endif // __EXTERNALCONNECTOR_H__

