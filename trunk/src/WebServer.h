//this file is part of aMule
//Copyright (C)2003 aMule Team ( http://www.amule-project.net )
//This fle Copyright (C)2003 Kry ( elkry@users.sourceforge.net   http://www.amule-project.net )
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

#include <wx/dynarray.h>

class TransferredData;
class CWebSocket;

#define theApp (*((CamulewebApp*)wxTheApp))

//shakraw, these are defined in PartFile.h, but if I include PartFile.h
//I get several wx-errors in compilation...
//Needed for categories (not working yet)
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

#ifndef AMULEWEBDLG
 static const wxCmdLineEntryDesc cmdLineDesc[] =
{
//	{ wxCMD_LINE_OPTION, "h", "help",  "show this help" },
	{ wxCMD_LINE_OPTION, "rh", "remote-host",  "host where aMule is running (default localhost)"},
	{ wxCMD_LINE_OPTION, "p", "port",   "aMule's port for External Connection", wxCMD_LINE_VAL_NUMBER},

	{ wxCMD_LINE_NONE }
};
#endif

#ifdef AMULEWEBDLG
 class CamulewebFrame : public wxFrame
{
public:
    // ctor(s)
    CamulewebFrame(const wxString& title, const wxPoint& pos, const wxSize& size,
            long style = wxDEFAULT_FRAME_STYLE);

    // event handlers (these functions should _not_ be virtual)
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnComandEnter(wxCommandEvent& event);
    void OnSize( wxSizeEvent& event );
    wxTextCtrl    *log_text;
    wxTextCtrl    *cmd_control;
private:
    wxLog*	logTargetOld;
    // any class wishing to process wxWindows events must use this macro
    DECLARE_EVENT_TABLE()
};
#endif

class CamulewebApp : public wxApp {
	public:

#ifndef AMULEWEBDLG
		virtual int OnRun();
		virtual void OnInitCmdLine(wxCmdLineParser& amuleweb_parser) {
			amuleweb_parser.SetDesc(cmdLineDesc); 	
		}
		virtual bool OnCmdLineParsed(wxCmdLineParser& amuleweb_parser);
		void ParseCommandLine();
#else
		virtual bool	OnInit();
		int			OnExit();
		CamulewebFrame *frame;
#endif
		wxString sPort;
		wxString hostName;
};


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
	int			nFileStatus;
	int			nFilePrio;
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
	uint32			nUsers;
	xDownloadSort	DownloadSort;
	bool			bDownloadSortReverse;
	xServerSort		ServerSort;
	bool			bServerSortReverse;
	xSharedSort		SharedSort;
	bool			bSharedSortReverse;	
	bool			bShowUploadQueue;

	ArrayOfUpDown PointsForWeb;
	ArrayOfSession Sessions;
	ArrayOfTransferredData badlogins;
	
	wxString sLastModified;
	wxString	sETag;
} GlobalParams;

typedef struct {
	wxString		sURL;
	in_addr		inadr;
	void			*pThis;
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
	wxString sDebugLog;
	wxString sStats;
	wxString sPreferences;
	wxString	sLogin;
	wxString	sConnectedServer;
	wxString	sAddServerBox;
	wxString	sWebSearch;
	wxString	sSearch;
	wxString	sProgressbarImgs;
	wxString sProgressbarImgsPercent;
	uint16	iProgressbarWidth;
	wxString sSearchResultLine;
	wxString sSearchHeader;
	wxString sClearCompleted;
	wxString sCatArrow;
} WebTemplates;

class CWebServer {
	friend class CWebSocket;

public:
	CWebServer(void);
	~CWebServer(void);

	int	 UpdateSessionCount();
	void StopServer(void);
	void StartServer(void);
	void RestartServer(void);
	void AddStatsLine(UpDown* line);
	void ReloadTemplates();
	uint16	GetSessionCount()	{ return m_Params.Sessions.GetCount();}
	bool IsRunning()	{ return true /*m_bServerWorking*/;} 
	int GetWSPort(); //shakraw

protected:
	static void		ProcessURL(ThreadData);
	static void		ProcessFileReq(ThreadData);
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
	static wxString 	_GetAddServerBox(ThreadData Data);
	static void		_RemoveServer(wxString sIP, wxString sPort);
	static wxString	_GetWebSearch(ThreadData Data);
	static wxString _GetSearch(ThreadData);

	static wxString	_ParseURL(wxString URL, wxString fieldname); 
	static wxString	_ParseURLArray(wxString URL, wxString fieldname);
	static void		_ConnectToServer(wxString sIP, wxString sPort); //shakraw, added sPort
	static bool		_IsLoggedIn(ThreadData Data, long lSession);
	static void		_RemoveTimeOuts(ThreadData Data, long lSession);
	static bool		_RemoveSession(ThreadData Data, long lSession);
	static bool		_GetFileHash(wxString sHash, unsigned char *FileHash);
	static wxString	_SpecialChars(wxString str);
	static wxString	_GetPlainResString(UINT nID, bool noquote = false);
	static int		_GzipCompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level);
	static void		_SetSharedFilePriority(wxString hash, uint8 priority);
	static wxString	_GetWebCharSet();
	wxString			_LoadTemplate(wxString sAll, wxString sTemplateName);
	static Session	GetSessionByID(ThreadData Data,long sessionID);
	static bool		IsSessionAdmin(ThreadData Data,wxString SsessionID);
	static wxString	GetPermissionDenied();
	static wxString	_GetDownloadGraph(ThreadData Data,wxString filehash);
	static void		InsertCatBox(wxString &Out,int preselect,wxString boxlabel, bool jump=false,bool extraCats=false);
	static wxString	GetSubCatLabel(int cat);
	// Common data
	GlobalParams	m_Params;
	WebTemplates	m_Templates;
	bool			m_bServerWorking;
	int				m_iSearchSortby;
	bool			m_bSearchAsc;
	
	// Elandal: Moved from CUpDownClient
	//shakraw, I think this should be re-moved to CUpDownClient...
	//static wxString	GetUploadFileInfo(CUpDownClient* client);
};

#endif // WEBSERVER_H
