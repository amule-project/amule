// This file is part of aMule.
//
// Copyright (C) 2004 Carlo Wood
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef CAMULEAPPBASE_H
#define CAMULEAPPBASE_H

// To keep things working, without needing to patch everything at once... =)
#include "amule.h"

#warning Cthulhu says: CamuleAppBase.h has been deprecated. Please use amule.h instead.
/* 
	The following code has been merged with the CamuleApp class.
	Since this file has been deprecated, please avoid including it,
	it will be removed later. Rather, include the amule.h file.
*/
#if 0

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/app.h>		// Needed for wxApp
#include "types.h"		// Needed for DWORD

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

#ifndef theApp
#define theApp (*((CamuleAppBase*)wxTheApp))
#endif

// These elements are accessed directly through "theApp.amuledlg" etc.
// If you need other theApp members then #include "amule.h".
struct CamuleAppBase : public wxApp {
  CamuleDlg*		amuledlg;
  CPreferences*		glob_prefs;
  CDownloadQueue*	downloadqueue;
  CUploadQueue*		uploadqueue;
  CServerConnect*	serverconnect;
  CSharedFileList*	sharedfiles;
  CServerList*		serverlist;
  CListenSocket*	listensocket;
  CClientList*		clientlist;
  CKnownFileList*	knownfiles;
  CSearchList*		searchlist;
  CClientCreditsList*	clientcredits;
  CClientUDPSocket*	clientudp;
  CFriendList*		friendlist;
  CIPFilter*		ipfilter;
};

#endif

#endif // CAMULEAPPBASE_H
