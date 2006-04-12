//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 Kry ( elkry@sourceforge.net / http://www.amule.org )
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifdef HAVE_CONFIG_H
	#include "config.h"		// Needed for VERSION
#endif

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cmath> // Needed for cos, M_PI
#include <string>

#include "WebServer.h"

//-------------------------------------------------------------------

#include <wx/arrimpl.cpp>	// this is a magic incantation which must be done!
#include <wx/tokenzr.h>		// for wxTokenizer
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/filename.h>

#include <ec/ECFileConfig.h>	// Needed for CECFileConfig
#include <ec/ECSpecialTags.h>
#include <ec/ECCodes.h>
#include <common/MD5Sum.h>
#include <common/Format.h>		// Needed for CFormat

//-------------------------------------------------------------------

#include "GetTickCount.h"	// Needed for GetTickCount
#include "OtherStructs.h"	// Needed for TransferredData
#include "OtherFunctions.h"	// Needed for atoll, ED2KFT_*
#include "NetworkFunctions.h"	// Needed for StringIPtoUint32
#include "Types.h"
#include "WebSocket.h"		// Needed for StopSockets()
#include "Color.h"		// Needed for COLORREF and RGB()
#include "ArchSpecific.h"	// Needed for ENDIAN_NTOHL()

#include "php_syntree.h"
#include "php_core_lib.h"

//-------------------------------------------------------------------

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
 * Url string decoder and parser
 */
CUrlDecodeTable::CUrlDecodeTable()
{
	for (int i = 0 ; i < 256 ; i++) {
		char fromReplace[4];		// decode URL
		snprintf(fromReplace, sizeof(fromReplace), "%%%02x", i);
		m_enc_l_str[i] = char2unicode(fromReplace);

		snprintf(fromReplace, sizeof(fromReplace), "%%%02X", i);
		m_enc_u_str[i] = char2unicode(fromReplace);

		char toReplace[2] = {(char)i, 0};	// decode URL
		m_dec_str[i] = char2unicode(toReplace);
	}
}

void CUrlDecodeTable::DecodeString(wxString &str)
{
	str.Replace(wxT("+"), wxT(" "));
	for (int i = 0 ; i < 256 ; i++) {
		str.Replace(m_enc_l_str[i], m_dec_str[i]);
		str.Replace(m_enc_u_str[i], m_dec_str[i]);
	}
}

CUrlDecodeTable*	CUrlDecodeTable::ms_instance;
wxCriticalSection	CUrlDecodeTable::ms_instance_guard;

CUrlDecodeTable* CUrlDecodeTable::GetInstance()
{
	wxCriticalSectionLocker lock(ms_instance_guard);
	if (ms_instance == NULL) {
		ms_instance = new CUrlDecodeTable();
	}
	return ms_instance;
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
	    	CUrlDecodeTable::GetInstance()->DecodeString(val);
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
	for(std::map<wxString, wxString>::iterator i = m_params.begin(); i != m_params.end(); i++) {
		std::string key(unicode2char(i->first)), value(unicode2char(i->second));
		dst[key] = value;
	}
}

CWebServerBase::CWebServerBase(CamulewebApp *webApp, const wxString& templateDir) :
	m_ServersInfo(webApp), m_SharedFileInfo(webApp), m_DownloadFileInfo(webApp, &m_ImageLib),
	m_UploadsInfo(webApp), m_SearchInfo(webApp), m_Stats(500, webApp),
	m_ImageLib(templateDir)
{
	webInterface = webApp;
	m_mutexChildren = new wxMutex();
	
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
}

//sends output to web interface
void CWebServerBase::Print(const wxString &s)
{
	webInterface->Show(s);
}

