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


#ifdef HAVE_CONFIG_H
	#include "config.h"		// Needed for VERSION
#endif

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "WebServer.h"
#include "MD5Sum.h"
#ifdef __FreeBSD__
	#include "otherfunctions.h"	// Needed for atoll
#endif

#include "resource.h"		// Needed by strings.en
#include "strings.en"		// Strings
#include "WebSocket.h"		// Needed for StopSockets()
#include "otherstructs.h"	// Needed for TransferredData
#include "GetTickCount.h"	// Needed for GetTickCount
#include "ECSocket.h"	

#include "types.h"
#include "WebServer.h"
#include "ED2KLink.h"
#include "MD5Sum.h"
#include <stdlib.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!

WX_DEFINE_OBJARRAY(ArrayOfUpDown);
WX_DEFINE_OBJARRAY(ArrayOfSession);
WX_DEFINE_OBJARRAY(ArrayOfTransferredData);
WX_DEFINE_OBJARRAY(ArrayOfSharedFiles);
WX_DEFINE_OBJARRAY(ArrayOfServerEntry);
WX_DEFINE_OBJARRAY(ArrayOfDownloadFiles);

#define HTTPInit "Server: aMule\r\nPragma: no-cache\r\nExpires: 0\r\nCache-Control: no-cache, no-store, must-revalidate\r\nConnection: close\r\nContent-Type: text/html\r\n"
#define HTTPInitGZ "Server: aMule\r\nPragma: no-cache\r\nExpires: 0\r\nCache-Control: no-cache, no-store, must-revalidate\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Encoding: gzip\r\n"

#define WEB_SERVER_TEMPLATES_VERSION	4


//common functions -- start
//shakraw, same as CastItoXBytes() from otherfunctions.h
wxString castItoXBytes(uint64 count) {
    wxString buffer;
	
    if (count < 1024)
        buffer.Printf("%.0f %s",(float)count, _("Bytes"));
    else if (count < 1048576)
        buffer.Printf("%.0f %s",(float)count/1024, _("KB"));
    else if (count < 1073741824)
        buffer.Printf("%.2f %s",(float)count/1048576, _("MB"));
    else if (count < 1099511627776LL)
        buffer.Printf("%.2f %s",(float)count/1073741824, _("GB"));
    else
        buffer.Printf("%.3f %s",(float)count/1099511627776LL, _("TB"));
	
    return buffer;
}

//shakraw, same as toHex() from otherfunctions.h
inline BYTE toHex(const BYTE &x) {
	return x > 9 ? x + 55: x + 48;
}

//shakraw, same as URLEncode() from otherfunctions.h
wxString URLEncode(wxString sIn) {
    wxString sOut;
	
    const int nLen = sIn.Length() + 1;

    register LPBYTE pOutTmp = NULL;
    LPBYTE pOutBuf = NULL;
    register LPBYTE pInTmp = NULL;
    LPBYTE pInBuf =(LPBYTE)sIn.GetWriteBuf(nLen); //GetData(); //GetBuffer(nLen);
	
    //alloc out buffer
    pOutBuf = (LPBYTE)sOut.GetWriteBuf(nLen*3-2); //GetData(); //GetBuffer(nLen  * 3 - 2);//new BYTE [nLen  * 3];

    if(pOutBuf)
    {
        pInTmp	= pInBuf;
	pOutTmp = pOutBuf;
		
	// do encoding
	while (*pInTmp)
	{
	    if(isalnum(*pInTmp))
	        *pOutTmp++ = *pInTmp;
	    else
	        if(isspace(*pInTmp))
		    *pOutTmp++ = '+';
		else
		{
		    *pOutTmp++ = '%';
		    *pOutTmp++ = toHex(*pInTmp>>4);
		    *pOutTmp++ = toHex(*pInTmp%16);
		}
	    pInTmp++;
	}
	*pOutTmp = '\0';
	sOut=pOutBuf;
	//delete [] pOutBuf;
	//Out.ReleaseBuffer();
	sOut.UngetWriteBuf();
    }
    //sIn.ReleaseBuffer();
    sIn.UngetWriteBuf();
    return sOut;
}

//shakraw, same as LeadingZero() from otherfunctions.h
wxString leadingZero(uint32 units) {
	wxString temp;
	if (units<10) temp.Printf("0%i",units); else temp.Printf("%i",units);
	return temp;
}

//shakraw, same as CastSecondsToHM() from otherfunctions.h
wxString castSecondsToHM(sint32 count) {
	wxString buffer;
	if (count < 0)
		buffer = "?"; 
	else if (count < 60)
		buffer.Printf("%i %s",count, _("secs")); 
	else if (count < 3600) 
		buffer.Printf("%i:%s %s",count/60,leadingZero(count-(count/60)*60).GetData(), _("mins"));
	else if (count < 86400) 
		buffer.Printf("%i:%s %s",count/3600,leadingZero((count-(count/3600)*3600)/60).GetData(), _("h"));
	else 
		buffer.Printf("%i %s %i %s",count/86400, _("D"), (count-(count/86400)*86400)/3600, _("h")); 
	return buffer;
} 
//common functions -- end



CWebServer::CWebServer(CamulewebApp *webApp) {
	webInterface = webApp;
	
	m_Params.bShowUploadQueue = false;

	m_Params.DownloadSort = DOWN_SORT_NAME;
	m_Params.bDownloadSortReverse = false;
	m_Params.ServerSort = SERVER_SORT_NAME;
	m_Params.bServerSortReverse = false;
	m_Params.SharedSort = SHARED_SORT_NAME;
	m_Params.bSharedSortReverse = false;
		
	m_Params.sLastModified=wxString("");
	m_Params.sETag=wxString("");
	m_iSearchSortby=3;
	m_bSearchAsc=0;

	m_bServerWorking = false; // not running (listening) yet
}


CWebServer::~CWebServer(void) {
	//stop web socket thread
	wsThread->Delete();
}


//start web socket and reload templates
void CWebServer::StartServer(void) {
	if (!m_bServerWorking) {
		ReloadTemplates();

		//create the thread...
		wsThread = new CWSThread(this);
		if ( wsThread->Create() != wxTHREAD_NO_ERROR ) {
			webInterface->Print("Can't create web socket thread\n");
		} else {
			//...and run it
			wsThread->Run();
 
			m_bServerWorking = true;
			webInterface->Print("Web Server: Started\n");
		}
	} else
		webInterface->Print("Web Server: running\n");
}

//restart web socket and reload templates
void CWebServer::RestartServer(void) {
	if (m_bServerWorking) {
		if (wsThread) wsThread->Delete();
	}
	
	//create the thread...
	wsThread = new CWSThread(this);
	if ( wsThread->Create() != wxTHREAD_NO_ERROR ) {
		webInterface->Print("Can't create web socket thread\n");
	} else {
		//...and run it
		wsThread->Run();
		webInterface->Print("Web Server: Restarted\n");
	}
}


//stop web socket
void CWebServer::StopServer(void) {
	if (m_bServerWorking) {
		m_bServerWorking = false;
		if (wsThread) wsThread->Delete();
		webInterface->Print("Web Server: Stopped\n");
	} else
		webInterface->Print("Web Server: not running\n");
}


//returns web server listening port
int CWebServer::GetWSPort(void) {
	return(atoi(webInterface->SendRecvMsg("PREFERENCES GETWSPORT").GetData()));
}

//sends output to web interface
void CWebServer::Print(char *sFormat, ...) {
	char buffer[5000];

	va_list argptr;
	va_start(argptr, sFormat);
	vsnprintf(buffer, 5000, sFormat, argptr);
	va_end(argptr);
	
	webInterface->Print(buffer);
}

//reload template file
void CWebServer::ReloadTemplates(void) {
	time_t t = time(NULL);
	char *s = new char[255];
	strftime(s, 255, "%a, %d %b %Y %H:%M:%S GMT", localtime(&t));
	m_Params.sLastModified = wxString(s);
	delete[] s;
	
	m_Params.sETag = MD5Sum(m_Params.sLastModified).GetHash();

	wxString sFile = getenv("HOME") + wxString("/.aMule/aMule.tmpl");
		
	if (!wxFileName::FileExists(sFile)) {
		// no file. do nothing.
		webInterface->SendRecvMsg(wxString::Format("LOGGING ADDLOGLINE %d %s", true, wxString::Format(_("Can't load templates: Can't open file %s"), sFile.GetData()).GetData()));
		return;
	}

	wxFileInputStream input(sFile);
	if (input.Ok()) {
		wxTextInputStream file(input);
		wxString sAll;
		while (!input.Eof()) {
			wxString sLine;
			sLine=file.ReadString();

			sAll += sLine + "\n";
		}

		wxString sVersion = _LoadTemplate(sAll,"TMPL_VERSION");
		long lVersion = atol(sVersion);
		if (lVersion < WEB_SERVER_TEMPLATES_VERSION) {
			webInterface->SendRecvMsg(wxString::Format("LOGGING ADDLOGLINE %d %s", true, wxString::Format(_("Can't load templates: Can't open file %s"), sFile.GetData()).GetData()));
		} else {
			m_Templates.sHeader = _LoadTemplate(sAll,"TMPL_HEADER");
			m_Templates.sHeaderMetaRefresh = _LoadTemplate(sAll,"TMPL_HEADER_META_REFRESH");
			m_Templates.sHeaderStylesheet = _LoadTemplate(sAll,"TMPL_HEADER_STYLESHEET");
			m_Templates.sFooter = _LoadTemplate(sAll,"TMPL_FOOTER");
			m_Templates.sServerList = _LoadTemplate(sAll,"TMPL_SERVER_LIST");
			m_Templates.sServerLine = _LoadTemplate(sAll,"TMPL_SERVER_LINE");
			m_Templates.sTransferImages = _LoadTemplate(sAll,"TMPL_TRANSFER_IMAGES");
			m_Templates.sTransferList = _LoadTemplate(sAll,"TMPL_TRANSFER_LIST");
			m_Templates.sTransferDownHeader = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_HEADER");
			m_Templates.sTransferDownFooter = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_FOOTER");
			m_Templates.sTransferDownLine = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_LINE");
			m_Templates.sTransferDownLineGood = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_LINE_GOOD");
			m_Templates.sTransferUpHeader = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_HEADER");
			m_Templates.sTransferUpFooter = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_FOOTER");
			m_Templates.sTransferUpLine = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_LINE");
			m_Templates.sTransferUpQueueShow = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_QUEUE_SHOW");
			m_Templates.sTransferUpQueueHide = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_QUEUE_HIDE");
			m_Templates.sTransferUpQueueLine = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_QUEUE_LINE");
			m_Templates.sTransferBadLink = _LoadTemplate(sAll,"TMPL_TRANSFER_BAD_LINK");
			m_Templates.sDownloadLink = _LoadTemplate(sAll,"TMPL_DOWNLOAD_LINK");
			m_Templates.sSharedList = _LoadTemplate(sAll,"TMPL_SHARED_LIST");
			m_Templates.sSharedLine = _LoadTemplate(sAll,"TMPL_SHARED_LINE");
			m_Templates.sSharedLineChanged = _LoadTemplate(sAll,"TMPL_SHARED_LINE_CHANGED");
			m_Templates.sGraphs = _LoadTemplate(sAll,"TMPL_GRAPHS");
			m_Templates.sLog = _LoadTemplate(sAll,"TMPL_LOG");
			m_Templates.sServerInfo = _LoadTemplate(sAll,"TMPL_SERVERINFO");
			m_Templates.sDebugLog = _LoadTemplate(sAll,"TMPL_DEBUGLOG");
			m_Templates.sStats = _LoadTemplate(sAll,"TMPL_STATS");
			m_Templates.sPreferences = _LoadTemplate(sAll,"TMPL_PREFERENCES");
			m_Templates.sLogin = _LoadTemplate(sAll,"TMPL_LOGIN");
			m_Templates.sConnectedServer = _LoadTemplate(sAll,"TMPL_CONNECTED_SERVER");
			m_Templates.sAddServerBox = _LoadTemplate(sAll,"TMPL_ADDSERVERBOX");
			m_Templates.sWebSearch = _LoadTemplate(sAll,"TMPL_WEBSEARCH");
			m_Templates.sSearch = _LoadTemplate(sAll,"TMPL_SEARCH");
			m_Templates.iProgressbarWidth=atoi(_LoadTemplate(sAll,"PROGRESSBARWIDTH").GetData());
			m_Templates.sSearchHeader = _LoadTemplate(sAll,"TMPL_SEARCH_RESULT_HEADER");			
			m_Templates.sSearchResultLine = _LoadTemplate(sAll,"TMPL_SEARCH_RESULT_LINE");			
			m_Templates.sProgressbarImgs = _LoadTemplate(sAll,"PROGRESSBARIMGS");
			m_Templates.sProgressbarImgsPercent = _LoadTemplate(sAll,"PROGRESSBARPERCENTIMG");
			m_Templates.sClearCompleted = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_CLEARBUTTON");
			m_Templates.sCatArrow= _LoadTemplate(sAll,"TMPL_CATARROW");			

			m_Templates.sProgressbarImgsPercent.Replace("[PROGRESSGIFNAME]","%s");
			m_Templates.sProgressbarImgsPercent.Replace("[PROGRESSGIFINTERNAL]","%i");
			m_Templates.sProgressbarImgs.Replace("[PROGRESSGIFNAME]","%s");
			m_Templates.sProgressbarImgs.Replace("[PROGRESSGIFINTERNAL]","%i");
		}
	} else {
		webInterface->SendRecvMsg(wxString::Format("LOGGING ADDLOGLINE %d %s", true, wxString::Format(_("Can't load templates: Can't open file %s"), sFile.GetData()).GetData()));
	}
}


wxString CWebServer::_LoadTemplate(wxString sAll, wxString sTemplateName) {
	wxString sRet = "";
	int nStart = sAll.Find("<--" + sTemplateName + "-->");
	int nEnd = sAll.Find("<--" + sTemplateName + "_END-->");
	
	if (nStart != -1 && nEnd != -1)	{
		nStart += sTemplateName.Length() + 7;
		sRet = sAll.Mid(nStart, nEnd - nStart - 1);
	}
	
	if (sRet.IsEmpty()) {
		if (sTemplateName=="TMPL_VERSION")
			webInterface->Print((char *) _("Can't find template version number!\nPlease replace aMule.tmpl with a newer version!"));
		webInterface->Print((char *) _("Failed to load template %s\n"), sTemplateName.GetData());
	}
	return sRet;
}


