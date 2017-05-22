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

#ifndef WEBSERVER_H
#define WEBSERVER_H

#ifdef HAVE_CONFIG_H
	#include "config.h"	// Needed for ASIO_SOCKETS
#endif

#ifdef WITH_LIBPNG
	#include <png.h>
#endif

#include "WebInterface.h"
#include <map>			// Needed for std::map
#include <set>			// Needed for std::set
#include "RLE.h"
#include "OtherStructs.h"
#include <ec/cpp/ECID.h>	// Needed for CECID

#ifdef ENABLE_UPNP
#	include "UPnPBase.h"
#endif

#include <wx/datetime.h>  // For DownloadFile::wxtLastSeenComplete

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#define snprintf sprintf_s
#define atoll _atoi64
#define strdup _strdup
#endif

class CWebSocket;
class CMD4Hash;

#define SESSION_TIMEOUT_SECS	300	// 5 minutes session expiration
#define SHORT_FILENAME_LENGTH	40	// Max size of file name.

wxString _SpecialChars(wxString str);

class CEC_PartFile_Tag;
class CEC_SharedFile_Tag;
class CEC_UpDownClient_Tag;
class CEC_SearchFile_Tag;
class CProgressImage;
class CEC_KadNode_Tag;

class CURLDecoder
{
      public:
	static wxString	Decode(const wxString& url);
};

class DownloadFile : public CECID {
	public:
		wxString	sFileName;
		uint8		nFileStatus;
		uint64		lFileSize;
		uint64		lFileCompleted;
		uint64		lFileTransferred;
		unsigned long	lFileSpeed;
		long		lSourceCount;
		long		lNotCurrentSourceCount;
		long		lTransferringSourceCount;
		long		lSourceCountA4AF;
		double		fCompleted;
		uint32		lFilePrio;
		bool		bFileAutoPriority;
		wxString	sFileHash;
		wxString	sED2kLink;
		uint8		nCat;
		wxDateTime	wxtLastSeenComplete;

		CMD4Hash	nHash;

		CProgressImage *m_Image;
		PartFileEncoderData m_Encoder;
		ArrayOfUInts16 m_PartInfo;
		std::vector<Gap_Struct> m_ReqParts;
		ArrayOfUInts64	m_Gaps;

		// container require this
		static class DownloadFileInfo *GetContainerInstance();
		DownloadFile(CEC_PartFile_Tag *);
		void ProcessUpdate(CEC_PartFile_Tag *);
		uint32 ID() { return ECID(); }
};

class SharedFile : public CECID {
	public:
		wxString	sFileName;
		uint64		lFileSize;
		uint64		nFileTransferred;
		uint64		nFileAllTimeTransferred;
		uint16		nFileRequests;
		uint32		nFileAllTimeRequests;
		uint16		nFileAccepts;
		uint32		nFileAllTimeAccepts;
		uint8		nFilePriority;
		bool		bFileAutoPriority;
		wxString	sFileHash;
		wxString	sED2kLink;

		CMD4Hash	nHash;

		static class SharedFileInfo *GetContainerInstance();
		SharedFile(CEC_SharedFile_Tag *);
		void ProcessUpdate(CEC_SharedFile_Tag *);
		uint32 ID() { return ECID(); }
};

class ServerEntry {
	public:
		wxString	sServerName;
		wxString	sServerDescription;
		uint32		nServerIP;
		uint16		nServerPort;
		wxString	sServerIP;
		int		nServerUsers;
		int		nServerMaxUsers;
		int		nServerFiles;

		static class ServersInfo *GetContainerInstance();
		uint32 ID() { return nServerIP; }
};

// This is a client we are uploading to, not a file
class UploadFile : public CECID {
	public:
		wxString  sUserName;
		uint32 nTransferredUp;
		uint32 nTransferredDown;
		uint32 nSpeed;
		uint32 nUploadFile;		// ECID of shared file uploading to client

		UploadFile(CEC_UpDownClient_Tag *tag);

		static class UploadsInfo *GetContainerInstance();
		uint32 ID() { return ECID(); }
};

class SearchFile : public CECID {
	public:
		wxString sFileName;
		uint64 lFileSize;
		CMD4Hash  nHash;
		wxString  sHash;
		long lSourceCount;
		bool bPresent;

		SearchFile(CEC_SearchFile_Tag *);