//returns web server listening port
long CWebServerBase::GetWSPrefs(void)
{
	CECPacket req(EC_OP_GET_PREFERENCES);
	req.AddTag(CECTag(EC_TAG_SELECT_PREFS, (uint32)EC_PREFS_REMOTECONTROLS));
	const CECPacket *reply = webInterface->SendRecvMsg_v2(&req);
	if (!reply) {
		return -1;
	}
	// we have selected only the webserver preferences
	const CECTag *wsprefs = reply->GetTagByIndexSafe(0);
	const CECTag *tag = wsprefs->GetTagByName(EC_TAG_WEBSERVER_PORT);
	long int wsport = tag ? (long int)tag->GetInt() : -1;

	if (webInterface->m_LoadSettingsFromAmule) {
		webInterface->m_AdminPass = wsprefs->GetTagByNameSafe(EC_TAG_PASSWD_HASH)->GetMD4Data();

		const CECTag *webserverGuest = wsprefs->GetTagByName(EC_TAG_WEBSERVER_GUEST);
		if (webserverGuest) {
			webInterface->m_AllowGuest = true;
			webInterface->m_GuestPass = webserverGuest->GetTagByNameSafe(EC_TAG_PASSWD_HASH)->GetMD4Data();
		} else {
			webInterface->m_AllowGuest = false;
		}

		// we only need to check the presence of this tag
		webInterface->m_UseGzip = wsprefs->GetTagByName(EC_TAG_WEBSERVER_USEGZIP) != NULL;
	
		const CECTag *webserverRefresh = wsprefs->GetTagByName(EC_TAG_WEBSERVER_REFRESH);
		if (webserverRefresh) {
			webInterface->m_PageRefresh = webserverRefresh->GetInt();
		} else {
			webInterface->m_PageRefresh = 120;
		}
	}

	delete reply;

	return wsport;
}