void CWebServer::_RemoveServer(CWebServer *pThis, wxString sIP, wxString sPort) {
	wxString request = wxString::Format("SERVER REMOVE %s %s", sIP.GetData(), sPort.GetData());
	pThis->webInterface->SendRecvMsg(request.GetData());
}


void CWebServer::_SetSharedFilePriority(CWebServer *pThis, wxString hash, uint8 priority) {	
	int prio = (int) priority;
	if (prio >= 0 && prio < 5) {
		pThis->webInterface->SendRecvMsg(wxString::Format("SHAREDFILES SETAUTOUPPRIORITY %s %d", hash.GetData(), 0));
		pThis->webInterface->SendRecvMsg(wxString::Format("SHAREDFILES SETUPPRIORITY %s %d", hash.GetData(), prio));
	} else if (prio == 5) {
		pThis->webInterface->SendRecvMsg(wxString::Format("SHAREDFILES SETAUTOUPPRIORITY %s %d", hash.GetData(), 1));
		pThis->webInterface->SendRecvMsg(wxString::Format("SHAREDFILES UPDATEAUTOUPPRIORITY %s", hash.GetData()));
	}
}


void CWebServer::AddStatsLine(UpDown* line) {
	m_Params.PointsForWeb.Add(line);
	if (m_Params.PointsForWeb.GetCount() > WEB_GRAPH_WIDTH) {
		delete m_Params.PointsForWeb[0];
		m_Params.PointsForWeb.RemoveAt(0);
	}
}


wxString CWebServer::_SpecialChars(wxString str) {
	str.Replace("&","&amp;");
	str.Replace("<","&lt;");
	str.Replace(">","&gt;");
	str.Replace("\"","&quot;");
	return str;
}


void CWebServer::_ConnectToServer(CWebServer *pThis, wxString sIP, wxString sPort) {
	wxString request = wxString("SERVER CONNECT ")+sIP+wxString(" ")+sPort;
	pThis->webInterface->SendRecvMsg(request.GetData());
}


void CWebServer::ProcessImgFileReq(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL) return;
		
	wxString filename=Data.sURL;
	wxString contenttype;

	pThis->webInterface->Print("inc. fname=%s\n",filename.GetData());
	if (filename.Right(4).MakeLower()==".gif") contenttype="Content-Type: image/gif\r\n";
	else if (filename.Right(4).MakeLower()==".jpg"  || filename.Right(5).MakeLower()==".jpeg") contenttype="Content-Type: image/jpg\r\n";
	else if (filename.Right(4).MakeLower()==".bmp") contenttype="Content-Type: image/bmp\r\n";
	else if (filename.Right(4).MakeLower()==".png") contenttype="Content-Type: image/png";
	//DonQ - additional filetypes
	else if (filename.Right(4).MakeLower()==".ico") contenttype="Content-Type: image/x-icon\r\n";
	else if (filename.Right(4).MakeLower()==".css") contenttype="Content-Type: text/css\r\n";
	else if (filename.Right(3).MakeLower()==".js") contenttype="Content-Type: text/javascript\r\n";
		
	contenttype += "Last-Modified: " + pThis->m_Params.sLastModified + "\r\n" + "ETag: " + pThis->m_Params.sETag + "\r\n";
		
	filename=filename.Right(filename.Length()-1);
	//filename.Replace("/","\\");
	//filename=wxString(theApp.glob_prefs->GetAppDir())+"webserver/"+filename;
	filename=getenv("HOME") + wxString("/.aMule/webserver/") + wxString(filename);
	pThis->webInterface->Print("**** imgrequest: %s\n",filename.GetData());

	if (!wxFileName::FileExists(filename)) {
		pThis->webInterface->Print("**** imgrequest: file %s does not exists\n", filename.GetData());
	}

	wxFileInputStream* fis = new wxFileInputStream(filename);
	if (fis->Ok()) {
		fis->SeekI(0,wxFromEnd);
		off_t koko=fis->TellI();
		fis->SeekI(0,wxFromStart);
		char* buffer=new char[koko];
		fis->Read((void*)buffer,koko);
		Data.pSocket->SendContent((char*)contenttype.GetData(),(void*)buffer,fis->LastRead());
		delete fis;
		delete[] buffer;
	}
}


void CWebServer::ProcessStyleFileReq(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL) return;

	wxString filename=Data.sURL;
	wxString contenttype;

	pThis->webInterface->Print("inc. fname=%s\n",filename.GetData());
	contenttype="Content-Type: text/css\r\n";

	filename=filename.Right(filename.Length()-1);
	//filename.Replace("/","\\");
	//filename=wxString(theApp.glob_prefs->GetAppDir())+"webserver/"+filename;
	filename=getenv("HOME") + wxString("/.aMule/webserver/") + wxString(filename);
	pThis->webInterface->Print("**** cssrequest: %s\n",filename.GetData());

	if (wxFileName::FileExists(filename)) {
		wxFileInputStream* fis = new wxFileInputStream(filename);
		if(fis->Ok()) {
			fis->SeekI(0,wxFromEnd);
			off_t koko=fis->TellI();
			fis->SeekI(0,wxFromStart);
			char* buffer=new char[koko];
			fis->Read((void*)buffer,koko);
			Data.pSocket->SendContent((char*)contenttype.GetData(),(void*)buffer,fis->LastRead());
			delete fis;
			delete[] buffer;
		}
	} else {
		pThis->webInterface->Print("**** imgrequest: file %s does not exists\n", filename.GetData());
	}
}


void CWebServer::ProcessURL(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return;

	bool isUseGzip = (atoi(pThis->webInterface->SendRecvMsg("PREFS GETWEBUSEGZIP").GetData()) == 0) ? false : true;
	wxString Out = "";
	wxString OutE = "";	// List Entry Templates
	wxString OutE2 = "";
	wxString OutS = "";	// ServerStatus Templates
	TCHAR *gzipOut = NULL;
	long gzipLen=0;

	wxString HTTPProcessData = "";
	wxString HTTPTemp = "";
	srand ( time(NULL) );

	long lSession = 0;
	wxString sSes = _ParseURL(Data, "ses");
	if (sSes.Cmp("") != 0) lSession = atol(sSes.GetData());
	wxString sSession; sSession.Printf("%ld", lSession);

	wxString sW = _ParseURL(Data, "w");

	// WE CANT TOUCH THE MAIN THREAD'S GUI!!!
	if (sW == "password") {
		bool login=false;
		wxString ip=inet_ntoa( Data.inadr );

		wxString strAuth = pThis->webInterface->SendRecvMsg(wxString::Format("PREFS GETWSPASS %s", MD5Sum(_ParseURL(Data, "p")).GetHash().GetData()).GetData());
		if (strAuth == "AdminLogin") {
			Session* ses=new Session();
			ses->admin=true;
			ses->startTime = time(NULL);
			ses->lSession = lSession = rand() * 10000L + rand();
			pThis->m_Params.Sessions.Add(ses);
			login=true;
		} else if (strAuth == "GuestLogin") {
			Session* ses=new Session();
			ses->admin=false;
			ses->startTime = time(NULL);
			ses->lSession = lSession = rand() * 10000L + rand();
			pThis->m_Params.Sessions.Add(ses);
			login=true;
		} else {
			TransferredData newban={inet_addr(ip), ::GetTickCount()}; // save failed attempt (ip,time)
			pThis->m_Params.badlogins.Add(&newban);
			login=false;
		}
		isUseGzip = false; // [Julien]
		if (login) {
			uint32 ipn=inet_addr( ip) ;
			for (size_t i = 0; i < pThis->m_Params.badlogins.GetCount();) {
				if (ipn == pThis->m_Params.badlogins[i]->datalen) {
					pThis->m_Params.badlogins.RemoveAt(i);
				} else 
					i++;
			}
		}
	}

	if (sW == "logout")
		_RemoveSession(Data, lSession);
		
	if (_IsLoggedIn(Data, lSession)) {
		Out += _GetHeader(Data, lSession);
		
		wxString sPage = sW;
		pThis->webInterface->Print("***** logged in, getting page %s\n", sPage.GetData());
		pThis->webInterface->Print("***** session is %s\n", sSession.GetData());
		
		if (sPage == "server") {
			Out += _GetServerList(Data);
		} else if (sPage == "download") {
			Out += _GetDownloadLink(Data);
		} else if (sPage == "shared") { 
			Out += _GetSharedFilesList(Data);
		} else if (sPage == "transfer") {
			Out += _GetTransferList(Data);
		} else if (sPage == "websearch") {
			Out += _GetWebSearch(Data);
		} else if (sPage == "search") {
			Out += _GetSearch(Data);
		} else if (sPage == "graphs") {
			Out += _GetGraphs(Data);
		} else if (sPage == "log") {
			Out += _GetLog(Data);
		} else if (sPage == "sinfo") {
			Out += _GetServerInfo(Data);
		} else if (sPage == "debuglog") {
			Out += _GetDebugLog(Data);
		} else if (sPage == "stats") {
			Out += _GetStats(Data);
		} else if (sPage == "options") {
			Out += _GetPreferences(Data);
		}
		
		Out += _GetFooter(Data);

		if (sPage == "")
			isUseGzip = false;

		if (isUseGzip) {
			bool bOk = false;
			try {
				uLongf destLen = Out.Length() + 1024;
				gzipOut = new TCHAR[destLen];
				if (_GzipCompress((Bytef*)gzipOut, &destLen, (const Bytef*)(TCHAR*)Out.GetData(), Out.Length(), Z_DEFAULT_COMPRESSION) == Z_OK) {
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
		uint32 faults=0;

		// check for bans
		for (size_t i = 0; i < pThis->m_Params.badlogins.GetCount();i++)
			if (pThis->m_Params.badlogins[i]->datalen==ip) faults++;

		if (faults>4) {
			Out += _("Access denied!");
				
			// set 15 mins ban by using the badlist
			TransferredData preventive={ip, ::GetTickCount() + (15*60*1000) };
			for (int i=0;i<=5;i++)
				pThis->m_Params.badlogins.Add(&preventive);
		} else
			Out += _GetLoginScreen(Data);
	}

	// send answer ...
	if (!isUseGzip)	{
		Data.pSocket->SendContent(HTTPInit, Out, Out.Length());
	} else {
		Data.pSocket->SendContent(HTTPInitGZ, gzipOut, gzipLen);
	}
	if (gzipOut != NULL)
		delete[] gzipOut;
}


wxString CWebServer::_ParseURLArray(ThreadData Data, wxString fieldname) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL) return("");
	
	wxString URL = Data.sURL;
	wxString res,temp;

	while (URL.Length()>0) {
		int pos=URL.MakeLower().Find(fieldname.MakeLower() +"=");
		if (pos>-1) {
			temp=_ParseURL(Data,fieldname);
			if (temp=="") break;
			res.Append(temp+"|");
			URL.Remove(pos,10);
		} else break;
	}
	return res;
}


wxString CWebServer::_ParseURL(ThreadData Data, wxString fieldname) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL) return("");
	
	wxString URL = Data.sURL;
	
	wxString value = "";
	wxString Parameter = "";
	char fromReplace[4] = "";	// decode URL
	char toReplace[2] = "";		// decode URL
	int i = 0;
	int findPos = -1;
	int findLength = 0;

	pThis->webInterface->Print("*** parsing url %s :: field %s\n", URL.GetData(), fieldname.GetData());
	if (URL.Find("?") > -1) {
		Parameter = URL.Mid(URL.Find("?")+1, URL.Length()-URL.Find("?")-1);

		// search the fieldname beginning / middle and strip the rest...
		if (Parameter.Find(fieldname + "=") == 0) {
			findPos = 0;
			findLength = fieldname.Length() + 1;
		}
		if (Parameter.Find("&" + fieldname + "=") > -1) {
			findPos = Parameter.Find("&" + fieldname + "=");
			findLength = fieldname.Length() + 2;
		}
		if (findPos > -1) {
			Parameter = Parameter.Mid(findPos + findLength, Parameter.Length());
			if (Parameter.Find("&") > -1) {
				Parameter = Parameter.Mid(0, Parameter.Find("&"));
			}
	
			value = Parameter;

			// decode value ...
			value.Replace("+", " ");
			for (i = 0 ; i <= 255 ; i++) {
				sprintf(fromReplace, "%%%02x", i);
				toReplace[0] = (char)i;
				toReplace[1] = 0;
				value.Replace(fromReplace, toReplace);
				sprintf(fromReplace, "%%%02X", i);
				value.Replace(fromReplace, toReplace);
			}
		}
	}
	pThis->webInterface->Print("*** URL parsed. returning %s\n",value.GetData());
	return value;
}


