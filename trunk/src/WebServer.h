// This file is part of the aMule Project
//
// Copyright (C) 2003-2004 aMule Team ( http://www.amule-project.net )
// This fle Copyright (C) 2003 Kry ( elkry@users.sourceforge.net   http://www.amule-project.net )
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

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <wx/string.h>
#include <wx/intl.h>
#ifdef __WXMSW__
	#include <wx/msw/winundef.h>
#endif
#include <wx/wx.h>
#define wxUSE_DDE_FOR_IPC  0
#include <wx/ipc.h>
#include <wx/filename.h>

#ifdef AMULEWEBDLG
#include <wx/textdlg.h>
#else
#include <wx/cmdline.h>
#endif

#ifndef WIN32
#include "config.h"
#endif

#ifdef __WXMSW__
	#include <winsock.h>
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif
#include <zlib.h>		// Needed for Bytef etc.
#include "types.h"

#include "WebInterface.h"
#include <wx/thread.h>

#include <wx/dynarray.h>

class TransferredData;
class CWSThread;
class CWebSocket;

//shakraw, these are defined in PartFile.h, but if I include PartFile.h
//I get several wx-errors in compilation...
//Needed for categories
#define	PS_READY			0
#define	PS_EMPTY			1
#define PS_WAITINGFORHASH	2
#define PS_HASHING			3
#define PS_ERROR			4
#define	PS_UNKNOWN			6
#define PS_PAUSED			7
#define PS_COMPLETING		8
#define PS_COMPLETE			9
//shakraw, end of imported definitions

//shakraw - webserver code below
#define WEB_GRAPH_HEIGHT		120
#define WEB_GRAPH_WIDTH			500

#define SESSION_TIMEOUT_SECS	300	// 5 minutes session expiration
#define SHORT_FILENAME_LENGTH	40	// Max size of file name.

typedef struct { float download; float upload; long connections; } UpDown;

typedef struct { time_t startTime; long lSession; bool admin;} Session;

typedef struct {
	wxString	sFileName;
	wxString	sFileStatus;
	long		lFileSize;
	long		lFileTransferred;
	long		lFileSpeed;
	long		lSourceCount;
	long		lNotCurrentSourceCount;
	long		lTransferringSourceCount;
	float		fCompleted;
	int		nFileStatus;
	int		nFilePrio;
	wxString	sFileHash;
	wxString	sED2kLink;
	wxString	sFileInfo;
} DownloadFiles;

typedef struct {
	wxString	sFileName;
	long		lFileSize;
	uint32		nFileTransferred;
    uint64		nFileAllTimeTransferred;
	uint16		nFileRequests;
	uint32		nFileAllTimeRequests;
	uint16		nFileAccepts;
	uint32		nFileAllTimeAccepts;
	uint8		nFilePriority;
	wxString	sFilePriority;
	bool		bFileAutoPriority;
	wxString 	sFileHash;
	wxString	sED2kLink;
} SharedFiles;

typedef enum {
	DOWN_SORT_NAME,
	DOWN_SORT_SIZE,
	DOWN_SORT_TRANSFERRED,
	DOWN_SORT_SPEED,
	DOWN_SORT_PROGRESS
} xDownloadSort;

typedef enum {
	SHARED_SORT_NAME,
	SHARED_SORT_SIZE,
	SHARED_SORT_TRANSFERRED,
	SHARED_SORT_ALL_TIME_TRANSFERRED,
	SHARED_SORT_REQUESTS,
	SHARED_SORT_ALL_TIME_REQUESTS,
	SHARED_SORT_ACCEPTS,
    SHARED_SORT_ALL_TIME_ACCEPTS,
	SHARED_SORT_PRIORITY
} xSharedSort;

typedef struct {
	wxString	sServerName;
	wxString	sServerDescription;
	int		nServerPort;
	wxString	sServerIP;
	int		nServerUsers;
	int		nServerMaxUsers;
	int		nServerFiles;
} ServerEntry;

typedef enum {
	SERVER_SORT_NAME,
	SERVER_SORT_DESCRIPTION,
	SERVER_SORT_IP,
	SERVER_SORT_USERS,
	SERVER_SORT_FILES
} xServerSort;


WX_DECLARE_OBJARRAY(UpDown*, ArrayOfUpDown);
WX_DECLARE_OBJARRAY(Session*, ArrayOfSession);
WX_DECLARE_OBJARRAY(TransferredData*, ArrayOfTransferredData);
WX_DECLARE_OBJARRAY(SharedFiles*, ArrayOfSharedFiles);
WX_DECLARE_OBJARRAY(ServerEntry*, ArrayOfServerEntry);
WX_DECLARE_OBJARRAY(DownloadFiles*, ArrayOfDownloadFiles);

typedef struct {
	uint32		nUsers;
	xDownloadSort	DownloadSort;
	bool		bDownloadSortReverse;
	xServerSort	ServerSort;
	bool		bServerSortReverse;
	xSharedSort	SharedSort;
	bool		bSharedSortReverse;	
	bool		bShowUploadQueue;

	ArrayOfUpDown		PointsForWeb;
	ArrayOfSession		Sessions;
	ArrayOfTransferredData	badlogins;
	
	wxString	sLastModified;
	wxString	sETag;
} GlobalParams;

