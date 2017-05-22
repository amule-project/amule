//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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


#include <cctype>	// Needed for std::toupper()
#include <wx/math.h>	// Needed for cos, M_PI
#include <string>	// Do_not_auto_remove (g++-4.0.1)

#include <wx/datetime.h>

//-------------------------------------------------------------------

#include <wx/tokenzr.h>		// for wxTokenizer
#include <wx/wfstream.h>

#include <ec/cpp/ECFileConfig.h>	// Needed for CECFileConfig
#include <ec/cpp/ECSpecialTags.h>
#include <common/MD5Sum.h>
#include <common/Format.h>		// Needed for CFormat

#include <protocol/ed2k/Constants.h>	// Needed for PARTSIZE
#include "Constants.h"			// Needed for PR_*

//-------------------------------------------------------------------

#include "WebSocket.h"		// Needed for StopSockets()
#include <amuleIPV4Address.h>

#include "php_syntree.h"
#include "php_core_lib.h"

//-------------------------------------------------------------------
typedef uint32_t COLORTYPE;

#ifdef RGB
#undef RGB
#endif

inline unsigned long RGB(int r, int g, int b)
{
	return ((b & 0xff) << 16) | ((g & 0xff) << 8) | (r & 0xff);
}

inline void set_rgb_color_val(unsigned char *start, uint32 val, unsigned char mod)
{
	unsigned char r = val, g = val >> 8, b = val >> 16;
	start[0] = ( r > mod ) ? (r - mod) : 1;
	start[1] = ( g > mod ) ? (g - mod) : 1;
	start[2] = ( b > mod ) ? (b - mod) : 1;
}

wxString _SpecialChars(wxString str) {
	str.Replace(wxT("&"),wxT("&amp;"));
	str.Replace(wxT("<"),wxT("&lt;"));
	str.Replace(wxT(">"),wxT("&gt;"));
	str.Replace(wxT("\""),wxT("&quot;"));
	return str;
}

uint8 GetHigherPrio(uint32 prio, bool autoprio)
{
	if (autoprio) {
		return PR_LOW;
	} else {
		switch (prio) {
			case PR_LOW: return PR_NORMAL;
			case PR_NORMAL: return PR_HIGH;
			case PR_HIGH: return PR_AUTO;
			case PR_AUTO: return PR_LOW;
			default: return PR_AUTO;
		}
	}
}

uint8 GetHigherPrioShared(uint32 prio, bool autoprio)
{
	if (autoprio) {
		return PR_VERYLOW;
	} else {
		switch (prio) {
			case PR_VERYLOW: return PR_LOW;
			case PR_LOW: return PR_NORMAL;
			case PR_NORMAL: return PR_HIGH;
			case PR_HIGH: return PR_VERYHIGH;
			case PR_VERYHIGH: return PR_POWERSHARE;
			case PR_POWERSHARE: return PR_AUTO;
			case PR_AUTO: return PR_VERYLOW;
			default: return PR_AUTO;
		}
	}
}


uint8 GetLowerPrio(uint32 prio, bool autoprio)
{
	if (autoprio) {
		return PR_HIGH;
	} else {
		switch (prio) {
			case PR_LOW: return PR_AUTO;
			case PR_NORMAL: return PR_LOW;
			case PR_HIGH: return PR_NORMAL;
			case PR_AUTO: return PR_HIGH;
			default: return PR_AUTO;
		}
	}
}

uint8 GetLowerPrioShared(uint32 prio, bool autoprio)
{
	if (autoprio) {
		return PR_POWERSHARE;
	} else {
		switch (prio) {
			case PR_VERYLOW: return PR_AUTO;
			case PR_LOW: return PR_VERYLOW;
			case PR_NORMAL: return PR_LOW;
			case PR_HIGH: return PR_NORMAL;
			case PR_VERYHIGH: return PR_HIGH;
			case PR_POWERSHARE: return PR_VERYHIGH;
			case PR_AUTO: return PR_POWERSHARE;
			default: return PR_AUTO;
		}
	}
}

/*
 * Url string decoder
 */
wxString CURLDecoder::Decode(const wxString& url)
{
	size_t n = url.length();
	std::vector<char> buffer(n + 1);
	size_t i, j;

	for (i = 0, j = 0; i < n; i++, j++) {
		if (url[i] == wxT('+')) {
			buffer[j] = ' ';
		} else if (url[i] == wxT('%') && i < n - 2) {
			char ch1 = std::toupper(url[i+1]);
			char ch2 = std::toupper(url[i+2]);
			if (((ch1 >= '0' && ch1 <= '9') || (ch1 >= 'A' && ch1 <= 'F')) &&
			    ((ch2 >= '0' && ch2 <= '9') || (ch2 >= 'A' && ch2 <= 'F'))) {
				i += 2;
				buffer[j] = ((ch1 > '9' ? ch1 - 'A' + 10 : ch1 - '0') << 4) | (ch2 > '9' ? ch2 - 'A' + 10 : ch2 - '0');
			} else {
				// Invalid %-escape sequence
				buffer[j] = url[i];
			}
		} else {
			buffer[j] = url[i];
		}
	}
	buffer[j] = '\0';

	return UTF82unicode(&buffer[0]);
}

CParsedUrl::CParsedUrl(const wxString &url)
{
	if ( url.Find('/') != -1 ) {
		m_path = url.BeforeFirst('/');
		m_file = url.AfterFirst('/');
	}

	if ( url.Find('?') != -1 ) {
		m_file.Truncate(m_file.Find('?'));

		wxString params = url.AfterFirst('?');

		wxStringTokenizer tkz(params, wxT("&"));
		while ( tkz.HasMoreTokens() ) {
			wxString param_val = tkz.GetNextToken();
			wxString key = param_val.BeforeFirst('=');
			wxString val = param_val.AfterFirst('=');
			val = CURLDecoder::Decode(val);
			if ( m_params.count(key) ) {
				m_params[key] = m_params[key] + wxT("|") + val;
			} else {
				m_params[key] = val;
			}
		}
    }
}

void CParsedUrl::ConvertParams(std::map<std::string, std::string> &dst)
{
	for(std::map<wxString, wxString>::iterator i = m_params.begin(); i != m_params.end(); ++i) {
		std::string key(unicode2char(i->first)), value(unicode2char(i->second));
		dst[key] = value;
	}
}

#ifndef ASIO_SOCKETS
BEGIN_EVENT_TABLE(CWebServerBase, wxEvtHandler)
	EVT_SOCKET(ID_WEBLISTENSOCKET_EVENT, CWebServerBase::OnWebSocketServerEvent)
	EVT_SOCKET(ID_WEBCLIENTSOCKET_EVENT, CWebServerBase::OnWebSocketEvent)
END_EVENT_TABLE()
#endif

CWebServerBase::CWebServerBase(CamulewebApp *webApp, const wxString& templateDir) :
	m_ServersInfo(webApp), m_SharedFileInfo(webApp), m_DownloadFileInfo(webApp, &m_ImageLib),
	m_UploadsInfo(webApp), m_SearchInfo(webApp), m_Stats(500, webApp),
	m_ImageLib(templateDir)
{
	webInterface = webApp;

	//
	// Init stat graphs
#ifdef WITH_LIBPNG
	m_ImageLib.AddImage(new CDynStatisticImage(200, true, m_Stats.DownloadSpeed()),
		wxT("/amule_stats_download.png"));
	m_ImageLib.AddImage(new CDynStatisticImage(200, true, m_Stats.UploadSpeed()),
		wxT("/amule_stats_upload.png"));
	m_ImageLib.AddImage(new CDynStatisticImage(200, false, m_Stats.ConnCount()),
		wxT("/amule_stats_conncount.png"));
	m_ImageLib.AddImage(new CDynStatisticImage(200, false, m_Stats.KadCount()),
		wxT("/amule_stats_kad.png"));
#endif

	m_upnpEnabled = webInterface->m_UPnPWebServerEnabled;
	m_upnpTCPPort = webInterface->m_UPnPTCPPort;

#ifdef ASIO_SOCKETS
	m_AsioService = new CAsioService;
#endif
}


// Probably always terminated by Ctrl-C or kill, but make a clean shutdown of the service anyway
CWebServerBase::~CWebServerBase()
{
#ifdef ASIO_SOCKETS
	m_AsioService->Stop();
	delete m_AsioService;
#endif
}


