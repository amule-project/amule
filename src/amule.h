//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef AMULE_H
#define AMULE_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/app.h>			// Needed for wxApp
#include <wx/intl.h>		// Needed for wxLocale

#include "types.h"			// Needed for int32, uint16 and uint64


class CAbstractFile;
class ExternalConn;
class CamuleDlg;
class CPreferences;
class CDownloadQueue;
class CUploadQueue;
class CServerConnect;
class CSharedFileList;
class CServerList;
class CListenSocket;
class CClientList;
class CKnownFileList;
class CSearchList;
class CClientCreditsList;
class CClientUDPSocket;
class CFriendList;
class CIPFilter;
class wxServer;

	
#define theApp wxGetApp()

class CamuleApp : public wxApp
{
public:
					CamuleApp() {};
	virtual			~CamuleApp() {};

	virtual bool	OnInit();
	int				OnExit();
	void			OnFatalException();


	// ed2k URL functions
	wxString		StripInvalidFilenameChars(const wxString& strText, bool bKeepSpaces = true);
	wxString		CreateED2kLink( CAbstractFile* f );
	wxString		CreateHTMLED2kLink( CAbstractFile* f );
	wxString		CreateED2kSourceLink( CAbstractFile* f );
	wxString		CreateED2kHostnameSourceLink( CAbstractFile* f );
	wxString		GenFakeCheckUrl(CAbstractFile *file);
	
	// Misc functions
	bool			CopyTextToClipboard( wxString strText );
	void			OnlineSig(bool zero = false); 
	void			Localize_mule();
	void			Trigger_New_version(wxString old_version, wxString new_version);
	void			LaunchUrl(const wxString &url);
	

	// Kry - External connections
	wxServer*		localserver;	
	
	// shakraw - new EC code using wxSocketBase
	ExternalConn*	ECServerHandler;

	// Kry - avoid chmod on win32
	bool use_chmod;
	bool IsReady;
	

	// Statistic functions. I plan on moving these to a class of their own -- Xaignar
	void			UpdateReceivedBytes(int32 bytesToAdd);
	uint64			GetUptimeMsecs();
	uint32			GetUptimeSecs();
	uint32			GetTransferSecs();
	uint32			GetServerSecs();
	void			UpdateSentBytes(int32 bytesToAdd);

	// Statistic variables. I plan on moving these to a class of their own -- Xaignar
	uint64			Start_time;
	double			sTransferDelay;
	uint64			stat_sessionReceivedBytes;
	uint64			stat_sessionSentBytes;
	uint32			stat_reconnects;
	uint64			stat_transferStarttime;
	uint64			stat_serverConnectTime;
	uint32			stat_filteredclients;


	// Other parts of the interface and such
	CamuleDlg*			amuledlg;
	CPreferences*		glob_prefs;
	CDownloadQueue*		downloadqueue;
	CUploadQueue*		uploadqueue;
	CServerConnect*		serverconnect;
	CSharedFileList*	sharedfiles;
	CServerList*		serverlist;
	CListenSocket*		listensocket;
	CClientList*		clientlist;
	CKnownFileList*		knownfiles;
	CSearchList*		searchlist;
	CClientCreditsList*	clientcredits;
	CClientUDPSocket*	clientudp;
	CFriendList*		friendlist;
	CIPFilter*			ipfilter;

protected:
	void 			SetTimeOnTransfer();

	wxLocale		m_locale;
};

DECLARE_APP(CamuleApp)

#endif // AMULE_H