wxString CWebServer::_GetHeader(ThreadData Data, long lSession) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession; sSession.Printf("%ld", lSession);

	wxString Out = pThis->m_Templates.sHeader;

	Out.Replace("[CharSet]", _GetWebCharSet());

	//get page header data
	wxString sHeaderList = pThis->webInterface->SendRecvMsg("WEBPAGE HEADER");
	int brk=sHeaderList.First("\t");
	
	int nRefresh = atoi(sHeaderList.Left(brk).GetData());
	sHeaderList = sHeaderList.Mid(brk+1); brk=sHeaderList.First("\t");
	
	if (nRefresh) {
		wxString sPage = _ParseURL(Data, "w");
		if ((sPage == "transfer") || (sPage == "server") ||
			(sPage == "graphs") || (sPage == "log") ||
			(sPage == "sinfo") || (sPage == "debuglog") ||
			(sPage == "stats")) {
			wxString sT = pThis->m_Templates.sHeaderMetaRefresh;
			wxString sRefresh; sRefresh.Printf("%d", nRefresh);
			sT.Replace("[RefreshVal]", sRefresh);
			
			wxString catadd="";
			if (sPage == "transfer")
				catadd="&cat="+_ParseURL(Data, "cat");
			sT.Replace("[wCommand]", sPage+catadd);
			
			Out.Replace("[HeaderMeta]", sT);
		}
	}
	
	Out.Replace("[Session]", sSession);
	pThis->webInterface->Print("*** replaced session with %s\n", sSession.GetData());
	Out.Replace("[HeaderMeta]", ""); // In case there are no meta
	Out.Replace("[aMuleAppName]", "aMule");
	Out.Replace("[version]", VERSION); //shakraw - was CURRENT_VERSION_LONG);
	Out.Replace("[StyleSheet]", pThis->m_Templates.sHeaderStylesheet);
	Out.Replace("[WebControl]", _("Web Control Panel"));
	Out.Replace("[Transfer]", _("Transfer"));
	Out.Replace("[Server]", _("Server list"));
	Out.Replace("[Shared]", _("Shared Files"));
	Out.Replace("[Download]", _("ED2K Link(s)"));
	Out.Replace("[Graphs]", _("Graphs"));
	Out.Replace("[Log]", _("Log"));
	Out.Replace("[ServerInfo]", _("Serverinfo"));
	Out.Replace("[DebugLog]", _("Debug Log"));
	Out.Replace("[Stats]", _("Statistics"));
	Out.Replace("[Options]", _("&Preferences"));
	Out.Replace("[Logout]", _("Logout"));
	Out.Replace("[Search]", _("Search"));

	char HTTPTempC[100] = "";
	wxString sConnected = "";
	
	if (sHeaderList.Left(brk) == "Connected") {
		sHeaderList = sHeaderList.Mid(brk+1); brk=sHeaderList.First("\t");
		if (sHeaderList.Left(brk) == "High ID") 
			sConnected = _("Connected");
		else
			sConnected = wxString(_("Connected"))+" ("+wxString(_("Low ID"))+")";
		
		sHeaderList = sHeaderList.Mid(brk+1); brk=sHeaderList.First("\t");
		sConnected += ": " + sHeaderList.Left(brk);
		sHeaderList = sHeaderList.Mid(brk+1); brk=sHeaderList.First("\t");
		sprintf(HTTPTempC, "%s ", sHeaderList.Left(brk).GetData());
		sConnected += " [" + wxString(HTTPTempC) + _("users") + "]";
	} else if (sHeaderList.Left(brk) == "Connecting") {
		sConnected = _("Connecting");
	} else {
		sConnected = _("Disconnected");
		if (IsSessionAdmin(Data,sSession))
			sConnected += " (<small><a href=\"?ses=" + sSession + "&w=server&c=connect\">"+wxString(_("Connect to any server"))+"</a></small>)";
	}
	Out.Replace("[Connected]", "<b>"+wxString(_("Connection"))+":</b> "+sConnected);

	sHeaderList = sHeaderList.Mid(brk+1); brk=sHeaderList.First("\t");
	double ul_speed = atof(sHeaderList.Left(brk).GetData());
	sHeaderList = sHeaderList.Mid(brk+1); brk=sHeaderList.First("\t");
	double dl_speed = atof(sHeaderList.Left(brk).GetData());

	sprintf(HTTPTempC, _("Up: %.1f | Down: %.1f"), ul_speed, dl_speed);

	wxString sLimits;
	// EC 25-12-2003
	sHeaderList = sHeaderList.Mid(brk+1); brk=sHeaderList.First("\t");
	wxString MaxUpload = sHeaderList.Left(brk);
	sHeaderList = sHeaderList.Mid(brk+1);
	wxString MaxDownload = sHeaderList.Left(brk);
	
	if (MaxUpload == "65535")  MaxUpload = _("Unlimited");
	if (MaxDownload == "65535") MaxDownload = _("Unlimited");
	sLimits.Printf("%s/%s", MaxUpload.GetData(), MaxDownload.GetData());
	// EC Ends
	Out.Replace("[Speed]", "<b>" +wxString(_("Speed")) + ":</b> " + wxString(HTTPTempC) + "<small> (" + wxString(_("Limits")) + ": " + sLimits + ")</small>");

	return Out;
}


wxString CWebServer::_GetFooter(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	return pThis->m_Templates.sFooter;
}


