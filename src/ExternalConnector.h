//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

/*
 * This file must be included with wxUSE_GUI defined to zero or one.
 * Usually on console applications, this will be taken care of in
 * configure time. This is because wx classes will be compiled 
 * differently in each case.
 * 
 */

#ifndef __EXTERNALCONNECTOR_H__
#define __EXTERNALCONNECTOR_H__

#include <wx/app.h>			// For wxApp
#include <wx/cmdline.h>		// For wxCmdLineEntryDesc
#include <ec/RemoteConnect.h>

#include <wx/intl.h>

#define CMD_DEPRECATED		0x1000
#define CMD_OK			 0
#define CMD_ID_QUIT		-1
#define CMD_ID_HELP		-2
#define CMD_ERR_SYNTAX		-3
#define CMD_ERR_PROCESS_CMD	-4
#define CMD_ERR_NO_PARAM	-5
#define CMD_ERR_MUST_HAVE_PARAM	-6
#define CMD_ERR_INVALID_ARG	-7
#define CMD_ERR_INCOMPLETE	-8

enum Params {
	CMD_PARAM_NEVER,
	CMD_PARAM_OPTIONAL,
	CMD_PARAM_ALWAYS
};

class CCommandTree;

typedef	std::list<const CCommandTree*>	CmdList_t;
typedef	std::list<const CCommandTree*>::iterator	CmdPos_t;
typedef	std::list<const CCommandTree*>::const_iterator	CmdPosConst_t;

class CaMuleExternalConnector;

class CCommandTree {
 public:
	CCommandTree(CaMuleExternalConnector& app)
		: m_command(wxEmptyString), m_cmd_id(CMD_ERR_SYNTAX), m_short(wxEmptyString), m_verbose(wxEmptyString), m_params(CMD_PARAM_OPTIONAL), m_parent(NULL), m_app(app)
		{}

	~CCommandTree();

	CCommandTree*	AddCommand(const wxString& command, int cmd_id, const wxString& shortDesc, const wxString& longDesc, enum Params params = CMD_PARAM_OPTIONAL)
		{
			return AddCommand(new CCommandTree(m_app, command, cmd_id, shortDesc, longDesc, params));
		}

	int	FindCommandId(const wxString& command, wxString& args, wxString& cmdstr) const;
	wxString GetFullCommand() const;
	void	PrintHelpFor(const wxString& command) const;

 private:
	CCommandTree(CaMuleExternalConnector& app, const wxString& command, int cmd_id, const wxString& shortDesc, const wxString& longDesc, enum Params params)
		: m_command(command), m_cmd_id(cmd_id), m_short(shortDesc), m_verbose(longDesc), m_params(params), m_parent(NULL), m_app(app)
		{}

	CCommandTree*	AddCommand(CCommandTree* cmdTree);

	wxString	m_command;
	int		m_cmd_id;
	wxString	m_short;
	wxString	m_verbose;
	enum Params	m_params;
	const CCommandTree*	m_parent;
	CaMuleExternalConnector& m_app;
	CmdList_t	m_subcommands;
};


class CECFileConfig;

class CaMuleExternalConnector : public wxApp
{
public:
	//
	// Constructor & Destructor
	// 
	CaMuleExternalConnector();
	~CaMuleExternalConnector();

	//
	// Virtual functions
	//
	virtual void Pre_Shell() {}
	virtual void Post_Shell() {}
	virtual int ProcessCommand(int) { return -1; }
	virtual void TextShell(const wxString &prompt);
	virtual void LoadConfigFile();
	virtual void SaveConfigFile();
	virtual void LoadAmuleConfig(CECFileConfig& cfg);
	virtual void OnInitCommandSet();
	virtual bool OnInit();
	virtual const wxString GetGreetingTitle() = 0;

	//
	// Other functions
	// 
	void Show(const wxString &s);
	void DebugShow(const wxString &s) { if (m_Verbose) Show(s); }
	const wxString& GetCmdArgs() const { return m_cmdargs; }
	const wxString& GetLastCmdStr() const { return m_lastcmdstr; }
	int GetIDFromString(const wxString& buffer) { return m_commands.FindCommandId(buffer, m_cmdargs, m_lastcmdstr); }
	void Process_Answer(const wxString& answer);
	bool Parse_Command(const wxString& buffer);
	void GetCommand(const wxString &prompt, char* buffer, size_t buffer_size);
	const CECPacket *SendRecvMsg_v2(const CECPacket *request) { return m_ECClient->SendRecvPacket(request); }
	void ConnectAndRun(const wxString &ProgName, const wxString& ProgName);
	void ShowGreet();

	//
	// Command line processing
	// 
	void OnInitCmdLine(wxCmdLineParser& amuleweb_parser);
	bool OnCmdLineParsed(wxCmdLineParser& parser);

	CECFileConfig*	m_configFile;
	wxString	m_configFileName;

protected:
	long	 	m_port;
	wxString 	m_host;
	CMD4Hash	m_password;
	bool		m_KeepQuiet;
	bool		m_Verbose;
	CCommandTree	m_commands;

#if !wxUSE_GUI && defined(__WXMAC__)
	virtual wxAppTraits* CreateTraits();
#endif

private:
	wxString	m_cmdargs;
	wxString	m_lastcmdstr;
	CRemoteConnect*	m_ECClient;
	char *		m_InputLine;
	bool		m_NeedsConfigSave;
	wxString	m_language;
	wxLocale	m_locale;
};

#endif // __EXTERNALCONNECTOR_H__
// File_checked_for_headers