		void ProcessUpdate(CEC_SearchFile_Tag *);
		static class SearchInfo *GetContainerInstance();
		uint32 ID() { return ECID(); }
};


/*!
 * T - type of items in container
 */
template <class T>
class ItemsContainer {
	protected:
		CamulewebApp *m_webApp;
		std::list<T> m_items;


		void EraseAll()
		{
			m_items.erase(m_items.begin(), m_items.end());
		}
	public:
		ItemsContainer(CamulewebApp *webApp)
		{
			m_webApp = webApp;
		}
		virtual ~ItemsContainer() { }


		int ItemCount()
		{
			return m_items.size();
		}


		T *AddItem(T &item)
		{
			m_items.push_back(item);
			T *real_ptr = &(m_items.back());
			return real_ptr;
		}

		/*!
		 * Re-query server: refresh all dataset
		 */
		virtual bool ReQuery() = 0;

		typedef typename std::list<T>::iterator ItemIterator;
		ItemIterator GetBeginIterator()
		{
			return m_items.begin();
		}
		ItemIterator GetEndIterator()
		{
			return m_items.end();
		}
};

/*!
 * T - type of items in container
 * I - type of item ID
 * G - type of tag in EC
 */
template <class T, class G, class I>
class UpdatableItemsContainer : public ItemsContainer<T> {
	protected:
		// need duplicate list with a map, so check "do we already have"
		// will take O(log(n)) instead of O(n)
		// map will contain pointers to items in list
		std::map<I, T *> m_items_hash;
	public:
		UpdatableItemsContainer(CamulewebApp *webApp) : ItemsContainer<T>(webApp)
		{
		}

		T *AddItem(T &item)
		{
			T *real_ptr = ItemsContainer<T>::AddItem(item);
			m_items_hash[item.ID()] = real_ptr;
			return real_ptr;
		}

		T *GetByID(I id)
		{
			// avoid creating nodes
			return m_items_hash.count(id) ? m_items_hash[id] : NULL;
		}

		T * GetByHash(const CMD4Hash &fileHash)
		{
			T * ret = 0;
			for (typename std::map<I, T *>::iterator it = m_items_hash.begin(); it != m_items_hash.end(); ++it) {
				if (it->second->nHash == fileHash) {
					ret = it->second;
					break;
				}
			}
			return ret;
		}

		/*!
		 * Process answer of update request, create list of new items for
		 * full request later. Also remove items that no longer exist in core
		 */
		void ProcessUpdate(const CECPacket *reply, CECPacket *full_req, int req_type)
		{
			std::set<I> core_files;
			for (CECPacket::const_iterator it = reply->begin(); it != reply->end(); ++it) {
				G *tag = (G *) & *it;

				core_files.insert(tag->ID());
				if ( m_items_hash.count(tag->ID()) ) {
					T *item = m_items_hash[tag->ID()];
					item->ProcessUpdate(tag);
				} else {
					full_req->AddTag(CECTag(req_type, tag->ID()));
				}
			}
			std::list<I> del_ids;
			for(typename std::list<T>::iterator j = this->m_items.begin(); j != this->m_items.end(); ++j) {
				if ( core_files.count(j->ID()) == 0 ) {
					// item may contain data that need to be freed externally, before
					// dtor is called and memory freed

					T *real_ptr = &*j;
					this->ItemDeleted(real_ptr);

					del_ids.push_back(j->ID());
				}
			}
			for(typename std::list<I>::iterator j = del_ids.begin(); j != del_ids.end(); ++j) {
				m_items_hash.erase(*j);
				for(typename std::list<T>::iterator k = this->m_items.begin(); k != this->m_items.end(); ++k) {
					if ( *j == k->ID() ) {
						this->m_items.erase(k);
						break;
					}
				}
			}
		}

		void ProcessFull(const CECPacket *reply)
		{
			for (CECPacket::const_iterator it = reply->begin(); it != reply->end(); ++it) {
				G *tag = (G *) & *it;
				// initialize item data from EC tag
				T item(tag);
				T *real_ptr = AddItem(item);
				// initialize any external data that may depend on this item
				this->ItemInserted(real_ptr);
			}
		}

