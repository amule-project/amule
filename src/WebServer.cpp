//
// This file is part of the aMule Project
//
// aMule Copyright (C) 2003-204 aMule Team ( http://www.amule-project.net )
// This file Copyright (C) 2003-2004 Kry (elkry@sourceforge.net  http://www.amule-project.net )
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

#ifdef HAVE_CONFIG_H
	#include "config.h"		// Needed for VERSION
#endif


#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctype.h>
#include <stdlib.h>

#pragma implementation
#include "WebServer.h"

//-------------------------------------------------------------------

#include <wx/arrimpl.cpp>	// this is a magic incantation which must be done!
#include <wx/tokenzr.h>		// for wxTokenizer
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

//-------------------------------------------------------------------

#include "ECSocket.h"
#include "ECSpecialTags.h"
#include "GetTickCount.h"	// Needed for GetTickCount
#include "MD5Sum.h"
#include "otherstructs.h"	// Needed for TransferredData
#include "otherfunctions.h"	// Needed for atoll, ED2KFT_*
#include "NetworkFunctions.h" // Needed for StringIPtoUint32
#include "types.h"
#include "WebSocket.h"		// Needed for StopSockets()
#include "ECcodes.h"
//-------------------------------------------------------------------

// Initialization of the static MyTimer member variables.
#if wxUSE_GUI && wxUSE_TIMER && !defined(AMULE_DAEMON)
uint32 MyTimer::tic32 = 0;
uint64 MyTimer::tic64 = 0;
#endif

//-------------------------------------------------------------------

WX_DEFINE_OBJARRAY(ArrayOfUpDown);
WX_DEFINE_OBJARRAY(ArrayOfSession);
WX_DEFINE_OBJARRAY(ArrayOfTransferredData);

#define HTTPInit "Server: aMule\r\nPragma: no-cache\r\nExpires: 0\r\nCache-Control: no-cache, no-store, must-revalidate\r\nConnection: close\r\nContent-Type: text/html\r\n"
#define HTTPInitGZ "Server: aMule\r\nPragma: no-cache\r\nExpires: 0\r\nCache-Control: no-cache, no-store, must-revalidate\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Encoding: gzip\r\n"

#define WEB_SERVER_TEMPLATES_VERSION	4

wxString CWebServer::imgs_folder;

using namespace otherfunctions;

wxString _SpecialChars(wxString str) {
	str.Replace(wxT("&"),wxT("&amp;"));
	str.Replace(wxT("<"),wxT("&lt;"));
	str.Replace(wxT(">"),wxT("&gt;"));
	str.Replace(wxT("\""),wxT("&quot;"));
	return str;
}

CWebServer::CWebServer(CamulewebApp *webApp):
	m_ServersInfo(webApp), m_SharedFilesInfo(webApp), m_DownloadFilesInfo(webApp) {
	webInterface = webApp;
	
	m_Params.bShowUploadQueue = false;

	m_Params.sLastModified = wxEmptyString;
	m_Params.sETag = wxEmptyString;
	m_iSearchSortby = 3;
	m_bSearchAsc = 0;

	imgs_folder = wxString(char2unicode(getenv("HOME"))) + wxT("/.aMule/webserver/");

	m_bServerWorking = false; // not running (listening) yet
}

CWebServer::~CWebServer(void) {
	//stop web socket thread
	if (wsThread) wsThread->Delete();
}

//start web socket and reload templates
void CWebServer::StartServer(void) {
	if (!m_bServerWorking) {
		ReloadTemplates();

		//create the thread...
		wsThread = new CWSThread(this);
		if ( wsThread->Create() != wxTHREAD_NO_ERROR ) {
			webInterface->Show(_("Can't create web socket thread\n"));
		} else {
			//...and run it
			wsThread->Run();
 
			m_bServerWorking = true;
			webInterface->Show(_("Web Server: Started\n"));
		}
	} else
		webInterface->Show(_("Web Server: running\n"));
}

//restart web socket and reload templates
void CWebServer::RestartServer(void) {
	if (m_bServerWorking) {
		if (wsThread) wsThread->Delete();
	}
	
	//create the thread...
	wsThread = new CWSThread(this);
	if ( wsThread->Create() != wxTHREAD_NO_ERROR ) {
		webInterface->Show(_("Can't create web socket thread\n"));
	} else {
		//...and run it
		wsThread->Run();
		webInterface->Show(_("Web Server: Restarted\n"));
	}
}


//stop web socket
void CWebServer::StopServer(void) {
	if (m_bServerWorking) {
		m_bServerWorking = false;
		if (wsThread) wsThread->Delete();
		webInterface->Show(_("Web Server: Stopped\n"));
	} else
		webInterface->Show(_("Web Server: not running\n"));
}


//returns web server listening port
int CWebServer::GetWSPrefs(void)
{
	CECPacket req(EC_OP_GET_PREFERENCES);
	req.AddTag(CECTag(EC_TAG_SELECT_PREFS, (uint32)EC_PREFS_REMOTECONTROLS));
	CECPacket *reply = webInterface->SendRecvMsg_v2(&req);

	if ( ! reply ) {
		return 0;
	}

	CECTag *wsprefs = reply->GetTagByIndex(0); // we have selected only the webserver preferences
	CECTag *tag;
	int wsport = wsprefs->GetTagByName(EC_TAG_WEBSERVER_PORT)->GetInt16Data();

	if ( ! webInterface->m_bForcedAdminPassword ) {
		tag = wsprefs->GetTagByName(EC_TAG_PASSWD_HASH);
		if (tag) {
			webInterface->m_AdminPass = tag->GetStringData();
		} else {
		webInterface->m_AdminPass = wxEmptyString;
		}
	}

	if ( ! webInterface->m_bForcedAllowGuest ) {
		tag = wsprefs->GetTagByName(EC_TAG_WEBSERVER_GUEST);
		if (tag) {
			webInterface->m_AllowGuest = true;
			if ( ! webInterface->m_bForcedGuestPassword ) {
				tag = tag->GetTagByName(EC_TAG_PASSWD_HASH);
				if (tag) {
					webInterface->m_GuestPass = tag->GetStringData();
				} else {
					webInterface->m_GuestPass = wxEmptyString;
				}
			}
		} else {
			webInterface->m_AllowGuest = false;
		}
	}

	if ( ! webInterface->m_bForcedUseGzip ) {
		// we only need to check the presence of this tag
		if ( wsprefs->GetTagByName(EC_TAG_WEBSERVER_USEGZIP) ) {
			webInterface->m_UseGzip = true;
		} else {
			webInterface->m_UseGzip = false;
		}
	}

	tag = wsprefs->GetTagByName(EC_TAG_WEBSERVER_REFRESH);
	if (tag) {
		m_nRefresh = tag->GetInt32Data();
	} else {
		m_nRefresh = 120;
	}

	delete reply;

	return wsport;
}

//sends output to web interface
void CWebServer::Print(const wxString &s) {
	webInterface->Show(s);
}

// send EC request and discard output
void CWebServer::Send_Discard_V2_Request(CECPacket *request)
{
		CECPacket *reply = webInterface->SendRecvMsg_v2(request);
		if ( reply ) {
			if ( reply->GetOpCode() == EC_OP_STRINGS ) {
				for(int i = 0;i < reply->GetTagCount();i++) {
					CECTag *tag = reply->GetTagByIndex(i);
					if ( tag->GetTagName() == EC_TAG_STRING ) {
						webInterface->Show(tag->GetStringData());
					}
				}
			} else if ( reply->GetOpCode() == EC_OP_FAILED ) {
				if (reply->GetTagCount()) {
					webInterface->Show(wxString(_("Request failed with the following error: ")) + wxString(wxGetTranslation(reply->GetTagByIndex(0)->GetStringData())) + wxT("."));
				} else {
					webInterface->Show(_("Request failed with an unknown error."));
				}
			}
			delete reply;
		}
}

//reload template file
void CWebServer::ReloadTemplates(void) {
	time_t t = time(NULL);
	char *s = new char[255];
	strftime(s, 255, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&t));
	m_Params.sLastModified = char2unicode(s);
	delete[] s;
	
	m_Params.sETag = MD5Sum(m_Params.sLastModified).GetHash();
	
	wxString sFile;
	if( webInterface->m_HasTemplate) {
		sFile = webInterface->m_TemplateFileName;
	} else {
		sFile = wxString(char2unicode(getenv("HOME"))) + wxT("/.aMule/aMule.tmpl");
	}
	if (!wxFileName::FileExists(sFile)) {
		// no file. do nothing.
		// Show error locally instead of logging into remote core
		webInterface->Show(wxString(_("Can't load templates: Can't open file ")) + sFile);
		//CECPacket req(EC_OP_ADDLOGLINE);
		//req.AddTag(CECEmptyTag(EC_TAG_LOG_TO_STATUS));	// to log to statusbar also
		//req.AddTag(CECTag(EC_TAG_STRING, wxString(_("Can't load templates: Can't open file ")) + sFile));
		//Send_Discard_V2_Request(&req);
		return;
	}

	wxFileInputStream input(sFile);
	if (input.Ok()) {
		wxTextInputStream file(input);
		wxString sAll;
		while (!input.Eof()) {
			wxString sLine = file.ReadString();
			sAll += sLine + wxT("\n");
		}
		wxString sVersion = _LoadTemplate(sAll,wxT("TMPL_VERSION"));
		long lVersion = StrToLong(sVersion);
		if (lVersion < WEB_SERVER_TEMPLATES_VERSION) {
			// Show error locally instead of logging into remote core
			webInterface->Show(wxString(_("Can't load templates: Can't open file ")) + sFile);
			//CECPacket req(EC_OP_ADDLOGLINE);
			//req.AddTag(CECEmptyTag(EC_TAG_LOG_TO_STATUS));	// to log to statusbar also
			//req.AddTag(CECTag(EC_TAG_STRING, wxString(_("Can't load templates: Can't open file ")) + sFile));
			//Send_Discard_V2_Request(&req);
		} else {
			m_Templates.sHeader = _LoadTemplate(sAll,wxT("TMPL_HEADER"));
			m_Templates.sHeaderMetaRefresh = _LoadTemplate(sAll,wxT("TMPL_HEADER_META_REFRESH"));
			m_Templates.sHeaderStylesheet = _LoadTemplate(sAll,wxT("TMPL_HEADER_STYLESHEET"));
			m_Templates.sFooter = _LoadTemplate(sAll,wxT("TMPL_FOOTER"));
			m_Templates.sServerList = _LoadTemplate(sAll,wxT("TMPL_SERVER_LIST"));
			m_Templates.sServerLine = _LoadTemplate(sAll,wxT("TMPL_SERVER_LINE"));
			m_Templates.sTransferImages = _LoadTemplate(sAll,wxT("TMPL_TRANSFER_IMAGES"));
			m_Templates.sTransferList = _LoadTemplate(sAll,wxT("TMPL_TRANSFER_LIST"));
			m_Templates.sTransferDownHeader = _LoadTemplate(sAll,wxT("TMPL_TRANSFER_DOWN_HEADER"));
			m_Templates.sTransferDownFooter = _LoadTemplate(sAll,wxT("TMPL_TRANSFER_DOWN_FOOTER"));
			m_Templates.sTransferDownLine = _LoadTemplate(sAll,wxT("TMPL_TRANSFER_DOWN_LINE"));
			m_Templates.sTransferDownLineGood = _LoadTemplate(sAll,wxT("TMPL_TRANSFER_DOWN_LINE_GOOD"));
			m_Templates.sTransferUpHeader = _LoadTemplate(sAll,wxT("TMPL_TRANSFER_UP_HEADER"));
			m_Templates.sTransferUpFooter = _LoadTemplate(sAll,wxT("TMPL_TRANSFER_UP_FOOTER"));
			m_Templates.sTransferUpLine = _LoadTemplate(sAll,wxT("TMPL_TRANSFER_UP_LINE"));
			m_Templates.sTransferUpQueueShow = _LoadTemplate(sAll,wxT("TMPL_TRANSFER_UP_QUEUE_SHOW"));
			m_Templates.sTransferUpQueueHide = _LoadTemplate(sAll,wxT("TMPL_TRANSFER_UP_QUEUE_HIDE"));
			m_Templates.sTransferUpQueueLine = _LoadTemplate(sAll,wxT("TMPL_TRANSFER_UP_QUEUE_LINE"));
			m_Templates.sTransferBadLink = _LoadTemplate(sAll,wxT("TMPL_TRANSFER_BAD_LINK"));
			m_Templates.sDownloadLink = _LoadTemplate(sAll,wxT("TMPL_DOWNLOAD_LINK"));
			m_Templates.sSharedList = _LoadTemplate(sAll,wxT("TMPL_SHARED_LIST"));
			m_Templates.sSharedLine = _LoadTemplate(sAll,wxT("TMPL_SHARED_LINE"));
			m_Templates.sSharedLineChanged = _LoadTemplate(sAll,wxT("TMPL_SHARED_LINE_CHANGED"));
			m_Templates.sGraphs = _LoadTemplate(sAll,wxT("TMPL_GRAPHS"));
			m_Templates.sLog = _LoadTemplate(sAll,wxT("TMPL_LOG"));
			m_Templates.sServerInfo = _LoadTemplate(sAll,wxT("TMPL_SERVERINFO"));
			m_Templates.sDebugLog = _LoadTemplate(sAll,wxT("TMPL_DEBUGLOG"));
			m_Templates.sStats = _LoadTemplate(sAll,wxT("TMPL_STATS"));
			m_Templates.sPreferences = _LoadTemplate(sAll,wxT("TMPL_PREFERENCES"));
			m_Templates.sLogin = _LoadTemplate(sAll,wxT("TMPL_LOGIN"));
			m_Templates.sConnectedServer = _LoadTemplate(sAll,wxT("TMPL_CONNECTED_SERVER"));
			m_Templates.sAddServerBox = _LoadTemplate(sAll,wxT("TMPL_ADDSERVERBOX"));
			m_Templates.sWebSearch = _LoadTemplate(sAll,wxT("TMPL_WEBSEARCH"));
			m_Templates.sSearch = _LoadTemplate(sAll,wxT("TMPL_SEARCH"));
			m_Templates.iProgressbarWidth=StrToLong(_LoadTemplate(sAll,wxT("PROGRESSBARWIDTH")));
			m_Templates.sSearchHeader = _LoadTemplate(sAll,wxT("TMPL_SEARCH_RESULT_HEADER"));
			m_Templates.sSearchResultLine = _LoadTemplate(sAll,wxT("TMPL_SEARCH_RESULT_LINE"));
			m_Templates.sProgressbarImgs = _LoadTemplate(sAll,wxT("PROGRESSBARIMGS"));
			m_Templates.sProgressbarImgsPercent = _LoadTemplate(sAll,wxT("PROGRESSBARPERCENTIMG"));
			m_Templates.sClearCompleted = _LoadTemplate(sAll,wxT("TMPL_TRANSFER_DOWN_CLEARBUTTON"));
			m_Templates.sCatArrow= _LoadTemplate(sAll,wxT("TMPL_CATARROW"));

			m_Templates.sProgressbarImgsPercent.Replace(wxT("[PROGRESSGIFNAME]"),wxT("%s"));
			m_Templates.sProgressbarImgsPercent.Replace(wxT("[PROGRESSGIFINTERNAL]"),wxT("%i"));
			m_Templates.sProgressbarImgs.Replace(wxT("[PROGRESSGIFNAME]"),wxT("%s"));
			m_Templates.sProgressbarImgs.Replace(wxT("[PROGRESSGIFINTERNAL]"),wxT("%i"));
		}
	} else {
		// Show error locally instead of logging into remote core
		webInterface->Show(wxString(_("Can't load templates: Can't open file ")) + sFile);
		//CECPacket req(EC_OP_ADDLOGLINE);
		//req.AddTag(CECEmptyTag(EC_TAG_LOG_TO_STATUS));	// to log to statusbar also
		//req.AddTag(CECTag(EC_TAG_STRING, wxString(_("Can't load templates: Can't open file ")) + sFile));
		//Send_Discard_V2_Request(&req);
	}
}