//sends output to web interface
void CWebServerBase::Print(const wxString &s)
{
	webInterface->Show(s);
}


void CWebServerBase::StartServer()
{
#ifdef ENABLE_UPNP
	if (m_upnpEnabled) {
		m_upnpMappings.resize(1);
		m_upnpMappings[0] = CUPnPPortMapping(
			webInterface->m_WebserverPort,
			"TCP",
			true,
			"aMule TCP Webserver Socket");
		m_upnp = new CUPnPControlPoint(m_upnpTCPPort);
		m_upnp->AddPortMappings(m_upnpMappings);
	}
#endif

	amuleIPV4Address addr;
	addr.AnyAddress();
	addr.Service(webInterface->m_WebserverPort);

	m_webserver_socket = new CWebLibSocketServer(addr, MULE_SOCKET_REUSEADDR, this);
#ifndef ASIO_SOCKETS
	m_webserver_socket->SetEventHandler(*this, ID_WEBLISTENSOCKET_EVENT);
	m_webserver_socket->SetNotify(wxSOCKET_CONNECTION_FLAG);
#endif
	m_webserver_socket->Notify(true);
	if (!m_webserver_socket->IsOk()) {
		delete m_webserver_socket;
		m_webserver_socket = 0;
	}

}

void CWebServerBase::StopServer()
{
	if ( m_webserver_socket ) {
		delete m_webserver_socket;
	}
#ifdef ENABLE_UPNP
	if (m_upnpEnabled) {
		m_upnp->DeletePortMappings(m_upnpMappings);
		delete m_upnp;
	}
#endif
}

#ifndef ASIO_SOCKETS
void CWebServerBase::OnWebSocketServerEvent(wxSocketEvent& WXUNUSED(event))
{
	m_webserver_socket->OnAccept();
}
#endif

CWebLibSocketServer::CWebLibSocketServer(const class amuleIPV4Address& adr, int flags, CWebServerBase * webServerBase)
	:	CLibSocketServer(adr, flags),
		m_webServerBase(webServerBase)
{
}

void CWebLibSocketServer::OnAccept()
{
	CWebSocket *client = new CWebSocket(m_webServerBase);

    if (AcceptWith(*client, false) ) {
		m_webServerBase->webInterface->Show(_("web client connection accepted\n"));
    } else {
		delete client;
		m_webServerBase->webInterface->Show(_("ERROR: cannot accept web client connection\n"));
    }
}

#ifndef ASIO_SOCKETS
void CWebServerBase::OnWebSocketEvent(wxSocketEvent& event)
{
	CWebSocket *socket = dynamic_cast<CWebSocket *>(event.GetSocket());
    wxCHECK_RET(socket, wxT("Socket event with a NULL socket!"));
    switch(event.GetSocketEvent()) {
    case wxSOCKET_LOST:
        socket->OnLost();
        break;
    case wxSOCKET_INPUT:
        socket->OnReceive(0);
        break;
    case wxSOCKET_OUTPUT:
        socket->OnSend(0);
        break;
    case wxSOCKET_CONNECTION:
        break;
    default:
        wxFAIL;
        break;
    }
}
#endif

void CScriptWebServer::ProcessImgFileReq(ThreadData Data)
{
	webInterface->DebugShow(wxT("**** imgrequest: ") + Data.sURL + wxT("\n"));

	const CSession* session = CheckLoggedin(Data);

	// To prevent access to non-template images, we disallow use of paths in filenames.
	wxString imgName = wxT("/") + wxFileName(Data.parsedURL.File()).GetFullName();
	CAnyImage *img = m_ImageLib.GetImage(imgName);

	// Only static images are available to visitors, in order to prevent
	// information leakage, but still allowing images on the login page.
	if (img && (session->m_loggedin || dynamic_cast<CFileImage*>(img))) {
		int img_size = 0;
		unsigned char* img_data = img->RequestData(img_size);
		// This unicode2char is ok.
		Data.pSocket->SendContent(unicode2char(img->GetHTTP()), img_data, img_size);
	} else if (!session->m_loggedin) {
		webInterface->DebugShow(wxT("**** imgrequest: failed, not logged in\n"));
		ProcessURL(Data);
	} else {
		webInterface->DebugShow(wxT("**** imgrequest: failed\n"));
	}
}

// send EC request and discard output
void CWebServerBase::Send_Discard_V2_Request(CECPacket *request)
{
	const CECPacket *reply = webInterface->SendRecvMsg_v2(request);
	const CECTag *tag = NULL;
	if (reply) {
		if ( reply->GetOpCode() == EC_OP_STRINGS ) {
			for (CECPacket::const_iterator it = reply->begin(); it != reply->end(); ++it) {
				tag = & *it;
				if (tag->GetTagName() == EC_TAG_STRING) {
					webInterface->Show(tag->GetStringData());
				}
			}
		} else if (reply->GetOpCode() == EC_OP_FAILED) {
			tag = reply->GetFirstTagSafe();
			if (tag->IsString()) {
				webInterface->Show(
					CFormat(_("Request failed with the following error: %s.")) %
					wxString(wxGetTranslation(tag->GetStringData())));
			} else {
				webInterface->Show(_("Request failed with an unknown error."));
			}
		}
		delete reply;
	}
}

//
// Command interface
//
void CWebServerBase::Send_SharedFile_Cmd(wxString file_hash, wxString cmd, uint32 opt_arg)
{
	CECPacket *ec_cmd = 0;
	CMD4Hash fileHash;
	wxCHECK2(fileHash.Decode(file_hash), /* Do nothing. */ );

	CECTag hashtag(EC_TAG_KNOWNFILE, fileHash);
	if (cmd == wxT("prio")) {
		ec_cmd = new CECPacket(EC_OP_SHARED_SET_PRIO);
		hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO, (uint8)opt_arg));
	} else if ( cmd == wxT("prioup") ) {
		SharedFile *file = m_SharedFileInfo.GetByHash(fileHash);
		if ( file ) {
			ec_cmd = new CECPacket(EC_OP_SHARED_SET_PRIO);
			hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO,
				GetHigherPrioShared(file->nFilePriority, file->bFileAutoPriority)));
		}
	} else if ( cmd == wxT("priodown") ) {
		SharedFile *file = m_SharedFileInfo.GetByHash(fileHash);
		if ( file ) {
			ec_cmd = new CECPacket(EC_OP_SHARED_SET_PRIO);
			hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO,
				GetLowerPrioShared(file->nFilePriority, file->bFileAutoPriority)));
		}
	}

	if ( ec_cmd ) {
		ec_cmd->AddTag(hashtag);
		Send_Discard_V2_Request(ec_cmd);
		delete ec_cmd;
	}
}

void CWebServerBase::Send_ReloadSharedFile_Cmd()
{
	CECPacket ec_cmd(EC_OP_SHAREDFILES_RELOAD);
	Send_Discard_V2_Request(&ec_cmd);
}

void CWebServerBase::Send_DownloadFile_Cmd(wxString file_hash, wxString cmd, uint32 opt_arg)
{
	CECPacket *ec_cmd = 0;
	CMD4Hash fileHash;
	wxCHECK2(fileHash.Decode(file_hash), /* Do nothing. */ );

	CECTag hashtag(EC_TAG_PARTFILE, fileHash);
	if (cmd == wxT("pause")) {
		ec_cmd = new CECPacket(EC_OP_PARTFILE_PAUSE);
	} else if (cmd == wxT("resume")) {
		ec_cmd = new CECPacket(EC_OP_PARTFILE_RESUME);
	} else if (cmd == wxT("cancel")) {
		ec_cmd = new CECPacket(EC_OP_PARTFILE_DELETE);
	} else if (cmd == wxT("prio")) {
		ec_cmd = new CECPacket(EC_OP_PARTFILE_PRIO_SET);
		hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO, (uint8)opt_arg));
	} else if (cmd == wxT("prioup")) {
		DownloadFile *file = m_DownloadFileInfo.GetByHash(fileHash);
		if ( file ) {
			ec_cmd = new CECPacket(EC_OP_PARTFILE_PRIO_SET);
			hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO,
				GetHigherPrio(file->lFilePrio, file->bFileAutoPriority)));
		}
	} else if (cmd == wxT("priodown")) {
		DownloadFile *file = m_DownloadFileInfo.GetByHash(fileHash);
		if ( file ) {
			ec_cmd = new CECPacket(EC_OP_PARTFILE_PRIO_SET);
			hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO,
				GetLowerPrio(file->lFilePrio, file->bFileAutoPriority)));
		}
	}

	if ( ec_cmd ) {
		ec_cmd->AddTag(hashtag);
		Send_Discard_V2_Request(ec_cmd);
		delete ec_cmd;
	}
}