typedef struct {
	wxString	sURL;
	in_addr		inadr;
	void		*pThis;
	CWebSocket	*pSocket;
} ThreadData;

typedef struct {
	wxString	sHeader;
	wxString	sHeaderMetaRefresh;
	wxString	sHeaderStylesheet;
	wxString	sFooter;
	wxString	sServerList;
	wxString	sServerLine;
	wxString	sTransferImages;
	wxString	sTransferList;
	wxString	sTransferDownHeader;
	wxString	sTransferDownFooter;
	wxString	sTransferDownLine;
	wxString	sTransferDownLineGood;
	wxString	sTransferUpHeader;
	wxString	sTransferUpFooter;
	wxString	sTransferUpLine;
	wxString	sTransferUpQueueShow;
	wxString	sTransferUpQueueHide;
	wxString	sTransferUpQueueLine;
	wxString	sTransferBadLink;
	wxString	sDownloadLink;
	wxString	sSharedList;
	wxString	sSharedLine;
	wxString	sSharedLineChanged;
	wxString	sGraphs;
	wxString	sLog;
	wxString	sServerInfo;
	wxString 	sDebugLog;
	wxString 	sStats;
	wxString 	sPreferences;
	wxString	sLogin;
	wxString	sConnectedServer;
	wxString	sAddServerBox;
	wxString	sWebSearch;
	wxString	sSearch;
	wxString	sProgressbarImgs;
	wxString 	sProgressbarImgsPercent;
	uint16		iProgressbarWidth;
	wxString	sSearchResultLine;
	wxString	sSearchHeader;
	wxString	sClearCompleted;
	wxString	sCatArrow;
} WebTemplates;

class CWebServer {
	friend class CWebSocket;

	public:
		CWebServer(CamulewebApp *webApp);
		~CWebServer(void);

		void 	StartServer(void);
		void 	RestartServer(void);
		void 	StopServer(void);
		void 	ReloadTemplates(void);
	
		int	UpdateSessionCount();
		void 	AddStatsLine(UpDown* line);
		uint16	GetSessionCount()	{ return m_Params.Sessions.GetCount();}
		bool 	IsRunning()	{ return true /*m_bServerWorking*/;}  //shakraw, useless now
		int 	GetWSPort(); //shakraw
		void	Print(char *sFormat, ...);
		
	protected:
		static void	ProcessURL(ThreadData);
		static void	ProcessFileReq(ThreadData);
		static void 	ProcessImgFileReq(ThreadData);
		static void 	ProcessStyleFileReq(ThreadData);
	
	private:
		static wxString	_GetHeader(ThreadData, long lSession);
		static wxString	_GetFooter(ThreadData);
		static wxString	_GetServerList(ThreadData);
		static wxString	_GetTransferList(ThreadData);
		static wxString	_GetDownloadLink(ThreadData);
		static wxString	_GetSharedFilesList(ThreadData);
		static wxString	_GetGraphs(ThreadData);
		static wxString	_GetLog(ThreadData);
		static wxString	_GetServerInfo(ThreadData);
		static wxString	_GetDebugLog(ThreadData);
		static wxString	_GetStats(ThreadData);
		static wxString	_GetPreferences(ThreadData);
		static wxString	_GetLoginScreen(ThreadData);
		static wxString	_GetConnectedServer(ThreadData);
		static wxString _GetAddServerBox(ThreadData Data);
		static void	_RemoveServer(CWebServer *pThis, wxString sIP, wxString sPort);
		static wxString	_GetWebSearch(ThreadData Data);
		static wxString _GetSearch(ThreadData);

		static wxString	_ParseURL(ThreadData Data, wxString fieldname); 
		static wxString	_ParseURLArray(ThreadData Data, wxString fieldname);
		static void	_ConnectToServer(CWebServer *pThis, wxString sIP, wxString sPort); //shakraw, added sPort
		static bool	_IsLoggedIn(ThreadData Data, long lSession);
		static void	_RemoveTimeOuts(ThreadData Data, long lSession);
		static bool	_RemoveSession(ThreadData Data, long lSession);
		static bool	_GetFileHash(wxString sHash, unsigned char *FileHash);
		static wxString	_SpecialChars(wxString str);
		static wxString	_GetPlainResString(UINT nID, bool noquote = false);
		static int	_GzipCompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level);
		static void	_SetSharedFilePriority(CWebServer *pThis, wxString hash, uint8 priority);
		static wxString	_GetWebCharSet();
		wxString	_LoadTemplate(wxString sAll, wxString sTemplateName);
		static Session	GetSessionByID(ThreadData Data,long sessionID);
		static bool	IsSessionAdmin(ThreadData Data,wxString SsessionID);
		static wxString	GetPermissionDenied();
		static wxString	_GetDownloadGraph(ThreadData Data,wxString filehash);
		static void	InsertCatBox(CWebServer *pThis, wxString &Out, int preselect, wxString boxlabel, bool jump=false, bool extraCats=false);
		static wxString	GetSubCatLabel(int cat);
		// Common data
		CamulewebApp	*webInterface;
		CWSThread		*wsThread;
		GlobalParams	m_Params;
		WebTemplates	m_Templates;
		bool		m_bServerWorking;
		int		m_iSearchSortby;
		bool		m_bSearchAsc;
};

#endif // WEBSERVER_H