wxString CWebServer::_LoadTemplate(wxString sAll, wxString sTemplateName) {
	wxString sRet;
	int nStart = sAll.Find(wxT("<--") + sTemplateName + wxT("-->"));
	int nEnd = sAll.Find(wxT("<--") + sTemplateName + wxT("_END-->"));
	
	if (nStart != -1 && nEnd != -1)	{
		nStart += sTemplateName.Length() + 7;
		sRet = sAll.Mid(nStart, nEnd - nStart - 1);
	}
	
	if (sRet.IsEmpty()) {
		if (sTemplateName==wxT("TMPL_VERSION"))
			webInterface->Show(_("Can't find template version number!\nPlease replace aMule.tmpl with a newer version!"));
		webInterface->Show(_("Failed to load template ") + sTemplateName + wxT("\n"));
	}
	return sRet;
}


void CWebServer::_SetSharedFilePriority(CWebServer *pThis, wxString hash, uint8 priority) {	
	int prio = (int) priority;
	if (prio >= 0 && prio < 5) {
		pThis->webInterface->SendRecvMsg(wxString::Format(wxT("SHAREDFILES SETAUTOUPPRIORITY %s %d"), hash.GetData(), 0));
		pThis->webInterface->SendRecvMsg(wxString::Format(wxT("SHAREDFILES SETUPPRIORITY %s %d"), hash.GetData(), prio));
	} else if (prio == 5) {
		pThis->webInterface->SendRecvMsg(wxString::Format(wxT("SHAREDFILES SETAUTOUPPRIORITY %s %d"), hash.GetData(), 1));
		pThis->webInterface->SendRecvMsg(wxString::Format(wxT("SHAREDFILES UPDATEAUTOUPPRIORITY %s"), hash.GetData()));
	}
}


void CWebServer::AddStatsLine(UpDown* line) {
	m_Params.PointsForWeb.Add(line);
	if (m_Params.PointsForWeb.GetCount() > WEB_GRAPH_WIDTH) {
		delete m_Params.PointsForWeb[0];
		m_Params.PointsForWeb.RemoveAt(0);
	}
}


void CWebServer::ProcessImgFileReq(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL) return;
		
	wxString filename=Data.sURL;
	wxString contenttype;

	pThis->webInterface->DebugShow(wxT("inc. fname=") + filename + wxT("\n"));
	if (filename.Right(4).MakeLower()==wxT(".gif")) contenttype=wxT("Content-Type: image/gif\r\n");
	else if (filename.Right(4).MakeLower()==wxT(".jpg") || filename.Right(5).MakeLower()==wxT(".jpeg")) contenttype=wxT("Content-Type: image/jpg\r\n");
	else if (filename.Right(4).MakeLower()==wxT(".bmp")) contenttype=wxT("Content-Type: image/bmp\r\n");
	else if (filename.Right(4).MakeLower()==wxT(".png")) contenttype=wxT("Content-Type: image/png");
	//DonQ - additional filetypes
	else if (filename.Right(4).MakeLower()==wxT(".ico")) contenttype=wxT("Content-Type: image/x-icon\r\n");
	else if (filename.Right(4).MakeLower()==wxT(".css")) contenttype=wxT("Content-Type: text/css\r\n");
	else if (filename.Right(3).MakeLower()==wxT(".js")) contenttype=wxT("Content-Type: text/javascript\r\n");
	
	contenttype += wxT("Last-Modified: ") + pThis->m_Params.sLastModified + wxT("\r\n");
	contenttype += wxT("ETag: ") + pThis->m_Params.sETag + wxT("\r\n");
	
	pThis->webInterface->DebugShow(wxT("**** imgrequest: ") + filename + wxT("\n"));

	wxFFile fis(imgs_folder + filename.Right(filename.Length()-1));
	if (!fis.IsOpened()) {
		pThis->webInterface->DebugShow(wxT("**** imgrequest: file ") + filename + wxT(" does not exists or you have no permisions\n"));
	} else {
		size_t file_size = fis.Length();
		char* img_data = new char[file_size];
		size_t bytes_read;
		if ((bytes_read = fis.Read(img_data,file_size)) != file_size) {
			pThis->webInterface->DebugShow(wxT("**** imgrequest: file ") + filename + wxT(" was not fully read\n"));				
		}
		// Try to send as much as possible if it failed
		Data.pSocket->SendContent(unicode2char(contenttype),(void*)img_data,bytes_read);	
		delete [] img_data;
	}
	
	
}


void CWebServer::ProcessStyleFileReq(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL) {
		return;
	}
	wxString filename = Data.sURL;
	wxString contenttype;
	pThis->webInterface->DebugShow(wxT("inc. fname=") + filename + wxT("\n"));
	contenttype = wxT("Content-Type: text/css\r\n");
	
	pThis->webInterface->DebugShow(wxT("**** cssrequest: ") + filename + wxT("\n"));
	
	wxFFile fis(imgs_folder + filename.Right(filename.Length()-1));
	if (!fis.IsOpened()) {
		pThis->webInterface->DebugShow(wxT("**** cssrequest: file ") + filename + wxT(" does not exists or you have no permisions\n"));
	} else {
		size_t file_size = fis.Length();
		char* css_data = new char[file_size];
		size_t bytes_read;
		if ((bytes_read = fis.Read(css_data,file_size)) != file_size) {
			pThis->webInterface->DebugShow(wxT("**** cssrequest: file ") + filename + wxT(" was not fully read\n"));				
		}
		// Try to send as much as possible if it failed
		Data.pSocket->SendContent(unicode2char(contenttype),(void*)css_data,bytes_read);	
		delete css_data;
	}
	
}

void CWebServer::ProcessURL(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL) {
		return;
	}
	bool isUseGzip = pThis->webInterface->m_UseGzip;
	wxString Out;
	// List Entry Templates
	wxString OutE;
	wxString OutE2;
	// ServerStatus Templates
	wxString OutS;
	TCHAR *gzipOut = NULL;

	long gzipLen = 0;
	wxString HTTPProcessData;
	wxString HTTPTemp;
	srand ( time(NULL) );
	long lSession = 0;
	wxString sSes = _ParseURL(Data, wxT("ses"));
	if (!sSes.IsEmpty()) {
		sSes.ToLong(&lSession);
	}
	wxString sSession = sSession.Format(wxT("%ld"), lSession);
	wxString sW = _ParseURL(Data, wxT("w"));
	if (sW == wxT("password")) {
		pThis->GetWSPrefs();
		wxString PwStr = _ParseURL(Data, wxT("p"));
		wxString PwHash = MD5Sum(PwStr).GetHash();
		bool login = false;
		wxString ip = char2unicode(inet_ntoa(Data.inadr));
		if ( (PwHash == pThis->webInterface->m_AdminPass) || (PwStr.IsEmpty() && pThis->webInterface->m_AdminPass.IsEmpty()) ) {
			Session* ses = new Session();
			ses->admin = true;
			ses->startTime = time(NULL);
			ses->lSession = lSession = rand() * 10000L + rand();
			pThis->m_Params.Sessions.Add(ses);
			login = true;
			pThis->webInterface->DebugShow(wxT("*** logged in as admin\n"));
		} else if ( (pThis->webInterface->m_AllowGuest) &&
				( (PwHash == pThis->webInterface->m_GuestPass) || (PwStr.IsEmpty() && pThis->webInterface->m_GuestPass.IsEmpty()) )) {
			Session* ses = new Session();
			ses->admin = false;
			ses->startTime = time(NULL);
			ses->lSession = lSession = rand() * 10000L + rand();
			pThis->m_Params.Sessions.Add(ses);
			login = true;
			pThis->webInterface->DebugShow(wxT("*** logged in as guest\n"));
		} else {
			// This call to ::GetTickCount has segfaulted once with this == 0x0, because
			// wxUSE_GUI was set to 1 in a console only application. This may happen due
			// to wrong wxWidgets configuration.
			//
			// save failed attempt (ip,time)
			TransferredData newban = {StringIPtoUint32(ip), ::GetTickCount()}; 
			pThis->m_Params.badlogins.Add(&newban);
			login = false;
			pThis->webInterface->DebugShow(wxT("*** login failed\n"));
		}
		isUseGzip = false;
		if (login) {
			uint32 ipn = StringIPtoUint32(ip);
			for (size_t i = 0; i < pThis->m_Params.badlogins.GetCount();) {
				if (ipn == pThis->m_Params.badlogins[i]->datalen) {
					pThis->m_Params.badlogins.RemoveAt(i);
				} else {
					++i;
				}
			}
		}
	}
	if (sW == wxT("logout")) {
		_RemoveSession(Data, lSession);
	}
	if (_IsLoggedIn(Data, lSession)) {
		Out += _GetHeader(Data, lSession);		
		wxString sPage = sW;
		pThis->webInterface->DebugShow(wxT("***** logged in, getting page ") + sPage + wxT("\n"));
		pThis->webInterface->DebugShow(wxT("***** session is ") + sSession + wxT("\n"));		
		if (sPage == wxT("server")) {
			Out += _GetServerList(Data);
		} else if (sPage == wxT("download")) {
			Out += _GetDownloadLink(Data);
		} else if (sPage == wxT("shared")) { 
			Out += _GetSharedFilesList(Data);
		} else if (sPage == wxT("transfer")) {
			Out += _GetTransferList(Data);
		} else if (sPage == wxT("websearch")) {
			Out += _GetWebSearch(Data);
		} else if (sPage == wxT("search")) {
			Out += _GetSearch(Data);
		} else if (sPage == wxT("graphs")) {
			Out += _GetGraphs(Data);
		} else if (sPage == wxT("log")) {
			Out += _GetLog(Data);
		} else if (sPage == wxT("sinfo")) {
			Out += _GetServerInfo(Data);
		} else if (sPage == wxT("debuglog")) {
			Out += _GetDebugLog(Data);
		} else if (sPage == wxT("stats")) {
			Out += _GetStats(Data);
		} else if (sPage == wxT("options")) {
			Out += _GetPreferences(Data);
		}		
		Out += _GetFooter(Data);
		if (sPage.IsEmpty()) {
			isUseGzip = false;
		}
		if (isUseGzip) {
#if wxUSE_UNICODE
			const wxCharBuffer buf = wxConvUTF8.cWC2MB(Out.wc_str(aMuleConv));
			const char *httpOut = (const char *)buf;
#else
			const char *httpOut = (const char *)Out;
#endif
			bool bOk = false;
			try {
				uLongf destLen = strlen(httpOut) + 1024;
				gzipOut = new TCHAR[destLen];
				if( _GzipCompress((Bytef*)gzipOut, &destLen, 
				   (const Bytef*)httpOut, strlen(httpOut), Z_DEFAULT_COMPRESSION) == Z_OK) {
					bOk = true;
					gzipLen = destLen;
				}
			} catch(...) {
			}			
			if (!bOk) {
				isUseGzip = false;
				if (gzipOut != NULL) {
					delete[] gzipOut;
					gzipOut = NULL;
				}
			}
		}
	} else {
		isUseGzip = false;		
		uint32 ip = inet_addr(inet_ntoa( Data.inadr ));
		uint32 faults = 0;
		// check for bans
		for (size_t i = 0; i < pThis->m_Params.badlogins.GetCount(); ++i) {
			if (pThis->m_Params.badlogins[i]->datalen==ip) ++faults;
		}
		if (faults>4) {
			Out += _("Access denied!");
			// set 15 mins ban by using the badlist
			TransferredData preventive={ip, ::GetTickCount() + (15*60*1000) };
			for (int i = 0; i <= 5; ++i) {
				pThis->m_Params.badlogins.Add(&preventive);
			}
		} else {
			Out += _GetLoginScreen(Data);
		}
	}
	//
	// send answer ...
	//
#if wxUSE_UNICODE
	const wxCharBuffer buf = wxConvUTF8.cWC2MB(Out.wc_str(aMuleConv));
	const char *httpOut = (const char *)buf;
#else
	const char *httpOut = (const char *)Out;
#endif

	if (!isUseGzip)	{
		Data.pSocket->SendContent(HTTPInit, httpOut, strlen(httpOut));
	} else {
		Data.pSocket->SendContent(HTTPInitGZ, gzipOut, gzipLen);
	}
	if (gzipOut != NULL) {
		delete[] gzipOut;
	}
}


wxString CWebServer::_ParseURLArray(ThreadData Data, wxString fieldname) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL) {
		return wxEmptyString;
	}	
	wxString URL = Data.sURL;
	wxString res,temp;
	while (URL.Length()>0) {
		int pos=URL.MakeLower().Find(fieldname.MakeLower() + wxT("="));
		if (pos>-1) {
			temp=_ParseURL(Data,fieldname);
			if (temp.IsEmpty()) break;
			res.Append(temp+wxT("|"));
			Data.sURL.Remove(pos, 10);
			URL=Data.sURL;
			//URL.Remove(pos,10);
		} else break;
	}
	return res;
}


wxString CWebServer::_ParseURL(ThreadData Data, wxString fieldname) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL) {
		return wxEmptyString;
	}	
	wxString URL = Data.sURL;	
	wxString value;
	wxString Parameter;
	char fromReplace[4] = "";	// decode URL
	char toReplace[2] = "";		// decode URL
	int i = 0;
	int findPos = -1;
	int findLength = 0;
	pThis->webInterface->DebugShow(wxT("*** parsing url ") + URL + wxT(" :: field ") + fieldname + wxT("\n"));
	if (URL.Find(wxT("?")) > -1) {
		Parameter = URL.Mid(URL.Find(wxT("?"))+1, URL.Length()-URL.Find(wxT("?"))-1);
		// search the fieldname beginning / middle and strip the rest...
		if (Parameter.Find(fieldname + wxT("=")) == 0) {
			findPos = 0;
			findLength = fieldname.Length() + 1;
		}
		if (Parameter.Find(wxT("&") + fieldname + wxT("=")) > -1) {
			findPos = Parameter.Find(wxT("&") + fieldname + wxT("="));
			findLength = fieldname.Length() + 2;
		}
		if (findPos > -1) {
			Parameter = Parameter.Mid(findPos + findLength, Parameter.Length());
			if (Parameter.Find(wxT("&")) > -1) {
				Parameter = Parameter.Mid(0, Parameter.Find(wxT("&")));
			}	
			value = Parameter;
			// decode value ...
			value.Replace(wxT("+"), wxT(" "));
			for (i = 0 ; i <= 255 ; ++i) {
				sprintf(fromReplace, "%%%02x", i);
				toReplace[0] = (char)i;
				toReplace[1] = 0;
				value.Replace(wxString::Format(wxT("%s"), fromReplace), wxString::Format(wxT("%s"), toReplace));
				sprintf(fromReplace, "%%%02X", i);
				value.Replace(wxString::Format(wxT("%s"), fromReplace), wxString::Format(wxT("%s"), toReplace));
			}
		}
	}
	// this for debug only
	//pThis->webInterface->Show(_("*** URL parsed. returning ") + value + wxT("\n"));
	return value;
}