		bool DoRequery(int cmd, int tag)
		{
			CECPacket req_sts(cmd, EC_DETAIL_UPDATE);

			//
			// Phase 1: request status
			const CECPacket *reply = this->m_webApp->SendRecvMsg_v2(&req_sts);
			if ( !reply ) {
				return false;
			}

			//
			// Phase 2: update status, mark new files for subsequent query
			CECPacket req_full(cmd);

			ProcessUpdate(reply, &req_full, tag);

			delete reply;

			// Phase 3: request full info about files we don't have yet
			if ( req_full.HasChildTags() ) {
				reply = this->m_webApp->SendRecvMsg_v2(&req_full);
				if ( !reply ) {
					return false;
				}
				ProcessFull(reply);
				delete reply;
			}
			return true;
		}

		virtual void ItemDeleted(T *) { }
		virtual void ItemInserted(T *) { }
};

class UploadsInfo : public ItemsContainer<UploadFile> {
	public:
		// can be only one instance.
		static UploadsInfo *m_This;

		UploadsInfo(CamulewebApp *webApp);

		virtual bool ReQuery();
};

class ServersInfo : public ItemsContainer<ServerEntry> {
	public:
		// can be only one instance.
		static ServersInfo *m_This;

		ServersInfo(CamulewebApp *webApp);

		virtual bool ReQuery();

};


class SharedFileInfo : public UpdatableItemsContainer<SharedFile, CEC_SharedFile_Tag, uint32> {
	public:
		// can be only one instance.
		static SharedFileInfo *m_This;

		SharedFileInfo(CamulewebApp *webApp);

		virtual bool ReQuery();

};

class SearchInfo : public UpdatableItemsContainer<SearchFile, CEC_SearchFile_Tag, uint32> {
	public:
		static SearchInfo *m_This;

		SearchInfo(CamulewebApp *webApp);

		virtual bool ReQuery();

};


class CImageLib;
class DownloadFileInfo : public UpdatableItemsContainer<DownloadFile, CEC_PartFile_Tag, uint32> {
		CImageLib *m_ImageLib;

		// parameters of progress images
		wxString m_Template;
		int m_width, m_height;
	public:
		// can be only one instance.
		static DownloadFileInfo *m_This;

		DownloadFileInfo(CamulewebApp *webApp, CImageLib *imlib);

		void LoadImageParams(wxString &tpl, int width, int height);

		virtual bool ReQuery();

		// container requirements
		void ItemInserted(DownloadFile *item);
		void ItemDeleted(DownloadFile *item);
};

class CAnyImage {
	protected:
		unsigned char *m_data;

		int m_width, m_height;
		wxString m_name;

		int m_size, m_alloc_size;
		wxString m_Http;

		void Realloc(int size);

		void SetHttpType(wxString ext);
	public:
		CAnyImage(int size);
		CAnyImage(int width, int height);
		virtual ~CAnyImage();

		const wxString& GetHTTP() const { return m_Http; }

		virtual unsigned char *RequestData(int &size);
};

class CFileImage : public virtual CAnyImage {
	public:
		CFileImage(const wxString& name);

		bool OpenOk() { return m_size != 0; }
};

class CImage3D_Modifiers {
		unsigned char *m_modifiers;
		int m_width;
	public:
		CImage3D_Modifiers(int width);
		~CImage3D_Modifiers();

		unsigned char operator[](int i)
		{
			return (i < m_width) ? m_modifiers[i] : 0;
		}
};

class CProgressImage : public virtual CAnyImage {
	protected:
		DownloadFile *m_file;

		wxString m_template;

		//
		// Turn list of gaps, partstatus into array of color strips
		typedef struct Color_Gap_Struct : public Gap_Struct {
			uint32 color;
		} Color_Gap_Struct;

		// result of rendering - single line
		uint32 *m_ColorLine;
		void CreateSpan();
	public:
		CProgressImage(int w, int h, wxString &tmpl, DownloadFile *file);

		~CProgressImage();

		const wxString &Name() { return m_name; }

		virtual wxString GetHTML() = 0;
};

#ifdef WITH_LIBPNG

//
// Dynamic png image generation
//
class CDynPngImage : public virtual CAnyImage {

	public:
		CDynPngImage(int w, int h);
		~CDynPngImage();

		virtual unsigned char *RequestData(int &size);

	protected:
		png_bytep m_img_data;
		png_bytep *m_row_ptrs;

		static void png_write_fn(png_structp png_ptr, png_bytep data, png_size_t length);

};