void CWebServerBase::Send_DownloadSearchFile_Cmd(wxString file_hash, uint8 cat)
{
	CMD4Hash fileHash;
	wxCHECK2(fileHash.Decode(file_hash), /* Do nothing. */ );

	CECPacket ec_cmd(EC_OP_DOWNLOAD_SEARCH_RESULT);
	CECTag link_tag(EC_TAG_KNOWNFILE, fileHash);

	link_tag.AddTag(CECTag(EC_TAG_PARTFILE_CAT, cat));
	ec_cmd.AddTag(link_tag);
	Send_Discard_V2_Request(&ec_cmd);
}

void CWebServerBase::Send_AddServer_Cmd(wxString addr, wxString port, wxString name)
{
	CECPacket ec_cmd(EC_OP_SERVER_ADD);

	ec_cmd.AddTag(CECTag(EC_TAG_SERVER_ADDRESS, addr.Trim() + wxT(":") + port.Trim()));
	ec_cmd.AddTag(CECTag(EC_TAG_SERVER_NAME, name));

	Send_Discard_V2_Request(&ec_cmd);
}

void CWebServerBase::Send_Server_Cmd(uint32 ip, uint16 port, wxString cmd)
{
	CECPacket *ec_cmd = 0;
	if ( cmd == wxT("connect") ) {
		ec_cmd = new CECPacket(EC_OP_SERVER_CONNECT);
	} else if ( cmd == wxT("remove") ) {
		ec_cmd = new CECPacket(EC_OP_SERVER_REMOVE);
	} else if ( cmd == wxT("disconnect") ) {
		ec_cmd = new CECPacket(EC_OP_SERVER_DISCONNECT);
	}
	if ( ec_cmd && ip ) {
		ec_cmd->AddTag(CECTag(EC_TAG_SERVER, EC_IPv4_t(ip, port)));
		Send_Discard_V2_Request(ec_cmd);
		delete ec_cmd;
	}
}

void CWebServerBase::Send_Search_Cmd(wxString search, wxString extention, wxString type,
	EC_SEARCH_TYPE search_type, uint32 avail, uint32 min_size, uint32 max_size)
{
	CECPacket search_req(EC_OP_SEARCH_START);
	search_req.AddTag(CEC_Search_Tag (search, search_type,
		type, extention, avail, min_size, max_size));
	Send_Discard_V2_Request(&search_req);
}

bool CWebServerBase::Send_DownloadEd2k_Cmd(wxString link, uint8 cat)
{
	CECPacket req(EC_OP_ADD_LINK);
	CECTag link_tag(EC_TAG_STRING, link);
	link_tag.AddTag(CECTag(EC_TAG_PARTFILE_CAT, cat));
	req.AddTag(link_tag);
	const CECPacket *response = webInterface->SendRecvMsg_v2(&req);
	bool result = (response->GetOpCode() == EC_OP_FAILED);
	delete response;
	return result;
}