wxString CWebServer::_GetHeader(ThreadData Data, long lSession) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (!pThis)
		return wxEmptyString;

	wxString sSession = sSession.Format(wxT("%ld"), lSession);

	wxString Out = pThis->m_Templates.sHeader;

	Out.Replace(wxT("[CharSet]"), _GetWebCharSet());

	if (pThis->m_nRefresh) {
		wxString sPage = _ParseURL(Data, wxT("w"));
		if ((sPage == wxT("transfer")) || (sPage == wxT("server")) ||
			(sPage == wxT("graphs")) || (sPage == wxT("log")) ||
			(sPage == wxT("sinfo")) || (sPage == wxT("debuglog")) ||
			(sPage == wxT("stats"))) {
			wxString sT = pThis->m_Templates.sHeaderMetaRefresh;
			wxString sRefresh = sRefresh.Format(wxT("%d"), pThis->m_nRefresh);
			sT.Replace(wxT("[RefreshVal]"), sRefresh);
			
			wxString catadd = wxEmptyString;
			if (sPage == wxT("transfer"))
				catadd=wxT("&cat=") + _ParseURL(Data, wxT("cat"));
			sT.Replace(wxT("[wCommand]"), sPage+catadd);
			
			Out.Replace(wxT("[HeaderMeta]"), sT);
		}
	}
	
	Out.Replace(wxT("[Session]"), sSession);
	pThis->webInterface->DebugShow(wxT("*** replaced session with ") + sSession + wxT("\n"));
	Out.Replace(wxT("[HeaderMeta]"), wxEmptyString); // In case there are no meta
	Out.Replace(wxT("[aMuleAppName]"), wxT("aMule"));
	Out.Replace(wxT("[version]"), wxT(VERSION));
	Out.Replace(wxT("[StyleSheet]"), pThis->m_Templates.sHeaderStylesheet);
	Out.Replace(wxT("[WebControl]"), _("Web Control Panel"));
	Out.Replace(wxT("[Transfer]"), _("Transfer"));
	Out.Replace(wxT("[Server]"), _("Server list"));
	Out.Replace(wxT("[Shared]"), _("Shared Files"));
	Out.Replace(wxT("[Download]"), _("ED2K Link(s)"));
	Out.Replace(wxT("[Graphs]"), _("Graphs"));
	Out.Replace(wxT("[Log]"), _("Log"));
	Out.Replace(wxT("[ServerInfo]"), _("Serverinfo"));
	Out.Replace(wxT("[DebugLog]"), _("Debug Log"));
	Out.Replace(wxT("[Stats]"), _("Statistics"));
	Out.Replace(wxT("[Options]"), _("Preferences"));
	Out.Replace(wxT("[Logout]"), _("Logout"));
	Out.Replace(wxT("[Search]"), _("Search"));

	wxString sConnected;

	CECPacket stat_req(EC_OP_STAT_REQ, EC_DETAIL_CMD);

	CECPacket *stats = pThis->webInterface->SendRecvMsg_v2(&stat_req);
	if ( !stats ) {
		return wxEmptyString;
	}
	
	switch (stats->GetTagByName(EC_TAG_CONNSTATE)->GetInt8Data()) {
		case 0:
			sConnected = _("Not connected");
			if (IsSessionAdmin(Data,sSession)) {
				sConnected += wxT(" (<small><a href=\"?ses=") + sSession +
					wxT("&w=server&c=connect\">Connect to any server</a></small>)");
			}
			break;
		case 1:
			sConnected = _("Now connecting");
			break;
		case 2:
		case 3: {
				CECTag *server = stats->GetTagByName(EC_TAG_CONNSTATE)->GetTagByIndex(0);
				sConnected = _("Connected to ");
				sConnected += server->GetTagByName(EC_TAG_SERVER_NAME)->GetStringData() + wxT(" ");
				sConnected += server->GetIPv4Data().StringIP() + wxT(" ");
				sConnected += stats->GetTagByName(EC_TAG_CONNSTATE)->GetInt8Data() == 2 ? _("with LowID") : _("with HighID");
			}
			break;
	}

	Out.Replace(wxT("[Connected]"), wxT("<b>Connection:</b> ") + sConnected);

	Out.Replace(wxT("[Speed]"),
		wxString::Format(wxT("<b>Speed:</b> Up: %.1f | Down: %.1f <small> (Limits: %.1f/%.1f)</small>"),
		((double)stats->GetTagByName(EC_TAG_STATS_UL_SPEED)->GetInt32Data())/1024.0,
		((double)stats->GetTagByName(EC_TAG_STATS_DL_SPEED)->GetInt32Data())/1024.0,
		((double)stats->GetTagByName(EC_TAG_STATS_UL_SPEED_LIMIT)->GetInt32Data())/1024.0,
		((double)stats->GetTagByName(EC_TAG_STATS_DL_SPEED_LIMIT)->GetInt32Data())/1024.0));

	return Out;
}


wxString CWebServer::_GetFooter(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return wxEmptyString;

	return pThis->m_Templates.sFooter;
}


wxString CWebServer::_GetServerList(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return wxEmptyString;

	wxString sSession = _ParseURL(Data, wxT("ses"));

	wxString sAddServerBox;

	wxString sCmd = _ParseURL(Data, wxT("c"));
	if (sCmd == wxT("connect") && IsSessionAdmin(Data,sSession) ) {
		wxString sIP = _ParseURL(Data, wxT("ip"));
		wxString sPort = _ParseURL(Data, wxT("port"));
		uint32 ip;
		uint32 port;
		CECPacket req(EC_OP_SERVER_CONNECT);
		if ( sIP.ToULong((unsigned long *)&ip, 16) && sPort.ToULong((unsigned long *)&port, 10) ) {
			req.AddTag(CECTag(EC_TAG_SERVER, EC_IPv4_t(ip, port)));
		}
		pThis->Send_Discard_V2_Request(&req);
	} else if (sCmd == wxT("disconnect") && IsSessionAdmin(Data,sSession)) {
		CECPacket req(EC_OP_SERVER_DISCONNECT);
		pThis->Send_Discard_V2_Request(&req);
	} else if (sCmd == wxT("remove") && IsSessionAdmin(Data,sSession)) {
		wxString sIP = _ParseURL(Data, wxT("ip"));
		wxString sPort = _ParseURL(Data, wxT("port"));
		uint32 ip;
		uint32 port;
		if ( sIP.ToULong((unsigned long *)&ip, 16) && sPort.ToULong((unsigned long *)&port, 10) ) {
			CECPacket req(EC_OP_SERVER_REMOVE);
			req.AddTag(CECTag(EC_TAG_SERVER, EC_IPv4_t(ip, port)));
			pThis->Send_Discard_V2_Request(&req);
		}
	} else if (sCmd == wxT("options")) {
		sAddServerBox = _GetAddServerBox(Data);
	}
	
	wxString sSortRev = _ParseURL(Data, wxT("sortreverse"));
	wxString sSort = _ParseURL(Data, wxT("sort"));

	// reverse sort direction in link
	pThis->m_ServersInfo.SetSortOrder(sSort, (sSortRev == wxT("true")));

	wxString Out = pThis->m_Templates.sServerList;
	Out.Replace(wxT("[ConnectedServerData]"), _GetConnectedServer(Data));
	Out.Replace(wxT("[AddServerBox]"), sAddServerBox);
	Out.Replace(wxT("[Session]"), sSession);

	pThis->m_ServersInfo.ProcessHeadersLine(Out);
	
	Out.Replace(wxT("[ServerList]"), _("Server list"));
	Out.Replace(wxT("[Servername]"), _("Server name"));
	Out.Replace(wxT("[Description]"), _("Description"));
	Out.Replace(wxT("[Address]"), _("IP"));
	Out.Replace(wxT("[Connect]"), _("Connect"));
	Out.Replace(wxT("[Users]"), _("users"));
	Out.Replace(wxT("[Files]"), _("files"));
	Out.Replace(wxT("[Actions]"), _("Actions"));
		
	wxString OutE = pThis->m_Templates.sServerLine;
	OutE.Replace(wxT("[Connect]"), _("Connect"));
	OutE.Replace(wxT("[RemoveServer]"), _("Remove selected server"));
	OutE.Replace(wxT("[ConfirmRemove]"), _("Are you sure to remove this server from list?"));


	// Populating array - query core and sort
	pThis->m_ServersInfo.ReQuery();

	// Displaying
	wxString sList;
	ServersInfo::ItemIterator i = pThis->m_ServersInfo.GetBeginIterator();
	while (i != pThis->m_ServersInfo.GetEndIterator()) {
		wxString HTTPProcessData = OutE; // Copy Entry Line to Temp
		HTTPProcessData.Replace(wxT("[1]"), i->sServerName);
		HTTPProcessData.Replace(wxT("[2]"), i->sServerDescription);
		HTTPProcessData.Replace(wxT("[3]"), i->sServerIP );
		
		wxString sT;
		if (i->nServerUsers > 0) {
			if (i->nServerMaxUsers > 0) {
				sT.Printf(wxT("%d (%d)"), i->nServerUsers, i->nServerMaxUsers);
			} else {
				sT.Printf(wxT("%d"), i->nServerUsers);
			}
		} else {
			sT = wxT("0");
		}
		
		HTTPProcessData.Replace(wxT("[4]"), sT);
		sT = wxString::Format(wxT("%d"), i->nServerFiles);
		
		HTTPProcessData.Replace(wxT("[5]"), sT);
		if ( IsSessionAdmin(Data,sSession) ) {
			HTTPProcessData.Replace(wxT("[6]"),
						wxT("?ses=") + sSession + 
							wxString::Format(wxT("&w=server&c=connect&ip=%08x&port=%d"),
								 i->nServerIP, i->nServerPort));
			HTTPProcessData.Replace(wxT("[LinkRemove]"),
						wxT("?ses=") + sSession + 
						wxString::Format(wxT("&w=server&c=remove&ip=%08x&port=%d"),
								  i->nServerIP, i->nServerPort));
		} else {
			HTTPProcessData.Replace(wxT("[6]"), GetPermissionDenied());
			HTTPProcessData.Replace(wxT("[LinkRemove]"), GetPermissionDenied());
		}

		sList += HTTPProcessData;
		i++;
	}
	Out.Replace(wxT("[ServersList]"), sList);

	return Out;
}