wxString CWebServer::_GetServerList(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data, "ses");

	wxString sAddServerBox = "";

	wxString sCmd = _ParseURL(Data, "c");
	if (sCmd == "connect" && IsSessionAdmin(Data,sSession) ) {
		wxString sIP = _ParseURL(Data, "ip");
		if (sIP.IsEmpty()) {
			pThis->webInterface->SendRecvMsg("SERVER RE-CONNECT");
		} else {
			wxString sPort = _ParseURL(Data, "port");
			if (sPort.IsEmpty()) sPort = "4661"; //try default port
			_ConnectToServer(pThis, sIP, sPort);
		}
	} else if (sCmd == "disconnect" && IsSessionAdmin(Data,sSession)) {
		pThis->webInterface->SendRecvMsg("SERVER DISCONNECT");
	} else if (sCmd == "remove" && IsSessionAdmin(Data,sSession)) {
		wxString sIP = _ParseURL(Data, "ip");
		if (!sIP.IsEmpty()) {
			wxString sPort = _ParseURL(Data, "port");
			if (sPort.IsEmpty()) sPort = "4661"; //try default port
			_RemoveServer(pThis, sIP, sPort);
		}
	} else if (sCmd == "options") {
		sAddServerBox = _GetAddServerBox(Data);
	}
	
	wxString sSort = _ParseURL(Data, "sort");
	if (sSort != "") {
		if (sSort == "name")
			pThis->m_Params.ServerSort = SERVER_SORT_NAME;
		else if(sSort == "description")
			pThis->m_Params.ServerSort = SERVER_SORT_DESCRIPTION;
		else if(sSort == "ip")
			pThis->m_Params.ServerSort = SERVER_SORT_IP;
		else if(sSort == "users")
			pThis->m_Params.ServerSort = SERVER_SORT_USERS;
		else if(sSort == "files")
			pThis->m_Params.ServerSort = SERVER_SORT_FILES;

		if(_ParseURL(Data, "sortreverse") == "")
			pThis->m_Params.bServerSortReverse = false;
	}
	
	wxString sSortRev = _ParseURL(Data, "sortreverse");
	if (sSortRev != "") {
		pThis->m_Params.bServerSortReverse = (sSortRev == "true");
	}
	
	wxString sServerSortRev;
	if (pThis->m_Params.bServerSortReverse)
		sServerSortRev = "false";
	else
		sServerSortRev = "true";

	wxString Out = pThis->m_Templates.sServerList;
	Out.Replace("[ConnectedServerData]", _GetConnectedServer(Data));
	Out.Replace("[AddServerBox]", sAddServerBox);
	Out.Replace("[Session]", sSession);
	
	if (pThis->m_Params.ServerSort == SERVER_SORT_NAME)
		Out.Replace("[SortName]", "&sortreverse=" + sServerSortRev);
	else
		Out.Replace("[SortName]", "");
	
	if (pThis->m_Params.ServerSort == SERVER_SORT_DESCRIPTION)
		Out.Replace("[SortDescription]", "&sortreverse=" + sServerSortRev);
	else
		Out.Replace("[SortDescription]", "");
	
	if (pThis->m_Params.ServerSort == SERVER_SORT_IP)
		Out.Replace("[SortIP]", "&sortreverse=" + sServerSortRev);
	else
		Out.Replace("[SortIP]", "");
	
	if (pThis->m_Params.ServerSort == SERVER_SORT_USERS)
		Out.Replace("[SortUsers]", "&sortreverse=" + sServerSortRev);
	else
		Out.Replace("[SortUsers]", "");
	
	if (pThis->m_Params.ServerSort == SERVER_SORT_FILES)
		Out.Replace("[SortFiles]", "&sortreverse=" + sServerSortRev);
	else
		Out.Replace("[SortFiles]", "");
	
	Out.Replace("[ServerList]", _("Server list"));
	Out.Replace("[Servername]", _("Server name"));
	Out.Replace("[Description]", _("Description"));
	Out.Replace("[Address]", _("IP"));
	Out.Replace("[Connect]", _("Connect"));
	Out.Replace("[Users]", _("users"));
	Out.Replace("[Files]", _("files"));
	Out.Replace("[Actions]", _("Actions"));
		
	wxString OutE = pThis->m_Templates.sServerLine;
	OutE.Replace("[Connect]", _("Connect"));
	OutE.Replace("[RemoveServer]", _("Remove selected server"));
	OutE.Replace("[ConfirmRemove]", _("Are you sure to remove this server from list?"));

	ArrayOfServerEntry ServerArray;

	// Populating array
	wxString sServerList = pThis->webInterface->SendRecvMsg("SERVER LIST");
	wxString sEntry;
	int brk=0, newLinePos;
	while (sServerList.Length() > 0) {
		newLinePos=sServerList.Find("\n");
		
		sEntry = sServerList.Left(newLinePos);
		sServerList = sServerList.Mid(newLinePos+1);

		ServerEntry* Entry = new ServerEntry;

		brk=sEntry.First("\t");
		Entry->sServerName = _SpecialChars(sEntry.Left(brk));
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		Entry->sServerDescription = _SpecialChars(sEntry.Left(brk));
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		Entry->nServerPort = atoi(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		Entry->sServerIP = sEntry.Left(brk);
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		Entry->nServerUsers = atoi(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		Entry->nServerMaxUsers = atoi(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1);
		Entry->nServerFiles = atoi(sEntry.GetData());
		
		ServerArray.Add(Entry);
	}

	// Sorting (simple bubble sort, we don't have tons of data here)
	bool bSorted = true;
	for (size_t nMax = 0;bSorted && nMax < ServerArray.GetCount()*2; nMax++) {
		bSorted = false;
		for (size_t i = 0; i < ServerArray.GetCount() - 1; i++) {
			bool bSwap = false;
			switch(pThis->m_Params.ServerSort) {
				case SERVER_SORT_NAME:
					bSwap = ServerArray[i]->sServerName.CmpNoCase(ServerArray[i+1]->sServerName) > 0;
					break;
				case SERVER_SORT_DESCRIPTION:
					bSwap = ServerArray[i]->sServerDescription.CmpNoCase(ServerArray[i+1]->sServerDescription) < 0;
					break;
				case SERVER_SORT_IP:
					bSwap = ServerArray[i]->sServerIP.CmpNoCase(ServerArray[i+1]->sServerIP) > 0;
					break;
				case SERVER_SORT_USERS:
					bSwap = ServerArray[i]->nServerUsers < ServerArray[i+1]->nServerUsers;
					break;
				case SERVER_SORT_FILES:
					bSwap = ServerArray[i]->nServerFiles < ServerArray[i+1]->nServerFiles;
					break;
			}
			
			if (pThis->m_Params.bServerSortReverse) {
				bSwap = !bSwap;
			}
			
			if (bSwap) {
				bSorted = true;
				ServerEntry* TmpEntry = ServerArray[i];
				ServerArray[i] = ServerArray[i+1];
				ServerArray[i+1] = TmpEntry;
			}
		}
	}
	
	// Displaying
	wxString sList = "";
	for (size_t i = 0; i < ServerArray.GetCount(); i++) {
		wxString HTTPProcessData = OutE; // Copy Entry Line to Temp
		HTTPProcessData.Replace("[1]", ServerArray[i]->sServerName);
		HTTPProcessData.Replace("[2]", ServerArray[i]->sServerDescription);
		wxString sPort; sPort.Printf(":%d", ServerArray[i]->nServerPort);
		HTTPProcessData.Replace("[3]", ServerArray[i]->sServerIP + sPort);
		
		wxString sT;
		if (ServerArray[i]->nServerUsers > 0) {
			if (ServerArray[i]->nServerMaxUsers > 0)
				sT.Printf("%d (%d)", ServerArray[i]->nServerUsers, ServerArray[i]->nServerMaxUsers);
			else
				sT.Printf("%d", ServerArray[i]->nServerUsers);
		}
		
		HTTPProcessData.Replace("[4]", sT);
		sT = "";
		if (ServerArray[i]->nServerFiles > 0)
			sT.Printf("%d", ServerArray[i]->nServerFiles);
		
		HTTPProcessData.Replace("[5]", sT);
		
		wxString sServerPort; sServerPort.Printf("%d", ServerArray[i]->nServerPort);
		
		HTTPProcessData.Replace("[6]", IsSessionAdmin(Data,sSession)? wxString("?ses=" + sSession + "&w=server&c=connect&ip=" + ServerArray[i]->sServerIP+"&port="+sServerPort):GetPermissionDenied());
		HTTPProcessData.Replace("[LinkRemove]", IsSessionAdmin(Data,sSession)?wxString("?ses=" + sSession + "&w=server&c=remove&ip=" + ServerArray[i]->sServerIP+"&port="+sServerPort):GetPermissionDenied());

		sList += HTTPProcessData;
	}
	Out.Replace("[ServersList]", sList);

	return Out;
}


wxString CWebServer::_GetTransferList(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data, "ses");
	int cat=atoi(_ParseURL(Data,"cat"));

	bool clcompl=(_ParseURL(Data,"ClCompl")=="yes" );
	wxString sCat = "";	if (cat!=0) sCat.Printf("&cat=%i",cat);

	wxString Out = "";

	if (clcompl && IsSessionAdmin(Data,sSession)) {
		pThis->webInterface->SendRecvMsg("TRANSFER CLEARCOMPLETE");
	}
	
	if (_ParseURL(Data, "c") != "" && IsSessionAdmin(Data,sSession)) {
		wxString HTTPTemp = _ParseURL(Data, "c");
		if (HTTPTemp.Right(1) != "/")
			HTTPTemp += "/";

		wxString request = wxString("TRANSFER ADDFILELINK ") + HTTPTemp;
		if (pThis->webInterface->SendRecvMsg(request) == "Bad Link") {
			char HTTPTempC[100] = "";
			sprintf(HTTPTempC,_("This ed2k link is invalid (%s)"), HTTPTemp.GetData());//error.GetData());
			Out += pThis->m_Templates.sTransferBadLink;
			Out.Replace("[InvalidLink]", HTTPTempC);
			Out.Replace("[Link]", HTTPTemp);
		}
	}

	if (_ParseURL(Data, "op") != "" && _ParseURL(Data, "file") != "") {
		uchar FileHash[16];
		_GetFileHash(_ParseURL(Data, "file"), FileHash);

		int found_file_index = -1;

		int dlFilePosition=0;
		//sFileHashes formatted as: %s\t%s\t....\t%s
		wxString sFileHashes = pThis->webInterface->SendRecvMsg("TRANSFER DL_FILEHASH");
		wxString sEntry;
		int tabPos;
		while (sFileHashes.Length()>0) {
			tabPos=sFileHashes.First("\t");
			sEntry = sFileHashes.Left(tabPos);
			sFileHashes = sFileHashes.Mid(tabPos+1);
			
			uchar filehash[16];
			for (int k = 0; k < 16; k++)
				filehash[k] = (uchar) sEntry.GetChar(k);

			bool bGood = true;
			for (int i = 0; i < 16; i++) {
				if ( filehash[i] != FileHash[i]) {
					bGood = false;
					break;
				}
			}
			
			if (bGood) {
				found_file_index = dlFilePosition;
				break;
			}			
			
			dlFilePosition++;
		}
		
		if (_ParseURL(Data, "op") == "pause" && IsSessionAdmin(Data,sSession)) {
			if (found_file_index >= 0) 
				pThis->webInterface->SendRecvMsg(wxString::Format("TRANSFER DL_FILEPAUSE %i", found_file_index));
		} else if (_ParseURL(Data, "op") == "resume" && IsSessionAdmin(Data,sSession)) {
			if (found_file_index >= 0)
				pThis->webInterface->SendRecvMsg(wxString::Format("TRANSFER DL_FILERESUME %i", found_file_index));
		} else if (_ParseURL(Data, "op") == "cancel" && IsSessionAdmin(Data,sSession)) {
			if (found_file_index >= 0)
				pThis->webInterface->SendRecvMsg(wxString::Format("TRANSFER DL_FILEDELETE %i", found_file_index));
		} else if (_ParseURL(Data, "op") == "prioup" && IsSessionAdmin(Data,sSession)) {
			if (found_file_index >= 0) {
				pThis->webInterface->SendRecvMsg(wxString::Format("TRANSFER DL_FILEPRIOUP %d", found_file_index));
			}
		} else if (_ParseURL(Data, "op") == "priodown" && IsSessionAdmin(Data,sSession)) {
			if (found_file_index >= 0) {
				pThis->webInterface->SendRecvMsg(wxString::Format("TRANSFER DL_FILEPRIODOWN %d", found_file_index));
			}
		}
	}

	if (_ParseURL(Data, "sort") != "") {
		if (_ParseURL(Data, "sort") == "name")
			pThis->m_Params.DownloadSort = DOWN_SORT_NAME;
		else if (_ParseURL(Data, "sort") == "size")
			pThis->m_Params.DownloadSort = DOWN_SORT_SIZE;
		else if (_ParseURL(Data, "sort") == "transferred")
			pThis->m_Params.DownloadSort = DOWN_SORT_TRANSFERRED;
		else if (_ParseURL(Data, "sort") == "speed")
			pThis->m_Params.DownloadSort = DOWN_SORT_SPEED;
		else if (_ParseURL(Data, "sort") == "progress")
			pThis->m_Params.DownloadSort = DOWN_SORT_PROGRESS;

		if (_ParseURL(Data, "sortreverse") == "")
			pThis->m_Params.bDownloadSortReverse = false;
	}
	
	if (_ParseURL(Data, "sortreverse") != "") {
		pThis->m_Params.bDownloadSortReverse = (_ParseURL(Data, "sortreverse") == "true");
	} 
	
	if (_ParseURL(Data, "showuploadqueue") == "true") {
		pThis->m_Params.bShowUploadQueue = true;
	}
	
	if (_ParseURL(Data, "showuploadqueue") == "false") {
		pThis->m_Params.bShowUploadQueue = false;
	}
	
	wxString sDownloadSortRev;
	if (pThis->m_Params.bDownloadSortReverse)
		sDownloadSortRev = "false";
	else
		sDownloadSortRev = "true";

	Out += pThis->m_Templates.sTransferImages;
	Out += pThis->m_Templates.sTransferList;
	Out.Replace("[DownloadHeader]", pThis->m_Templates.sTransferDownHeader);
	Out.Replace("[DownloadFooter]", pThis->m_Templates.sTransferDownFooter);
	Out.Replace("[UploadHeader]", pThis->m_Templates.sTransferUpHeader);
	Out.Replace("[UploadFooter]", pThis->m_Templates.sTransferUpFooter);
	Out.Replace("[Session]", sSession);

	InsertCatBox(Out,cat,"",true,true);
	
	if (pThis->m_Params.DownloadSort == DOWN_SORT_NAME)
		Out.Replace("[SortName]", "&sortreverse=" + sDownloadSortRev);
	else
		Out.Replace("[SortName]", "");
	
	if (pThis->m_Params.DownloadSort == DOWN_SORT_SIZE)
		Out.Replace("[SortSize]", "&sortreverse=" + sDownloadSortRev);
	else
		Out.Replace("[SortSize]", "");
	
	if (pThis->m_Params.DownloadSort == DOWN_SORT_TRANSFERRED)
		Out.Replace("[SortTransferred]", "&sortreverse=" + sDownloadSortRev);
	else
		Out.Replace("[SortTransferred]", "");
	
	if (pThis->m_Params.DownloadSort == DOWN_SORT_SPEED)
		Out.Replace("[SortSpeed]", "&sortreverse=" + sDownloadSortRev);
	else
		Out.Replace("[SortSpeed]", "");
	
	if (pThis->m_Params.DownloadSort == DOWN_SORT_PROGRESS)
		Out.Replace("[SortProgress]", "&sortreverse=" + sDownloadSortRev);
	else
		Out.Replace("[SortProgress]", "");
	
	Out.Replace("[Filename]", _("File Name"));
	Out.Replace("[Size]", _("Size"));
	Out.Replace("[Transferred]", _("Complete"));
	Out.Replace("[Progress]", _("Progress"));
	Out.Replace("[Speed]", _("Speed"));
	Out.Replace("[Sources]", _("Sources"));
	Out.Replace("[Actions]", _("Actions"));
	Out.Replace("[User]", _("Username"));
	Out.Replace("[TotalDown]", _("Downloaded total"));
	Out.Replace("[TotalUp]", _("Uploaded total"));
	Out.Replace("[Prio]", _("Priority"));
	Out.Replace("[CatSel]",sCat);
	wxString OutE = pThis->m_Templates.sTransferDownLine;
	wxString OutE2 = pThis->m_Templates.sTransferDownLineGood;

	float fTotalSize = 0, fTotalTransferred = 0, fTotalSpeed = 0;
	ArrayOfDownloadFiles FilesArray;

	// Populating array
	wxString sTransferDLList = pThis->webInterface->SendRecvMsg("TRANSFER DL_LIST");
	bool completedAv=false;
	wxString sEntry;
	int newLinePos, brk=0;
	while (sTransferDLList.Length()>0) {
		newLinePos=sTransferDLList.First("\n");

		sEntry = sTransferDLList.Left(newLinePos);
		sTransferDLList = sTransferDLList.Mid(newLinePos+1);

		DownloadFiles *dFile = new DownloadFiles;

		brk=sEntry.First("\t");
		dFile->sFileName = _SpecialChars(sEntry.Left(brk));
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->lFileSize = atol(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->lFileTransferred = atol(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->fCompleted = atof(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->lFileSpeed = atol(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFileStatus = atoi(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->sFileStatus = sEntry.Left(brk);
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFilePrio = atoi(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->sFileHash = sEntry.Left(brk);
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->lSourceCount = atol(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->lNotCurrentSourceCount = atol(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->lTransferringSourceCount = atol(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->sED2kLink = sEntry.Left(brk);
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->sFileInfo = _SpecialChars(sEntry.Left(brk));
		sEntry = sEntry.Mid(brk+1);
		completedAv = (atoi(sEntry.GetData()) == 0) ? false : true;

		FilesArray.Add(dFile);

#if 0
		//categories
			if (cat>0 && cur_file->GetCategory()!=cat) continue;
			if (cat<0) {
				switch (cat) {
					case -1 : if (cur_file->GetCategory()!=0) continue; break;
					case -2 : if (!cur_file->IsPartFile()) continue; break;
					case -3 : if (cur_file->IsPartFile()) continue; break;
					case -4 : if (!((cur_file->GetStatus()==PS_READY|| cur_file->GetStatus()==PS_EMPTY) && cur_file->GetTransferingSrcCount()==0)) continue; break;
					case -5 : if (!((cur_file->GetStatus()==PS_READY|| cur_file->GetStatus()==PS_EMPTY) && cur_file->GetTransferingSrcCount()>0)) continue; break;
					case -6 : if (cur_file->GetStatus()!=PS_ERROR) continue; break;
					case -7 : if (cur_file->GetStatus()!=PS_PAUSED) continue; break;
					case -8 : if (!cur_file->IsStopped()) continue; break;
					case -9 : if (!cur_file->IsMovie()) continue; break;
					case -10 : if (ED2KFT_AUDIO != GetED2KFileTypeID(cur_file->GetFileName())) continue; break;
					case -11 : if (!cur_file->IsArchive()) continue; break;
					case -12 : if (ED2KFT_CDIMAGE != GetED2KFileTypeID(cur_file->GetFileName())) continue; break;
				}
			}
#endif			

	}
	
	// Sorting (simple bubble sort, we don't have tons of data here)
	bool bSorted = true;
	for (size_t nMax = 0;bSorted && nMax < FilesArray.GetCount()*2; nMax++) {
		bSorted = false;
		for (size_t i = 0; i < FilesArray.GetCount() - 1; i++) {
			bool bSwap = false;
			switch (pThis->m_Params.DownloadSort) {
				case DOWN_SORT_NAME:
					bSwap = FilesArray[i]->sFileName.CmpNoCase(FilesArray[i+1]->sFileName) > 0;
					break;
				case DOWN_SORT_SIZE:
					bSwap = FilesArray[i]->lFileSize < FilesArray[i+1]->lFileSize;
					break;
				case DOWN_SORT_TRANSFERRED:
					bSwap = FilesArray[i]->lFileTransferred < FilesArray[i+1]->lFileTransferred;
					break;
				case DOWN_SORT_SPEED:
					bSwap = FilesArray[i]->lFileSpeed < FilesArray[i+1]->lFileSpeed;
					break;
				case DOWN_SORT_PROGRESS:
					bSwap = FilesArray[i]->fCompleted  < FilesArray[i+1]->fCompleted ;
					break;
			}
			
			if (pThis->m_Params.bDownloadSortReverse) {
				bSwap = !bSwap;
			}
			
			if (bSwap) {
				bSorted = true;
				DownloadFiles* TmpFile = FilesArray[i];
				FilesArray[i] = FilesArray[i+1];
				FilesArray[i+1] = TmpFile;
			}
		}
	}
	
	// Displaying
	wxString sDownList = "";
	wxString HTTPTemp;

	for (size_t i = 0; i < FilesArray.GetCount(); i++) {
		wxString JSfileinfo=FilesArray[i]->sFileInfo;
		//JSfileinfo.Replace("\n","\\n");
		JSfileinfo.Replace("|","\\n");
		wxString sActions = "<acronym title=\"" + FilesArray[i]->sFileStatus + "\"><a href=\"javascript:alert(\'"+ JSfileinfo+"')\"><img src=\"l_info.gif\" alt=\"" + FilesArray[i]->sFileStatus + "\"></a></acronym> ";

		wxString sED2kLink;
		sED2kLink.Printf("<acronym title=\"[Ed2klink]\"><a href=\""+ FilesArray[i]->sED2kLink +"\"><img src=\"l_ed2klink.gif\" alt=\"[Ed2klink]\"></a></acronym> ");
		sED2kLink.Replace("[Ed2klink]", _("ED2K Link(s)"));
		sActions += sED2kLink;

		bool bCanBeDeleted = true;
		switch (FilesArray[i]->nFileStatus) {
			case PS_COMPLETING:
			case PS_COMPLETE:
				bCanBeDeleted = false;
				break;
			case PS_HASHING: 
			case PS_WAITINGFORHASH:
			case PS_ERROR:
				break;
			case PS_PAUSED:
				if (IsSessionAdmin(Data,sSession)) {
					wxString sResume;
					sResume.Printf("<acronym title=\"[Resume]\"><a href=\"[Link]\"><img src=\"l_resume.gif\" alt=\"[Resume]\"></a></acronym> ");
					sResume.Replace("[Link]", IsSessionAdmin(Data,sSession)?wxString("?ses=" + sSession + "&w=transfer&op=resume&file=" + FilesArray[i]->sFileHash):GetPermissionDenied()) ;
					sActions += sResume;
				}
				break; 
			default: // waiting or downloading
				if (IsSessionAdmin(Data,sSession)) {
					wxString sPause;
					sPause.Printf("<acronym title=\"[Pause]\"><a href=\"[Link]\"><img src=\"l_pause.gif\" alt=\"[Pause]\"></a></acronym> ");
					sPause.Replace("[Link]", IsSessionAdmin(Data,sSession)?wxString("?ses=" + sSession + "&w=transfer&op=pause&file=" + FilesArray[i]->sFileHash):GetPermissionDenied());
					sActions += sPause;
				}
				break;
		}
		
		if (bCanBeDeleted) {
			if (IsSessionAdmin(Data,sSession)) {
				wxString sCancel;
				sCancel.Printf("<acronym title=\"[Cancel]\"><a href=\"[Link]\" onclick=\"return confirm(\'[ConfirmCancel]\')\"><img src=\"l_cancel.gif\" alt=\"[Cancel]\"></a></acronym> ");
				sCancel.Replace("[Link]", IsSessionAdmin(Data,sSession)?wxString("?ses=" + sSession + "&w=transfer&op=cancel&file=" + FilesArray[i]->sFileHash):GetPermissionDenied());
				sActions += sCancel;
			}
		}
		
		if (IsSessionAdmin(Data,sSession)) {
			sActions.Replace("[Resume]", _("&Resume"));
			sActions.Replace("[Pause]", _("&Pause"));
			sActions.Replace("[Cancel]", _("Cancel"));
			sActions.Replace("[ConfirmCancel]", _("Are you sure that you want to cancel and delete this file?\\n"));

			if (FilesArray[i]->nFileStatus!=PS_COMPLETE && FilesArray[i]->nFileStatus!=PS_COMPLETING) {
				sActions.Append("<acronym title=\"[PriorityUp]\"><a href=\"?ses=[Session]&amp;w=transfer&op=prioup&file=" + FilesArray[i]->sFileHash+sCat+"\"><img src=\"l_up.gif\" alt=\"[PriorityUp]\"></a></acronym>");
				sActions.Append("&nbsp;<acronym title=\"[PriorityDown]\"><a href=\"?ses=[Session]&amp;w=transfer&op=priodown&file=" + FilesArray[i]->sFileHash+sCat +"\"><img src=\"l_down.gif\" alt=\"[PriorityDown]\"></a></acronym>");
			}
		}
		
		wxString HTTPProcessData;
		// if downloading, draw in other color
		if (FilesArray[i]->lFileSpeed > 0)
			HTTPProcessData = OutE2;
		else
			HTTPProcessData = OutE;

		if (FilesArray[i]->sFileName.Length() > SHORT_FILENAME_LENGTH)
			HTTPProcessData.Replace("[ShortFileName]", FilesArray[i]->sFileName.Left(SHORT_FILENAME_LENGTH) + "...");
		else
			HTTPProcessData.Replace("[ShortFileName]", FilesArray[i]->sFileName);

		HTTPProcessData.Replace("[FileInfo]", FilesArray[i]->sFileInfo);

		fTotalSize += FilesArray[i]->lFileSize;

		HTTPProcessData.Replace("[2]", castItoXBytes(FilesArray[i]->lFileSize));

		if (FilesArray[i]->lFileTransferred > 0) {
			fTotalTransferred += FilesArray[i]->lFileTransferred;

			HTTPProcessData.Replace("[3]", castItoXBytes(FilesArray[i]->lFileTransferred));
		} else
			HTTPProcessData.Replace("[3]", "-");

		HTTPProcessData.Replace("[DownloadBar]", _GetDownloadGraph(Data,FilesArray[i]->sFileHash));

		if (FilesArray[i]->lFileSpeed > 0.0f) {
			fTotalSpeed += FilesArray[i]->lFileSpeed;

			HTTPTemp.Printf("%8.2f %s", FilesArray[i]->lFileSpeed/1024.0 ,_("kB/s"));
			HTTPProcessData.Replace("[4]", HTTPTemp);
		} else
			HTTPProcessData.Replace("[4]", "-");
		
		if (FilesArray[i]->lSourceCount > 0) {
			HTTPTemp.Printf("%li&nbsp;/&nbsp;%8li&nbsp;(%li)",
				FilesArray[i]->lSourceCount-FilesArray[i]->lNotCurrentSourceCount,
				FilesArray[i]->lSourceCount,
				FilesArray[i]->lTransferringSourceCount
			);
			HTTPProcessData.Replace("[5]", HTTPTemp);
		} else
			HTTPProcessData.Replace("[5]", "-");
		
		switch (FilesArray[i]->nFilePrio) {
			case 0: HTTPTemp=_("Low");break;
			case 10: HTTPTemp=_("Auto [Lo]");break;

			case 1: HTTPTemp=_("Normal");break;
			case 11: HTTPTemp=_("Auto [No]");break;

			case 2: HTTPTemp=_("High");break;
			case 12: HTTPTemp=_("Auto [Hi]");break;
			//shakraw - it seems there is a problem with dl file priorities
			//i.e. I've got fileprio=3, VERYHIGH, but we can't set this priority 
			//in dl window. so, why fileprio=3?
			default: HTTPTemp="-"; break; 
		}
	
		HTTPProcessData.Replace("[PrioVal]", HTTPTemp);
		HTTPProcessData.Replace("[6]", sActions);

		sDownList += HTTPProcessData;
	}

	Out.Replace("[DownloadFilesList]", sDownList);
	Out.Replace("[PriorityUp]", _("Increase Priority"));
	Out.Replace("[PriorityDown]", _("Decrease Priority"));
	// Elandal: cast from float to integral type always drops fractions.
	// avoids implicit cast warning
	Out.Replace("[TotalDownSize]", castItoXBytes((uint64)fTotalSize));
	Out.Replace("[TotalDownTransferred]", castItoXBytes((uint64)fTotalTransferred));
	
	Out.Replace("[ClearCompletedButton]",(completedAv && IsSessionAdmin(Data,sSession))?pThis->m_Templates.sClearCompleted :wxString(wxT("")));

	HTTPTemp.Printf("%8.2f %s", fTotalSpeed/1024.0,_("kB/s"));
	Out.Replace("[TotalDownSpeed]", HTTPTemp);
	OutE = pThis->m_Templates.sTransferUpLine;
	
	HTTPTemp.Printf("%i",pThis->m_Templates.iProgressbarWidth);
	Out.Replace("[PROGRESSBARWIDTHVAL]",HTTPTemp);

	fTotalSize = 0;
	fTotalTransferred = 0;
	fTotalSpeed = 0;

	wxString sUpList = "";

	//upload list
	wxString sTransferULList = pThis->webInterface->SendRecvMsg("TRANSFER UL_LIST");
	wxString HTTPProcessData;
	float transfDown, transfUp, transfDatarate; 
	while (sTransferULList.Length()>0) {
		newLinePos=sTransferULList.First("\n");

		sEntry = sTransferULList.Left(newLinePos);
		sTransferULList = sTransferULList.Mid(newLinePos+1);

		HTTPProcessData = OutE;

		brk=sEntry.First("\t");
		HTTPProcessData.Replace("[1]", _SpecialChars(sEntry.Left(brk)));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		HTTPProcessData.Replace("[FileInfo]", _SpecialChars(sEntry.Left(brk)));		
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		HTTPProcessData.Replace("[2]", _SpecialChars(sEntry.Left(brk)));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		
		transfDown = atoi(sEntry.Left(brk).GetData());
		fTotalSize += transfDown;
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		
		transfUp = (float) atoi(sEntry.Left(brk).GetData());
		fTotalTransferred += transfUp;
		sEntry=sEntry.Mid(brk+1);

		wxString HTTPTemp;
		HTTPTemp.Printf("%s / %s", castItoXBytes((uint64)transfDown).GetData(),castItoXBytes((uint64)transfUp).GetData());
		HTTPProcessData.Replace("[3]", HTTPTemp);

		transfDatarate = (float) atoi(sEntry.GetData());
		fTotalSpeed += transfDatarate;
		
		HTTPTemp.Printf("%8.2f " +wxString(_("kB/s")), transfDatarate/1024.0);
		HTTPProcessData.Replace("[4]", HTTPTemp);
		
		sUpList += HTTPProcessData;
	}
	
	Out.Replace("[UploadFilesList]", sUpList);
	// Elandal: cast from float to integral type always drops fractions.
	// avoids implicit cast warning
	HTTPTemp.Printf("%s / %s", castItoXBytes((uint64)fTotalSize).GetData(), castItoXBytes((uint64)fTotalTransferred).GetData());
	Out.Replace("[TotalUpTransferred]", HTTPTemp);
	HTTPTemp.Printf("%8.2f " +wxString(_("kB/s")), fTotalSpeed/1024.0);
	Out.Replace("[TotalUpSpeed]", HTTPTemp);

	if (pThis->m_Params.bShowUploadQueue) {
		Out.Replace("[UploadQueue]", pThis->m_Templates.sTransferUpQueueShow);
		Out.Replace("[UploadQueueList]", _("On Queue"));
		Out.Replace("[UserNameTitle]", _("Username"));
		Out.Replace("[FileNameTitle]", _("File Name"));
		Out.Replace("[ScoreTitle]", _("Score"));
		Out.Replace("[BannedTitle]", _("Banned"));

		OutE = pThis->m_Templates.sTransferUpQueueLine;
		// Replace [xx]
		wxString sQueue = "";

		//waiting list
		wxString sTransferWList = pThis->webInterface->SendRecvMsg("TRANSFER W_LIST");
		while (sTransferWList.Length()>0) {
			newLinePos=sTransferWList.First("\n");

			sEntry = sTransferWList.Left(newLinePos);
			sTransferWList = sTransferWList.Mid(newLinePos+1);

			char HTTPTempC[100] = "";
			HTTPProcessData = OutE;

			brk=sTransferWList.First("\t");
			if (brk==-1) {
				HTTPProcessData.Replace("[UserName]", _SpecialChars(sEntry));
				continue;
			}
			HTTPProcessData.Replace("[UserName]", _SpecialChars(sEntry.Left(brk)));
			sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
			HTTPProcessData.Replace("[FileName]", _SpecialChars(sEntry.Left(brk)));
			sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
			sprintf(HTTPTempC, "%i" , atoi(sEntry.Left(brk).GetData()));
			sEntry=sEntry.Mid(brk+1);
			
			wxString HTTPTemp = HTTPTempC;
			HTTPProcessData.Replace("[Score]", HTTPTemp);

			if (atoi(sEntry.GetData()))
				HTTPProcessData.Replace("[Banned]", _("Yes"));
			else
				HTTPProcessData.Replace("[Banned]", _("No"));
		}
		
		Out.Replace("[QueueList]", sQueue);
	} else {
		Out.Replace("[UploadQueue]", pThis->m_Templates.sTransferUpQueueHide);
	}

	Out.Replace("[ShowQueue]", _("Show Queue"));
	Out.Replace("[HideQueue]", _("Hide Queue"));
	Out.Replace("[Session]", sSession);
	Out.Replace("[CLEARCOMPLETED]",_("C&lear completed"));

	wxString buffer;
	buffer.Printf("%s (%i)", _("Downloads"),FilesArray.GetCount());
	Out.Replace("[DownloadList]",buffer);
	buffer.Printf("%s (%i)",_("Upload"),atoi(pThis->webInterface->SendRecvMsg("QUEUE UL_GETLENGTH")));
	Out.Replace("[UploadList]", buffer );
	Out.Replace("[CatSel]",sCat);

	return Out;
}


wxString CWebServer::_GetDownloadLink(ThreadData Data) {
#if 0 //shakraw
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data, "ses");

	if (!IsSessionAdmin(Data,sSession)) {
		wxString ad="<br><br><div align=\"center\" class=\"message\">[Message]</div>";
		ad.Replace("[Message]",_("Access denied!"));
		return ad;
	}
	
	wxString Out = pThis->m_Templates.sDownloadLink;

	Out.Replace("[Download]", _("Download Selected"));
	Out.Replace("[Ed2klink]", _("ED2K Link(s)"));
	Out.Replace("[Start]", _("Start"));
	Out.Replace("[Session]", sSession);

	if (theApp.glob_prefs->GetCatCount()>1)
		InsertCatBox(Out,0, pThis->m_Templates.sCatArrow );
	else Out.Replace("[CATBOX]","");

	return Out;
#endif
	return "";
}


wxString CWebServer::_GetSharedFilesList(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data, "ses");
	
	if (_ParseURL(Data, "sort") != "")  {
		if(_ParseURL(Data, "sort") == "name")
			pThis->m_Params.SharedSort = SHARED_SORT_NAME;
		else if(_ParseURL(Data, "sort") == "size")
			pThis->m_Params.SharedSort = SHARED_SORT_SIZE;
		else if(_ParseURL(Data, "sort") == "transferred")
			pThis->m_Params.SharedSort = SHARED_SORT_TRANSFERRED;
		else if(_ParseURL(Data, "sort") == "alltimetransferred")
			pThis->m_Params.SharedSort = SHARED_SORT_ALL_TIME_TRANSFERRED;
		else if(_ParseURL(Data, "sort") == "requests")
			pThis->m_Params.SharedSort = SHARED_SORT_REQUESTS;
		else if(_ParseURL(Data, "sort") == "alltimerequests")
			pThis->m_Params.SharedSort = SHARED_SORT_ALL_TIME_REQUESTS;
		else if(_ParseURL(Data, "sort") == "accepts")
			pThis->m_Params.SharedSort = SHARED_SORT_ACCEPTS;
		else if(_ParseURL(Data, "sort") == "alltimeaccepts")
			pThis->m_Params.SharedSort = SHARED_SORT_ALL_TIME_ACCEPTS;
		else if(_ParseURL(Data, "sort") == "priority")
			pThis->m_Params.SharedSort = SHARED_SORT_PRIORITY;

		if(_ParseURL(Data, "sortreverse") == "")
			pThis->m_Params.bSharedSortReverse = false;
	}
	
	if (_ParseURL(Data, "sortreverse") != "") 
		pThis->m_Params.bSharedSortReverse = (_ParseURL(Data, "sortreverse") == "true");

	if (_ParseURL(Data, "hash") != "" && _ParseURL(Data, "setpriority") != "" && IsSessionAdmin(Data,sSession)) 
		_SetSharedFilePriority(pThis, _ParseURL(Data, "hash"),atoi(_ParseURL(Data, "setpriority")));

	if (_ParseURL(Data, "reload") == "true") {
		pThis->webInterface->SendRecvMsg("SHAREDFILES RELOAD");
	}

	wxString sSharedSortRev;
	if (pThis->m_Params.bSharedSortReverse) 
		sSharedSortRev = "false";
	else
		sSharedSortRev = "true";
	
	//Name sorting link
	wxString Out = pThis->m_Templates.sSharedList;
	if (pThis->m_Params.SharedSort == SHARED_SORT_NAME)
		Out.Replace("[SortName]", "sort=name&sortreverse=" + sSharedSortRev);
	else
		Out.Replace("[SortName]", "sort=name");
	
	//Size sorting Link
	if (pThis->m_Params.SharedSort == SHARED_SORT_SIZE)
		Out.Replace("[SortSize]", "sort=size&sortreverse=" + sSharedSortRev);
	else
		Out.Replace("[SortSize]", "sort=size");
	
	//Priority sorting Link
	if (pThis->m_Params.SharedSort == SHARED_SORT_PRIORITY)
		Out.Replace("[SortPriority]", "sort=priority&sortreverse=" + sSharedSortRev);
	else
		Out.Replace("[SortPriority]", "sort=priority");
	
	//Transferred sorting link
	if (pThis->m_Params.SharedSort == SHARED_SORT_TRANSFERRED) {
		if (pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortTransferred]", "sort=alltimetransferred&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortTransferred]", "sort=transferred&sortreverse=" + sSharedSortRev);
	} else if(pThis->m_Params.SharedSort == SHARED_SORT_ALL_TIME_TRANSFERRED) {
		if (pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortTransferred]", "sort=transferred&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortTransferred]", "sort=alltimetransferred&sortreverse=" + sSharedSortRev);
	} else
        Out.Replace("[SortTransferred]", "&sort=transferred&sortreverse=false");
	
	//Request sorting link
	if (pThis->m_Params.SharedSort == SHARED_SORT_REQUESTS) {
		if (pThis->m_Params.bSharedSortReverse)
			Out.Replace("[SortRequests]", "sort=alltimerequests&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortRequests]", "sort=requests&sortreverse=" + sSharedSortRev);
	} else if(pThis->m_Params.SharedSort == SHARED_SORT_ALL_TIME_REQUESTS) {
		if (pThis->m_Params.bSharedSortReverse)
			Out.Replace("[SortRequests]", "sort=requests&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortRequests]", "sort=alltimerequests&sortreverse=" + sSharedSortRev);
	} else
        	Out.Replace("[SortRequests]", "&sort=requests&sortreverse=false");
	
	//Accepts sorting link
	if (pThis->m_Params.SharedSort == SHARED_SORT_ACCEPTS) {
		if (pThis->m_Params.bSharedSortReverse)
			Out.Replace("[SortAccepts]", "sort=alltimeaccepts&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortAccepts]", "sort=accepts&sortreverse=" + sSharedSortRev);
	} else if (pThis->m_Params.SharedSort == SHARED_SORT_ALL_TIME_ACCEPTS) {
		if (pThis->m_Params.bSharedSortReverse)
			Out.Replace("[SortAccepts]", "sort=accepts&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortAccepts]", "sort=alltimeaccepts&sortreverse=" + sSharedSortRev);
	} else
		Out.Replace("[SortAccepts]", "&sort=accepts&sortreverse=false");

	if (_ParseURL(Data, "reload") == "true") {
#warning fix GetLastLogEntry()
		wxString resultlog = ""; //_SpecialChars(theApp.amuledlg->GetLastLogEntry());
		//resultlog = resultlog.TrimRight('\n');
		//resultlog = resultlog.Mid(resultlog.ReverseFind('\n'));
		Out.Replace("[Message]",resultlog);
	} else
		Out.Replace("[Message]","");

	Out.Replace("[Filename]", _("File Name"));
	Out.Replace("[Priority]",  _("Priority"));
	Out.Replace("[FileTransferred]",  _("Transferred Data"));
	Out.Replace("[FileRequests]",  _("Requests"));
	Out.Replace("[FileAccepts]",  _("Accepted Requests"));
	Out.Replace("[Size]", _("Size"));
	Out.Replace("[Ed2klink]", _("ED2K Link(s)"));
	Out.Replace("[Reload]", _("Reload"));
	Out.Replace("[Session]", sSession);

	wxString OutE = pThis->m_Templates.sSharedLine; 
	OutE.Replace("[Ed2klink]", _("ED2K Link(s)"));
	OutE.Replace("[PriorityUp]", _("Increase Priority"));
	OutE.Replace("[PriorityDown]", _("Decrease Priority"));

	wxString OutE2 = pThis->m_Templates.sSharedLineChanged; 
	OutE2.Replace("[Ed2klink]", _("ED2K Link(s)"));
	OutE2.Replace("[PriorityUp]", _("Increase Priority"));
	OutE2.Replace("[PriorityUp]", _("Decrease Priority"));

	ArrayOfSharedFiles SharedArray;
	
	// Populating array
	// sSharedFilesList as:
	// %s\t%ld\t%s\t%ld\t%ll\t%d\t%d\t%d\t%d\t%s\t%s\t%d\t%d\n
	wxString sSharedFilesList = pThis->webInterface->SendRecvMsg("SHAREDFILES LIST");
	wxString sEntry;
	int brk=0, newLinePos;
	while (sSharedFilesList.Length()>0) {
		newLinePos=sSharedFilesList.First("\n");

		sEntry = sSharedFilesList.Left(newLinePos);
		
		sSharedFilesList = sSharedFilesList.Mid(newLinePos+1);

		SharedFiles* dFile=new SharedFiles();
		//%s\t%ld\t%s\t%ld\t%ll\t%d\t%d\t%d\t%d\t%s\t%s\t%d\t%d\n
		brk=sEntry.First("\t");
		dFile->sFileName = _SpecialChars(sEntry.Left(brk));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->lFileSize = atol(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->sED2kLink = sEntry.Left(brk);
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFileTransferred = atol(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFileAllTimeTransferred = atoll(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFileRequests = atoi(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFileAllTimeRequests = atol(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFileAccepts = atoi(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFileAllTimeAccepts = atol(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->sFileHash = sEntry.Left(brk);
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->sFilePriority = sEntry.Left(brk);
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFilePriority = atoi(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1);
		if (atoi(sEntry.GetData())==0) {
			dFile->bFileAutoPriority = false;
		} else {
			dFile->bFileAutoPriority = true;
		}

		SharedArray.Add(dFile);
	}
	
	// Sorting (simple bubble sort, we don't have tons of data here)
	bool bSorted = true;
	
	for (size_t nMax = 0;bSorted && nMax < SharedArray.GetCount()*2; nMax++) {
		bSorted = false;
		for (size_t i = 0; i < SharedArray.GetCount() - 1; i++) {
			bool bSwap = false;
			switch (pThis->m_Params.SharedSort) {
				case SHARED_SORT_NAME:
					bSwap = SharedArray[i]->sFileName.CmpNoCase(SharedArray[i+1]->sFileName) > 0;
					break;
				case SHARED_SORT_SIZE:
					bSwap = SharedArray[i]->lFileSize < SharedArray[i+1]->lFileSize;
					break;
				case SHARED_SORT_TRANSFERRED:
					bSwap = SharedArray[i]->nFileTransferred < SharedArray[i+1]->nFileTransferred;
					break;
				case SHARED_SORT_ALL_TIME_TRANSFERRED:
					bSwap = SharedArray[i]->nFileAllTimeTransferred < SharedArray[i+1]->nFileAllTimeTransferred;
					break;
				case SHARED_SORT_REQUESTS:
					bSwap = SharedArray[i]->nFileRequests < SharedArray[i+1]->nFileRequests;
					break;
				case SHARED_SORT_ALL_TIME_REQUESTS:
					bSwap = SharedArray[i]->nFileAllTimeRequests < SharedArray[i+1]->nFileAllTimeRequests;
					break;
				case SHARED_SORT_ACCEPTS:
					bSwap = SharedArray[i]->nFileAccepts < SharedArray[i+1]->nFileAccepts;
					break;
				case SHARED_SORT_ALL_TIME_ACCEPTS:
					bSwap = SharedArray[i]->nFileAllTimeAccepts < SharedArray[i+1]->nFileAllTimeAccepts;
					break;
				case SHARED_SORT_PRIORITY:
					//Very low priority is define equal to 4 ! Must adapte sorting code
					if (SharedArray[i]->nFilePriority == 4) {
						if (SharedArray[i+1]->nFilePriority == 4)
							bSwap = false;
						else
							bSwap = true;
					} else {
						if (SharedArray[i+1]->nFilePriority == 4) {
							if (SharedArray[i]->nFilePriority == 4)
								bSwap = true;
							else
								bSwap = false;
						} else
							bSwap = SharedArray[i]->nFilePriority < SharedArray[i+1]->nFilePriority;
					}
					break;
			}
			
			if (pThis->m_Params.bSharedSortReverse) {
				bSwap = !bSwap;
			}
			
			if (bSwap) {
				bSorted = true;
				SharedFiles* TmpFile = SharedArray[i];
				SharedArray[i] = SharedArray[i+1];
				SharedArray[i+1] = TmpFile;
			}
		}
	}

	// Displaying
	wxString sSharedList = "";
	for (size_t i = 0; i < SharedArray.GetCount(); i++) {
		char HTTPTempC[100] = "";
		wxString HTTPProcessData;
		if (SharedArray[i]->sFileHash == _ParseURL(Data,"hash") )
			HTTPProcessData = OutE2;
		else
			HTTPProcessData = OutE;

		HTTPProcessData.Replace("[FileName]", _SpecialChars(SharedArray[i]->sFileName));
		if (SharedArray[i]->sFileName.Length() > SHORT_FILENAME_LENGTH)
			HTTPProcessData.Replace("[ShortFileName]", _SpecialChars(SharedArray[i]->sFileName.Left(SHORT_FILENAME_LENGTH)) + "...");
		else
			HTTPProcessData.Replace("[ShortFileName]", _SpecialChars(SharedArray[i]->sFileName));

		sprintf(HTTPTempC, "%s",castItoXBytes(SharedArray[i]->lFileSize).GetData());
		HTTPProcessData.Replace("[FileSize]", wxString(HTTPTempC));
		HTTPProcessData.Replace("[FileLink]", SharedArray[i]->sED2kLink);

		sprintf(HTTPTempC, "%s",castItoXBytes(SharedArray[i]->nFileTransferred).GetData());
		HTTPProcessData.Replace("[FileTransferred]", wxString(HTTPTempC));

		sprintf(HTTPTempC, "%s",castItoXBytes(SharedArray[i]->nFileAllTimeTransferred).GetData());
		HTTPProcessData.Replace("[FileAllTimeTransferred]", wxString(HTTPTempC));

		sprintf(HTTPTempC, "%i", SharedArray[i]->nFileRequests);
		HTTPProcessData.Replace("[FileRequests]", wxString(HTTPTempC));

		sprintf(HTTPTempC, "%i", SharedArray[i]->nFileAllTimeRequests);
		HTTPProcessData.Replace("[FileAllTimeRequests]", wxString(HTTPTempC));

		sprintf(HTTPTempC, "%i", SharedArray[i]->nFileAccepts);
		HTTPProcessData.Replace("[FileAccepts]", wxString(HTTPTempC));

		sprintf(HTTPTempC, "%i", SharedArray[i]->nFileAllTimeAccepts);
		HTTPProcessData.Replace("[FileAllTimeAccepts]", wxString(HTTPTempC));

		HTTPProcessData.Replace("[Priority]", SharedArray[i]->sFilePriority);

		HTTPProcessData.Replace("[FileHash]", SharedArray[i]->sFileHash);

		uint8 upperpriority=0, lesserpriority=0;
		if (SharedArray[i]->nFilePriority == 4) {
			upperpriority = 0;	lesserpriority = 4;
		} else if (SharedArray[i]->nFilePriority == 0) {
			upperpriority = 1;	lesserpriority = 4;
		} else if (SharedArray[i]->nFilePriority == 1) {
			upperpriority = 2;	lesserpriority = 0;
		} else if (SharedArray[i]->nFilePriority == 2) {
			upperpriority = 3;	lesserpriority = 1;
		} else if (SharedArray[i]->nFilePriority == 3) {
			upperpriority = 5;	lesserpriority = 2;
		} else if (SharedArray[i]->nFilePriority == 5) {
			upperpriority = 5;	lesserpriority = 3;
		} if (SharedArray[i]->bFileAutoPriority) {
			upperpriority = 5;	lesserpriority = 3;
		}
		
		sprintf(HTTPTempC, "%i", upperpriority);
		HTTPProcessData.Replace("[PriorityUpLink]", "hash=" + SharedArray[i]->sFileHash +"&setpriority=" + wxString(HTTPTempC));
		sprintf(HTTPTempC, "%i", lesserpriority);
		HTTPProcessData.Replace("[PriorityDownLink]", "hash=" + SharedArray[i]->sFileHash +"&setpriority=" + wxString(HTTPTempC)); 

		sSharedList += HTTPProcessData;
	}
	Out.Replace("[SharedFilesList]", sSharedList);
	Out.Replace("[Session]", sSession);

	return Out;
}


wxString CWebServer::_GetGraphs(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString Out = pThis->m_Templates.sGraphs;
	
	wxString sGraphDownload = "", sGraphUpload = "", sGraphCons = "";
	wxString sTmp = "";
	
	for (size_t i = 0; i < WEB_GRAPH_WIDTH; i++) {
		if (i < pThis->m_Params.PointsForWeb.GetCount()) {
			if (i != 0) {
				sGraphDownload += ",";
				sGraphUpload += ",";
				sGraphCons += ",";
			}
			// download
			sTmp.Format("%d" , (uint32) (pThis->m_Params.PointsForWeb[i]->download*1024));
			sGraphDownload += sTmp;
			// upload
			sTmp.Format("%d" , (uint32) (pThis->m_Params.PointsForWeb[i]->upload*1024));
			sGraphUpload += sTmp;
			// connections
			sTmp.Format("%d" , (uint32) (pThis->m_Params.PointsForWeb[i]->connections));
			sGraphCons += sTmp;
		}
	}
	
	Out.Replace("[GraphDownload]", sGraphDownload);
	Out.Replace("[GraphUpload]", sGraphUpload);
	Out.Replace("[GraphConnections]", sGraphCons);
	
	Out.Replace("[TxtDownload]", _("Download"));
	Out.Replace("[TxtUpload]", _("Upload"));
	Out.Replace("[TxtTime]", _("Time"));
	Out.Replace("[TxtConnections]", _("Active connections"));
	Out.Replace("[KByteSec]", _("kB/s"));
	Out.Replace("[TxtTime]", _("Time"));

	//sGraphs formatted as: %d\t%d\t%d\t%d
	wxString sGraphs = pThis->webInterface->SendRecvMsg("WEBPAGE GETGRAPH");
	int brk = sGraphs.First("\t");
	
	wxString sScale;
	sScale.Printf("%s", castSecondsToHM(atoi(sGraphs.Left(brk).GetData()) * WEB_GRAPH_WIDTH).GetData() );
	sGraphs = sGraphs.Mid(brk+1); brk=sGraphs.First("\t");

	wxString s1, s2, s3;
	s1.Printf("%d", atoi(sGraphs.Left(brk).GetData()) + 4);
	sGraphs = sGraphs.Mid(brk+1); brk=sGraphs.First("\t");
	s2.Printf("%d", atoi(sGraphs.Left(brk).GetData()) + 4);
	sGraphs = sGraphs.Mid(brk+1);
	s3.Printf("%d", atoi(sGraphs.GetData()) + 20);
	
	Out.Replace("[ScaleTime]", sScale);
	Out.Replace("[MaxDownload]", s1);
	Out.Replace("[MaxUpload]", s2);
	Out.Replace("[MaxConnections]", s3);

	return Out;
}


wxString CWebServer::_GetAddServerBox(ThreadData Data) {	
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data, "ses");

	if (!IsSessionAdmin(Data,sSession)) return "";

	wxString Out = pThis->m_Templates.sAddServerBox;
	if (_ParseURL(Data, "addserver") == "true") {
		wxString sIP = _ParseURL(Data, "serveraddr");
		wxString sPort = _ParseURL(Data, "serverport");
		wxString sName = _ParseURL(Data, "servername");
		
		wxString request = wxString("SERVER ADD ")+sIP+wxString(" ")+sPort+wxString(" ")+sName;
		pThis->webInterface->SendRecvMsg(request.GetData());

#warning fix GetLastLogEntry
		wxString resultlog = ""; //_SpecialChars(theApp.amuledlg->GetLastLogEntry());
		//resultlog = resultlog.TrimRight('\n');
		//resultlog = resultlog.Mid(resultlog.ReverseFind('\n'));
		Out.Replace("[Message]",resultlog);
	} else if (_ParseURL(Data, "updateservermetfromurl") == "true") {
		wxString request = wxString("SERVER UPDATEMET ") + wxString(_ParseURL(Data, "servermeturl"));
		pThis->webInterface->SendRecvMsg(request);
		
#warning fix GetLastLogEntry
		wxString resultlog = ""; //_SpecialChars(theApp.amuledlg->GetLastLogEntry());
		//resultlog = resultlog.TrimRight('\n');
		//resultlog = resultlog.Mid(resultlog.ReverseFind('\n'));
		Out.Replace("[Message]",resultlog);
	} else
		Out.Replace("[Message]", "");
	
	Out.Replace("[AddServer]", _("Received %d new servers"));
	Out.Replace("[IP]", _("IP or Address"));
	Out.Replace("[Port]", _("Port"));
	Out.Replace("[Name]", _("Name"));
	Out.Replace("[Add]", _("Add to list"));
	Out.Replace("[UpdateServerMetFromURL]", _("Update server.met from URL"));
	Out.Replace("[URL]", _("URL"));
	Out.Replace("[Apply]", _("Apply"));

	return Out;
}


wxString CWebServer::_GetWebSearch(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data, "ses");
    
	wxString Out = pThis->m_Templates.sWebSearch;
	if (_ParseURL(Data, "tosearch") != "") {
		wxString query;
		wxString tosearch = _ParseURL(Data, "tosearch");
		query = "http://www.filedonkey.com/fdsearch/index.php?media=";
		query += _ParseURL(Data, "media");
		tosearch = URLEncode(tosearch);
		tosearch.Replace("%20","+");
		query += "&pattern=";
		query += _ParseURL(Data, "tosearch");
		query += "&action=search&name=FD-Search&op=modload&file=index&requestby=amule";
		Out += "\n<script language=\"javascript\">";
		Out += "\n searchwindow=window.open('"+ query + "','searchwindow');";
		Out += "\n</script>";
	}
	
	Out.Replace("[Session]", sSession);
	Out.Replace("[Name]", _("Name"));
	Out.Replace("[Type]", _("Type"));
	Out.Replace("[Any]", _("Any"));
	Out.Replace("[Archives]", _("Archive"));
	Out.Replace("[Audio]", _("Audio"));
	Out.Replace("[CD-Images]", _("CD-Images"));
	Out.Replace("[Pictures]", _("Pictures"));
	Out.Replace("[Programs]", _("Programs"));
	Out.Replace("[Videos]", _("Video"));
	Out.Replace("[Search]", _("Start"));
	Out.Replace("[WebSearch]", _("Web-based Search"));
	
	return Out;
}


wxString CWebServer::_GetLog(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data, "ses");

	wxString Out = pThis->m_Templates.sLog;

	if (_ParseURL(Data, "clear") == "yes" && IsSessionAdmin(Data,sSession)) {
		pThis->webInterface->SendRecvMsg("LOG RESETLOG");
	}
	
	Out.Replace("[Clear]", _("Reset"));
	Out.Replace("[Log]", _SpecialChars(pThis->webInterface->SendRecvMsg("LOG GETALLLOGENTRIES"))+"<br><a name=\"end\"></a>" );
	Out.Replace("[Session]", sSession);

	return Out;
}


wxString CWebServer::_GetServerInfo(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data, "ses");

	wxString Out = pThis->m_Templates.sServerInfo;

	if (_ParseURL(Data, "clear") == "yes") {
		pThis->webInterface->SendRecvMsg("LOG CLEARSERVERINFO");
	}
	
	Out.Replace("[Clear]", _("Reset"));
	Out.Replace("[ServerInfo]", _SpecialChars(pThis->webInterface->SendRecvMsg("LOG GETSERVERINFO")));
	Out.Replace("[Session]", sSession);

	return Out;
}


//shakraw, this is useless in amule 'cause debuglog and log windows are the same.
//so, at the moment, GETALLDEBUGLOGENTRIES has the same behaviour of GETALLLOGENTRIES.
//Note that, when clearing, the log file ~/.aMule/logfile will not be removed here.
wxString CWebServer::_GetDebugLog(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data, "ses");

	wxString Out = pThis->m_Templates.sDebugLog;

	if (_ParseURL(Data, "clear") == "yes" && IsSessionAdmin(Data,sSession)) {
		pThis->webInterface->SendRecvMsg("LOG RESETDEBUGLOG");
	}
	Out.Replace("[Clear]", _("Reset"));

	Out.Replace("[DebugLog]", _SpecialChars(pThis->webInterface->SendRecvMsg("LOG GETALLDEBUGLOGENTRIES"))+"<br><a name=\"end\"></a>" );
	Out.Replace("[Session]", sSession);

	return Out;
}


wxString CWebServer::_GetStats(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	pThis->webInterface->Print("***_GetStats arrived\n");

	wxString sSession = _ParseURL(Data, "ses");

	// refresh statistics.. ARGH. NO NO NO NO
	// (it will be done in statisticsdlg and in main thread)
	//theApp.amuledlg->statisticswnd.ShowStatistics(true);
	
	wxString Out = pThis->m_Templates.sStats;
	
	wxString sStats = pThis->webInterface->SendRecvMsg("WEBPAGE STATISTICS");
	int brk = sStats.First("\t");
	
	Out.Replace("[STATSDATA]", sStats.Left(brk));
	Out.Replace("[Stats]", sStats.Mid(brk+1));

	return Out;
}


wxString CWebServer::_GetPreferences(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	int brk;
	
	wxString sSession = _ParseURL(Data, "ses");

	wxString Out = pThis->m_Templates.sPreferences;
	Out.Replace("[Session]", sSession);

	if ((_ParseURL(Data, "saveprefs") == "true") && IsSessionAdmin(Data,sSession) ) {
		wxString prefList("");
		if (_ParseURL(Data, "gzip") == "true" || _ParseURL(Data, "gzip") == "on") {
			prefList.Append("1\t");
		}
		if (_ParseURL(Data, "gzip") == "false" || _ParseURL(Data, "gzip") == "") {
			prefList.Append("0\t");
		}
		if (_ParseURL(Data, "showuploadqueue") == "true" || _ParseURL(Data, "showuploadqueue") == "on" ) {
			pThis->m_Params.bShowUploadQueue = true;
		}
		if(_ParseURL(Data, "showuploadqueue") == "false" || _ParseURL(Data, "showuploadqueue") == "") {
			pThis->m_Params.bShowUploadQueue = false;
		}
		if (_ParseURL(Data, "refresh") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data, "refresh")));
		}
		if (_ParseURL(Data, "maxdown") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data, "maxdown")));
		}
		if (_ParseURL(Data, "maxup") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data, "maxup")));
		}
		if (_ParseURL(Data, "maxcapdown") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data, "maxcapdown")));
		}
		if (_ParseURL(Data, "maxcapup") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data, "maxcapup")));
		}
		if (_ParseURL(Data, "maxsources") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data, "maxsources")));
		}
		if (_ParseURL(Data, "maxconnections") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data, "maxconnections")));
		}
		if (_ParseURL(Data, "maxconnectionsperfive") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data, "maxconnectionsperfive")));
		}

		prefList+=wxString::Format("%d\t", ((_ParseURL(Data, "fullchunks").MakeLower() == "on") ? 1 : 0));
		prefList+=wxString::Format("%d\t", ((_ParseURL(Data, "firstandlast").MakeLower() == "on") ? 1 : 0));
		
		pThis->webInterface->SendRecvMsg(wxString::Format("WEBPAGE SETPREFERENCES %s", prefList.GetData()).GetData());
	}

	// Fill form
	//sPreferencesList formatted as: %d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d
	wxString sPreferences = pThis->webInterface->SendRecvMsg("WEBPAGE GETPREFERENCES");
	brk = sPreferences.First("\t");
	if (atoi(sPreferences.Left(brk).GetData())) {
		Out.Replace("[UseGzipVal]", "checked");
	} else {
		Out.Replace("[UseGzipVal]", "");
	}
	if(pThis->m_Params.bShowUploadQueue) {
		Out.Replace("[ShowUploadQueueVal]", "checked");
	} else {
		Out.Replace("[ShowUploadQueueVal]", "");
	}
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	if (atoi(sPreferences.Left(brk).GetData())) {
		Out.Replace("[FirstAndLastVal]", "checked");
	} else {
		Out.Replace("[FirstAndLastVal]", "");
	}
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	if (atoi(sPreferences.Left(brk).GetData())) {
		Out.Replace("[FullChunksVal]", "checked");
	} else {
		Out.Replace("[FullChunksVal]", "");
	}
	
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	wxString sRefresh; sRefresh.Printf("%d", atoi(sPreferences.Left(brk).GetData()));
	Out.Replace("[RefreshVal]", sRefresh);
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	sRefresh.Printf("%d", atoi(sPreferences.Left(brk).GetData()));
	Out.Replace("[MaxSourcesVal]", sRefresh);
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	sRefresh.Printf("%d", atoi(sPreferences.Left(brk).GetData()));
	Out.Replace("[MaxConnectionsVal]", sRefresh);
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	sRefresh.Printf("%d", atoi(sPreferences.Left(brk).GetData()));
	Out.Replace("[MaxConnectionsPer5Val]", sRefresh);

	Out.Replace("[KBS]", wxString(_("kB/s"))+":");
	Out.Replace("[FileSettings]", wxString(_("File Settings"))+":");
	Out.Replace("[LimitForm]", wxString(_("Connection Limits"))+":");
	Out.Replace("[MaxSources]", wxString(_("Max Sources Per File"))+":");
	Out.Replace("[MaxConnections]", wxString(_("Max. Connections"))+":");
	Out.Replace("[MaxConnectionsPer5]", wxString(_("max. new connections / 5secs"))+":");
	Out.Replace("[UseGzipForm]", _("Gzip Compression"));
	Out.Replace("[UseGzipComment]", _("Save traffic, especially in graphs."));
	Out.Replace("[ShowUploadQueueForm]", _("Show Queue"));
	Out.Replace("[ShowUploadQueueComment]", _("Enable or disable the display of waiting queue in transfer page."));
	Out.Replace("[ShowQueue]", _("Show Queue"));
	Out.Replace("[HideQueue]", _("Hide Queue"));
	Out.Replace("[RefreshTimeForm]", _("Refresh-Time of Pages"));
	Out.Replace("[RefreshTimeComment]", _("Time in seconds (zero=disabled):"));
	Out.Replace("[SpeedForm]", _("Speed Limits"));
	Out.Replace("[MaxDown]", _("Download"));
	Out.Replace("[MaxUp]", _("Upload"));
	Out.Replace("[SpeedCapForm]", _("Bandwidth Limits"));
	Out.Replace("[MaxCapDown]", _("Download"));
	Out.Replace("[MaxCapUp]", _("Upload"));
	Out.Replace("[TryFullChunks]", _("Try to transfer full chunks to all uploads"));
	Out.Replace("[FirstAndLast]", _("Try to download first and last chunks first"));
	Out.Replace("[WebControl]", _("Web Control Panel"));
	Out.Replace("[aMuleAppName]", "aMule");
	Out.Replace("[Apply]", _("Apply"));

	wxString sT;
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	int n = atoi(sPreferences.Left(brk).GetData());
	if (n < 0 || n == 65535) n = 0;
	sT.Printf("%d", n);
	Out.Replace("[MaxDownVal]", sT);
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	n = atoi(sPreferences.Left(brk).GetData());
	if (n < 0 || n == 65535) n = 0;
	sT.Printf("%d", n);
	Out.Replace("[MaxUpVal]", sT);
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	sT.Printf("%d", atoi(sPreferences.Left(brk).GetData()));
	Out.Replace("[MaxCapDownVal]", sT);
	sPreferences=sPreferences.Mid(brk+1);
	sT.Printf("%d", atoi(sPreferences.GetData()));
	Out.Replace("[MaxCapUpVal]", sT);

	return Out;
}