void CScriptWebServer::ProcessImgFileReq(ThreadData Data)
{
	webInterface->DebugShow(wxT("**** imgrequest: ") + Data.sURL + wxT("\n"));

	const CSession* session = CheckLoggedin(Data);

	// To prevent access to non-template images, we disallow use of paths in filenames.
	wxString imgName = wxFileName::GetPathSeparator() + wxFileName(Data.sURL).GetFullName();
	CAnyImage *img = m_ImageLib.GetImage(imgName);
	
	// Only static images are available to visitors, in order to prevent
	// information leakage, but still allowing images on the login page.
	if (session->m_loggedin or dynamic_cast<CFileImage*>(img)) {
		int img_size = 0;
		unsigned char* img_data = img->RequestData(img_size);
		// This unicode2char is ok.
		Data.pSocket->SendContent(unicode2char(img->GetHTTP()), img_data, img_size);
	} else if (not session->m_loggedin) {
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
			for(int i = 0; i < reply->GetTagCount(); ++i) {
				if (	(tag = reply->GetTagByIndex(i)) &&
					(tag->GetTagName() == EC_TAG_STRING)) {
					webInterface->Show(tag->GetStringData());
				}
			}
		} else if (reply->GetOpCode() == EC_OP_FAILED) {
			if (	reply->GetTagCount() &&
				(tag = reply->GetTagByIndex(0)) ) {
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
		SharedFile *file = m_SharedFileInfo.GetByID(fileHash);
		if ( file ) {
			ec_cmd = new CECPacket(EC_OP_SHARED_SET_PRIO);
			hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO,
				GetHigherPrioShared(file->nFilePriority, file->bFileAutoPriority)));
		}
	} else if ( cmd == wxT("priodown") ) {
		SharedFile *file = m_SharedFileInfo.GetByID(fileHash);
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
		DownloadFile *file = m_DownloadFileInfo.GetByID(fileHash);
		if ( file ) {
			ec_cmd = new CECPacket(EC_OP_PARTFILE_PRIO_SET);
			hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO,
				GetHigherPrio(file->lFilePrio, file->bFileAutoPriority)));
		}
	} else if (cmd == wxT("priodown")) {
		DownloadFile *file = m_DownloadFileInfo.GetByID(fileHash);
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

	addr.Trim();
	port.Trim();
	
	CECTag srv_tag(EC_TAG_SERVER, addr + wxT(":") + port);
	srv_tag.AddTag(CECTag(EC_TAG_SERVER_ADDRESS, addr + wxT(":") + port));
	srv_tag.AddTag(CECTag(EC_TAG_SERVER_NAME, name));
	
	ec_cmd.AddTag(srv_tag);
	
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
	CECPacket req(EC_OP_ED2K_LINK);
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

bool ServersInfo::ServersInfo::ReQuery()
{
	CECPacket srv_req(EC_OP_GET_SERVER_LIST);
	const CECPacket *srv_reply = m_webApp->SendRecvMsg_v2(&srv_req);
	if (!srv_reply) {
		return false;
	}
	//
	// query succeded - flush existing values and refill
	EraseAll();
	for (int i = 0; i < srv_reply->GetTagCount(); ++i) {
		const CECTag *tag = srv_reply->GetTagByIndex(i);
		
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


SharedFile::SharedFile(CEC_SharedFile_Tag *tag)
{
		sFileName = _SpecialChars(tag->FileName());
		lFileSize = tag->SizeFull();
		sED2kLink = _SpecialChars(tag->FileEd2kLink());
		nHash = tag->ID();
		
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
	nFilePriority = tag->Prio();
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
	UpdatableItemsContainer<SharedFile, CEC_SharedFile_Tag, CMD4Hash>(webApp)
{
	m_This = this;
}


bool SharedFileInfo::ReQuery()
{
	DoRequery(EC_OP_GET_SHARED_FILES, EC_TAG_KNOWNFILE);

	return true;
}


DownloadFile::DownloadFile(CEC_PartFile_Tag *tag)
{
	nHash = tag->ID();
	sFileName = _SpecialChars(tag->FileName());
	lFileSize = tag->SizeFull();
	sFileHash = nHash.Encode();
	sED2kLink = _SpecialChars(tag->FileEd2kLink());
	lFileCompleted = tag->SizeDone();
	lFileTransferred = tag->SizeXfer();
	lFileSpeed = tag->Speed();
	fCompleted = (100.0*lFileCompleted) / lFileSize;
	
	m_Encoder = PartFileEncoderData( (lFileSize + (PARTSIZE - 1)) / PARTSIZE, 10);

	ProcessUpdate(tag);							
}

void DownloadFile::ProcessUpdate(CEC_PartFile_Tag *tag)
{
	if (!tag) {
		return;
	}
	
	lFilePrio = tag->Prio();
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
	if (gaptag && parttag && reqtag) {
		m_Encoder.Decode(
			(unsigned char *)gaptag->GetTagData(), gaptag->GetTagDataLen(),
			(unsigned char *)parttag->GetTagData(), parttag->GetTagDataLen());

		const Gap_Struct *reqparts = (const Gap_Struct *)reqtag->GetTagData();
		int reqcount = reqtag->GetTagDataLen() / sizeof(Gap_Struct);
		m_ReqParts.resize(reqcount);
		for (int i = 0; i < reqcount;i++) {
			m_ReqParts[i].start = ENDIAN_NTOHLL(reqparts[i].start);
			m_ReqParts[i].end   = ENDIAN_NTOHLL(reqparts[i].end);
		}
	}
}

DownloadFileInfo *DownloadFile::GetContainerInstance()
{
	return DownloadFileInfo::m_This;
}

DownloadFileInfo *DownloadFileInfo::m_This = 0;

DownloadFileInfo::DownloadFileInfo(CamulewebApp *webApp, CImageLib *imlib) :
	UpdatableItemsContainer<DownloadFile, CEC_PartFile_Tag, CMD4Hash>(webApp)
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
#endif
	delete item->m_Image;
}

bool DownloadFileInfo::ReQuery()
{
	DoRequery(EC_OP_GET_DLOAD_QUEUE, EC_TAG_PARTFILE);
	
	return true;
}


UploadFile::UploadFile(CEC_UpDownClient_Tag *tag)
{
	nHash = tag->FileID();
	sUserName = _SpecialChars(tag->ClientName());
	nSpeed = tag->SpeedUp();
	nTransferredUp = tag->XferUp();
	nTransferredDown = tag->XferDown();
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
	for(int i = 0; i < up_reply->GetTagCount(); i ++) {
		
		UploadFile curr((CEC_UpDownClient_Tag *)up_reply->GetTagByIndex(i));
		
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
	UpdatableItemsContainer<SearchFile, CEC_SearchFile_Tag, CMD4Hash>(webApp)
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
	wxFFile fis(m_name);
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

	m_gap_buf_size = m_gap_alloc_size = m_file->m_Encoder.m_gap_status.Size() / (2 * sizeof(uint64));
	m_gap_buf = new Gap_Struct[m_gap_alloc_size];
	
	m_ColorLine = new uint32[m_width];
}

CProgressImage::~CProgressImage()
{
	delete [] m_gap_buf;
	delete [] m_ColorLine;
}

void CProgressImage::ReallocGapBuffer()
{
	int size = m_file->m_Encoder.m_gap_status.Size() / (2 * sizeof(uint64));
	if ( size == m_gap_alloc_size ) {
		return;
	}
	if ( (size > m_gap_alloc_size) || (size < m_gap_alloc_size/2) ) {
		m_gap_buf_size = m_gap_alloc_size = size;
		delete [] m_gap_buf;
		m_gap_buf = new Gap_Struct[m_gap_alloc_size];
	} else {
		m_gap_buf_size = size;
	}
}

void CProgressImage::InitSortedGaps()
{
	ReallocGapBuffer();

	const uint64 *gap_info = (const uint64 *)m_file->m_Encoder.m_gap_status.Buffer();
	m_gap_buf_size = m_file->m_Encoder.m_gap_status.Size() / (2 * sizeof(uint64));
	
	//memcpy(m_gap_buf, gap_info, m_gap_buf_size*2*sizeof(uint32));
	for (int j = 0; j < m_gap_buf_size;j++) {
		uint64 gap_start = ENDIAN_NTOHLL(gap_info[2*j]);
		uint64 gap_end = ENDIAN_NTOHLL(gap_info[2*j+1]);
		m_gap_buf[j].start = gap_start;
		m_gap_buf[j].end = gap_end;
	}
	qsort(m_gap_buf, m_gap_buf_size, 2*sizeof(uint64), compare_gaps);
}

void CProgressImage::CreateSpan()
{
	// Step 1: sort gaps list in accending order
	InitSortedGaps();
	
	// allocate for worst case !
	int color_gaps_alloc = 2 * (2*m_gap_buf_size + m_file->lFileSize / PARTSIZE + 1);
	Color_Gap_Struct *colored_gaps = new Color_Gap_Struct[color_gaps_alloc];
	
	// Step 2: combine gap and part status information
	const unsigned char *part_info = m_file->m_Encoder.m_part_status.Buffer();
	
	// Init first item to dummy info, so we will always have "previous" item
	int colored_gaps_size = 0;
	colored_gaps[0].start = 0;
	colored_gaps[0].end = 0;
	colored_gaps[0].color = 0xffffffff;
	for (int j = 0; j < m_gap_buf_size;j++) {
		uint64 gap_start = m_gap_buf[j].start;
		uint64 gap_end = m_gap_buf[j].end;

		uint32 start = gap_start / PARTSIZE;
		uint32 end = (gap_end / PARTSIZE) + 1;

		for (uint32 i = start; i < end; i++) {
			COLORREF color = RGB(255, 0, 0);
			if ( part_info[i] ) {
				int blue = 210 - ( 22 * ( part_info[i] - 1 ) );
				color = RGB( 0, ( blue < 0 ? 0 : blue ), 255 );
			}

			uint32 fill_gap_begin = ( (i == start)   ? gap_start: PARTSIZE * i );
			uint32 fill_gap_end   = ( (i == (end - 1)) ? gap_end   : PARTSIZE * ( i + 1 ) );
			
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
		if (m_file->m_ReqParts.size()) {
			for(int i = 0; i < m_width; ++i) {
				m_ColorLine[i] = RGB(255, 208, 0);
			}
		} else if ( colored_gaps_size ) {
			for(int i = 0; i < m_width; ++i) {
				m_ColorLine[i] = colored_gaps[0].color;
			}
		}
	} else {
		uint32 factor = m_file->lFileSize / m_width;
		for(int i = 1; i <= colored_gaps_size;i++) {
			uint32 start = colored_gaps[i].start / factor;
			uint32 end = colored_gaps[i].end / factor;
			for(uint32 j = start; j < end; j++) {
				m_ColorLine[j] = colored_gaps[i].color;
			}
		}
		// overwrite requested parts
		for(uint32 i = 0; i < m_file->m_ReqParts.size(); i++) {
			uint32 start = m_file->m_ReqParts[i].start / factor;
			uint32 end = m_file->m_ReqParts[i].end / factor;
			for(uint32 j = start; j < end; j++) {
				m_ColorLine[j] = RGB(255, 208, 0);
			}
		}
	}
	delete [] colored_gaps;
}

int CProgressImage::compare_gaps(const void *g1, const void *g2)
{
	return ((const Gap_Struct *)g1)->start - ((const Gap_Struct *)g2)->start;
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
	CDynPngImage *This = (CDynPngImage *)png_get_io_ptr(png_ptr);
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
	// template contain %s (name) %d (width)
	return (CFormat(m_template) % m_name % m_width).GetString();
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
				   		% (i - lastindex)).GetString();
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
	m_name = wxString::Format(wxT("dyn_%ld_stat.png"), (unsigned long int) data);
	
	m_num_font_w_size = 8;
	m_num_font_h_size = 16;
	
	// leave enough space for 3 digit number
	int img_delta = m_num_font_w_size / 4;
	m_left_margin = 3*(m_num_font_w_size + img_delta) + img_delta;
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
	static const COLORREF bg_color = RGB(0x00, 0x00, 0x40);
	for(int i = 0; i < m_height; i++) {
		png_bytep u_row = m_row_bg_ptrs[i];
		for(int j = 0; j < m_width; j++) {
			set_rgb_color_val(u_row+3*j, bg_color, 0);
		}
	}
	//
	// draw axis
	//
	static const COLORREF axis_color = RGB(0xff, 0xff, 0xff);
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
	static const COLORREF graph_color = RGB(0xff, 0x00, 0x00);
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
	// When data is scaled down, axis are scaled UP and visa versa
	int y_axis_max = m_y_axis_size;
	if ( m_scale_down != 1 ) {
		y_axis_max *= m_scale_down;
	} else if ( m_scale_up != 1 ) {
		y_axis_max /= m_scale_up;
	}

	if ( y_axis_max > 99 ) {
		m_digits[y_axis_max / 100]->Apply(m_row_ptrs, img_delta, img_delta);
	}
	m_digits[(y_axis_max % 100) / 10]->Apply(m_row_ptrs, 2*img_delta+m_num_font_w_size, img_delta);
	m_digits[y_axis_max % 10]->Apply(m_row_ptrs, 3*img_delta+2*m_num_font_w_size, img_delta);

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
			wxASSERT(0);
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
	CAnyImage *prev = m_image_map[name];
	if ( prev ) {
		delete prev;
	}
	m_image_map[name] = img;
}

void CImageLib::RemoveImage(const wxString &name)
{
	CAnyImage *prev = m_image_map[name];
	if ( prev ) {
		m_image_map.erase(name);
	}
}

CAnyImage *CImageLib::GetImage(wxString &name)
{
	CAnyImage *img = m_image_map[name];
	if ( img ) {
		return img;
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
	wxString img_tmpl(wxT("<img src=%s height=20 width=%d>"));
	m_DownloadFileInfo.LoadImageParams(img_tmpl, 200, 20);

	if ( ::wxFileExists(wxFileName(m_wwwroot, wxT("index.html")).GetFullPath()) ) {
		m_index = wxT("index.html");
	} else if ( ::wxFileExists(wxFileName(m_wwwroot, wxT("index.php")).GetFullPath()) ) {
		m_index = wxT("index.php");
	} else {
		webInterface->Show(_("Index file not found: bad template\n"));
	}
}

CScriptWebServer::~CScriptWebServer()
{
}


void CScriptWebServer::StartServer()
{
	if (!webInterface->m_LoadSettingsFromAmule) {
		if (webInterface->m_configFile) {
			webInterface->m_PageRefresh = webInterface->m_configFile->Read(wxT("/Webserver/PageRefreshTime"), 120l);
		}
	}

	wsThread = new CWSThread(this);
	if ( wsThread->Create() != wxTHREAD_NO_ERROR ) {
		webInterface->Show(_("Can't create web socket thread\n"));
	} else {
		//...and run it
		wsThread->Run();
 
		webInterface->Show(_("Web Server: Started\n"));
	}
}

void CScriptWebServer::StopServer()
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
		return GetErrorPage("fseek failed", size);
	}

	size = ftell(f);
	char *buf = new char [size+1];
	rewind(f);
	fread(buf, 1, size, f);
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
	wxMutexLocker lock(*m_mutexChildren);

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
		if (webInterface->m_AdminPass.IsEmpty() and webInterface->m_GuestPass.IsEmpty()) {
			session->m_vars["login_error"] = "No passwords specified, login impossible!";
		} else if ( PwStr.Length() ) {
			Print(_("Checking password\n"));
			session->m_loggedin = false;
			
			CMD4Hash PwHash;
			if (not PwHash.Decode(MD5Sum(PwStr).GetHash())) {
				Print(_("Password hash invalid\n"));
				session->m_vars["login_error"] = "Invalid password hash, please report!";
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
			Print(_("Session is not logged and request have no password\n"));
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
		wxString::Format(_("%d"), webInterface->m_PageRefresh));
	
	wxString req_file(wxFileName(m_wwwroot, filename).GetFullPath());
	if ( req_file.Find(wxT(".html")) != -1 ) {
		httpOut = ProcessHtmlRequest(unicode2char(req_file), httpOutLen);
	} else if ( req_file.Find(wxT(".php")) != -1 ) {
		httpOut = ProcessPhpRequest(unicode2char(req_file), session, httpOutLen);
	} else {
		httpOut = GetErrorPage("This file type amuleweb doesn't handle", httpOutLen);
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
		Data.pSocket->SendHttpHeaders(isUseGzip, httpOutLen, Data.SessionID);
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
	 * into hard-coded page as last resolt.
	 */
	char *httpOut = ""
	"<html>"
	"<head>"
		"<title>aMuleWeb error page</title>"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
	"</head>"
	"<body>"
	"<p>You seeing this page instead of aMuleWeb page because valid template has not been found.</p>"
	"<p>This means that there's problem with aMule installation </p>"
	"<ul>"
		"<li>Before installation please ensure that you uninstalled previous versions of amule</li>"
	"<li>If you installing by recompiling from source, check configuration and run &quot;make install&quot; again </li>"
		"<li>If you installing binary package, you may need to contact package maintainer </li>"
	"</ul>"
	"<p>For more information please visit</p>"
	"<p><a href=\"http://www.amule.org\">aMule main site</a> <a href=\"http://forum.amule.org\">aMule forum</a></p>"
	"</body>"
	"</html>";

	long httpOutLen = strlen(httpOut);

	Data.pSocket->SendHttpHeaders(false, httpOutLen, 0);
	Data.pSocket->SendData(httpOut, httpOutLen);
}