wxString CWebServer::_GetTransferList(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL) {
		return wxEmptyString;
	}
	//
	wxString sSession = _ParseURL(Data, wxT("ses"));
	wxString sCat = _ParseURL(Data, wxT("cat"));

	wxString sOp = _ParseURL(Data, wxT("op"));
	wxString sFileHash = _ParseURL(Data, wxT("file"));
	wxString sSort = _ParseURL(Data, wxT("sort"));
	wxString sDownloadSortRev = _ParseURL(Data, wxT("sortreverse"));

	wxString Out;
	wxString HTTPTemp = _ParseURL(Data, wxT("c"));
	if (!HTTPTemp.IsEmpty() && IsSessionAdmin(Data, sSession)) {
		if (HTTPTemp.Right(1) != wxT("/")) {
			HTTPTemp += wxT("/");
		}
		CECPacket req(EC_OP_ED2K_LINK);
		req.AddTag(CECTag(EC_TAG_STRING, HTTPTemp));
		CECPacket *response = pThis->webInterface->SendRecvMsg_v2(&req);
		if ( response->GetOpCode() == EC_OP_FAILED) {
			wxString HTTPTempC = _("This ed2k link is invalid (") + HTTPTemp + wxT(")");
			Out = pThis->m_Templates.sTransferBadLink;
			Out.Replace(wxT("[InvalidLink]"), HTTPTempC);
			Out.Replace(wxT("[Link]"), HTTPTemp);
		}
		delete response;
	}
	//
	// Commands
	//
	if (!sOp.IsEmpty() && !sFileHash.IsEmpty()) {
		CECPacket *file_cmd = 0;
		if (sOp == wxT("pause")) {
			file_cmd = new CECPacket(EC_OP_PARTFILE_PAUSE);
		} else if (sOp == wxT("resume")) {
			file_cmd = new CECPacket(EC_OP_PARTFILE_RESUME);
		} else if (sOp == wxT("cancel")) {
			file_cmd = new CECPacket(EC_OP_PARTFILE_DELETE);
		} else if (sOp == wxT("prioup")) {
			//file_cmd = new CECPacket(EC_OP_KNOWNFILE_SET_UP_PRIO);
		} else if (sOp == wxT("priodown")) {
			//file_cmd = new CECPacket(EC_OP_KNOWNFILE_SET_UP_PRIO);
		}
		if ( file_cmd ) {
			file_cmd->AddTag(CECTag(EC_TAG_PARTFILE, sFileHash));
			pThis->Send_Discard_V2_Request(file_cmd);
			delete file_cmd;
		}
	}

	pThis->m_DownloadFilesInfo.SetSortOrder(sSort, sDownloadSortRev);
	pThis->m_Params.bShowUploadQueue = _ParseURL(Data, wxT("showuploadqueue")) == wxT("true");

	Out += pThis->m_Templates.sTransferImages;
	Out += pThis->m_Templates.sTransferList;
	Out.Replace(wxT("[DownloadHeader]"), pThis->m_Templates.sTransferDownHeader);
	Out.Replace(wxT("[DownloadFooter]"), pThis->m_Templates.sTransferDownFooter);
	Out.Replace(wxT("[UploadHeader]"), pThis->m_Templates.sTransferUpHeader);
	Out.Replace(wxT("[UploadFooter]"), pThis->m_Templates.sTransferUpFooter);
	Out.Replace(wxT("[Session]"), sSession);


	Out.Replace(wxT("[CATBOX]"), GetStatusBox(sCat));
	//InsertCatBox(pThis, Out, cat, wxEmptyString, true, true);
	
	pThis->m_DownloadFilesInfo.ProcessHeadersLine(Out);
	
	Out.Replace(wxT("[Filename]"), _("File Name"));
	Out.Replace(wxT("[Size]"), _("Size"));
	Out.Replace(wxT("[Completed]"), _("Complete"));
	Out.Replace(wxT("[Transferred]"), _("Transferred"));
	Out.Replace(wxT("[Progress]"), _("Progress"));
	Out.Replace(wxT("[Speed]"), _("Speed"));
	Out.Replace(wxT("[Sources]"), _("Sources"));
	Out.Replace(wxT("[Actions]"), _("Actions"));
	Out.Replace(wxT("[User]"), _("Username"));
	Out.Replace(wxT("[TotalDown]"), _("Downloaded total"));
	Out.Replace(wxT("[TotalUp]"), _("Uploaded total"));
	Out.Replace(wxT("[Prio]"), _("Priority"));

	Out.Replace(wxT("[CatSel]"), sCat.Length() ? (wxT("&cat=") + sCat) : wxString(wxEmptyString));

	wxString OutE = pThis->m_Templates.sTransferDownLine;
	wxString OutE2 = pThis->m_Templates.sTransferDownLineGood;

	double fTotalSize = 0, fTotalTransferred = 0, fTotalCompleted = 0, fTotalSpeed = 0;
	
	pThis->m_DownloadFilesInfo.ReQuery();

	//
	// Displaying
	//
	wxString sDownList;
	DownloadFilesInfo::ItemIterator i = pThis->m_DownloadFilesInfo.GetBeginIterator();
	while (i != pThis->m_DownloadFilesInfo.GetEndIterator()) {

		if ( sCat.Length() && (sCat != i->sFileStatus)) {
			i++;
			continue;
		}
		
		wxString JSfileinfo = i->sFileName + wxT("-") + i->sFileStatus;

		JSfileinfo.Replace(wxT("|"),wxT("\\n"));
		wxString sActions = 
			wxT("<acronym title=\"") +
			i->sFileStatus +
			wxT("\"><a ref=\"javascript:alert('") +
			JSfileinfo +
			wxT("')\"><img src=\"l_info.gif\" alt=\"") +
			i->sFileStatus +
			wxT("\"></a></acronym>");
		wxString sED2kLink =
			wxT("<acronym title=\"[Ed2klink]\"><a href=\"") +
			i->sED2kLink +
			wxT("\"><img src=\"l_ed2klink.gif\" alt=\"[Ed2klink]\"></a></acronym>");
		sED2kLink.Replace(wxT("[Ed2klink]"), _("ED2K Link(s)"));
		sActions += sED2kLink;

		wxString sNextAction;
		if (IsSessionAdmin(Data,sSession)) {
			if ( i->nFileStatus == PS_PAUSED) {
				sActions += wxT("<acronym title=\"Resume\"><a href=\"?ses=") +
					sSession + wxT("&w=transfer&op=resume&file=") + i->sFileHash +
					wxT("\"><img src=\"l_resume.gif\" alt=\"Resume\"></a></acronym> ");
			} else {
				sActions += wxT("<acronym title=\"Pause\"><a href=\"?ses=") +
					sSession + wxT("&w=transfer&op=pause&file=") + i->sFileHash +
					wxT("\"><img src=\"l_pause.gif\" alt=\"Pause\"></a></acronym> ");
			}
			sActions += wxT("<acronym title=\"Cancel\"><a href=\"?ses=") +
			sSession + wxT("&w=transfer&op=cancel&file=") + i->sFileHash +
			wxT("\" onclick=\"return confirm(\'Are you sure that you want to cancel and delete this file?\\n\')\">"
				"<img src=\"l_cancel.gif\" alt=\"Cancel\"></a></acronym> ");
				
			sActions += wxT("<acronym title=\"[PriorityUp]\"><a href=\"?ses=") +
				sSession + wxT("&amp;w=transfer&op=prioup&file=") +	i->sFileHash +
				wxT("\"><img src=\"l_up.gif\" alt=\"[PriorityUp]\"></a></acronym>");
			sActions += wxT("&nbsp;<acronym title=\"[PriorityDown]\"><a href=\"?ses=") +
				sSession + wxT("&amp;w=transfer&op=priodown&file=") + i->sFileHash +
				wxT("\"><img src=\"l_down.gif\" alt=\"[PriorityDown]\"></a></acronym>");
		} else {
			// lfroen: put something instead of action links ?
		}
		
		wxString HTTPProcessData;
		// if downloading, draw in other color
		if (i->lFileSpeed > 0)
			HTTPProcessData = OutE2;
		else
			HTTPProcessData = OutE;

		if (i->sFileName.Length() > SHORT_FILENAME_LENGTH)
			HTTPProcessData.Replace(wxT("[ShortFileName]"), i->sFileName.Left(SHORT_FILENAME_LENGTH) + wxT("..."));
		else
			HTTPProcessData.Replace(wxT("[ShortFileName]"), i->sFileName);

		HTTPProcessData.Replace(wxT("[FileInfo]"), i->sFileInfo);

		fTotalSize += i->lFileSize;

		HTTPProcessData.Replace(wxT("[2]"), CastItoXBytes(i->lFileSize));

		if (i->lFileTransferred > 0) {
			fTotalTransferred += i->lFileTransferred;
			HTTPProcessData.Replace(wxT("[3]"), CastItoXBytes(i->lFileCompleted));
		} else {
			HTTPProcessData.Replace(wxT("[3]"), wxT("-"));
		}

		if (i->lFileCompleted > 0) {
			fTotalCompleted += i->lFileCompleted;
			HTTPProcessData.Replace(wxT("[4]"), CastItoXBytes(i->lFileTransferred));
		} else {
			HTTPProcessData.Replace(wxT("[4]"), wxT("-"));
		}
		
		HTTPProcessData.Replace(wxT("[DownloadBar]"), _GetDownloadGraph(Data, (int)i->fCompleted, i->sPartStatus));

		if (i->lFileSpeed > 0.0f) {
			fTotalSpeed += i->lFileSpeed;

			HTTPProcessData.Replace(wxT("[5]"), wxString::Format(wxT("%8.2f %s"),
				i->lFileSpeed/1024.0 ,_("kB/s")));
		} else
			HTTPProcessData.Replace(wxT("[5]"), wxT("-"));
		
		if (i->lSourceCount > 0) {
			HTTPProcessData.Replace(wxT("[6]"), wxString::Format(wxT("%li&nbsp;/&nbsp;%8li&nbsp;(%li)"),
				i->lSourceCount - i->lNotCurrentSourceCount,
				i->lSourceCount, i->lTransferringSourceCount));
		} else
			HTTPProcessData.Replace(wxT("[6]"), wxT("-"));
		
		switch (i->lFilePrio) {
			case 0: HTTPTemp=_("Low");break;
			case 10: HTTPTemp=_("Auto [Lo]");break;

			case 1: HTTPTemp=_("Normal");break;
			case 11: HTTPTemp=_("Auto [No]");break;

			case 2: HTTPTemp=_("High");break;
			case 12: HTTPTemp=_("Auto [Hi]");break;
			//shakraw - it seems there is a problem with dl file priorities
			//i.e. I've got fileprio=3, VERYHIGH, but we can't set this priority 
			//in dl window. so, why fileprio=3?
			default: HTTPTemp=wxT("-"); break; 
		}
	
		HTTPProcessData.Replace(wxT("[PrioVal]"), HTTPTemp);
		HTTPProcessData.Replace(wxT("[7]"), sActions);

		sDownList += HTTPProcessData;
		i++;
	}

	Out.Replace(wxT("[DownloadFilesList]"), sDownList);
	Out.Replace(wxT("[PriorityUp]"), _("Increase Priority"));
	Out.Replace(wxT("[PriorityDown]"), _("Decrease Priority"));
	// Elandal: cast from float to integral type always drops fractions.
	// avoids implicit cast warning
	Out.Replace(wxT("[TotalDownSize]"), CastItoXBytes((uint64)fTotalSize));
	Out.Replace(wxT("[TotalDownCompleted]"), CastItoXBytes((uint64)fTotalCompleted));
	Out.Replace(wxT("[TotalDownTransferred]"), CastItoXBytes((uint64)fTotalTransferred));
	
	HTTPTemp.Printf(wxT("%8.2f %s"), fTotalSpeed/1024.0,_("kB/s"));
	Out.Replace(wxT("[TotalDownSpeed]"), HTTPTemp);
	OutE = pThis->m_Templates.sTransferUpLine;
	
	HTTPTemp.Printf(wxT("%i"),pThis->m_Templates.iProgressbarWidth);
	Out.Replace(wxT("[PROGRESSBARWIDTHVAL]"),HTTPTemp);

	fTotalSize = 0;
	fTotalTransferred = 0;
	fTotalSpeed = 0;

	wxString sUpList;

	//upload list
	wxString sTransferULList = pThis->webInterface->SendRecvMsg(wxT("TRANSFER UL_LIST"));
	wxString HTTPProcessData;
	unsigned long transfDown, transfUp;
	long transfDatarate; 
	while (sTransferULList.Length()>0) {
		int newLinePos = sTransferULList.First(wxT("\n"));

		wxString sEntry = sTransferULList.Left(newLinePos);
		sTransferULList = sTransferULList.Mid(newLinePos+1);

		HTTPProcessData = OutE;

		int brk=sEntry.First(wxT("\t"));
		HTTPProcessData.Replace(wxT("[1]"), _SpecialChars(sEntry.Left(brk)));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
		HTTPProcessData.Replace(wxT("[FileInfo]"), _SpecialChars(sEntry.Left(brk)));		
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
		HTTPProcessData.Replace(wxT("[2]"), _SpecialChars(sEntry.Left(brk)));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
		
		sEntry.Left(brk).ToULong(&transfDown);
		fTotalSize += transfDown;
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
		
		sEntry.Left(brk).ToULong(&transfUp);
		fTotalTransferred += transfUp;
		sEntry=sEntry.Mid(brk+1);

		HTTPProcessData.Replace(wxT("[3]"), CastItoXBytes((uint64)transfDown) + wxT(" / ") + CastItoXBytes((uint64)transfUp));

		sEntry.ToLong(&transfDatarate);
		fTotalSpeed += transfDatarate;
		
		HTTPProcessData.Replace(wxT("[4]"), wxString::Format(wxT("%8.2f kB/s"), transfDatarate/1024.0));
		
		sUpList += HTTPProcessData;
	}
	
	Out.Replace(wxT("[UploadFilesList]"), sUpList);
	// Elandal: cast from float to integral type always drops fractions.
	// avoids implicit cast warning
	Out.Replace(wxT("[TotalUpTransferred]"), CastItoXBytes((uint64)fTotalSize) + wxT(" / ") + CastItoXBytes((uint64)fTotalTransferred));
	Out.Replace(wxT("[TotalUpSpeed]"), wxString::Format(wxT("%8.2f kB/s"), fTotalSpeed/1024.0));

	if (pThis->m_Params.bShowUploadQueue) {
		Out.Replace(wxT("[UploadQueue]"), pThis->m_Templates.sTransferUpQueueShow);
		Out.Replace(wxT("[UploadQueueList]"), _("On Queue"));
		Out.Replace(wxT("[UserNameTitle]"), _("Username"));
		Out.Replace(wxT("[FileNameTitle]"), _("File Name"));
		Out.Replace(wxT("[ScoreTitle]"), _("Score"));
		Out.Replace(wxT("[BannedTitle]"), _("Banned"));

		OutE = pThis->m_Templates.sTransferUpQueueLine;
		// Replace [xx]
		wxString sQueue;

		//waiting list
		wxString sTransferWList = pThis->webInterface->SendRecvMsg(wxT("TRANSFER W_LIST"));
		while (sTransferWList.Length()>0) {
			int newLinePos=sTransferWList.First(wxT("\n"));

			wxString sEntry = sTransferWList.Left(newLinePos);
			sTransferWList = sTransferWList.Mid(newLinePos+1);

			char HTTPTempC[100] = "";
			HTTPProcessData = OutE;

			int brk = sTransferWList.First(wxT("\t"));
			if (brk==-1) {
				HTTPProcessData.Replace(wxT("[UserName]"), _SpecialChars(sEntry));
				continue;
			}
			HTTPProcessData.Replace(wxT("[UserName]"), _SpecialChars(sEntry.Left(brk)));
			sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
			HTTPProcessData.Replace(wxT("[FileName]"), _SpecialChars(sEntry.Left(brk)));
			sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
			sprintf(HTTPTempC, "%i" , (int)StrToLong(sEntry.Left(brk)));
			sEntry=sEntry.Mid(brk+1);
			
			wxString HTTPTemp = HTTPTemp.Format(wxT("%s"), HTTPTempC);
			HTTPProcessData.Replace(wxT("[Score]"), HTTPTemp);

			if (StrToLong(sEntry)) {
				HTTPProcessData.Replace(wxT("[Banned]"), _("Yes"));
			} else {
				HTTPProcessData.Replace(wxT("[Banned]"), _("No"));
			}
		}
		
		Out.Replace(wxT("[QueueList]"), sQueue);
	} else {
		Out.Replace(wxT("[UploadQueue]"), pThis->m_Templates.sTransferUpQueueHide);
	}

	Out.Replace(wxT("[ShowQueue]"), _("Show Queue"));
	Out.Replace(wxT("[HideQueue]"), _("Hide Queue"));
	Out.Replace(wxT("[Session]"), sSession);
	Out.Replace(wxT("[CLEARCOMPLETED]"), _("C&lear completed"));

	Out.Replace(wxT("[DownloadList]"),
		wxString::Format(wxT("Downloads (%u)"), pThis->m_DownloadFilesInfo.ItemCount()));
	Out.Replace(wxT("[UploadList]"),
		wxString::Format(_("Uploads (%i)"),
			StrToLong(pThis->webInterface->SendRecvMsg(wxT("QUEUE UL_GETLENGTH")))));
	Out.Replace(wxT("[CatSel]"), sCat);

	return Out;
}


wxString CWebServer::_GetDownloadLink(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return wxEmptyString;

	wxString sSession = _ParseURL(Data, wxT("ses"));

	if (!IsSessionAdmin(Data,sSession)) {
		wxString ad=ad.Format(wxT("<br><br><div align=\"center\" class=\"message\">[Message]</div>"));
		ad.Replace(wxT("[Message]"),_("Access denied!"));
		return ad;
	}
	
	wxString Out = pThis->m_Templates.sDownloadLink;

	Out.Replace(wxT("[Download]"), _("Download Selected"));
	Out.Replace(wxT("[Ed2klink]"), _("ED2K Link(s)"));
	Out.Replace(wxT("[Start]"), _("Start"));
	Out.Replace(wxT("[Session]"), sSession);

	// categories
	CECPacket req(EC_OP_GET_PREFERENCES, EC_DETAIL_WEB);
	req.AddTag(CECTag(EC_TAG_SELECT_PREFS, (uint32)EC_PREFS_CATEGORIES));
	CECPacket *reply = pThis->webInterface->SendRecvMsg_v2(&req);
	if ( reply->GetTagCount() ) { 	// if there are no categories, not even the EC_TAG_PREFS_CATEGORIES will be included :)
		InsertCatBox(Out, 0, pThis->m_Templates.sCatArrow, reply->GetTagByIndex(0));
	} else {
		Out.Replace(wxT("[CATBOX]"),wxEmptyString);
	}
	delete reply;

	return Out;
}


wxString CWebServer::_GetSharedFilesList(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return wxEmptyString;

	wxString sSession = _ParseURL(Data, wxT("ses"));
	
	//
	// commands: setpriority, reload
	if (!_ParseURL(Data, wxT("hash")).IsEmpty() && !_ParseURL(Data, wxT("setpriority")).IsEmpty() && IsSessionAdmin(Data,sSession)) 
		_SetSharedFilePriority(pThis, _ParseURL(Data, wxT("hash")), StrToLong(_ParseURL(Data, wxT("setpriority"))));

	if (_ParseURL(Data, wxT("reload")) == wxT("true")) {
		CECPacket req(EC_OP_SHAREDFILES_RELOAD);
		pThis->Send_Discard_V2_Request(&req);
	}

	wxString sSortRev = _ParseURL(Data, wxT("sortreverse"));
	wxString sSort = _ParseURL(Data, wxT("sort"));

	pThis->m_SharedFilesInfo.SetSortOrder(sSort, (sSortRev == wxT("true")));

	//Name sorting link
	wxString Out = pThis->m_Templates.sSharedList;
	pThis->m_SharedFilesInfo.ProcessHeadersLine(Out);

	Out.Replace(wxT("[Session]"), sSession);

	Out.Replace(wxT("[Message]"), wxEmptyString);

	wxString OutE = pThis->m_Templates.sSharedLine; 

	wxString OutE2 = pThis->m_Templates.sSharedLineChanged; 
	
	// Populating array
	pThis->m_SharedFilesInfo.ReQuery();	

	// Displaying
	wxString sSharedList = wxEmptyString;
	SharedFilesInfo::ItemIterator i = pThis->m_SharedFilesInfo.GetBeginIterator();
	while (i != pThis->m_SharedFilesInfo.GetEndIterator()) {
		wxString HTTPProcessData;
		if (i->sFileHash == _ParseURL(Data,wxT("hash")))
			HTTPProcessData = OutE2;
		else
			HTTPProcessData = OutE;

		HTTPProcessData.Replace(wxT("[FileName]"), _SpecialChars(i->sFileName));
		if (i->sFileName.Length() > SHORT_FILENAME_LENGTH)
			HTTPProcessData.Replace(wxT("[ShortFileName]"), _SpecialChars(i->sFileName.Left(SHORT_FILENAME_LENGTH)) + wxT("..."));
		else
			HTTPProcessData.Replace(wxT("[ShortFileName]"), _SpecialChars(i->sFileName));

		HTTPProcessData.Replace(wxT("[FileSize]"), CastItoXBytes(i->lFileSize));
		HTTPProcessData.Replace(wxT("[FileLink]"), i->sED2kLink);

		HTTPProcessData.Replace(wxT("[FileTransferred]"), CastItoXBytes(i->nFileTransferred));

		HTTPProcessData.Replace(wxT("[FileAllTimeTransferred]"), CastItoXBytes(i->nFileAllTimeTransferred));

		HTTPProcessData.Replace(wxT("[FileRequests]"), wxString::Format(wxT("%i"), i->nFileRequests));

		HTTPProcessData.Replace(wxT("[FileAllTimeRequests]"), wxString::Format(wxT("%i"), i->nFileAllTimeRequests));

		HTTPProcessData.Replace(wxT("[FileAccepts]"), wxString::Format(wxT("%i"), i->nFileAccepts));

		HTTPProcessData.Replace(wxT("[FileAllTimeAccepts]"), wxString::Format(wxT("%i"), i->nFileAllTimeAccepts));

		HTTPProcessData.Replace(wxT("[Priority]"), i->sFilePriority);

		HTTPProcessData.Replace(wxT("[FileHash]"), i->sFileHash);

		uint8 upperpriority=0, lesserpriority=0;
		if (i->bFileAutoPriority) {
			upperpriority = 5;	lesserpriority = 3;
		} else {
			switch (i->nFilePriority) {
				case 0: upperpriority = 1;	lesserpriority = 4; break;
				case 1: upperpriority = 2;	lesserpriority = 0; break;
				case 2: upperpriority = 3;	lesserpriority = 1; break;
				case 3: upperpriority = 5;	lesserpriority = 2; break;
				case 4: upperpriority = 0;	lesserpriority = 4; break;
				case 5: upperpriority = 5;	lesserpriority = 3; break;
			}
		}
		
		HTTPProcessData.Replace(wxT("[PriorityUpLink]"),
			wxT("hash=") +  i->sFileHash + wxString::Format(wxT("&setpriority=%i"), upperpriority));

		HTTPProcessData.Replace(wxT("[PriorityDownLink]"),
			wxT("hash=") +  i->sFileHash + wxString::Format(wxT("&setpriority=%i"), lesserpriority));

		sSharedList += HTTPProcessData;
		i++;
	}
	Out.Replace(wxT("[SharedFilesList]"), sSharedList);
	Out.Replace(wxT("[Session]"), sSession);

	return Out;
}