wxString CWebServer::_GetLoginScreen(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data, "ses");

	wxString Out = "";

	Out += pThis->m_Templates.sLogin;

	Out.Replace("[CharSet]", _GetWebCharSet());
	Out.Replace("[aMulePlus]", "aMule");
	Out.Replace("[aMuleAppName]", "aMule");
	Out.Replace("[version]", VERSION); //shakraw - was CURRENT_VERSION_LONG);
	Out.Replace("[Login]", _("Login"));
	Out.Replace("[EnterPassword]", _("Enter your password here"));
	Out.Replace("[LoginNow]", _("Login Now"));
	Out.Replace("[WebControl]", _("Web Control Panel"));

	return Out;
}


wxString CWebServer::_GetConnectedServer(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data, "ses");

	wxString HTTPTemp = "";
	char	HTTPTempC[100] = "";
	wxString OutS = pThis->m_Templates.sConnectedServer;
	OutS.Replace("[ConnectedServer]", _("Server"));
	OutS.Replace("[Servername]", _("Server name"));
	OutS.Replace("[Status]", _("Status"));
	OutS.Replace("[Usercount]", _("users"));
	OutS.Replace("[Action]", _("Connecting"));
	OutS.Replace("[URL_Disconnect]", IsSessionAdmin(Data,sSession)?wxString("?ses=" + sSession + "&w=server&c=disconnect"):GetPermissionDenied());
	OutS.Replace("[URL_Connect]", IsSessionAdmin(Data,sSession)?wxString("?ses=" + sSession + "&w=server&c=connect"):GetPermissionDenied());
	OutS.Replace("[Disconnect]", _("Disconnect"));
	OutS.Replace("[Connect]", _("Connect to any server"));
	OutS.Replace("[URL_ServerOptions]", IsSessionAdmin(Data,sSession)?wxString("?ses=" + sSession + "&w=server&c=options"):GetPermissionDenied());
	OutS.Replace("[ServerOptions]", wxString(_("Server"))+wxString(_("&Preferences")));
	OutS.Replace("[WebSearch]", _("Web-based Search"));

	wxString sServerStat = pThis->webInterface->SendRecvMsg("SERVER STAT");
	int brk=sServerStat.First("\t");
	if (sServerStat.Left(brk) == "Connected") {
		sServerStat=sServerStat.Mid(brk+1);brk=sServerStat.First("\t");
		if (sServerStat.Left(brk) == "High ID")
			OutS.Replace("[1]", _("Connected"));
		else
			OutS.Replace("[1]", wxString(_("Connected")) + " (" + wxString(_("Low ID")) + ")");

		sServerStat=sServerStat.Mid(brk+1);brk=sServerStat.First("\t");
		OutS.Replace("[2]", sServerStat.Left(brk));
		
		sServerStat=sServerStat.Mid(brk+1);brk=sServerStat.First("\t");
		sprintf(HTTPTempC, "%10i", atoi(sServerStat.Left(brk).GetData()));
		HTTPTemp = HTTPTempC;												
		OutS.Replace("[3]", HTTPTemp);
	} else if (sServerStat.Left(brk) == "Connecting") {
		OutS.Replace("[1]", _("Connecting"));
		OutS.Replace("[2]", "");
		OutS.Replace("[3]", "");
	} else {
		OutS.Replace("[1]", _("Disconnected"));
		OutS.Replace("[2]", "");
		OutS.Replace("[3]", "");
	}

	return OutS;
}