// We have to add gz-header and some other stuff
// to standard zlib functions in order to use gzip in web pages
int CWebServerBase::GzipCompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level)
{
	static const int gz_magic[2] = {0x1f, 0x8b}; // gzip magic header
	z_stream stream = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;
	uLong crc = crc32(0L, Z_NULL, 0);
	// init Zlib stream
	// NOTE windowBits is passed < 0 to suppress zlib header
	int err = deflateInit2(&stream, level, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	if (err != Z_OK) {
		return err;
	}

	snprintf((char*)dest, *destLen, "%c%c%c%c%c%c%c%c%c%c", gz_magic[0], gz_magic[1],
		Z_DEFLATED, 0 /*flags*/, 0,0,0,0 /*time*/, 0 /*xflags*/, 255);

	// wire buffers
	stream.next_in = const_cast<Bytef*>(source);
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



/*
 * Item container implementation
 */

ServersInfo *ServerEntry::GetContainerInstance()
{
	return ServersInfo::m_This;
}

ServersInfo *ServersInfo::m_This = 0;

ServersInfo::ServersInfo(CamulewebApp *webApp) : ItemsContainer<ServerEntry>(webApp)
{
	m_This = this;

}

bool ServersInfo::ReQuery()
{
	CECPacket srv_req(EC_OP_GET_SERVER_LIST);
	const CECPacket *srv_reply = m_webApp->SendRecvMsg_v2(&srv_req);
	if (!srv_reply) {
		return false;
	}
	//
	// query succeded - flush existing values and refill
	EraseAll();
	for (CECPacket::const_iterator it = srv_reply->begin(); it != srv_reply->end(); ++it) {
		const CECTag *tag = & *it;

		ServerEntry Entry;
		Entry.sServerName =
			_SpecialChars(tag->GetTagByNameSafe(EC_TAG_SERVER_NAME)->GetStringData());
		Entry.sServerDescription =
			_SpecialChars(tag->GetTagByNameSafe(EC_TAG_SERVER_DESC)->GetStringData());
		Entry.sServerIP = tag->GetIPv4Data().StringIP(false);
		Entry.nServerIP = tag->GetIPv4Data().IP();
		Entry.nServerPort = tag->GetIPv4Data().m_port;
		Entry.nServerUsers =
			tag->GetTagByNameSafe(EC_TAG_SERVER_USERS)->GetInt();
		Entry.nServerMaxUsers =
			tag->GetTagByNameSafe(EC_TAG_SERVER_USERS_MAX)->GetInt();
		Entry.nServerFiles =
			tag->GetTagByNameSafe(EC_TAG_SERVER_FILES)->GetInt();
		AddItem(Entry);
	}
	delete srv_reply;

	return true;
}


SharedFile::SharedFile(CEC_SharedFile_Tag *tag) : CECID(tag->ID())
{
		sFileName = _SpecialChars(tag->FileName());
		lFileSize = tag->SizeFull();
		sED2kLink = _SpecialChars(tag->FileEd2kLink());
		nHash = tag->FileHash();

		ProcessUpdate(tag);
}

void SharedFile::ProcessUpdate(CEC_SharedFile_Tag *tag)
{
	nFileTransferred = tag->GetXferred();
	nFileAllTimeTransferred = tag->GetAllXferred();
	nFileRequests = tag->GetRequests();
	nFileAllTimeRequests = tag->GetAllRequests();
	nFileAccepts = tag->GetAccepts();
	nFileAllTimeAccepts = tag->GetAllAccepts();
	sFileHash = nHash.Encode();
	nFilePriority = tag->UpPrio();
	if ( nFilePriority >= 10 ) {
		bFileAutoPriority = true;
		nFilePriority -= 10;
	} else {
		bFileAutoPriority = false;
	}
}

SharedFileInfo *SharedFile::GetContainerInstance()
{
	return SharedFileInfo::m_This;
}

SharedFileInfo *SharedFileInfo::m_This = 0;

SharedFileInfo::SharedFileInfo(CamulewebApp *webApp) :
	UpdatableItemsContainer<SharedFile, CEC_SharedFile_Tag, uint32>(webApp)
{
	m_This = this;
}


bool SharedFileInfo::ReQuery()
{
	DoRequery(EC_OP_GET_SHARED_FILES, EC_TAG_KNOWNFILE);

	return true;
}


DownloadFile::DownloadFile(CEC_PartFile_Tag *tag) : CECID(tag->ID())
{
	nHash = tag->FileHash();
	sFileName = _SpecialChars(tag->FileName());
	lFileSize = tag->SizeFull();
	sFileHash = nHash.Encode();
	sED2kLink = _SpecialChars(tag->FileEd2kLink());
	lFileCompleted = tag->SizeDone();
	lFileTransferred = tag->SizeXfer();
	lFileSpeed = tag->Speed();
	fCompleted = (100.0*lFileCompleted) / lFileSize;
	wxtLastSeenComplete = wxDateTime( tag->LastSeenComplete() );

	ProcessUpdate(tag);
}

void DownloadFile::ProcessUpdate(CEC_PartFile_Tag *tag)
{
	if (!tag) {
		return;
	}

	lFilePrio = tag->DownPrio();
	if ( lFilePrio >= 10 ) {
		lFilePrio -= 10;
		bFileAutoPriority = true;
	} else {
		bFileAutoPriority = false;
	}
	nCat = tag->FileCat();

	nFileStatus = tag->FileStatus();
	lSourceCount = tag->SourceCount();
	lNotCurrentSourceCount = tag->SourceNotCurrCount();
	lTransferringSourceCount = tag->SourceXferCount();
	lSourceCountA4AF = tag->SourceCountA4AF();
	if ( lTransferringSourceCount > 0 ) {
		lFileCompleted = tag->SizeDone();
		lFileTransferred = tag->SizeXfer();
		lFileSpeed = tag->Speed();
		fCompleted = (100.0*lFileCompleted) / lFileSize;
	} else {
		lFileSpeed = 0;
	}
	CECTag *gaptag = tag->GetTagByName(EC_TAG_PARTFILE_GAP_STATUS);
	CECTag *parttag = tag->GetTagByName(EC_TAG_PARTFILE_PART_STATUS);
	CECTag *reqtag = tag->GetTagByName(EC_TAG_PARTFILE_REQ_STATUS);

	if (gaptag) {
		m_Encoder.DecodeGaps(gaptag, m_Gaps);
	}
	if (parttag) {
		m_Encoder.DecodeParts(parttag, m_PartInfo);
	}
	if (reqtag) {
		ArrayOfUInts64 reqs;
		m_Encoder.DecodeReqs(reqtag, reqs);
		int reqcount = reqs.size() / 2;
		m_ReqParts.resize(reqcount);
		for (int i = 0; i < reqcount;i++) {
			m_ReqParts[i].start = reqs[2*i];
			m_ReqParts[i].end   = reqs[2*i+1];
		}
	}
}

DownloadFileInfo *DownloadFile::GetContainerInstance()
{
	return DownloadFileInfo::m_This;
}

DownloadFileInfo *DownloadFileInfo::m_This = 0;

DownloadFileInfo::DownloadFileInfo(CamulewebApp *webApp, CImageLib *imlib) :
	UpdatableItemsContainer<DownloadFile, CEC_PartFile_Tag, uint32>(webApp)
{
	m_This = this;
	m_ImageLib = imlib;
}

void DownloadFileInfo::LoadImageParams(wxString &tpl, int width, int height)
{
	m_Template = tpl;
	m_width = width;
	m_height = height;
}

void DownloadFileInfo::ItemInserted(DownloadFile *item)
{
	item->m_Image = new CDynProgressImage(m_width, m_height, m_Template, item);

#ifdef WITH_LIBPNG
	m_ImageLib->AddImage(item->m_Image, wxT("/") + item->m_Image->Name());
#endif
}

void DownloadFileInfo::ItemDeleted(DownloadFile *item)
{
#ifdef WITH_LIBPNG
	m_ImageLib->RemoveImage(wxT("/") + item->m_Image->Name());
#else
	delete item->m_Image;
#endif
}

bool DownloadFileInfo::ReQuery()
{
	DoRequery(EC_OP_GET_DLOAD_QUEUE, EC_TAG_PARTFILE);

	return true;
}


UploadFile::UploadFile(CEC_UpDownClient_Tag *tag) : CECID(tag->ID())
{
	sUserName = _SpecialChars(tag->ClientName());
	nSpeed = tag->SpeedUp();
	nTransferredUp = tag->XferUp();
	nTransferredDown = tag->XferDown();
	nUploadFile = 0;
	tag->UploadFile(nUploadFile);
}

UploadsInfo *UploadFile::GetContainerInstance()
{
	return UploadsInfo::m_This;
}

UploadsInfo *UploadsInfo::m_This = 0;

UploadsInfo::UploadsInfo(CamulewebApp *webApp) : ItemsContainer<UploadFile>(webApp)
{
	m_This = this;
}

bool UploadsInfo::ReQuery()
{
	CECPacket up_req(EC_OP_GET_ULOAD_QUEUE);
	const CECPacket *up_reply = m_webApp->SendRecvMsg_v2(&up_req);
	if (!up_reply) {
		return false;
	}
	//
	// query succeded - flush existing values and refill
	EraseAll();
	for (CECPacket::const_iterator it = up_reply->begin(); it != up_reply->end(); ++it) {

		UploadFile curr((CEC_UpDownClient_Tag *) & *it);

		AddItem(curr);
	}
	delete up_reply;

	return true;
}

SearchFile::SearchFile(CEC_SearchFile_Tag *tag)
{
	nHash = tag->FileHash();
	sHash = nHash.Encode();
	sFileName = _SpecialChars(tag->FileName());
	lFileSize = tag->SizeFull();
	lSourceCount = tag->SourceCount();
	bPresent = tag->AlreadyHave();
}

void SearchFile::ProcessUpdate(CEC_SearchFile_Tag *tag)
{
	lSourceCount = tag->SourceCount();
}

SearchInfo *SearchFile::GetContainerInstance()
{
	return SearchInfo::m_This;
}

SearchInfo *SearchInfo::m_This = 0;

SearchInfo::SearchInfo(CamulewebApp *webApp) :
	UpdatableItemsContainer<SearchFile, CEC_SearchFile_Tag, uint32>(webApp)
{
	m_This = this;
}

bool SearchInfo::ReQuery()
{
	DoRequery(EC_OP_SEARCH_RESULTS, EC_TAG_SEARCHFILE);

	return true;
}


/*!
 * Image classes:
 *
 * CFileImage: simply represent local file
 * CDynProgressImage: dynamically generated from gap info
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

CAnyImage::CAnyImage(int width, int height) : m_width(width), m_height(height)
{
	m_size = 0;
	// allocate considering image header
	m_alloc_size = width * height * sizeof(uint32) + 0x100;
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

void CAnyImage::SetHttpType(wxString ext)
{
	m_Http = wxT("Content-Type: ") + ext + wxT("\r\n");

	time_t t = time(NULL);
	char tmp[255];
	strftime(tmp, 255, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&t));
	m_Http += wxT("Last-Modified: ") + wxString(char2unicode(tmp)) + wxT("\r\n");

	m_Http += wxT("ETag: ") + MD5Sum(char2unicode(tmp)).GetHash() + wxT("\r\n");
}

CFileImage::CFileImage(const wxString& name) : CAnyImage(0)
{
	m_size = 0;
	m_name = name;
#ifdef __WINDOWS__
	wxFFile fis(m_name, wxT("rb"));
#else
	wxFFile fis(m_name);
#endif
	// FIXME: proper logging is needed
	if ( fis.IsOpened() ) {
		size_t file_size = fis.Length();
		if ( file_size ) {
			Realloc(fis.Length());
			m_size = fis.Read(m_data,file_size);
		} else {
			printf("CFileImage: file %s have zero length\n", (const char *)unicode2char(m_name));
		}
		wxString ext = m_name.Right(3).MakeLower();
		if ( ext == wxT("css") ) {
			SetHttpType(wxT("text/css"));
		} else {
			SetHttpType(wxT("image/") + ext);
		}
	} else {
		printf("CFileImage: failed to open %s\n", (const char *)unicode2char(m_name));
	}
}

/*!
 * "Modifiers" for 3D look of progress bar. Those modifiers must be substracted from
 * image (with saturation), values, and not multiplied, as amule doesn for some reason.
 *
 */
CImage3D_Modifiers::CImage3D_Modifiers(int width)
{
	m_width = width;
	m_modifiers = new unsigned char[m_width];
	for(int i = 0; i < m_width; i++) {
		// "70" - webserver uses fixed depth
		double f_curr_mod = 30 * (1 + cos( (2 * M_PI) * ( m_width - (((double)i)/m_width) ) ) );
		m_modifiers[i] = (unsigned char)f_curr_mod;
	}
}

CImage3D_Modifiers::~CImage3D_Modifiers()
{
	delete [] m_modifiers;
}

CProgressImage::CProgressImage(int width, int height, wxString &tmpl, DownloadFile *file) :
		CAnyImage(width, height), m_template(tmpl)
{
	m_file = file;
	m_ColorLine = new uint32[m_width];
}

CProgressImage::~CProgressImage()
{
	delete [] m_ColorLine;
}

void CProgressImage::CreateSpan()
{
	// Step 1: get gap list
	const Gap_Struct * gap_list = (const Gap_Struct *) &(m_file->m_Gaps[0]);
	int gap_list_size = m_file->m_Gaps.size() / 2;

	// allocate for worst case !
	int color_gaps_alloc = 2 * (2 * gap_list_size + m_file->lFileSize / PARTSIZE + 1);
	Color_Gap_Struct *colored_gaps = new Color_Gap_Struct[color_gaps_alloc];

	// Step 2: combine gap and part status information

	// Init first item to dummy info, so we will always have "previous" item
	int colored_gaps_size = 0;
	colored_gaps[0].start = 0;
	colored_gaps[0].end = 0;
	colored_gaps[0].color = 0xffffffff;
	for (int j = 0; j < gap_list_size;j++) {
		uint64 gap_start = gap_list[j].start;
		uint64 gap_end = gap_list[j].end;

		uint32 start = gap_start / PARTSIZE;
		uint32 end = (gap_end / PARTSIZE) + 1;

		for (uint32 i = start; i < end; i++) {
			COLORTYPE color = RGB(255, 0, 0);
			if (m_file->m_PartInfo.size() > i && m_file->m_PartInfo[i]) {
				int blue = 210 - ( 22 * ( m_file->m_PartInfo[i] - 1 ) );
				color = RGB( 0, ( blue < 0 ? 0 : blue ), 255 );
			}

			uint64 fill_gap_begin = ( (i == start)   ? gap_start: PARTSIZE * i );
			uint64 fill_gap_end   = ( (i == (end - 1)) ? gap_end   : PARTSIZE * ( i + 1 ) );

			wxASSERT(colored_gaps_size < color_gaps_alloc);

			if ( (colored_gaps[colored_gaps_size].end == fill_gap_begin) &&
				(colored_gaps[colored_gaps_size].color == color) ) {
				colored_gaps[colored_gaps_size].end = fill_gap_end;
			} else {
				colored_gaps_size++;
				colored_gaps[colored_gaps_size].start = fill_gap_begin;
				colored_gaps[colored_gaps_size].end = fill_gap_end;
				colored_gaps[colored_gaps_size].color = color;
			}
		}

	}
	//
	// Now line rendering
	for(int i = 0; i < m_width; ++i) {
		m_ColorLine[i] = 0x0;
	}
	if (m_file->lFileSize < (uint32)m_width) {
		//
		// if file is that small, draw it in single step
		//
		if (!m_file->m_ReqParts.empty()) {
			for(int i = 0; i < m_width; ++i) {
				m_ColorLine[i] = RGB(255, 208, 0);
			}
		} else if ( colored_gaps_size ) {
			for(int i = 0; i < m_width; ++i) {
				m_ColorLine[i] = colored_gaps[1].color;
			}
		}
	} else {
		uint32 factor = m_file->lFileSize / m_width;
		for (int i = 1; i <= colored_gaps_size;i++) {
			uint32 start = colored_gaps[i].start / factor;
			uint32 end = colored_gaps[i].end / factor;
			if ((int)end > m_width) {
				end = m_width;
			}
			for (uint32 j = start; j < end; j++) {
				m_ColorLine[j] = colored_gaps[i].color;
			}
		}
		// overwrite requested parts
		for (uint32 i = 0; i < m_file->m_ReqParts.size(); i++) {
			uint32 start = m_file->m_ReqParts[i].start / factor;
			uint32 end = m_file->m_ReqParts[i].end / factor;
			if ((int)end > m_width) {
				end = m_width;
			}
			for (uint32 j = start; j < end; j++) {
				m_ColorLine[j] = RGB(255, 208, 0);
			}
		}
	}
	delete [] colored_gaps;
}

#ifdef WITH_LIBPNG

CDynPngImage::CDynPngImage(int w, int h) : CAnyImage(w, h)
{

	//
	// Allocate array of "row pointers" - libpng need it in this form
	// Fill it also with the image data
	//
	m_img_data = new png_byte[3*m_width*m_height];
	memset(m_img_data, 0, 3*m_width*m_height);
	m_row_ptrs = new png_bytep [m_height];
	for (int i = 0; i < m_height;i++) {
		m_row_ptrs[i] = &m_img_data[3*m_width*i];
	}

}

CDynPngImage::~CDynPngImage()
{
	delete [] m_row_ptrs;
	delete [] m_img_data;
}

void CDynPngImage::png_write_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
	CDynPngImage *This = static_cast<CDynPngImage *>(png_get_io_ptr(png_ptr));
	wxASSERT((png_size_t)(This->m_size + length) <= (png_size_t)This->m_alloc_size);
	memcpy(This->m_data + This->m_size, data, length);
	This->m_size += length;
}


unsigned char *CDynPngImage::RequestData(int &size)
{
	// write png into buffer
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	png_set_IHDR(png_ptr, info_ptr, m_width, m_height, 8, PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_set_write_fn(png_ptr, this, png_write_fn, 0);

	m_size = 0;
	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, (png_bytep *)m_row_ptrs);
	png_write_end(png_ptr, 0);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	return CAnyImage::RequestData(size);
}

CDynProgressImage::CDynProgressImage(int width, int height, wxString &tmpl, DownloadFile *file) :
	CAnyImage(width, height),
	CProgressImage(width, height, tmpl, file),
	CDynPngImage(width, height),
	m_modifiers(height)
{
	m_name = wxT("dyn_") + m_file->sFileHash + wxT(".png");
}

CDynProgressImage::~CDynProgressImage()
{
}

wxString CDynProgressImage::GetHTML()
{
	// template contain %s (name) %d (width) %s (alt)
	return (CFormat(m_template) % m_name % m_width % wxT("Progress bar")).GetString();
}

void CDynProgressImage::DrawImage()
{
	CreateSpan();

	for(int i = 0; i < m_height; i++) {
			memset(m_row_ptrs[i], 0, 3*m_width);
	}
	for(int i = 0; i < m_height/2; i++) {
		png_bytep u_row = m_row_ptrs[i];
		png_bytep d_row = m_row_ptrs[m_height-i-1];
		for(int j = 0; j < m_width; j++) {
			set_rgb_color_val(u_row+3*j, m_ColorLine[j], m_modifiers[i]);
			set_rgb_color_val(d_row+3*j, m_ColorLine[j], m_modifiers[i]);
		}
	}
}

unsigned char *CDynProgressImage::RequestData(int &size)
{
	// create new one
	DrawImage();

	return CDynPngImage::RequestData(size);
}

#else

CDynProgressImage::CDynProgressImage(int width, int height, wxString &tmpl, DownloadFile *file) :
	CAnyImage(width, height),
	CProgressImage(width, height, tmpl, file)
{
	m_name = wxT("dyn_") + m_file->sFileHash + wxT(".png");

}


wxString CDynProgressImage::GetHTML()
{
	static wxChar *progresscolor[12] = {
		wxT("transparent.gif"), wxT("black.gif"), wxT("yellow.gif"), wxT("red.gif"),
		wxT("blue1.gif"),       wxT("blue2.gif"), wxT("blue3.gif"),  wxT("blue4.gif"),
		wxT("blue5.gif"),       wxT("blue6.gif"), wxT("green.gif"),  wxT("greenpercent.gif") };

	CreateSpan();

	wxString str;
	uint32 lastcolor = m_ColorLine[0];
	int lastindex = 0;
	for(int i = 0; i < m_width; i++) {
		if ( (lastcolor != m_ColorLine[i]) || (i == (m_width - 1)) ) {
			int color_idx = -1;
			if ( lastcolor & RGB(0, 0, 0xff) ) { // blue
				int green = (lastcolor >> 8) & 0xff;
				// reverse calculation:  green = 210 - ( 22 * ( part_info[i] - 1 ) )
				wxASSERT( !green || (green < 211) );
				color_idx = (green) ? (210 - green) / 22 + 1 : 11;
				// now calculate it same way as PartFile did
				color_idx = (color_idx > 10) ? 9 : (4 + color_idx / 2);
			} else {
				if ( lastcolor & RGB(0, 0xff, 0) ) { // yellow
					color_idx = 2;
				} else {
					if ( lastcolor & RGB(0xff, 0, 0) ) { // red
						color_idx = 3;
					} else {
						color_idx = 1;
					}
				}
			}
			str += (CFormat(m_template) % progresscolor[color_idx]
						% (i - lastindex) % wxString(progresscolor[color_idx]).BeforeLast(wxT('.'))).GetString();
			lastindex = i;
			lastcolor = m_ColorLine[i];
		}
	}

	return str;
}

#endif

CStatsData::CStatsData(int size)
{
	m_size = size;
	m_data = new uint32[size];
	m_max_value = 0;
	//
	// initial situation: all data is 0's
	//
	memset(m_data, 0, m_size*sizeof(int));
	m_start_index = m_curr_index = 0;
	m_end_index = size - 1;
}

CStatsData::~CStatsData()
{
	delete [] m_data;
}

uint32 CStatsData::GetFirst()
{
	m_curr_index = m_start_index;
	return m_data[m_curr_index];
}

uint32 CStatsData::GetNext()
{
	m_curr_index++;
	m_curr_index %= m_size;
	return m_data[m_curr_index];
}

void CStatsData::PushSample(uint32 sample)
{
	m_start_index = (m_start_index + 1) % m_size;
	m_end_index = (m_end_index + 1) % m_size;
	m_data[m_start_index] = sample;

	if ( m_max_value < sample ) {
		m_max_value = sample;
	}
}

CStatsCollection::CStatsCollection(int size, CamulewebApp *iface)
{
	m_down_speed = new CStatsData(size);
	m_up_speed = new CStatsData(size);
	m_conn_number = new CStatsData(size);
	m_kad_count = new CStatsData(size);

	m_iface = iface;
	m_LastTimeStamp = 0.0;
	m_size = size;
}

CStatsCollection::~CStatsCollection()
{
	delete m_down_speed;
	delete m_up_speed;
	delete m_conn_number;
	delete m_kad_count;
}

void CStatsCollection::ReQuery()
{
	CECPacket request(EC_OP_GET_STATSGRAPHS);

	request.AddTag(CECTag(EC_TAG_STATSGRAPH_WIDTH, (uint16)m_size));

	uint16 m_nGraphScale = 1;
	request.AddTag(CECTag(EC_TAG_STATSGRAPH_SCALE, m_nGraphScale));
	if (m_LastTimeStamp > 0.0) {
		request.AddTag(CECTag(EC_TAG_STATSGRAPH_LAST, m_LastTimeStamp));
	}

	const CECPacket *response = m_iface->SendRecvMsg_v2(&request);

	m_LastTimeStamp = response->GetTagByNameSafe(EC_TAG_STATSGRAPH_LAST)->GetDoubleData();

	const CECTag *dataTag = response->GetTagByName(EC_TAG_STATSGRAPH_DATA);
	const uint32 *data = (const uint32 *)dataTag->GetTagData();
	unsigned int count = dataTag->GetTagDataLen() / sizeof(uint32);
	for (unsigned int i = 0; i < count; i += 4) {
		m_down_speed->PushSample(ENDIAN_NTOHL(data[i+0]));
		m_up_speed->PushSample(ENDIAN_NTOHL(data[i+1]));
		m_conn_number->PushSample(ENDIAN_NTOHL(data[i+2]));
		m_kad_count->PushSample(ENDIAN_NTOHL(data[i+3]));
	}
}

//
// Dynamically generated statistic images
//
#ifdef WITH_LIBPNG

CDynStatisticImage::CDynStatisticImage(int height, bool scale1024, CStatsData *data) :
	CAnyImage(data->Size(), height), CDynPngImage(data->Size(), height)
{
	m_data = data;
	m_scale1024 = scale1024;

	// actual name doesn't matter, just make it unique
	m_name = CFormat(wxT("dyn_%p_stat.png")) % data;

	m_num_font_w_size = 8;
	m_num_font_h_size = 16;

	// leave enough space for 4 digit number
	int img_delta = m_num_font_w_size / 4;
	m_left_margin = 4*(m_num_font_w_size + img_delta) + img_delta;
	// leave enough space for number height
	m_bottom_margin = m_num_font_h_size;


	m_y_axis_size = m_height - m_bottom_margin;
	// allocate storage for background. Using 1 chunk to speed up
	// the rendering
	m_background = new png_byte[m_width*m_height*3];
	m_row_bg_ptrs = new png_bytep[m_height];
	for(int i = 0; i < m_height; i++) {
		m_row_bg_ptrs[i] = &m_background[i*m_width*3];
	}

	//
	// Prepare background
	//
	static const COLORTYPE bg_color = RGB(0x00, 0x00, 0x40);
	for(int i = 0; i < m_height; i++) {
		png_bytep u_row = m_row_bg_ptrs[i];
		for(int j = 0; j < m_width; j++) {
			set_rgb_color_val(u_row+3*j, bg_color, 0);
		}
	}
	//
	// draw axis
	//
	static const COLORTYPE axis_color = RGB(0xff, 0xff, 0xff);
	// Y
	for(int i = m_bottom_margin; i < m_y_axis_size; i++) {
		png_bytep u_row = m_row_bg_ptrs[i];
		set_rgb_color_val(u_row+3*(m_left_margin + 0), axis_color, 0);
		set_rgb_color_val(u_row+3*(m_left_margin + 1), axis_color, 0);
	}
	// X
	for(int j = m_left_margin; j < m_width; j++) {
		set_rgb_color_val(m_row_bg_ptrs[m_y_axis_size - 0]+3*j, axis_color, 0);
		set_rgb_color_val(m_row_bg_ptrs[m_y_axis_size - 1]+3*j, axis_color, 0);
	}

	// horisontal grid
	int v_grid_size = m_y_axis_size / 4;
	for(int i = m_y_axis_size - v_grid_size; i >= v_grid_size; i -= v_grid_size) {
		png_bytep u_row = m_row_bg_ptrs[i];
		for(int j = m_left_margin; j < m_width; j++) {
			if ( (j % 10) < 5 ) {
				set_rgb_color_val(u_row+3*j, axis_color, 0);
			}
		}
	}

	//
	// Pre-create masks for digits
	//
	for(int i = 0; i < 10; i++) {
		m_digits[i] = new CNumImageMask(i, m_num_font_w_size, m_num_font_h_size);
	}
}

CDynStatisticImage::~CDynStatisticImage()
{
	delete [] m_row_bg_ptrs;
	delete [] m_background;
	for(int i = 0; i < 10; i++) {
		delete m_digits[i];
	}
}

void CDynStatisticImage::DrawImage()
{
	// copy background first
	memcpy(m_img_data, m_background, m_width*m_height*3);

	//
	// Now graph itself
	//
	static const COLORTYPE graph_color = RGB(0xff, 0x00, 0x00);
	int maxval = m_data->Max();

	if ( m_scale1024 ) {
		if ( maxval > 1024 ) {
			maxval /= 1024;
		} else {
			maxval = 1;
		}
	}
	//
	// Check if we need to scale data up or down
	//
	int m_scale_up = 1, m_scale_down = 1;
	if ( maxval >= (m_height - m_bottom_margin) ) {
		m_scale_down = 1 + (maxval / (m_y_axis_size - 10));
	}
	// if maximum value is 1/5 or less of graph height - scale it UP, to make 2/3
	if ( maxval && (maxval < (m_y_axis_size / 5)) ) {
		m_scale_up = (2*m_y_axis_size / 3) / maxval;
	}

	//
	// draw axis scale
	//
	int img_delta = m_num_font_w_size / 4;
	// Number "0" is always there
	m_digits[0]->Apply(m_row_ptrs, 3*img_delta+2*m_num_font_w_size, m_y_axis_size-m_num_font_h_size-5);

	//
	// When data is scaled down, axis are scaled UP and viceversa
	int y_axis_max = m_y_axis_size;
	if ( m_scale_down != 1 ) {
		y_axis_max *= m_scale_down;
	} else if ( m_scale_up != 1 ) {
		y_axis_max /= m_scale_up;
	}

	// X---
	if ( y_axis_max > 999 ) {
		m_digits[y_axis_max / 1000]->Apply(m_row_ptrs, img_delta, img_delta);
	}
	// -X--
	if ( y_axis_max > 99 ) {
		m_digits[(y_axis_max % 1000) / 100]->Apply(m_row_ptrs,
			2*img_delta+m_num_font_w_size, img_delta);
	}
	// --X-
	if ( y_axis_max > 9 ) {
		m_digits[(y_axis_max % 100) / 10]->Apply(m_row_ptrs,
			3*img_delta+2*m_num_font_w_size, img_delta);
	}
	// ---X
	m_digits[y_axis_max % 10]->Apply(m_row_ptrs, 4*img_delta+3*m_num_font_w_size, img_delta);

	int prev_data = m_data->GetFirst();
	if ( m_scale_down != 1 ) {
		prev_data /= m_scale_down;
	} else if ( m_scale_up != 1 ) {
		prev_data *= m_scale_up;
	}
	if ( m_scale1024 ) {
		if ( prev_data > 1024) {
			prev_data /= 1024;
		} else {
			prev_data = 1;
		}
	}
	for(int j = m_left_margin + 1, curr_data = m_data->GetNext(); j < m_width; j++, curr_data = m_data->GetNext()) {
		if ( m_scale_down != 1 ) {
			curr_data /= m_scale_down;
		} else if ( m_scale_up != 1 ) {
			curr_data *= m_scale_up;
		}
		if ( m_scale1024 ) {
			if ( curr_data > 1024) {
				curr_data /= 1024;
			} else {
				curr_data = 1;
			}
		}
		//
		// draw between curr_data and prev_data
		//
		int min_y, max_y;
		if ( prev_data > curr_data ) {
			min_y = curr_data; max_y = prev_data;
		} else {
			min_y = prev_data; max_y = curr_data;
		}
		for(int k = min_y; k <= max_y; k++) {
			int i = m_y_axis_size - k;
			png_bytep u_row = m_row_ptrs[i];
			set_rgb_color_val(u_row+3*j, graph_color, 0);
		}
		prev_data = curr_data;
	}
}

unsigned char *CDynStatisticImage::RequestData(int &size)
{
	DrawImage();

	return CDynPngImage::RequestData(size);
}

wxString CDynStatisticImage::GetHTML()
{
	return wxEmptyString;
}

//
// Imprint numbers on generated png's
//
//                                                 0     1     2     3     4     5     6     7     8     9
const int CNumImageMask::m_num_to_7_decode[] = {0x77, 0x24, 0x5d, 0x6d, 0x2e, 0x5d, 0x7a, 0x25, 0x7f, 0x2f};

CNumImageMask::CNumImageMask(int number, int width, int height)
{
	m_v_segsize = height / 2;
	m_h_segsize = width;
	m_height = height;
	m_width = width;

	m_row_mask_ptrs = new png_bytep[m_height];
	for(int i = 0; i < m_height; i++) {
		m_row_mask_ptrs[i] = new png_byte[3*m_width];
		memset(m_row_mask_ptrs[i], 0x00, 3*m_width);
	}

	int seg_status = m_num_to_7_decode[number];
	for(int i = 0; i < 7; i++) {
		if ( seg_status & (1 << i) ) {
			DrawSegment(i);
		}
	}
}

CNumImageMask::~CNumImageMask()
{
	for(int i = 0; i < m_height; i++) {
		delete [] m_row_mask_ptrs[i];
	}
	delete [] m_row_mask_ptrs;
}

void CNumImageMask::Apply(png_bytep *image, int offx, int offy)
{
	offx *= 3;
	for(int i = 0; i < m_height; i++) {
		png_bytep img_row = image[offy + i];
		png_bytep num_row = m_row_mask_ptrs[i];
		for(int j = 0; j < m_width*3; j++) {
			img_row[offx + j] |= num_row[j];
		}
	}
}

void CNumImageMask::DrawHorzLine(int off)
{
	png_bytep m_row = m_row_mask_ptrs[off*(m_v_segsize-1)];
	for(int i = 0; i < m_h_segsize; i++) {
		m_row[i*3] = m_row[i*3+1] = m_row[i*3+2] = 0xff;
	}
}

void CNumImageMask::DrawVertLine(int offx, int offy)
{
	for(int i = 0; i < m_v_segsize; i++) {
		png_bytep m_row = m_row_mask_ptrs[offy*(m_v_segsize-1)+i];
		int x_idx = offx*(m_h_segsize-1)*3;
		m_row[x_idx] = m_row[x_idx+1] = m_row[x_idx+2] = 0xff;
	}
}

/*
 * Segment id decoding defined as following
 *
 *   ---- 0 ----
 *   |         |
 *   1         2
 *   |___ 3 ___|
 *   |         |
 *   4         5
 *   |___ 6 ___|
 */
void CNumImageMask::DrawSegment(int id)
{
	switch(id) {
		case 0:
			DrawHorzLine(0);
			break;
		case 1:
			DrawVertLine(0, 0);
			break;
		case 2:
			DrawVertLine(1, 0);
			break;
		case 3:
			DrawHorzLine(1);
			break;
		case 4:
			DrawVertLine(0, 1);
			break;
		case 5:
			DrawVertLine(1, 1);
			break;
		case 6:
			DrawHorzLine(2);
			break;
		default:
			wxFAIL;
			break;
	}
}

#endif

CImageLib::CImageLib(wxString image_dir) : m_image_dir(image_dir)
{
}

CImageLib::~CImageLib()
{
}

void CImageLib::AddImage(CAnyImage *img, const wxString &name)
{
	ImageMap::iterator it = m_image_map.find(name);
	if (it == m_image_map.end()) {
		m_image_map[name] = img;
	} else {
		delete it->second;
		it->second = img;
	}
}

void CImageLib::RemoveImage(const wxString &name)
{
	ImageMap::iterator it = m_image_map.find(name);
	if (it != m_image_map.end()) {
		delete it->second;
		m_image_map.erase(it);
	}
}

CAnyImage *CImageLib::GetImage(const wxString &name)
{
	ImageMap::iterator it = m_image_map.find(name);
	if (it != m_image_map.end()) {
		return it->second;
	}
	wxFileName filename(m_image_dir + name);
	CFileImage *fimg = new CFileImage(filename.GetFullPath());
	if ( fimg->OpenOk() ) {
		m_image_map[name] = fimg;
		return fimg;
	} else {
		delete fimg;
		return 0;
	}
}

/*
 * Script-based webserver
 */
CScriptWebServer::CScriptWebServer(CamulewebApp *webApp, const wxString& templateDir)
	: CWebServerBase(webApp, templateDir), m_wwwroot(templateDir)
{
	wxString img_tmpl(wxT("<img src=\"%s\" height=\"20\" width=\"%d\" alt=\"%s\">"));
	m_DownloadFileInfo.LoadImageParams(img_tmpl, 200, 20);

	if ( ::wxFileExists(wxFileName(m_wwwroot, wxT("index.html")).GetFullPath()) ) {
		m_index = wxT("index.html");
	} else if ( ::wxFileExists(wxFileName(m_wwwroot, wxT("index.php")).GetFullPath()) ) {
		m_index = wxT("index.php");
	} else {
		webInterface->Show(_("Index file not found: ") +
			wxFileName(m_wwwroot, wxT("index.{html,php}")).GetFullPath() + wxT("\n"));
	}
}

CScriptWebServer::~CScriptWebServer()
{
}

char *CScriptWebServer::GetErrorPage(const char *message, long &size)
{
	char *buf = new char [1024];
	snprintf(buf, 1024,
		"<html><title> Error -%s </title></html>", message);

	size = strlen(buf);

	return buf;
}

char *CScriptWebServer::Get_404_Page(long &size)
{
		char *buf = new char [1024];
		strcpy(buf, "<html><title> Error - requested page not found </title></html>");

		size = strlen(buf);

		return buf;
}

char *CScriptWebServer::ProcessHtmlRequest(const char *filename, long &size)
{
	FILE *f = fopen(filename, "r");
	if ( !f ) {
		return Get_404_Page(size);
	}
	if ( fseek(f, 0, SEEK_END) != 0 ) {
		fclose(f);
		return GetErrorPage("fseek failed", size);
	}

	size = ftell(f);
	char *buf = new char [size+1];
	rewind(f);
	// fread may actually read less if it is a CR-LF-file in Windows
	size = fread(buf, 1, size, f);
	buf[size] = 0;
	fclose(f);

	return buf;
}

char *CScriptWebServer::ProcessPhpRequest(const char *filename, CSession *sess, long &size)
{
	FILE *f = fopen(filename, "r");
	if ( !f ) {
		return Get_404_Page(size);
	}

	CWriteStrBuffer buffer;
	CPhpFilter(this, sess, filename, &buffer);

	size = buffer.Length();
	char *buf = new char [size+1];
	buffer.CopyAll(buf);

	fclose(f);

	return buf;
}

CSession *CScriptWebServer::CheckLoggedin(ThreadData &Data)
{
	time_t curr_time = time(0);
	CSession *session = 0;
	if ( Data.SessionID && m_sessions.count(Data.SessionID) ) {
		session = &m_sessions[Data.SessionID];
		// session times out in 2 hours
		if ( (curr_time - session->m_last_access) > 7200 ) {
			Print(_("Session expired - requesting login\n"));
			m_sessions.erase(Data.SessionID);
			session = 0;
		} else {
			if ( session->m_loggedin ) {
				Print(_("Session ok, logged in\n"));
			} else {
				Print(_("Session ok, not logged in\n"));
			}
			session->m_last_access = curr_time;
		}
	} else {
		Print(_("No session opened - will request login\n"));
	}
	if ( !session ) {
		while ( !Data.SessionID || m_sessions.count(Data.SessionID) ) {
			Data.SessionID = rand();
		}
		session = &m_sessions[Data.SessionID];
		session->m_last_access = curr_time;
		session->m_loggedin = false;
		Print(_("Session created - requesting login\n"));
	}
	Data.parsedURL.ConvertParams(session->m_get_vars);
	return session;
}


void CScriptWebServer::ProcessURL(ThreadData Data)
{
	long httpOutLen;
	char *httpOut = 0;

	// Strip out any path-component to prevent information leakage.
	wxString filename = wxFileName(Data.parsedURL.File()).GetFullName();

	Print(_("Processing request [original]: ") + filename + wxT("\n"));

	if ( filename.Length() == 0 ) {
		filename = m_index;
	}

	CSession *session = CheckLoggedin(Data);

	session->m_vars["login_error"] = "";
	if ( !session->m_loggedin ) {
		filename = wxT("login.php");

		wxString PwStr(Data.parsedURL.Param(wxT("pass")));
		if (webInterface->m_AdminPass.IsEmpty() && webInterface->m_GuestPass.IsEmpty()) {
			session->m_vars["login_error"] = "No password specified, login will not be allowed.";
			Print(_("No password specified, login will not be allowed."));
		} else if ( PwStr.Length() ) {
			Print(_("Checking password\n"));
			session->m_loggedin = false;

			CMD4Hash PwHash;
			if (!PwHash.Decode(MD5Sum(PwStr).GetHash())) {
				Print(_("Password hash invalid\n"));
				session->m_vars["login_error"] = "Invalid password hash, please report on http://forum.amule.org";
			} else if ( PwHash == webInterface->m_AdminPass ) {
				session->m_loggedin = true;
				// m_vars is map<string, string> - so _() will not work here !
				session->m_vars["guest_login"] = "0";
			} else if ( PwHash == webInterface->m_GuestPass ) {
				session->m_loggedin = true;
				session->m_vars["guest_login"] = "1";
			} else {
				session->m_vars["login_error"] = "Password incorrect, please try again.";
			}

			if ( session->m_loggedin ) {
				filename = m_index;
				Print(_("Password ok\n"));
			} else {
				Print(_("Password bad\n"));
			}
		} else {
			Print(_("You did not enter any password. Blank password is not allowed.\n"));
		}
	} else {
		//
		// if logged in, but requesting login page again,
		// means logout command
		//
		if ( filename == wxT("login.php") ) {
			Print(_("Logout requested\n"));
			session->m_loggedin = false;
		}
	}

	Print(_("Processing request [redirected]: ") + filename + wxT("\n"));

	session->m_vars["auto_refresh"] = (const char *)unicode2char(
		wxString(CFormat(wxT("%d")) % webInterface->m_PageRefresh));
	session->m_vars["content_type"] = "text/html";

	wxString req_file(wxFileName(m_wwwroot, filename).GetFullPath());
	if (req_file.EndsWith(wxT(".html"))) {
		httpOut = ProcessHtmlRequest(unicode2char(req_file), httpOutLen);
	} else if (req_file.EndsWith(wxT(".php"))) {
		httpOut = ProcessPhpRequest(unicode2char(req_file), session, httpOutLen);
	} else if (req_file.EndsWith(wxT(".css"))) {
		session->m_vars["content_type"] = "text/css";
		httpOut = ProcessHtmlRequest(unicode2char(req_file), httpOutLen);
	} else if (req_file.EndsWith(wxT(".js"))) {
		session->m_vars["content_type"] = "text/javascript";
		httpOut = ProcessHtmlRequest(unicode2char(req_file), httpOutLen);
	} else if (	req_file.EndsWith(wxT(".dtd"))
				|| req_file.EndsWith(wxT(".xsd"))
				|| req_file.EndsWith(wxT(".xsl"))) {
		session->m_vars["content_type"] = "text/xml";
		httpOut = ProcessHtmlRequest(unicode2char(req_file), httpOutLen);
	} else if (req_file.EndsWith(wxT(".appcache"))
		   || req_file.EndsWith(wxT(".manifest"))) {
		session->m_vars["content_type"] = "text/cache-manifest";
		httpOut = ProcessHtmlRequest(unicode2char(req_file), httpOutLen);
	} else if (req_file.EndsWith(wxT(".json"))) {
		session->m_vars["content_type"] = "application/json";
		httpOut = ProcessHtmlRequest(unicode2char(req_file), httpOutLen);
	} else {
		httpOut = GetErrorPage("aMuleweb doesn't handle the requested file type ", httpOutLen);
	}

	bool isUseGzip = webInterface->m_UseGzip;

	if (isUseGzip)	{
		bool bOk = false;
		uLongf destLen = httpOutLen + 1024;
		char *gzipOut = new char[destLen];
		if( GzipCompress((Bytef*)gzipOut, &destLen,
		   (const Bytef*)httpOut, httpOutLen, Z_DEFAULT_COMPRESSION) == Z_OK) {
			bOk = true;
		}
		if ( bOk ) {
			delete [] httpOut;
			httpOut = gzipOut;
			httpOutLen = destLen;
		} else {
			isUseGzip = false;
			delete[] gzipOut;
		}
	}

	if ( httpOut ) {
		Data.pSocket->SendHttpHeaders(session->m_vars["content_type"].c_str(), isUseGzip, httpOutLen, Data.SessionID);
		Data.pSocket->SendData(httpOut, httpOutLen);
		delete [] httpOut;
	}
}

CNoTemplateWebServer::CNoTemplateWebServer(CamulewebApp *webApp) : CScriptWebServer(webApp, wxEmptyString)
{
}

CNoTemplateWebServer::~CNoTemplateWebServer()
{
}

void CNoTemplateWebServer::ProcessURL(ThreadData Data)
{
	/*
	 * Since template has not been found, I suspect that installation is broken. Falling back
	 * into hardcoded page as last resort.
	 */
	const char *httpOut = ""
	"<html>"
		"<head>"
			"<title>aMuleWeb error page</title>"
			"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
		"</head>"
		"<body>"
			"<p>You are seeing this page instead of aMuleWeb login page because a valid template has not been found.</p>"
			"<p>This probably means that there's a problem with your aMule installation </p>"
			"<ul>"
				"<li>Before installing new versions, please ensure that you uninstalled older versions of aMule.</li>"
				"<li>If you are installing by compiling from source, check configuration and run &quot;make&quot; and &quot;make install&quot; again </li>"
				"<li>If you are installing by using a precompiled package, you may need to contact the package maintainer </li>"
			"</ul>"
			"<p>For more information please visit</p>"
			"<p><a href=\"http://www.amule.org\">aMule main site</a> or <a href=\"http://forum.amule.org\">aMule forums</a></p>"
		"</body>"
	"</html>";

	long httpOutLen = strlen(httpOut);

	Data.pSocket->SendHttpHeaders("text/html", false, httpOutLen, 0);
	Data.pSocket->SendData(httpOut, httpOutLen);
}

// Dummy functions for EC logging
bool ECLogIsEnabled()
{
	return false;
}

void DoECLogLine(const wxString &)
{
}


// File_checked_for_headers