wxString CWebServer::_GetGraphs(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (!pThis)
		return wxEmptyString;
	
	wxString Out = pThis->m_Templates.sGraphs;	
	wxString sGraphDownload, sGraphUpload, sGraphCons;
	wxString sTmp;	
	for (size_t i = 0; i < WEB_GRAPH_WIDTH; ++i) {
		if (i < pThis->m_Params.PointsForWeb.GetCount()) {
			if (i != 0) {
				sGraphDownload.Append(wxT(","));
				sGraphUpload.Append(wxT(","));
				sGraphCons.Append(wxT(","));
			}
			// download
			sTmp.Format(wxT("%d") , (uint32) (pThis->m_Params.PointsForWeb[i]->download*1024));
			sGraphDownload += sTmp;
			// upload
			sTmp.Format(wxT("%d") , (uint32) (pThis->m_Params.PointsForWeb[i]->upload*1024));
			sGraphUpload += sTmp;
			// connections
			sTmp.Format(wxT("%d") , (uint32) (pThis->m_Params.PointsForWeb[i]->connections));
			sGraphCons += sTmp;
		}
	}	
	Out.Replace(wxT("[GraphDownload]"), sGraphDownload);
	Out.Replace(wxT("[GraphUpload]"), sGraphUpload);
	Out.Replace(wxT("[GraphConnections]"), sGraphCons);
	
	Out.Replace(wxT("[TxtDownload]"), _("Download"));
	Out.Replace(wxT("[TxtUpload]"), _("Upload"));
	Out.Replace(wxT("[TxtTime]"), _("Time"));
	Out.Replace(wxT("[TxtConnections]"), _("Active connections"));
	Out.Replace(wxT("[KByteSec]"), _("kB/s"));
	Out.Replace(wxT("[TxtTime]"), _("Time"));

	//sGraphs formatted as: %d\t%d\t%d\t%d
	wxString sGraphs = pThis->webInterface->SendRecvMsg(wxT("WEBPAGE GETGRAPH"));
	int brk = sGraphs.First(wxT("\t"));
	
	wxString sScale;
	sScale = CastSecondsToHM(StrToLong(sGraphs.Left(brk)) * WEB_GRAPH_WIDTH);
	sGraphs = sGraphs.Mid(brk+1); 
	brk=sGraphs.First(wxT("\t"));

	wxString s1, s2, s3;
	s1.Printf(wxT("%li"), StrToLong(sGraphs.Left(brk)) + 4);
	sGraphs = sGraphs.Mid(brk+1); 
	brk=sGraphs.First(wxT("\t"));
	s2.Printf(wxT("%li"), StrToLong(sGraphs.Left(brk)) + 4);
	sGraphs = sGraphs.Mid(brk+1);
	s3.Printf(wxT("%li"), StrToLong(sGraphs) + 20);
	
	Out.Replace(wxT("[ScaleTime]"), sScale);
	Out.Replace(wxT("[MaxDownload]"), s1);
	Out.Replace(wxT("[MaxUpload]"), s2);
	Out.Replace(wxT("[MaxConnections]"), s3);

	return Out;
}


wxString CWebServer::_GetAddServerBox(ThreadData Data) {	
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (!pThis)
		return wxEmptyString;

	wxString sSession = _ParseURL(Data, wxT("ses"));

	if (!IsSessionAdmin(Data,sSession)) return wxEmptyString;

	wxString Out = pThis->m_Templates.sAddServerBox;
	if (_ParseURL(Data, wxT("addserver")) == wxT("true")) {
		wxString sIP = _ParseURL(Data, wxT("serveraddr"));
		wxString sPort = _ParseURL(Data, wxT("serverport"));
		wxString sName = _ParseURL(Data, wxT("servername"));
		
		wxString ed2k = wxT("ed2k://|server|") + sIP + wxT("|") + sPort + wxT("|/");
		CECPacket request(EC_OP_ED2K_LINK);
		request.AddTag(CECTag(EC_TAG_STRING, ed2k)),
		pThis->Send_Discard_V2_Request(&request);

		CECPacket req(EC_OP_GET_LAST_LOG_ENTRY);
		CECPacket *response = pThis->webInterface->SendRecvMsg_v2(&req);
		Out.Replace(wxT("[Message]"),_SpecialChars(response->GetTagByIndex(0)->GetStringData()));
		delete response;
	} else if (_ParseURL(Data, wxT("updateservermetfromurl")) == wxT("true")) {
		CECPacket request(EC_OP_SERVER_UPDATE_FROM_URL);
		request.AddTag(CECTag(EC_TAG_STRING, _ParseURL(Data, wxT("servermeturl"))));
		pThis->Send_Discard_V2_Request(&request);
		
		CECPacket req(EC_OP_GET_LAST_LOG_ENTRY);
		CECPacket *response = pThis->webInterface->SendRecvMsg_v2(&req);
		Out.Replace(wxT("[Message]"),_SpecialChars(response->GetTagByIndex(0)->GetStringData()));
		delete response;
	} else
		Out.Replace(wxT("[Message]"), wxEmptyString);
	
	Out.Replace(wxT("[AddServer]"), _("Received %d new servers"));
	Out.Replace(wxT("[IP]"), _("IP or Address"));
	Out.Replace(wxT("[Port]"), _("Port"));
	Out.Replace(wxT("[Name]"), _("Name"));
	Out.Replace(wxT("[Add]"), _("Add to list"));
	Out.Replace(wxT("[UpdateServerMetFromURL]"), _("Update server.met from URL"));
	Out.Replace(wxT("[URL]"), _("URL"));
	Out.Replace(wxT("[Apply]"), _("Apply"));

	return Out;
}


wxString CWebServer::_GetWebSearch(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (!pThis)
		return wxEmptyString;

	wxString sSession = _ParseURL(Data, wxT("ses"));
    
	wxString Out = pThis->m_Templates.sWebSearch;
	if (_ParseURL(Data, wxT("tosearch")) != wxEmptyString) {

		Out += wxT("\n<script language=\"javascript\">\n searchwindow=window.open('http://www.filedonkey.com/fdsearch/index.php?media=") +
					_ParseURL(Data, wxT("media")) +
					wxT("&pattern=") + 
					 _ParseURL(Data, wxT("tosearch")) + 
					wxT("&action=search&name=FD-Search&op=modload&file=index&requestby=amule','searchwindow');\n</script>");

	}
	
	Out.Replace(wxT("[Session]"), sSession);
	Out.Replace(wxT("[Name]"), _("Name"));
	Out.Replace(wxT("[Type]"), _("Type"));
	Out.Replace(wxT("[Any]"), _("Any"));
	Out.Replace(wxT("[Archives]"), _("Archive"));
	Out.Replace(wxT("[Audio]"), _("Audio"));
	Out.Replace(wxT("[CD-Images]"), _("CD-Images"));
	Out.Replace(wxT("[Pictures]"), _("Pictures"));
	Out.Replace(wxT("[Programs]"), _("Programs"));
	Out.Replace(wxT("[Texts]"), _("Texts"));
	Out.Replace(wxT("[Videos]"), _("Video"));
	Out.Replace(wxT("[Search]"), _("Start"));
	Out.Replace(wxT("[WebSearch]"), _("Web-based Search"));
	
	return Out;
}


wxString CWebServer::_GetLog(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (!pThis)
		return wxEmptyString;

	wxString sSession = _ParseURL(Data, wxT("ses"));

	wxString Out = pThis->m_Templates.sLog;

	if (_ParseURL(Data, wxT("clear")) == wxT("yes") && IsSessionAdmin(Data,sSession)) {
		CECPacket req(EC_OP_RESET_LOG);
		pThis->Send_Discard_V2_Request(&req);
	}
	
	Out.Replace(wxT("[Clear]"), _("Reset"));
	CECPacket req(EC_OP_GET_LOG);
	CECPacket *response = pThis->webInterface->SendRecvMsg_v2(&req);
	Out.Replace(wxT("[Log]"), _SpecialChars(response->GetTagByIndex(0)->GetStringData()) + wxT("<br><a name=\"end\"></a>"));
	delete response;
	Out.Replace(wxT("[Session]"), sSession);

	return Out;
}


wxString CWebServer::_GetServerInfo(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (!pThis)
		return wxEmptyString;

	wxString sSession = _ParseURL(Data, wxT("ses"));

	wxString Out = pThis->m_Templates.sServerInfo;

	if (_ParseURL(Data, wxT("clear")) == wxT("yes")) {
		CECPacket req(EC_OP_CLEAR_SERVERINFO);
		pThis->Send_Discard_V2_Request(&req);
	}
	
	CECPacket req(EC_OP_GET_SERVERINFO);
	CECPacket *response = pThis->webInterface->SendRecvMsg_v2(&req);
	Out.Replace(wxT("[Clear]"), _("Reset"));
	Out.Replace(wxT("[ServerInfo]"), _SpecialChars(response->GetTagByIndex(0)->GetStringData()));
	Out.Replace(wxT("[Session]"), sSession);
	delete response;

	return Out;
}


//shakraw, this is useless in amule 'cause debuglog and log windows are the same.
//so, at the moment, GETALLDEBUGLOGENTRIES has the same behaviour of GETALLLOGENTRIES.
//Note that, when clearing, the log file ~/.aMule/logfile will not be removed here.
wxString CWebServer::_GetDebugLog(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (!pThis)
		return wxEmptyString;

	wxString sSession = _ParseURL(Data, wxT("ses"));

	wxString Out = pThis->m_Templates.sDebugLog;

	if (_ParseURL(Data, wxT("clear")) == wxT("yes") && IsSessionAdmin(Data,sSession)) {
		CECPacket req(EC_OP_RESET_DEBUGLOG);
		pThis->Send_Discard_V2_Request(&req);
	}
	Out.Replace(wxT("[Clear]"), _("Reset"));

	CECPacket req(EC_OP_GET_DEBUGLOG);
	CECPacket *response = pThis->webInterface->SendRecvMsg_v2(&req);
	Out.Replace(wxT("[DebugLog]"), _SpecialChars(response->GetTagByIndex(0)->GetStringData()) + wxT("<br><a name=\"end\"></a>"));
	delete response;
	Out.Replace(wxT("[Session]"), sSession);

	return Out;
}


wxString CWebServer::_GetStats(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (!pThis)
		return wxEmptyString;

	pThis->webInterface->DebugShow(wxT("***_GetStats arrived\n"));

	wxString sSession = _ParseURL(Data, wxT("ses"));

	// refresh statistics.. ARGH. NO NO NO NO
	// (it will be done in statisticsdlg and in main thread)
	//theApp.amuledlg->statisticswnd.ShowStatistics(true);
	
	wxString Out = pThis->m_Templates.sStats;
	
	wxString sStats = pThis->webInterface->SendRecvMsg(wxT("WEBPAGE STATISTICS"));
	int brk = sStats.First(wxT("\t"));
	
	Out.Replace(wxT("[STATSDATA]"), sStats.Left(brk));
	Out.Replace(wxT("[Stats]"), sStats.Mid(brk+1));

	return Out;
}