// We have to add gz-header and some other stuff
// to standard zlib functions in order to use gzip in web pages
int CWebServer::_GzipCompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level) { 
	const static int gz_magic[2] = {0x1f, 0x8b}; // gzip magic header
	int err;
	uLong crc;
	z_stream stream = {0};
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
	for (size_t i = 0; i < pThis->m_Params.Sessions.GetCount(); i++) {
		if (pThis->m_Params.Sessions[i]->lSession == lSession && lSession != 0) {
			// if found, also reset expiration time
			pThis->m_Params.Sessions[i]->startTime = time(NULL);
			return true;
		}
	}

	return false;
}


void CWebServer::_RemoveTimeOuts(ThreadData Data, long lSession) {
	// remove expired sessions
	CWebServer *pThis = (CWebServer *)Data.pThis;
	pThis->UpdateSessionCount();
}


bool CWebServer::_RemoveSession(ThreadData Data, long lSession) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	// find our session
	for (size_t i = 0; i < pThis->m_Params.Sessions.GetCount(); i++) {
		if (pThis->m_Params.Sessions[i]->lSession == lSession && lSession != 0) {
			pThis->m_Params.Sessions.RemoveAt(i);
			pThis->webInterface->SendRecvMsg(wxString::Format("LOG ADDLOGLINE %s", _("Webserver: Logout")));
			return true;
		}
	}
	return false;
}