//
// Dynamic png image generation from gap info
class CDynProgressImage : public virtual CProgressImage, public virtual CDynPngImage {
		CImage3D_Modifiers m_modifiers;

		void DrawImage();
	public:
		CDynProgressImage(int w, int h,	wxString &tmpl, DownloadFile *file);
		~CDynProgressImage();

		virtual unsigned char *RequestData(int &size);
		virtual wxString GetHTML();
};

#else


//
// Fallback to original implementation
class CDynProgressImage : public virtual CProgressImage {
	public:
		CDynProgressImage(int w, int h,	wxString &tmpl, DownloadFile *file);

		virtual wxString GetHTML();
};

#endif

//
// Representing statistical sample for some parameter. Circular buffer
// inside to avoid rellocations
//
class CStatsData {
		uint32 *m_data;
		uint32 m_max_value;
		int m_size;
		int m_start_index, m_end_index, m_curr_index;
	public:
		CStatsData(int size);
		~CStatsData();

		int Size() const { return m_size; }
		uint32 Max() const { return m_max_value; }
		uint32 GetFirst();
		uint32 GetNext();

		void PushSample(uint32 sample);
};

class CStatsCollection {
		CStatsData *m_down_speed, *m_up_speed,
			*m_conn_number, *m_kad_count;

		CamulewebApp *m_iface;
		double m_LastTimeStamp;
		int m_size;
	public:
		CStatsCollection(int size, CamulewebApp	*iface);
		~CStatsCollection();

		CStatsData *DownloadSpeed() { return m_down_speed; }
		CStatsData *UploadSpeed() { return m_up_speed; }
		CStatsData *ConnCount() { return m_conn_number; }
		CStatsData *KadCount() { return m_kad_count; }

		void ReQuery();
};

#ifdef WITH_LIBPNG

//
// This gonna to represent data used to "write" numbers on
// dynamically generated images.
// Easiest way to represt numbers: 7-segments model
//
class CNumImageMask {
		png_bytep *m_row_mask_ptrs;
		int m_width, m_height;
		int m_v_segsize, m_h_segsize;

		// mask generation
		void DrawHorzLine(int off);
		void DrawVertLine(int offx, int offy);
		void DrawSegment(int id);

		static const int m_num_to_7_decode[10];
	public:
		CNumImageMask(int number, int width, int height);
		~CNumImageMask();

		void Apply(png_bytep *image, int offx, int offy);
};

class CDynStatisticImage : public virtual CDynPngImage {
		CStatsData *m_data;

		// size of "font" of imprinted numbers
		int m_num_font_w_size, m_num_font_h_size;

		int m_left_margin, m_bottom_margin;
		int m_y_axis_size;

		// hope nobody needs "define" for 10 !
		CNumImageMask *m_digits[10];

		// indicates whether data should be divided on 1024 before
		// drawing graph.
		bool m_scale1024;

		//
		// Prepared background
		//
		png_bytep m_background;
		png_bytep *m_row_bg_ptrs;

		void DrawImage();
	public:
		CDynStatisticImage(int height, bool scale1024, CStatsData *data);
		~CDynStatisticImage();

		virtual unsigned char *RequestData(int &size);
		virtual wxString GetHTML();
};

#endif

class CImageLib {
		typedef std::map<wxString, CAnyImage *> ImageMap;
		ImageMap m_image_map;
		wxString m_image_dir;
	public:
		CImageLib(wxString image_dir);
		~CImageLib();

		CAnyImage *GetImage(const wxString &name);
		void AddImage(CAnyImage *img, const wxString &name);
		void RemoveImage(const wxString &name);
};

class CParsedUrl {
		wxString m_path, m_file;
		std::map<wxString, wxString> m_params;
	public:
		CParsedUrl(const wxString &url);

		const wxString &Path() { return m_path; }
		const wxString &File() { return m_file; }

		const wxString &Param(const wxString &key)
		{
			return m_params[key];
		}

		void ConvertParams(std::map<std::string, std::string> &);
};

// Changing this to a typedef struct{} makes egcs compiler do it all wrong and crash on run
struct ThreadData {
	CParsedUrl	parsedURL;
	wxString	sURL;
	int		SessionID;
	CWebSocket	*pSocket;
};