wxString CWebServer::_GetPreferences(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (!pThis)
		return wxEmptyString;

	wxString sSession = _ParseURL(Data, wxT("ses"));

	wxString Out = pThis->m_Templates.sPreferences;
	Out.Replace(wxT("[Session]"), sSession);

	if ((_ParseURL(Data, wxT("saveprefs")) == wxT("true")) && IsSessionAdmin(Data,sSession) ) {
		CECPacket prefs(EC_OP_SET_PREFERENCES);
		CECEmptyTag filePrefs(EC_TAG_PREFS_FILES);
		CECEmptyTag connPrefs(EC_TAG_PREFS_CONNECTIONS);
		CECEmptyTag webPrefs(EC_TAG_PREFS_REMOTECTRL);
		if (_ParseURL(Data, wxT("gzip")) == wxT("true") || _ParseURL(Data, wxT("gzip")) == wxT("on")) {
			webPrefs.AddTag(CECTag(EC_TAG_WEBSERVER_USEGZIP, (uint8)1));
			pThis->webInterface->m_UseGzip = true;
		}
		if (_ParseURL(Data, wxT("gzip")) == wxT("false") || _ParseURL(Data, wxT("gzip")) == wxEmptyString) {
			webPrefs.AddTag(CECTag(EC_TAG_WEBSERVER_USEGZIP, (uint8)0));
			pThis->webInterface->m_UseGzip = false;
		}
		if (_ParseURL(Data, wxT("showuploadqueue")) == wxT("true") || _ParseURL(Data, wxT("showuploadqueue")) == wxT("on") ) {
			pThis->m_Params.bShowUploadQueue = true;
		}
		if(_ParseURL(Data, wxT("showuploadqueue")) == wxT("false") || _ParseURL(Data, wxT("showuploadqueue")) == wxEmptyString) {
			pThis->m_Params.bShowUploadQueue = false;
		}
		if (_ParseURL(Data, wxT("refresh")) != wxEmptyString) {
			webPrefs.AddTag(CECTag(EC_TAG_WEBSERVER_REFRESH, (uint32)StrToLong(_ParseURL(Data, wxT("refresh")))));
		}
		if (_ParseURL(Data, wxT("maxdown")) != wxEmptyString) {
			connPrefs.AddTag(CECTag(EC_TAG_CONN_MAX_DL, (uint16)StrToLong(_ParseURL(Data, wxT("maxdown")))));
		}
		if (_ParseURL(Data, wxT("maxup")) != wxEmptyString) {
			connPrefs.AddTag(CECTag(EC_TAG_CONN_MAX_UL, (uint16)StrToLong(_ParseURL(Data, wxT("maxup")))));
		}
		if (_ParseURL(Data, wxT("maxcapdown")) != wxEmptyString) {
			connPrefs.AddTag(CECTag(EC_TAG_CONN_DL_CAP, (uint32)StrToLong(_ParseURL(Data, wxT("maxcapdown")))));
		}
		if (_ParseURL(Data, wxT("maxcapup")) != wxEmptyString) {
			connPrefs.AddTag(CECTag(EC_TAG_CONN_UL_CAP, (uint32)StrToLong(_ParseURL(Data, wxT("maxcapup")))));
		}
		if (_ParseURL(Data, wxT("maxsources")) != wxEmptyString) {
			connPrefs.AddTag(CECTag(EC_TAG_CONN_MAX_FILE_SOURCES, (uint16)StrToLong(_ParseURL(Data, wxT("maxsources")))));
		}
		if (_ParseURL(Data, wxT("maxconnections")) != wxEmptyString) {
			connPrefs.AddTag(CECTag(EC_TAG_CONN_MAX_CONN, (uint16)StrToLong(_ParseURL(Data, wxT("maxconnections")))));
		}
		if (_ParseURL(Data, wxT("maxconnectionsperfive")) != wxEmptyString) {
			CECEmptyTag twPrefs(EC_TAG_PREFS_CORETWEAKS);
			twPrefs.AddTag(CECTag(EC_TAG_CORETW_MAX_CONN_PER_FIVE, (uint16)StrToLong(_ParseURL(Data, wxT("maxconnectionsperfive")))));
			prefs.AddTag(twPrefs);
		}

		filePrefs.AddTag(CECTag(EC_TAG_FILES_UL_FULL_CHUNKS, (uint8)((_ParseURL(Data, wxT("fullchunks")).MakeLower() == wxT("on")) ? 1 : 0)));
		filePrefs.AddTag(CECTag(EC_TAG_FILES_PREVIEW_PRIO, (uint8)((_ParseURL(Data, wxT("firstandlast")).MakeLower() == wxT("on")) ? 1 : 0)));

		prefs.AddTag(filePrefs);
		prefs.AddTag(connPrefs);
		prefs.AddTag(webPrefs);
		
		pThis->Send_Discard_V2_Request(&prefs);
	}

	CECPacket req(EC_OP_GET_PREFERENCES);
	req.AddTag(CECTag(EC_TAG_SELECT_PREFS, (uint32)(EC_PREFS_CONNECTIONS | EC_PREFS_REMOTECONTROLS | EC_PREFS_FILES | EC_PREFS_CORETWEAKS)));
	CECPacket *response = pThis->webInterface->SendRecvMsg_v2(&req);
	CECTag *filePrefs = response->GetTagByName(EC_TAG_PREFS_FILES);
	CECTag *connPrefs = response->GetTagByName(EC_TAG_PREFS_CONNECTIONS);
	CECTag *webPrefs = response->GetTagByName(EC_TAG_PREFS_REMOTECTRL);
	if (webPrefs->GetTagByName(EC_TAG_WEBSERVER_USEGZIP)) {
		Out.Replace(wxT("[UseGzipVal]"), wxT("checked"));
	} else {
		Out.Replace(wxT("[UseGzipVal]"), wxEmptyString);
	}
	if(pThis->m_Params.bShowUploadQueue) {
		Out.Replace(wxT("[ShowUploadQueueVal]"), wxT("checked"));
	} else {
		Out.Replace(wxT("[ShowUploadQueueVal]"), wxEmptyString);
	}
	if (filePrefs->GetTagByName(EC_TAG_FILES_PREVIEW_PRIO)) {
		Out.Replace(wxT("[FirstAndLastVal]"), wxT("checked"));
	} else {
		Out.Replace(wxT("[FirstAndLastVal]"), wxEmptyString);
	}
	if (filePrefs->GetTagByName(EC_TAG_FILES_UL_FULL_CHUNKS)) {
		Out.Replace(wxT("[FullChunksVal]"), wxT("checked"));
	} else {
		Out.Replace(wxT("[FullChunksVal]"), wxEmptyString);
	}

	wxString sRefresh = sRefresh.Format(wxT("%i"), webPrefs->GetTagByName(EC_TAG_WEBSERVER_REFRESH)->GetInt32Data());
	Out.Replace(wxT("[RefreshVal]"), sRefresh);
	sRefresh.Printf(wxT("%i"), connPrefs->GetTagByName(EC_TAG_CONN_MAX_FILE_SOURCES)->GetInt16Data());
	Out.Replace(wxT("[MaxSourcesVal]"), sRefresh);
	sRefresh.Printf(wxT("%i"), connPrefs->GetTagByName(EC_TAG_CONN_MAX_CONN)->GetInt16Data());
	Out.Replace(wxT("[MaxConnectionsVal]"), sRefresh);
	sRefresh.Printf(wxT("%i"), response->GetTagByName(EC_TAG_PREFS_CORETWEAKS)->GetTagByName(EC_TAG_CORETW_MAX_CONN_PER_FIVE)->GetInt16Data());
	Out.Replace(wxT("[MaxConnectionsPer5Val]"), sRefresh);

	wxString colon(wxT(":"));
	Out.Replace(wxT("[KBS]"), _("kB/s")+colon);
	Out.Replace(wxT("[FileSettings]"), _("File Settings")+colon);
	Out.Replace(wxT("[LimitForm]"), _("Connection Limits")+colon);
	Out.Replace(wxT("[MaxSources]"), _("Max Sources Per File")+colon);
	Out.Replace(wxT("[MaxConnections]"), _("Max. Connections")+colon);
	Out.Replace(wxT("[MaxConnectionsPer5]"), _("max. new connections / 5secs")+colon);
	Out.Replace(wxT("[UseGzipForm]"), _("Gzip Compression"));
	Out.Replace(wxT("[UseGzipComment]"), _("Save traffic, especially in graphs."));
	Out.Replace(wxT("[ShowUploadQueueForm]"), _("Show Queue"));
	Out.Replace(wxT("[ShowUploadQueueComment]"), _("Enable or disable the display of waiting queue in transfer page."));
	Out.Replace(wxT("[ShowQueue]"), _("Show Queue"));
	Out.Replace(wxT("[HideQueue]"), _("Hide Queue"));
	Out.Replace(wxT("[RefreshTimeForm]"), _("Refresh-Time of Pages"));
	Out.Replace(wxT("[RefreshTimeComment]"), _("Time in seconds (zero=disabled)")+colon);
	Out.Replace(wxT("[SpeedForm]"), _("Speed Limits"));
	Out.Replace(wxT("[MaxDown]"), _("Download"));
	Out.Replace(wxT("[MaxUp]"), _("Upload"));
	Out.Replace(wxT("[SpeedCapForm]"), _("Bandwidth Limits"));
	Out.Replace(wxT("[MaxCapDown]"), _("Download"));
	Out.Replace(wxT("[MaxCapUp]"), _("Upload"));
	Out.Replace(wxT("[TryFullChunks]"), _("Try to transfer full chunks to all uploads"));
	Out.Replace(wxT("[FirstAndLast]"), _("Try to download first and last chunks first"));
	Out.Replace(wxT("[WebControl]"), _("Web Control Panel"));
	Out.Replace(wxT("[aMuleAppName]"), wxT("aMule"));
	Out.Replace(wxT("[Apply]"), _("Apply"));

	int n = connPrefs->GetTagByName(EC_TAG_CONN_MAX_DL)->GetInt16Data();
	if (n < 0 || n == 65535) {
		n = 0;
	}
	Out.Replace(wxT("[MaxDownVal]"), wxString::Format(wxT("%d"), n));
	
	n = connPrefs->GetTagByName(EC_TAG_CONN_MAX_UL)->GetInt16Data();
	if (n < 0 || n == 65535)  {
		n = 0;
	}
	Out.Replace(wxT("[MaxUpVal]"), wxString::Format(wxT("%d"), n));
	
	Out.Replace(wxT("[MaxCapDownVal]"), wxString::Format(wxT("%i"), connPrefs->GetTagByName(EC_TAG_CONN_DL_CAP)->GetInt32Data()));
	
	Out.Replace(wxT("[MaxCapUpVal]"), wxString::Format(wxT("%i"), connPrefs->GetTagByName(EC_TAG_CONN_UL_CAP)->GetInt32Data()));

	delete response;
	return Out;
}


wxString CWebServer::_GetLoginScreen(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (!pThis)
		return wxEmptyString;

	wxString sSession = _ParseURL(Data, wxT("ses"));

	wxString Out = wxEmptyString;

	Out += pThis->m_Templates.sLogin;

	Out.Replace(wxT("[CharSet]"), _GetWebCharSet());
	Out.Replace(wxT("[aMulePlus]"), wxT("aMule"));
	Out.Replace(wxT("[aMuleAppName]"), wxT("aMule"));
	Out.Replace(wxT("[version]"), wxT(VERSION)); //shakraw - was CURRENT_VERSION_LONG);
	Out.Replace(wxT("[Login]"), _("Login"));
	Out.Replace(wxT("[EnterPassword]"), _("Enter your password here"));
	Out.Replace(wxT("[LoginNow]"), _("Login Now"));
	Out.Replace(wxT("[WebControl]"), _("Web Control Panel"));

	return Out;
}


wxString CWebServer::_GetConnectedServer(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (!pThis)
		return wxEmptyString;

	wxString sSession = _ParseURL(Data, wxT("ses"));

	wxString OutS = pThis->m_Templates.sConnectedServer;
	OutS.Replace(wxT("[ConnectedServer]"), _("Server"));
	OutS.Replace(wxT("[Servername]"), _("Server name"));
	OutS.Replace(wxT("[Status]"), _("Status"));
	OutS.Replace(wxT("[Usercount]"), _("users"));
	OutS.Replace(wxT("[Action]"), _("Connecting"));
	OutS.Replace(wxT("[URL_Disconnect]"), IsSessionAdmin(Data,sSession) ? wxString::Format(wxT("?ses=%s&w=server&c=disconnect"), sSession.GetData()) : GetPermissionDenied());
	OutS.Replace(wxT("[URL_Connect]"), IsSessionAdmin(Data,sSession) ? wxString::Format(wxT("?ses=%s&w=server&c=connect"), sSession.GetData()) : GetPermissionDenied());
	OutS.Replace(wxT("[Disconnect]"), _("Disconnect"));
	OutS.Replace(wxT("[Connect]"), _("Connect to any server"));
	OutS.Replace(wxT("[URL_ServerOptions]"), IsSessionAdmin(Data,sSession) ? wxString::Format(wxT("?ses=%s&w=server&c=options"), sSession.GetData()) : GetPermissionDenied());
	OutS.Replace(wxT("[ServerOptions]"), _("Server Preferences"));
	OutS.Replace(wxT("[WebSearch]"), _("Web-based Search"));

	CECPacket connstate_req(EC_OP_GET_CONNSTATE, EC_DETAIL_WEB);
	CECPacket *sServerStat = pThis->webInterface->SendRecvMsg_v2(&connstate_req);
	if ( !sServerStat ) {
		return wxEmptyString;
	}
	uint8 connstate = sServerStat->GetTagByIndex(0)->GetInt8Data();
	switch (connstate) {
		case 0:
			OutS.Replace(wxT("[1]"), _("Disconnected"));
			OutS.Replace(wxT("[2]"), wxEmptyString);
			OutS.Replace(wxT("[3]"), wxEmptyString);
			break;
		case 1:
			OutS.Replace(wxT("[1]"), _("Connecting"));
			OutS.Replace(wxT("[2]"), wxEmptyString);
			OutS.Replace(wxT("[3]"), wxEmptyString);
			break;
		case 2:
		case 3:
			OutS.Replace(wxT("[1]"), wxString(_("Connected ")) + ((connstate == 2) ? wxString(_("with LowID")) : wxString(_("with HighID"))));
			CECTag *server = sServerStat->GetTagByIndex(0)->GetTagByIndex(0);
			OutS.Replace(wxT("[2]"), server->GetTagByName(EC_TAG_SERVER_NAME)->GetStringData());
			OutS.Replace(wxT("[3]"), wxString::Format(wxT("%10i"),server->GetTagByName(EC_TAG_SERVER_USERS)->GetInt32Data()));
	}
	delete sServerStat;
	return OutS;
}


// We have to add gz-header and some other stuff
// to standard zlib functions in order to use gzip in web pages
int CWebServer::_GzipCompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level) { 
	static const int gz_magic[2] = {0x1f, 0x8b}; // gzip magic header
	int err;
	uLong crc;
	z_stream stream = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;
	crc = crc32(0L, Z_NULL, 0);
	// init Zlib stream
	// NOTE windowBits is passed < 0 to suppress zlib header
	err = deflateInit2(&stream, level, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	if (err != Z_OK)
		return err;

	sprintf((char*)dest , "%c%c%c%c%c%c%c%c%c%c", gz_magic[0], gz_magic[1],
		Z_DEFLATED, 0 /*flags*/, 0,0,0,0 /*time*/, 0 /*xflags*/, 255);
	// wire buffers
	stream.next_in = (Bytef*) source ;
	stream.avail_in = (uInt)sourceLen;
	stream.next_out = ((Bytef*) dest) + 10;
	stream.avail_out = *destLen - 18;
	// doit
	err = deflate(&stream, Z_FINISH);
	if (err != Z_STREAM_END) {
		deflateEnd(&stream);
		return err;
	}
	err = deflateEnd(&stream);
	crc = crc32(crc, (const Bytef *) source ,  sourceLen );
	//CRC
	*(((Bytef*) dest)+10+stream.total_out) = (Bytef)(crc & 0xFF);
	*(((Bytef*) dest)+10+stream.total_out+1) = (Bytef)((crc>>8) & 0xFF);
	*(((Bytef*) dest)+10+stream.total_out+2) = (Bytef)((crc>>16) & 0xFF);
	*(((Bytef*) dest)+10+stream.total_out+3) = (Bytef)((crc>>24) & 0xFF);
	// Length
	*(((Bytef*) dest)+10+stream.total_out+4) = (Bytef)( sourceLen  & 0xFF);
	*(((Bytef*) dest)+10+stream.total_out+5) = (Bytef)(( sourceLen >>8) & 0xFF);
	*(((Bytef*) dest)+10+stream.total_out+6) = (Bytef)(( sourceLen >>16) &	0xFF);
	*(((Bytef*) dest)+10+stream.total_out+7) = (Bytef)(( sourceLen >>24) &	0xFF);
	// return  destLength
	*destLen = 10 + stream.total_out + 8;
	return err;
}


bool CWebServer::_IsLoggedIn(ThreadData Data, long lSession) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	_RemoveTimeOuts(Data,lSession);

	// find our session
	for (size_t i = 0; i < pThis->m_Params.Sessions.GetCount(); ++i) {
		if (pThis->m_Params.Sessions[i]->lSession == lSession && lSession != 0) {
			// if found, also reset expiration time
			pThis->m_Params.Sessions[i]->startTime = time(NULL);
			return true;
		}
	}

	return false;
}


void CWebServer::_RemoveTimeOuts(ThreadData Data, long WXUNUSED(lSession)) {
	// remove expired sessions
	CWebServer *pThis = (CWebServer *)Data.pThis;
	pThis->UpdateSessionCount();
}


bool CWebServer::_RemoveSession(ThreadData Data, long lSession) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	// find our session
	for (size_t i = 0; i < pThis->m_Params.Sessions.GetCount(); ++i) {
		if (pThis->m_Params.Sessions[i]->lSession == lSession && lSession != 0) {
			pThis->m_Params.Sessions.RemoveAt(i);
			CECPacket req(EC_OP_ADDLOGLINE);
			req.AddTag(CECTag(EC_TAG_STRING, wxString(_("Webserver: Logout"))));
			pThis->Send_Discard_V2_Request(&req);
			return true;
		}
	}
	return false;
}


Session CWebServer::GetSessionByID(ThreadData Data,long sessionID) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	
	if (pThis != NULL) {
		for (size_t i = 0; i < pThis->m_Params.Sessions.GetCount(); ++i) {
			if (pThis->m_Params.Sessions[i]->lSession == sessionID && sessionID != 0)
				return *(pThis->m_Params.Sessions[i]);
		}
	}

	Session ses;
	ses.admin=false;
	ses.startTime = 0;

	return ses;
}


bool CWebServer::IsSessionAdmin(ThreadData Data,wxString SsessionID) {
	long sessionID= StrToLong(SsessionID);
	CWebServer *pThis = (CWebServer *)Data.pThis;
	
	if (pThis != NULL) {
		for (size_t i = 0; i < pThis->m_Params.Sessions.GetCount(); ++i) {
			if (pThis->m_Params.Sessions[i]->lSession == sessionID && sessionID != 0)
				return pThis->m_Params.Sessions[i]->admin;
		}
	}
	return false;
}


wxString CWebServer::GetPermissionDenied() {
	return wxString::Format(wxT("javascript:alert(\'%s\')"), _("Access denied!"));
}


bool CWebServer::_GetFileHash(wxString sHash, uchar *FileHash) {
	char hex_byte[3];
	int byte;
	hex_byte[2] = '\0';
	for (int i = 0; i < 16; ++i) {
		hex_byte[0] = sHash.GetChar(i*2);
		hex_byte[1] = sHash.GetChar((i*2 + 1));
		sscanf(hex_byte, "%02x", &byte);
		FileHash[i] = (uchar)byte;
	}
	return true;
}


// EC + kuchin
wxString CWebServer::_GetWebCharSet() {
#if 0
	switch (theApp.glob_prefs->GetLanguageID()) {
		case MAKELANGID(LANG_POLISH,SUBLANG_DEFAULT):			return "windows-1250";
		case MAKELANGID(LANG_RUSSIAN,SUBLANG_DEFAULT):			return "windows-1251";
		case MAKELANGID(LANG_GREEK,SUBLANG_DEFAULT):			return "ISO-8859-7";
		case MAKELANGID(LANG_HEBREW,SUBLANG_DEFAULT):			return "ISO-8859-8";
		case MAKELANGID(LANG_KOREAN,SUBLANG_DEFAULT):			return "EUC-KR";
		case MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SIMPLIFIED):	return "GB2312";
		case MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_TRADITIONAL):	return "Big5";
		case MAKELANGID(LANG_LITHUANIAN,SUBLANG_DEFAULT):		return "windows-1257";
		case MAKELANGID(LANG_TURKISH,SUBLANG_DEFAULT):			return "windows-1254";
	}
#endif
#if wxUSE_UNICODE
	return wxT("UTF-8");