Session CWebServer::GetSessionByID(ThreadData Data,long sessionID) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	
	if (pThis != NULL) {
		for (size_t i = 0; i < pThis->m_Params.Sessions.GetCount(); i++) {
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
	long sessionID=atoll(SsessionID);
	CWebServer *pThis = (CWebServer *)Data.pThis;
	
	if (pThis != NULL) {
		for (size_t i = 0; i < pThis->m_Params.Sessions.GetCount(); i++) {
			if (pThis->m_Params.Sessions[i]->lSession == sessionID && sessionID != 0)
				return pThis->m_Params.Sessions[i]->admin;
		}
	}
	return false;
}


wxString CWebServer::GetPermissionDenied() {
	return "javascript:alert(\'"+wxString(_("Access denied!"))+"\')";
}


bool CWebServer::_GetFileHash(wxString sHash, uchar *FileHash) {
	char hex_byte[3];
	int byte;
	hex_byte[2] = '\0';
	for (int i = 0; i < 16; i++) {
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
	// Western (Latin) includes Catalan, Danish, Dutch, English, Faeroese, Finnish, French,
	// German, Galician, Irish, Icelandic, Italian, Norwegian, Portuguese, Spanish and Swedish
	return "ISO-8859-1";
}


// Ornis: creating the progressbar. colored if ressources are given/available
wxString CWebServer::_GetDownloadGraph(ThreadData Data,wxString filehash) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";
	
	// cool style
	wxString progresscolor[12];
	progresscolor[0]="transparent.gif";
	progresscolor[1]="black.gif";
	progresscolor[2]="yellow.gif";
	progresscolor[3]="red.gif";

	progresscolor[4]="blue1.gif";
	progresscolor[5]="blue2.gif";
	progresscolor[6]="blue3.gif";
	progresscolor[7]="blue4.gif";
	progresscolor[8]="blue5.gif";
	progresscolor[9]="blue6.gif";

	progresscolor[10]="green.gif";
	progresscolor[11]="greenpercent.gif";

	wxString Out = "";
	wxString temp;

	wxString response = pThis->webInterface->SendRecvMsg(wxString::Format("WEBPAGE PROGRESSBAR %d %s", pThis->m_Templates.iProgressbarWidth, filehash.GetData()).GetData());
	int brk=response.First("\t");
	
	if (atoi(response.Left(brk).GetData())) {
		temp.Printf(wxString(pThis->m_Templates.sProgressbarImgsPercent+"<br>").GetData(),progresscolor[11].GetData(),pThis->m_Templates.iProgressbarWidth);
		Out+=temp;
		temp.Printf(pThis->m_Templates.sProgressbarImgs.GetData(),progresscolor[10].GetData(),pThis->m_Templates.iProgressbarWidth);
		Out+=temp;
	} else {
		response=response.Mid(brk+1); brk=response.First("\t");
		wxString s_ChunkBar = response.Left(brk);
		// and now make a graph out of the array - need to be in a progressive way
		uint8 lastcolor=1;
		uint16 lastindex=0;
		for (uint16 i=0;i<pThis->m_Templates.iProgressbarWidth;i++) {
			if (lastcolor!= atoi(s_ChunkBar.Mid(i,1).GetData())) {
				if (i>lastindex) {
					temp.Printf(pThis->m_Templates.sProgressbarImgs.GetData() ,progresscolor[lastcolor].GetData(),i-lastindex);

					Out+=temp;
				}
				lastcolor=atoi(s_ChunkBar.Mid(i,1).GetData());
				lastindex=i;
			}
		}

		temp.Printf(pThis->m_Templates.sProgressbarImgs.GetData(), progresscolor[lastcolor].GetData(), pThis->m_Templates.iProgressbarWidth-lastindex);
		Out+=temp;

		response=response.Mid(brk+1);
		double percentComplete = atof(response.GetData());
		int complx=(int)((pThis->m_Templates.iProgressbarWidth/100.0)*percentComplete);
		(complx>0)?temp.Printf(wxString(pThis->m_Templates.sProgressbarImgsPercent+"<br>").GetData(), progresscolor[11].GetData(),complx):temp.Printf(wxString(pThis->m_Templates.sProgressbarImgsPercent+"<br>").GetData(),progresscolor[0].GetData(),5);
		Out=temp+Out;
	}

	return Out;
}


wxString CWebServer::_GetSearch(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data, "ses");
	wxString Out = pThis->m_Templates.sSearch;

	wxString downloads=_ParseURLArray(Data,"downloads");
	if (downloads != "" && IsSessionAdmin(Data,sSession) ) {
		int brk;
		while (downloads.Length()>0) {
			brk=downloads.First("|");
			pThis->webInterface->SendRecvMsg(wxString::Format("SEARCH DOWNLOADFILE %s", downloads.Left(brk).GetData()));
			downloads=downloads.Mid(brk+1);
		}
	}

	wxString sToSearch = _ParseURL(Data, "tosearch");
	if (sToSearch != "" && IsSessionAdmin(Data,sSession)) {
		wxString sParams;
		sParams.Printf(sToSearch+"\n");
		sParams.Append(_ParseURL(Data, "type")+"\n");
		sParams.Append(wxString::Format("%ld\n", atol(_ParseURL(Data, "min").GetData())*1048576));
		sParams.Append(wxString::Format("%ld\n", atol(_ParseURL(Data, "max").GetData())*1048576));
		sParams.Append(_ParseURL(Data, "avail")+"\n");
		sParams.Append(_ParseURL(Data, "ext")+"\n");
		sParams.Append(_ParseURL(Data, "method")+"\n");

		pThis->webInterface->SendRecvMsg(wxString::Format("SEARCH DONEWSEARCH %s", sParams.GetData()));
		Out.Replace("[Message]",_("Search in progress. Refetch results in a moment!"));
	} else if (sToSearch != "" && !IsSessionAdmin(Data,sSession) ) {
		Out.Replace("[Message]",_("Access denied!"));
	} else 
		Out.Replace("[Message]","");

	wxString sSort = _ParseURL(Data, "sort");
	if (sSort.Length()>0) pThis->m_iSearchSortby=atoi(sSort);
	sSort = _ParseURL(Data, "sortAsc");
	if (sSort.Length()>0) pThis->m_bSearchAsc=atoi(sSort);

	wxString result = pThis->m_Templates.sSearchHeader + pThis->webInterface->SendRecvMsg(wxString::Format("SEARCH WEBLIST %s\t%d\t%d", pThis->m_Templates.sSearchResultLine.GetData(), pThis->m_iSearchSortby, pThis->m_bSearchAsc));
	
	if (atoi(pThis->webInterface->SendRecvMsg("SEARCH GETCATCOUNT").GetData()) > 1)
		InsertCatBox(Out,0,pThis->m_Templates.sCatArrow);
	else
		Out.Replace("[CATBOX]","");

	Out.Replace("[SEARCHINFOMSG]","");
	Out.Replace("[RESULTLIST]", result);
	Out.Replace("[Result]", _("Search Results"));
	Out.Replace("[Session]", sSession);
	Out.Replace("[WebSearch]", _("Web-based Search"));
	Out.Replace("[Name]", _("Name"));
	Out.Replace("[Type]", _("Type"));
	Out.Replace("[Any]", _("Any"));
	Out.Replace("[Archives]", _("Archive"));
	Out.Replace("[Audio]", _("Audio"));
	Out.Replace("[CD-Images]", _("CD-Images"));
	Out.Replace("[Pictures]", _("Pictures"));
	Out.Replace("[Programs]", _("Programs"));
	Out.Replace("[Videos]", _("Video"));
	Out.Replace("[Search]", _("Search"));
	Out.Replace("[RefetchResults]", _("Refetch Results"));
	Out.Replace("[Download]", _("Download"));
	
	Out.Replace("[Filesize]", _("Size"));
	Out.Replace("[Sources]", _("Sources"));
	Out.Replace("[Filehash]", _("File Hash"));
	Out.Replace("[Filename]", _("File Name"));
	Out.Replace("[WebSearch]", _("Web-based Search"));

	Out.Replace("[SizeMin]", _("Min Size"));
	Out.Replace("[SizeMax]", _("Max Size"));
	Out.Replace("[Availabl]", _("Min Availability"));
	Out.Replace("[Extention]", _("Extension"));
	Out.Replace("[Global]", _("Global Search"));
	Out.Replace("[MB]", _("MB"));
		
	Out.Replace("[METHOD]", _("Method"));
	Out.Replace("[USESSERVER]", _("Server"));
	Out.Replace("[Global]", _("Global (Server)"));

	wxString val;
	val.Printf("%i",(pThis->m_iSearchSortby!=0 || (pThis->m_iSearchSortby==0 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace("[SORTASCVALUE0]", val);
	val.Printf("%i",(pThis->m_iSearchSortby!=1 || (pThis->m_iSearchSortby==1 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace("[SORTASCVALUE1]", val);
	val.Printf("%i",(pThis->m_iSearchSortby!=2 || (pThis->m_iSearchSortby==2 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace("[SORTASCVALUE2]", val);
	val.Printf("%i",(pThis->m_iSearchSortby!=3 || (pThis->m_iSearchSortby==3 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace("[SORTASCVALUE3]", val);
	
	return Out;
}


int CWebServer::UpdateSessionCount() {
	// remove old bans
	for (size_t i = 0; i < m_Params.badlogins.GetCount();) {
		uint32 diff= ::GetTickCount() - m_Params.badlogins[i]->timestamp ;
		if (diff >1000U*60U*15U && (::GetTickCount() > m_Params.badlogins[i]->timestamp)) {
			m_Params.badlogins.RemoveAt(i);
		} else 
			i++;
	}

	// count & remove old session
	for (size_t i = 0; i < m_Params.Sessions.GetCount();) {
	  time_t ts=time(NULL)-m_Params.Sessions[i]->startTime;
	  if (ts > SESSION_TIMEOUT_SECS) {
	    m_Params.Sessions.RemoveAt(i);
	  } else
	  	i++;
	}

	return m_Params.Sessions.GetCount();
}


void CWebServer::InsertCatBox(wxString &Out,int preselect,wxString boxlabel,bool jump,bool extraCats) {
#if 0 //shakraw
	wxString tempBuf2,tempBuf3;
	if (jump) tempBuf2="onchange=GotoCat(this.form.cat.options[this.form.cat.selectedIndex].value)>";
	else tempBuf2=">";
	wxString tempBuf="<form><select name=\"cat\" size=\"1\""+tempBuf2;
	for (int i=0;i< theApp.glob_prefs->GetCatCount();i++) {
		tempBuf3= (i==preselect)? " selected":"";
		webInterface->Print("before\n");
		tempBuf2.Printf("<option%s value=\"%i\">%s</option>",tempBuf3,i, (i==0)?_("all"):theApp.glob_prefs->GetCategory(i)->title );
		webInterface->Print("after\n");
		tempBuf.Append(tempBuf2);
	}
	webInterface->Print("hello 4\n");
	if (extraCats) {
		if (theApp.glob_prefs->GetCatCount()>1){
			tempBuf2.Printf("<option>------------</option>");
			tempBuf.Append(tempBuf2);
		}
	
		for (int i=(theApp.glob_prefs->GetCatCount()>1)?1:2;i<=12;i++) {
			tempBuf3= ( (-i)==preselect)? " selected":"";
			tempBuf2.Printf("<option%s value=\"%i\">%s</option>",tempBuf3,-i, GetSubCatLabel(-i) );
			tempBuf.Append(tempBuf2);
		}
	}
	tempBuf.Append("</select></form>");
	Out.Replace("[CATBOX]",boxlabel+tempBuf);
#endif
}


wxString CWebServer::GetSubCatLabel(int cat) {
	switch (cat) {
		case -1: return _("all others");
		case -2: return _("Incomplete");
		case -3: return _("Completed");
		case -4: return _("Waiting");
		case -5: return _("Downloading");
		case -6: return _("Erroneous");
		case -7: return _("Paused");
		case -8: return _("Stopped");
		case -9: return _("Video");
		case -10: return _("Audio");
		case -11: return _("Archive");
		case -12: return _("CD-Images");
	}
	return "?";
}
