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
#include <wx/intl.h>		// Needed for wxLocale

#include "CamuleAppBase.h"	// Needed for CamuleAppBase
#include "types.h"			// Needed for int32, uint16 and uint64
#include "GetTickCount.h"	// Needed for GetTickCount64
#include "SearchList.h"     // Needed for fakecheck

class CAbstractFile;
class MuleConnection;
class MuleServer;
class ExternalConn;
	
#undef theApp
#define theApp (*((CamuleApp*)wxTheApp))

class CamuleApp : public CamuleAppBase {
public:
					CamuleApp();
	virtual			~CamuleApp();
	int				OnExit();
	void			OnFatalException();

	virtual bool	OnInit();
	wxString*		pendinglink;

	uint64			stat_sessionReceivedBytes;
	uint64			stat_sessionSentBytes;
	uint32			stat_reconnects;
	uint64			stat_transferStarttime;
	uint64			stat_serverConnectTime;
	uint32			stat_filteredclients;

	uint64			Start_time;
	double			sTransferDelay;
	uint64			GetUptimeMsecs()	{ return ::GetTickCount64()-Start_time; }
	uint32			GetUptimeSecs()		{ return GetUptimeMsecs()/1000; }
	uint32			GetTransferSecs()	{ return (::GetTickCount64()-stat_transferStarttime)/1000; }
	uint32			GetServerSecs()		{ return (::GetTickCount64()-stat_serverConnectTime)/1000; }

	// Implementierung
	// ed2k link functions
	wxString		StripInvalidFilenameChars(wxString strText, bool bKeepSpaces = true);
	wxString		CreateED2kLink( CAbstractFile* f );
	wxString		CreateHTMLED2kLink( CAbstractFile* f );
	wxString		CreateED2kSourceLink( CAbstractFile* f );
	wxString		CreateED2kHostnameSourceLink( CAbstractFile* f );
	wxString		GenFakeCheckUrl(CAbstractFile *file);
	bool            Action(CKnownFile* knownfile, CPartFile* partfile = NULL, CSearchFile* searchfile = NULL); // deltahf -> fakecheck
	bool			CopyTextToClipboard( wxString strText );
	void			OnlineSig(bool zero = false); 
	void			UpdateReceivedBytes(int32 bytesToAdd);
	void			UpdateSentBytes(int32 bytesToAdd);

//	eagle:	handling of geometry-Option
	int				geometry_is_set;		// will be set to 1 if --geometry Option is present
	int				geometry_x, geometry_y;
	unsigned int	geometry_width, geometry_height;
	
// Kry - External connections
	MuleConnection*	conn;
	MuleServer*		localserver;	
	MuleServer*		ipcserver;
	wxString 		server;
	void 			CreateECServer();
	void 			ShutDownECServer();
	
	// shakraw - new EC code using wxSocketBase
	ExternalConn*	ECServerHandler;

// Kry - avoid chmod on win32
	int use_chmod;
	bool IsReady;
	
// Kry - i18n
	void Localize_mule();

// Kry - New Versions
	void Trigger_New_version(wxString old_version, wxString new_version);

	// Launches default browser.
	void LaunchUrl(const wxString &url);
	
	//DECLARE_MESSAGE_MAP()
protected:

	bool 			ProcessCommandline();
	void 			SetTimeOnTransfer();

	wxLocale		m_locale;
};

#endif // AMULE_H