#else
	// Western (Latin) includes Catalan, Danish, Dutch, English, Faeroese, Finnish, French,
	// German, Galician, Irish, Icelandic, Italian, Norwegian, Portuguese, Spanish and Swedish
	return wxT("ISO-8859-1");
#endif
}


// Ornis: creating the progressbar. colored if ressources are given/available
wxString CWebServer::_GetDownloadGraph(ThreadData Data, int percent, wxString &s_ChunkBar) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (!pThis)
		return wxEmptyString;
	
	// cool style
	wxString progresscolor[12] = {
		wxT("transparent.gif"), wxT("black.gif"), wxT("yellow.gif"), wxT("red.gif"),
		wxT("blue1.gif"),       wxT("blue2.gif"), wxT("blue3.gif"),  wxT("blue4.gif"),
		wxT("blue5.gif"),       wxT("blue6.gif"), wxT("green.gif"),  wxT("greenpercent.gif") };

	wxString Out = wxEmptyString;

	// and now make a graph out of the array - need to be in a progressive way
	uint8 lastcolor = StrToLong(s_ChunkBar.Left(1));
	uint16 lastindex=0;
	
	for (uint16 i = 0; i < s_ChunkBar.Length(); i++) {
		if ( (lastcolor!= StrToLong(s_ChunkBar.Mid(i,1))) || (i == s_ChunkBar.Length()-1) ) {
				Out += wxString::Format(pThis->m_Templates.sProgressbarImgs,
					unicode2char(progresscolor[lastcolor]), i - lastindex);

			lastcolor = StrToLong(s_ChunkBar.Mid(i,1));
			lastindex = i;
		}
	}

	int complx = pThis->m_Templates.iProgressbarWidth*percent/100;
	if ( complx ) {
		Out = wxString::Format((pThis->m_Templates.sProgressbarImgsPercent+wxT("<br>")),
			unicode2char(progresscolor[11]),complx) + Out;
	} else {
		Out = wxString::Format((pThis->m_Templates.sProgressbarImgsPercent+wxT("<br>")),
			unicode2char(progresscolor[0]),5) + Out;
	}
	return Out;
}