#ifndef ASIO_SOCKETS
enum {
    // Socket handlers
    ID_WEBLISTENSOCKET_EVENT = wxID_HIGHEST+123,  // random safe ID
    ID_WEBCLIENTSOCKET_EVENT,
};
#endif

#ifdef ENABLE_UPNP
class CUPnPControlPoint;
class CUPnPPortMapping;
#endif

class CWebLibSocketServer : public CLibSocketServer {
public:
	CWebLibSocketServer(const class amuleIPV4Address& adr, int flags, CWebServerBase * webServerBase);

	virtual	void OnAccept();
private:
	CWebServerBase * m_webServerBase;
};

class CWebServerBase : public wxEvtHandler {
	protected:
		CWebLibSocketServer *m_webserver_socket;

		ServersInfo m_ServersInfo;
		SharedFileInfo m_SharedFileInfo;
		DownloadFileInfo m_DownloadFileInfo;
		UploadsInfo m_UploadsInfo;
		SearchInfo m_SearchInfo;

		CStatsCollection m_Stats;

		CImageLib m_ImageLib;

		virtual void ProcessURL(ThreadData) = 0;
		virtual void ProcessImgFileReq(ThreadData) = 0;

		int GzipCompress(Bytef *dest, uLongf *destLen,
			const Bytef *source, uLong sourceLen, int level);

		friend class CWebSocket;
		friend class CPhPLibContext;

		bool m_upnpEnabled;
		int m_upnpTCPPort;
#ifdef ENABLE_UPNP
		CUPnPControlPoint *m_upnp;
		std::vector<CUPnPPortMapping> m_upnpMappings;
#endif
#ifdef ASIO_SOCKETS
		CAsioService *m_AsioService;
#else
		void OnWebSocketServerEvent(wxSocketEvent& event);
		void OnWebSocketEvent(wxSocketEvent& event);
		DECLARE_EVENT_TABLE();
#endif
	public:
		CWebServerBase(CamulewebApp *webApp, const wxString& templateDir);
		virtual ~CWebServerBase();

		void Send_Discard_V2_Request(CECPacket *request);

		void StartServer();
		void StopServer();

		void Print(const wxString &s);

		long GetWSPrefs();

		//
		// Command interface
		//
		void Send_ReloadSharedFile_Cmd();

		void Send_SharedFile_Cmd(wxString file_hash, wxString cmd, uint32 opt_arg = 0);
		void Send_DownloadFile_Cmd(wxString file_hash, wxString cmd, uint32 opt_arg = 0);

		void Send_DownloadSearchFile_Cmd(wxString file_hash, uint8 cat);

		void Send_Server_Cmd(uint32 ip, uint16 port, wxString cmd);
		void Send_AddServer_Cmd(wxString addr, wxString port, wxString name);

		void Send_Search_Cmd(wxString search, wxString extention, wxString type,
			EC_SEARCH_TYPE search_type, uint32 avail, uint32 min_size, uint32 max_size);

		bool Send_DownloadEd2k_Cmd(wxString link, uint8 cat);

		void Reload_Stats()
		{
			m_Stats.ReQuery();
		}

		CamulewebApp	*webInterface;

};

class CSession {
	public:
		bool m_loggedin;
		time_t m_last_access;
		std::map<std::string, std::string> m_vars, m_get_vars;

		void LoadVars(CParsedUrl &url);
};

/*
 * Script based webserver
 */
class CScriptWebServer : public CWebServerBase {
		wxString m_wwwroot;
		wxString m_index;

		char *ProcessHtmlRequest(const char *filename, long &size);
		char *ProcessPhpRequest(const char *filename, CSession *sess, long &size);

		char *GetErrorPage(const char *message, long &size);
		char *Get_404_Page(long &size);

		std::map<int, CSession> m_sessions;

		CSession *CheckLoggedin(ThreadData &);
	protected:
		virtual void ProcessURL(ThreadData);
		virtual void ProcessImgFileReq(ThreadData);
	public:
		CScriptWebServer(CamulewebApp *webApp, const wxString& templateDir);
		~CScriptWebServer();

};

class CNoTemplateWebServer : public CScriptWebServer {
	protected:
		virtual void ProcessURL(ThreadData);
	public:
		CNoTemplateWebServer(CamulewebApp *webApp);
		~CNoTemplateWebServer();
};

#endif // WEBSERVER_H
// File_checked_for_headers