wxString CWebServer::_GetSearch(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (!pThis)
		return wxEmptyString;

	wxString sSession = _ParseURL(Data, wxT("ses"));
	wxString Out = pThis->m_Templates.sSearch;

	wxString downloads=_ParseURLArray(Data,wxT("downloads"));
	if (!downloads.IsEmpty() && IsSessionAdmin(Data,sSession) ) {
		int brk;
		while (downloads.Length()>0) {
			brk=downloads.First(wxT("|"));
			pThis->webInterface->SendRecvMsg(wxString::Format(wxT("SEARCH DOWNLOADFILE %s"), downloads.Left(brk).GetData()));
			downloads=downloads.Mid(brk+1);
		}
	}

	wxString sToSearch = _ParseURL(Data, wxT("tosearch"));
	if (!sToSearch.IsEmpty() && IsSessionAdmin(Data,sSession)) {
		wxString sParams;
		sParams.Printf(sToSearch+wxT("\n"));
		sParams.Append(_ParseURL(Data, wxT("type"))+wxT("\n"));
		sParams.Append(wxString::Format(wxT("%ld\n"), StrToLong(_ParseURL(Data, wxT("min")))*1048576));
		sParams.Append(wxString::Format(wxT("%ld\n"), StrToLong(_ParseURL(Data, wxT("max")))*1048576));
		sParams.Append(_ParseURL(Data, wxT("avail"))+wxT("\n"));
		sParams.Append(_ParseURL(Data, wxT("ext"))+wxT("\n"));
		sParams.Append(_ParseURL(Data, wxT("method"))+wxT("\n"));

		pThis->webInterface->SendRecvMsg(wxString::Format(wxT("SEARCH DONEWSEARCH %s"), sParams.GetData()));
		Out.Replace(wxT("[Message]"),_("Search in progress. Refetch results in a moment!"));
	} else if (!sToSearch.IsEmpty() && !IsSessionAdmin(Data,sSession) ) {
		Out.Replace(wxT("[Message]"),_("Access denied!"));
	} else 
		Out.Replace(wxT("[Message]"),wxEmptyString);

	wxString sSort = _ParseURL(Data, wxT("sort"));
	if (sSort.Length()>0) pThis->m_iSearchSortby = StrToLong(sSort);
	sSort = _ParseURL(Data, wxT("sortAsc"));
	if (sSort.Length()>0) pThis->m_bSearchAsc = StrToLong(sSort);

	wxString result = pThis->m_Templates.sSearchHeader + pThis->webInterface->SendRecvMsg(wxString::Format(wxT("SEARCH WEBLIST %s\t%d\t%d"), pThis->m_Templates.sSearchResultLine.GetData(), pThis->m_iSearchSortby, pThis->m_bSearchAsc));
	
	// categoriesa
	CECPacket req(EC_OP_GET_PREFERENCES, EC_DETAIL_WEB);
	req.AddTag(CECTag(EC_TAG_SELECT_PREFS, (uint32)EC_PREFS_CATEGORIES));
	CECPacket *reply = pThis->webInterface->SendRecvMsg_v2(&req);
	if ( reply->GetTagCount() ) { 	// if there are no categories, not even the EC_TAG_PREFS_CATEGORIES will be included :)
		InsertCatBox(Out, 0, pThis->m_Templates.sCatArrow, reply->GetTagByIndex(0));
	} else {
		Out.Replace(wxT("[CATBOX]"),wxEmptyString);
	}
	delete reply;

	Out.Replace(wxT("[SEARCHINFOMSG]"),wxEmptyString);
	Out.Replace(wxT("[RESULTLIST]"), result);
	Out.Replace(wxT("[Result]"), _("Search Results"));
	Out.Replace(wxT("[Session]"), sSession);
	Out.Replace(wxT("[WebSearch]"), _("Web-based Search"));
	Out.Replace(wxT("[Name]"), _("Name"));
	Out.Replace(wxT("[Type]"), _("Type"));
	Out.Replace(wxT("[Any]"), _("Any"));
	Out.Replace(wxT("[Archives]"), _("Archive"));
	Out.Replace(wxT("[Audio]"), _("Audio"));
	Out.Replace(wxT("[CD-Images]"), _("CD-Images"));
	Out.Replace(wxT("[Pictures]"), _("Pictures"));
	Out.Replace(wxT("[Programs]"), _("Programs"));
	Out.Replace(wxT("[Texts]"), _("Texts"));
	Out.Replace(wxT("[Videos]"), _("Video"));
	Out.Replace(wxT("[Search]"), _("Search"));
	Out.Replace(wxT("[RefetchResults]"), _("Refetch Results"));
	Out.Replace(wxT("[Download]"), _("Download"));
	
	Out.Replace(wxT("[Filesize]"), _("Size"));
	Out.Replace(wxT("[Sources]"), _("Sources"));
	Out.Replace(wxT("[Filehash]"), _("File Hash"));
	Out.Replace(wxT("[Filename]"), _("File Name"));
	Out.Replace(wxT("[WebSearch]"), _("Web-based Search"));

	Out.Replace(wxT("[SizeMin]"), _("Min Size"));
	Out.Replace(wxT("[SizeMax]"), _("Max Size"));
	Out.Replace(wxT("[Availabl]"), _("Min Availability"));
	Out.Replace(wxT("[Extention]"), _("Extension"));
	Out.Replace(wxT("[Global]"), _("Global Search"));
	Out.Replace(wxT("[MB]"), _("MB"));
		
	Out.Replace(wxT("[METHOD]"), _("Method"));
	Out.Replace(wxT("[USESSERVER]"), _("Server"));
	Out.Replace(wxT("[Global]"), _("Global (Server)"));

	wxString val;
	val.Printf(wxT("%i"),(pThis->m_iSearchSortby!=0 || (pThis->m_iSearchSortby==0 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace(wxT("[SORTASCVALUE0]"), val);
	val.Printf(wxT("%i"),(pThis->m_iSearchSortby!=1 || (pThis->m_iSearchSortby==1 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace(wxT("[SORTASCVALUE1]"), val);
	val.Printf(wxT("%i"),(pThis->m_iSearchSortby!=2 || (pThis->m_iSearchSortby==2 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace(wxT("[SORTASCVALUE2]"), val);
	val.Printf(wxT("%i"),(pThis->m_iSearchSortby!=3 || (pThis->m_iSearchSortby==3 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace(wxT("[SORTASCVALUE3]"), val);
	
	return Out;
}


int CWebServer::UpdateSessionCount() {
	// remove old bans
	for (size_t i = 0; i < m_Params.badlogins.GetCount();) {
		uint32 diff= ::GetTickCount() - m_Params.badlogins[i]->timestamp ;
		if (diff >1000U*60U*15U && (::GetTickCount() > m_Params.badlogins[i]->timestamp)) {
			m_Params.badlogins.RemoveAt(i);
		} else 
			++i;
	}

	// count & remove old session
	for (size_t i = 0; i < m_Params.Sessions.GetCount();) {
	  time_t ts=time(NULL)-m_Params.Sessions[i]->startTime;
	  if (ts > SESSION_TIMEOUT_SECS) {
	    m_Params.Sessions.RemoveAt(i);
	  } else
	  	++i;
	}

	return m_Params.Sessions.GetCount();
}

wxString CWebServer::GetStatusBox(wxString &preselect)
{
	wxString result(wxT("<form><select name=\"cat\" size=\"1\""
	"onchange=GotoCat(this.form.cat.options[this.form.cat.selectedIndex].value)>"));
		
	const wxString catnames[] = {
		wxTRANSLATE("all others"),
		wxTRANSLATE("Waiting"),
		wxTRANSLATE("Downloading"),
		wxTRANSLATE("Erroneous"),
		wxTRANSLATE("Paused"),
		wxTRANSLATE("Stopped") };
		
	// those are values that CPartFile->GetPartfileStatus return. They are not meant be translated
	const wxChar *catvalues[] = {
		wxT(""),
		wxT("Waiting"),
		wxT("Downloading"),
		wxT("Erroneous"),
		wxT("Paused"),
		wxT("Stopped") };
		
	for (int i = 0; i < (int)(sizeof(catnames)/sizeof(wxString)); i++) {
		if ( catvalues[i] == preselect ) {
			result += wxT("<option selected value=\"");
		} else {
			result += wxT("<option value=\"");
		}
		result +=
			wxString(catvalues[i]) +
			wxT("\">") +
			wxGetTranslation(catnames[i]) +
			wxT("</option>");
	}
	result += wxT("</select></form>");
	
	return result;
}

void CWebServer::InsertCatBox(wxString &Out, int preselect, wxString boxlabel, CECTag *cats, bool jump) {
	wxString tempBuf;
	wxString catTitle;
	
	tempBuf = wxT("<form><select name=\"cat\" size=\"1\"");
	
	if (jump)  {
		tempBuf += wxT("onchange=GotoCat(this.form.cat.options[this.form.cat.selectedIndex].value)>");
	} else {
		tempBuf += wxT(">");
	}

	// Construct the categories options string.
	int catCount = cats->GetTagCount();
	for (int i = 0; i < catCount; ++i) {
		catTitle = cats->GetTagByIndex(i)->GetTagByName(EC_TAG_CATEGORY_TITLE)->GetStringData();
		tempBuf += wxString() + 
						wxT("<option") + ((i==preselect) ? wxT(" selected") : wxT(""))	+ 
						wxString::Format(wxT(" value=\"%i\">"),i) + ((i==0) ? wxString(_("all")) : catTitle) + 
						wxT("</option>");
	}
	
	Out.Replace(wxT("[CATBOX]"), boxlabel + tempBuf + wxT("</select></form>"));
}


/* 
 * Item container implementation
 */

ServersInfo *ServerEntry::GetContainerInstance()
{
	return ServersInfo::m_This;
}

ServersInfo *ServersInfo::m_This = 0;

ServersInfo::ServersInfo(CamulewebApp *webApp) : ItemsContainer<ServerEntry, xServerSort>(webApp)
{
	m_This = this;
	
	 // Init sorting order maps
	 m_SortHeaders[SERVER_SORT_NAME] = wxT("[SortName]");
	 m_SortHeaders[SERVER_SORT_DESCRIPTION] = wxT("[SortDescription]");
	 m_SortHeaders[SERVER_SORT_IP] = wxT("[SortIP]");
	 m_SortHeaders[SERVER_SORT_USERS] = wxT("[SortUsers]");
	 m_SortHeaders[SERVER_SORT_FILES] = wxT("[SortFiles]");
	 
	 m_SortStrVals[wxT("name")] = SERVER_SORT_NAME;
	 m_SortStrVals[wxT("description")] = SERVER_SORT_DESCRIPTION;
	 m_SortStrVals[wxT("ip")] = SERVER_SORT_IP;
	 m_SortStrVals[wxT("users")] = SERVER_SORT_USERS;
	 m_SortStrVals[wxT("files")] = SERVER_SORT_FILES;
	 
}

bool ServersInfo::ServersInfo::ReQuery()
{
	CECPacket srv_req(EC_OP_GET_SERVER_LIST, EC_DETAIL_WEB);
	CECPacket *srv_reply = m_webApp->SendRecvMsg_v2(&srv_req);
	if ( !srv_reply ) {
		return false;
	}
	//
	// query succeded - flush existing values and refill
	EraseAll();
	for(int i = 0; i < srv_reply->GetTagCount(); i ++) {
		CECTag *tag = srv_reply->GetTagByIndex(i);
		CECTag *tmpTag;
		
		ServerEntry Entry;
		Entry.sServerName = tag->GetTagByName(EC_TAG_SERVER_NAME)->GetStringData();
		if ((tmpTag = tag->GetTagByName(EC_TAG_SERVER_DESC)) != NULL) {
			Entry.sServerDescription = tmpTag->GetStringData();
		} else {
			Entry.sServerDescription = wxEmptyString;
		}

		Entry.sServerIP = tag->GetIPv4Data().StringIP(false);
		Entry.nServerIP = tag->GetIPv4Data().IP();
		Entry.nServerPort = tag->GetIPv4Data().port;

		if ((tmpTag = tag->GetTagByName(EC_TAG_SERVER_USERS)) != NULL) {
			Entry.nServerUsers = tmpTag->GetInt32Data();
		} else {
			Entry.nServerUsers = 0;
		}
		if ((tmpTag = tag->GetTagByName(EC_TAG_SERVER_USERS_MAX)) != NULL) {
			Entry.nServerMaxUsers = tmpTag->GetInt32Data();
		} else {
			Entry.nServerMaxUsers = 0;
		}
		if ((tmpTag = tag->GetTagByName(EC_TAG_SERVER_FILES)) != NULL) {
			Entry.nServerFiles = tmpTag->GetInt32Data();
		} else {
			Entry.nServerFiles = 0;
		}
		
		AddItem(Entry);
	}
	delete srv_reply;
	SortItems();
	
	return true;
}

bool ServersInfo::ProcessUpdate(CECPacket *)
{
	// no updates expected
	return false;
}

bool ServersInfo::CompareItems(const ServerEntry &i1, const ServerEntry &i2)
{
	bool Result;
	switch(m_SortOrder) {
		case SERVER_SORT_NAME:
			Result = i1.sServerName.CmpNoCase(i2.sServerName) > 0;
			break;
		case SERVER_SORT_DESCRIPTION:
			Result = i1.sServerDescription.CmpNoCase(i2.sServerDescription) > 0;
			break;
		case SERVER_SORT_IP:
			Result = i1.sServerIP.CmpNoCase(i2.sServerIP) > 0;
			break;
		case SERVER_SORT_USERS:
			Result = i1.nServerUsers > i2.nServerUsers;
			break;
		case SERVER_SORT_FILES:
			Result = i1.nServerFiles > i2.nServerFiles;
			break;
	}
	return Result ^ m_SortReverse;
}

SharedFilesInfo *SharedFiles::GetContainerInstance()
{
	return SharedFilesInfo::m_This;
}

SharedFilesInfo *SharedFilesInfo::m_This = 0;

SharedFilesInfo::SharedFilesInfo(CamulewebApp *webApp) : ItemsContainer<SharedFiles, xSharedSort>(webApp)
{
	m_This = this;
	m_SortOrder = SHARED_SORT_NAME;
	/*
	 * */
	 // Init sorting order maps
	 m_SortHeaders[SHARED_SORT_NAME] = wxT("[SortName]");
	 m_SortHeaders[SHARED_SORT_SIZE] = wxT("[SortSize]");
	 m_SortHeaders[SHARED_SORT_TRANSFERRED] = wxT("[SortTransferred]");
	 m_SortHeaders[SHARED_SORT_ALL_TIME_TRANSFERRED] = wxT("[SortAllTimeTransferred]");
	 m_SortHeaders[SHARED_SORT_REQUESTS] = wxT("[SortRequests]");
	 m_SortHeaders[SHARED_SORT_ALL_TIME_REQUESTS] = wxT("[SortAllTimeRequests]");
	 m_SortHeaders[SHARED_SORT_ACCEPTS] = wxT("[SortAccepts]");
	 m_SortHeaders[SHARED_SORT_ALL_TIME_ACCEPTS] = wxT("[SortAllTimeAccepts]");
	 m_SortHeaders[SHARED_SORT_PRIORITY] = wxT("[SortPriority]");

	 m_SortStrVals[wxT("")] = SHARED_SORT_NAME;
	 m_SortStrVals[wxT("name")] = SHARED_SORT_NAME;
	 m_SortStrVals[wxT("size")] = SHARED_SORT_SIZE;
	 m_SortStrVals[wxT("transferred")] = SHARED_SORT_TRANSFERRED;
	 m_SortStrVals[wxT("alltimetransferred")] = SHARED_SORT_ALL_TIME_TRANSFERRED;
	 m_SortStrVals[wxT("requests")] = SHARED_SORT_REQUESTS;
	 m_SortStrVals[wxT("alltimerequests")] = SHARED_SORT_ALL_TIME_REQUESTS;
	 m_SortStrVals[wxT("accepts")] = SHARED_SORT_ACCEPTS;
	 m_SortStrVals[wxT("alltimeaccepts")] = SHARED_SORT_ALL_TIME_ACCEPTS;
	 m_SortStrVals[wxT("priority")] = SHARED_SORT_PRIORITY;

}

bool SharedFilesInfo::ProcessUpdate(CECPacket *)
{
	// no updates expected
	return false;
}

//
// Using v1 interface ! - FIXME
bool SharedFilesInfo::ReQuery()
{
	// sSharedFilesList as:
	// %s\t%ld\t%s\t%ld\t%ll\t%d\t%d\t%d\t%d\t%s\t%s\t%d\t%d\n
	wxString sSharedFilesList = m_webApp->SendRecvMsg(wxT("SHAREDFILES LIST"));
	//
	// query succeded - flush existing values and refill
	EraseAll();
	wxString sEntry;
	int brk=0, newLinePos;
	while (sSharedFilesList.Length()>0) {
		newLinePos=sSharedFilesList.First(wxT("\n"));

		sEntry = sSharedFilesList.Left(newLinePos);
		
		sSharedFilesList = sSharedFilesList.Mid(newLinePos+1);

		SharedFiles dFile;
		
		//%s\t%ld\t%s\t%ld\t%ll\t%d\t%d\t%d\t%d\t%s\t%s\t%d\t%d\n
		brk=sEntry.First(wxT("\t"));
		dFile.sFileName = _SpecialChars(sEntry.Left(brk));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
		dFile.lFileSize = StrToLong(sEntry.Left(brk));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
		dFile.sED2kLink = sEntry.Left(brk);
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
		dFile.nFileTransferred = StrToLong(sEntry.Left(brk));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
		dFile.nFileAllTimeTransferred = atoll(unicode2char(sEntry.Left(brk)));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
		dFile.nFileRequests = StrToLong(sEntry.Left(brk));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
		dFile.nFileAllTimeRequests = StrToLong(sEntry.Left(brk));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
		dFile.nFileAccepts = StrToLong(sEntry.Left(brk));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
		dFile.nFileAllTimeAccepts = StrToLong(sEntry.Left(brk));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
		dFile.sFileHash = sEntry.Left(brk);
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
		dFile.sFilePriority = sEntry.Left(brk);
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First(wxT("\t"));
		dFile.nFilePriority = StrToLong(sEntry.Left(brk));
		sEntry=sEntry.Mid(brk+1);
		if (StrToLong(sEntry.Left(brk))==0) {
			dFile.bFileAutoPriority = false;
		} else {
			dFile.bFileAutoPriority = true;
		}

		AddItem(dFile);
	}

	SortItems();

	return true;
}

bool SharedFilesInfo::CompareItems(const SharedFiles &i1, const SharedFiles &i2)
{
	bool Result;
	switch(m_SortOrder) {
       case SHARED_SORT_NAME:
            Result = i1.sFileName.CmpNoCase(i2.sFileName) > 0;
            break;
        case SHARED_SORT_SIZE:
            Result = i1.lFileSize < i2.lFileSize;
            break;
        case SHARED_SORT_TRANSFERRED:
            Result = i1.nFileTransferred < i2.nFileTransferred;
            break;
        case SHARED_SORT_ALL_TIME_TRANSFERRED:
            Result = i1.nFileAllTimeTransferred < i2.nFileAllTimeTransferred;
            break;
        case SHARED_SORT_REQUESTS:
            Result = i1.nFileRequests < i2.nFileRequests;
            break;
       case SHARED_SORT_ALL_TIME_REQUESTS:
            Result = i1.nFileAllTimeRequests < i2.nFileAllTimeRequests;
            break;
        case SHARED_SORT_ACCEPTS:
            Result = i1.nFileAccepts < i2.nFileAccepts;
            break;
        case SHARED_SORT_ALL_TIME_ACCEPTS:
            Result = i1.nFileAllTimeAccepts < i2.nFileAllTimeAccepts;
            break;
        case SHARED_SORT_PRIORITY:
           //Very low priority is define equal to 4 ! Must adapte sorting code
            if (i1.nFilePriority == 4) {
                Result = (i2.nFilePriority != 4);
            } else {
                if (i2.nFilePriority == 4) {
                        Result = (i1.nFilePriority == 4);
                } else
                        Result = i1.nFilePriority < i2.nFilePriority;
            }
            break;
	}
	return Result ^ m_SortReverse;
}

DownloadFilesInfo *DownloadFiles::GetContainerInstance()
{
	return DownloadFilesInfo::m_This;
}

DownloadFilesInfo *DownloadFilesInfo::m_This = 0;

DownloadFilesInfo::DownloadFilesInfo(CamulewebApp *webApp) : ItemsContainer<DownloadFiles, xDownloadSort>(webApp)
{
	m_This = this;

	m_SortHeaders[DOWN_SORT_NAME] = wxT("[SortName]");
	m_SortHeaders[DOWN_SORT_SIZE] = wxT("[SortSize]");
	m_SortHeaders[DOWN_SORT_COMPLETED] = wxT("[SortCompleted]");
	m_SortHeaders[DOWN_SORT_TRANSFERRED] = wxT("[SortTransferred]");
	m_SortHeaders[DOWN_SORT_SPEED] = wxT("[SortSpeed]");
	m_SortHeaders[DOWN_SORT_PROGRESS] = wxT("[SortProgress]");
	
	m_SortStrVals[wxT("")] = DOWN_SORT_NAME;
	m_SortStrVals[wxT("name")] = DOWN_SORT_NAME;
	m_SortStrVals[wxT("size")] = DOWN_SORT_SIZE;
	m_SortStrVals[wxT("completed")] = DOWN_SORT_COMPLETED;
	m_SortStrVals[wxT("transferred")] = DOWN_SORT_TRANSFERRED;
	m_SortStrVals[wxT("progress")] = DOWN_SORT_PROGRESS;
	m_SortStrVals[wxT("speed")] = DOWN_SORT_SPEED;
	
	m_This = this;
}

bool DownloadFilesInfo::ReQuery()
{
	CECPacket req_sts(EC_OP_GET_DLOAD_QUEUE, EC_DETAIL_UPDATE);
	//
	// Phase 1: request status
	CECPacket *reply = m_webApp->SendRecvMsg_v2(&req_sts);
	if ( !reply ) {
		return false;
	}
	
	//
	// Phase 2: update status, mark new files for subsequent query
	std::set<uint32> core_files;
	CECPacket req_full(EC_OP_GET_DLOAD_QUEUE);

	for (int i = 0;i < reply->GetTagCount();i++) {
		CEC_PartFile_Tag *tag = (CEC_PartFile_Tag *)reply->GetTagByIndex(i);

		core_files.insert(tag->FileID());
		if ( m_files.count(tag->FileID()) ) {
			// already have it - update status
			DownloadFiles *file = m_files[tag->FileID()];
			file->lSourceCount = tag->SourceCount();
			file->lNotCurrentSourceCount = tag->SourceNotCurrCount();
			file->nFileStatus = tag->FileStatus();
			file->sFileStatus = tag->GetFileStatusString();
			file->sPartStatus = tag->PartStatus();
			file->lTransferringSourceCount = tag->SourceXferCount();
			if ( file->lTransferringSourceCount > 0 ) {
				file->lFileCompleted = tag->SizeDone();
				file->lFileTransferred = tag->SizeXfer();
				file->lFileSpeed = tag->Speed();
				file->fCompleted = (100.0*file->lFileCompleted) / file->lFileSize;
			}
			CECTag *gaptag = tag->GetTagByName(EC_TAG_PARTFILE_GAP_STATUS);
			if ( gaptag ) {
				file->m_Encoder.Decode((unsigned char *)gaptag->GetTagData(), gaptag->GetTagDataLen());
			}
		} else {
			// don't have it - prepare to request full info
			req_full.AddTag(CECTag(EC_TAG_PARTFILE, tag->FileID()));
		}
	}
	delete reply;

	//
	// Phase 2.5: remove files that core no longer have; mark files with
	// status = "downloading" for parts query
	for(std::list<DownloadFiles>::iterator i = m_items.begin(); i != m_items.end();i++) {
		if ( core_files.count(i->file_id) == 0 ) {
			m_files.erase(i->file_id);
			m_items.erase(i);
		}
	}
	//
	// Phase 3: request full info about files we don't have yet
	if ( req_full.GetTagCount() ) {
		reply = m_webApp->SendRecvMsg_v2(&req_full);
		if ( !reply ) {
			return false;
		}
		for (int i = 0;i < reply->GetTagCount();i++) {
			CEC_PartFile_Tag *tag = (CEC_PartFile_Tag *)reply->GetTagByIndex(i);

			DownloadFiles file;
			file.file_id = tag->FileID();
			file.sFileName = tag->FileName();
			file.lFileSize = tag->SizeFull();
			file.lFileCompleted = tag->SizeDone();
			file.lFileTransferred = tag->SizeXfer();
			file.lFileSpeed = tag->Speed();
			file.lSourceCount = tag->SourceCount();
			file.lNotCurrentSourceCount = tag->SourceNotCurrCount();
			file.lTransferringSourceCount = tag->SourceXferCount();
			file.fCompleted = (100.0*file.lFileCompleted) / file.lFileSize;
			file.nFileStatus = tag->FileStatus();
			file.sFileStatus = tag->GetFileStatusString();
			file.lFilePrio = tag->Prio();
			file.sFileHash = wxString::Format(wxT("%08x"), tag->FileID());
			file.sED2kLink = tag->FileEd2kLink();
			file.sPartStatus = tag->PartStatus();
			
			
			file.m_Encoder = PartFileEncoderData( (file.lFileSize + (PARTSIZE - 1)) / PARTSIZE, 10);

			m_items.push_back(file);
			m_files[file.file_id] = &(m_items.back());

		}
	}
	
	SortItems();

	return true;
}

bool DownloadFilesInfo::ProcessUpdate(CECPacket *)
{
	return false;
}

bool DownloadFilesInfo::CompareItems(const DownloadFiles &i1, const DownloadFiles &i2)
{
	bool Result;
	switch(m_SortOrder) {
		case DOWN_SORT_NAME:
            Result = i1.sFileName.CmpNoCase(i2.sFileName) > 0;
			break;
		case DOWN_SORT_SIZE:
			Result = i1.lFileSize < i2.lFileSize;
			break;
		case DOWN_SORT_COMPLETED:
			Result = i1.lFileCompleted < i2.lFileCompleted;
			break;
		case DOWN_SORT_TRANSFERRED:
			Result = i1.lFileTransferred < i2.lFileTransferred;
			break;
		case DOWN_SORT_SPEED:
			Result = i1.lFileSpeed < i2.lFileSpeed;
			break;
		case DOWN_SORT_PROGRESS:
			Result = i1.fCompleted < i2.fCompleted;
			break;
	}
	return Result ^ m_SortReverse;
}

/*!
 * Image classes:
 * 
 * CFileImage: simply represent local file
 * CDynImage: dynamically generated from gap info
 */
 
CAnyImage::CAnyImage(int size)
{
	m_size = 0;
	m_alloc_size = size;
	if ( m_alloc_size ) {
		m_data = new unsigned char[m_alloc_size];
	} else {
		m_data = 0;
	}
}

CAnyImage::~CAnyImage()
{
	if ( m_data ) {
		delete [] m_data;
	}
}

void CAnyImage::Realloc(int size)
{
	if ( size == m_alloc_size ) {
		return;
	}
	// always grow, but shrink only x2
	if ( (size > m_alloc_size) || (size < (m_alloc_size / 2)) ) {
		m_alloc_size = size;
		if ( m_data ) {
			delete [] m_data;
		}
		m_data = new unsigned char[m_alloc_size];
	}
}

unsigned char *CAnyImage::RequestData(int &size)
{
	size = m_size;
	return m_data;
}

CFileImage::CFileImage(const char *name) : CAnyImage(0), m_name(char2unicode(name))
{
	m_size = 0;
	wxFFile fis(m_name);
	// FIXME: proper logging is needed
	if ( !fis.IsOpened() ) {
		size_t file_size = fis.Length();
		if ( file_size ) {
			Realloc(fis.Length());
			m_size = fis.Read(m_data,file_size);
		} else {
			printf("CFileImage: file %s have zero length\n", unicode2char(m_name));
		}
	} else {
		printf("CFileImage: failed to open %s\n", unicode2char(m_name));
	}
}

#ifdef WITH_LIBPNG

CDynImage::CDynImage(uint32 id, int width, int height, wxString &tmpl, otherfunctions::PartFileEncoderData &encoder) :
	CProgressImage(width * height * sizeof(uint32), tmpl), m_Encoder(encoder),
	m_name(wxString::Format(wxT("%d"), id))
{
	m_id = id;
	m_width = width;
	m_height = height;
	
	//
	// Allocate array of "row pointers" - libpng need it in this form
	//
	
	// When allocating an array with a new expression, GCC used to allow 
	// parentheses around the type name. This is actually ill-formed and it is 
	// now rejected. From gcc3.4
	m_row_ptrs = new unsigned char *[m_height];
	for (int i = 0; i < m_height;i++) {
		m_row_ptrs[i] = m_data + i * m_width;
	}
	
	m_png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	m_info_ptr = png_create_info_struct(m_png_ptr);
	png_set_IHDR(m_png_ptr, m_info_ptr, m_width, m_height, 8, PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		
	png_set_write_fn(m_png_ptr, this, png_write_fn, 0);
}

void CDynImage::png_write_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
	CDynImage *This = (CDynImage *)png_get_io_ptr(png_ptr);
	memcpy(This->m_data + This->m_write_idx, data, length);
}

wxString CDynImage::GetHTML()
{
	// template contain %s (name) %d (width)
	return wxString::Format(m_tmpl, m_name.GetData(), m_width);
}

unsigned char *CDynImage::RequestData(int &size)
{
	// create image
	// ....
	
	//
	// write png into buffer
	m_write_idx = 0;
	png_write_info(m_png_ptr, m_info_ptr);
	png_write_image(m_png_ptr, m_row_ptrs);
	png_write_end(m_png_ptr, 0);
	
	return CProgressImage::RequestData(size);
}

#else

wxString CDynImage::GetHTML()
{
	// template contain %s (name) %d (width)
	return wxString::Format(m_tmpl, unicode2char(m_name), m_width);
}

#endif
