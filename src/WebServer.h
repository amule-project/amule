//
// This file is part of the aMule Project
//
// Copyright (C) 2003-2004 aMule Team ( http://www.amule-project.net )
// This fle Copyright (C) 2003 Kry ( elkry@users.sourceforge.net   http://www.amule-project.net )
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

#ifndef WEBSERVER_H
#define WEBSERVER_H

#pragma interface
//-------------------------------------------------------------------
#ifdef __WXMSW__
	#include <winsock.h>
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif
#include <zlib.h>		// Needed for Bytef etc.

#ifdef WITH_LIBPNG
	#include <png.h>
#endif

#include <list>
#include <map>
#include <vector>


//-------------------------------------------------------------------
#ifndef WIN32
	#include "config.h"
#endif
//-------------------------------------------------------------------
#define wxUSE_DDE_FOR_IPC  0
//#include <wx/ipc.h>
#include <wx/wx.h>
#include <wx/filename.h>

#ifdef __WXMSW__
	#include <wx/msw/winundef.h>
#endif

#include "types.h"
#include "otherfunctions.h"
#include "WebInterface.h"
#include "KnownFile.h"
#include "ECPacket.h"

class TransferredData;
class CWSThread;
class CWebSocket;

//shakraw - webserver code below
#define WEB_GRAPH_HEIGHT		120
#define WEB_GRAPH_WIDTH			500

#define SESSION_TIMEOUT_SECS	300	// 5 minutes session expiration
#define SHORT_FILENAME_LENGTH	40	// Max size of file name.

typedef struct { float download; float upload; long connections; } UpDown;

typedef struct { time_t startTime; long lSession; bool admin;} Session;

class CProgressImage;
class DownloadFiles {
	public:
		wxString	sFileName;
		uint8		nFileStatus;
		wxString	sFileStatus;
		unsigned long	lFileSize;
		unsigned long	lFileCompleted;
		unsigned long	lFileTransferred;
		unsigned long	lFileSpeed;
		long		lSourceCount;
		long		lNotCurrentSourceCount;
		long		lTransferringSourceCount;
		double		fCompleted;
		long		lFilePrio;
		wxString	sFileHash;
		wxString	sED2kLink;
		wxString	sFileInfo;
		wxString	sPartStatus;
		
		otherfunctions::PartFileEncoderData m_Encoder;

		static class DownloadFilesInfo *GetContainerInstance();

		uint32 file_id;
		CProgressImage *m_Image;
};

class SharedFiles {
	public:
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

		static class SharedFilesInfo *GetContainerInstance();
};

typedef enum {
	DOWN_SORT_NAME,
	DOWN_SORT_SIZE,
	DOWN_SORT_COMPLETED,
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


typedef enum {
	SERVER_SORT_NAME,
	SERVER_SORT_DESCRIPTION,
	SERVER_SORT_IP,
	SERVER_SORT_USERS,
	SERVER_SORT_FILES
} xServerSort;

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
};

/*!
 * Each item of type T must implement GetContainerInstance(T) to return ptr
 * to container holding such items.
 * Parameter "T" is used for compiler to distinguish between functions of
 * different types
 */

template <class T>
bool operator < (const T &i1, const T &i2)
{
	return T::GetContainerInstance()->CompareItems(i1, i2);
}


WX_DECLARE_OBJARRAY(UpDown*, ArrayOfUpDown);
WX_DECLARE_OBJARRAY(Session*, ArrayOfSession);
WX_DECLARE_OBJARRAY(TransferredData*, ArrayOfTransferredData);

/*!
 * T - type of items in container
 * E - type of enum for sort order
 */
template <class T, class E>
class ItemsContainer {
	protected:
		CamulewebApp *m_webApp;
		std::list<T> m_items;
		
		// map string value to enum: derived class
		// must init this map in ctor
		std::map<wxString, E> m_SortStrVals;
		// map sort order enums to names of sections
		// in html template: derived class
		// must init this map in ctor
		std::map<E, wxString> m_SortHeaders;
		
		
		bool m_SortReverse;
		// type is int, so derived class must cast it
		// to right enum type
		E m_SortOrder;
	
		void EraseAll()
		{
			m_items.erase(m_items.begin(), m_items.end());
		}
	public:
		ItemsContainer(CamulewebApp *webApp)
		{
			m_webApp = webApp;
			m_SortReverse = false;
		}
		virtual ~ItemsContainer() { }
		
		void SortItems()
		{
			m_items.sort();
		}

		E GetSortOrder()
		{
			return m_SortOrder;
		}

		int ItemCount()
		{
			return m_items.size();
		}
		
		bool IsSortingReverse()
		{
			return m_SortReverse;
		}
		
		void AddItem(T &item)
		{
			m_items.push_back(item);
		}

		/*!
		 * Substitute sort-order templates
		 */
		void ProcessHeadersLine(wxString &line)
		{
			wxString sField = m_SortHeaders[m_SortOrder];
			// invert sort order in link
			wxString sSortRev(!m_SortReverse ? wxT("&sortreverse=true") : wxT("&sortreverse=false"));
			for(typename std::map<E, wxString>::iterator i = m_SortHeaders.begin();
				i != m_SortHeaders.end(); i++) {
					if (sField == i->second) {
						line.Replace(i->second, sSortRev);
					} else {
						line.Replace(i->second, wxEmptyString);
					}
				}
		}
		
		/*!
		 * Convert string to right enum value
		 */
		void SetSortOrder(wxString &order, bool reverse)
		{
			m_SortOrder = m_SortStrVals[order];
			m_SortReverse = reverse;
		}
		/*!
		 * Re-query server: refresh all dataset
		 */
		virtual bool ReQuery() = 0;
		virtual bool ProcessUpdate(CECPacket *update) = 0;


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

class ServersInfo : public ItemsContainer<ServerEntry, xServerSort> {
	public:
		// can be only one instance.
		static ServersInfo *m_This;
		
		ServersInfo(CamulewebApp *webApp);

		virtual bool ReQuery();
		virtual bool ProcessUpdate(CECPacket *update);

		bool CompareItems(const ServerEntry &i1, const ServerEntry &i2);
};


class SharedFilesInfo : public ItemsContainer<SharedFiles, xSharedSort> {
	public:
		// can be only one instance.
		static SharedFilesInfo *m_This;

		SharedFilesInfo(CamulewebApp *webApp);

		virtual bool ReQuery();
		virtual bool ProcessUpdate(CECPacket *update);

		bool CompareItems(const SharedFiles &i1, const SharedFiles &i2);
};

class CImageLib;
class DownloadFilesInfo : public ItemsContainer<DownloadFiles, xDownloadSort> {
		// need duplicate list with a map, so check "do we already have"
		// will take O(log(n)) instead of O(n)
		// map will contain pointers to items in list 
		std::map<uint32, DownloadFiles *> m_files;
		CImageLib *m_ImageLib;
		
		// parameters of progress images
		wxString m_Template;
		int m_width, m_height;
	public:
		// can be only one instance.
		static DownloadFilesInfo *m_This;
		
		DownloadFilesInfo(CamulewebApp *webApp, CImageLib *imlib);
		
		void LoadImageParams(wxString &tpl, int width, int height);
		
		virtual bool ReQuery();
		virtual bool ProcessUpdate(CECPacket *update);

		bool CompareItems(const DownloadFiles &i1, const DownloadFiles &i2);
};

class CAnyImage {
	protected:
		unsigned char *m_data;
		int m_size, m_alloc_size;
		wxString m_Http;
		
		void Realloc(int size);
		
		void SetHttpType(wxString ext);
	public:
		CAnyImage(int size);
		virtual ~CAnyImage();

		char *GetHTTP() { return (char *)unicode2char(m_Http); }
		
		virtual unsigned char *RequestData(int &size);
};

class CFileImage : public CAnyImage {
		wxString m_name;
	public:
		CFileImage(const char *name);
		
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

class CProgressImage : public CAnyImage {
	protected:
		wxString m_template;
		int m_width, m_height;
		otherfunctions::PartFileEncoderData *m_Encoder;
		wxString m_name;
		uint32 m_file_size;
		
		//
		// sorted list of gaps
		int m_gap_buf_size, m_gap_alloc_size;
		Gap_Struct *m_gap_buf;
		
		void ReallocGapBuffer();
		void InitSortedGaps();
		// for qsort
		static int compare_gaps(const void *, const void *);
		
		//
		// Turn list of gaps, partstatus into array of color strips
		typedef struct Color_Gap_Struct : public Gap_Struct {
			uint32 color;
		} Color_Gap_Struct;

		// result of rendering - single line
		uint32 *m_ColorLine;
		void CreateSpan();
	public:
		CProgressImage(int w, int h, uint32 filesize, wxString &tmpl, otherfunctions::PartFileEncoderData *encoder);

		~CProgressImage();
				
		const wxString &Name() { return m_name; }
				
		virtual wxString GetHTML() = 0;
};

#ifdef WITH_LIBPNG

//
// Dynamic png image generation from gap info
class CDynImage : public CProgressImage {
		CImage3D_Modifiers m_modifiers;
		uint32 m_id;
		png_bytep *m_row_ptrs;
		
		static void png_write_fn(png_structp png_ptr, png_bytep data, png_size_t length);
		
		void DrawImage();
	public:
		CDynImage(uint32 id, int w, int h, uint32 filesize,
			wxString &tmpl, otherfunctions::PartFileEncoderData *encoder);
		~CDynImage();
		
		virtual unsigned char *RequestData(int &size);
		virtual wxString GetHTML();
};

#else


//
// Fallback to original implementation
class CDynImage : public CProgressImage {
		uint32 m_id;
	public:
		CDynImage(uint32 id, int w, int h, uint32 filesize,
			wxString &tmpl, otherfunctions::PartFileEncoderData *encoder);

		virtual wxString GetHTML();
};

#endif

class CImageLib {
		std::map<wxString, CAnyImage *> m_image_map;
		wxString m_image_dir;
	public:
		CImageLib(wxString image_dir);
		~CImageLib();
		
		CAnyImage *GetImage(wxString &name);
		void AddImage(CAnyImage *img, const wxString &name);
		void RemoveImage(const wxString &name);
};

typedef struct {
	uint32		nUsers;
	bool		bShowUploadQueue;

	ArrayOfUpDown		PointsForWeb;
	ArrayOfSession		Sessions;
	ArrayOfTransferredData	badlogins;

} GlobalParams;

typedef struct {
	wxString	sURL;
	in_addr		inadr;
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

	ServersInfo m_ServersInfo;
	SharedFilesInfo m_SharedFilesInfo;
	DownloadFilesInfo m_DownloadFilesInfo;
	CImageLib m_ImageLib;
	
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
		int 	GetWSPrefs();
		void	Print(const wxString &s);
		void	Send_Discard_V2_Request(CECPacket *request);

	protected:
		void	ProcessURL(ThreadData);
		void	ProcessFileReq(ThreadData);
		void 	ProcessImgFileReq(ThreadData);
		void 	ProcessStyleFileReq(ThreadData);
	
	private:
		wxString	_GetHeader(ThreadData, long lSession);
		wxString	_GetFooter(ThreadData);
		wxString	_GetServerList(ThreadData);
		wxString	_GetTransferList(ThreadData);
		wxString	_GetDownloadLink(ThreadData);
		wxString	_GetSharedFilesList(ThreadData);
		wxString	_GetGraphs(ThreadData);
		wxString	_GetLog(ThreadData);
		wxString	_GetServerInfo(ThreadData);
		wxString	_GetDebugLog(ThreadData);
		wxString	_GetStats(ThreadData);
		wxString	_GetPreferences(ThreadData);
		wxString	_GetLoginScreen(ThreadData);
		wxString	_GetConnectedServer(ThreadData);
		wxString _GetAddServerBox(ThreadData Data);
		wxString	_GetWebSearch(ThreadData Data);
		wxString _GetSearch(ThreadData);

		wxString	_ParseURL(ThreadData Data, wxString fieldname); 
		wxString	_ParseURLArray(ThreadData Data, wxString fieldname);
		bool	_IsLoggedIn(ThreadData Data, long lSession);
		void	_RemoveTimeOuts(ThreadData Data, long lSession);
		bool	_RemoveSession(ThreadData Data, long lSession);
		bool	_GetFileHash(wxString sHash, unsigned char *FileHash);
		wxString	_GetPlainResString(UINT nID, bool noquote = false);
		int	_GzipCompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level);
		void	_SetSharedFilePriority(wxString hash, uint8 priority);
		wxString	_GetWebCharSet();
		wxString	_LoadTemplate(wxString sAll, wxString sTemplateName);
		Session	GetSessionByID(ThreadData Data,long sessionID);
		bool	IsSessionAdmin(ThreadData Data,wxString SsessionID);
		wxString	GetPermissionDenied();
		wxString	_GetDownloadGraph(ThreadData Data,int percent, wxString &s_ChunkBar);

		void	InsertCatBox(wxString &Out, int preselect, wxString boxlabel, CECTag *cats, bool jump=false);
		wxString GetStatusBox(wxString &preselect);

		// Common data
		CamulewebApp	*webInterface;
		CWSThread	*wsThread;
		GlobalParams	m_Params;
		WebTemplates	m_Templates;
		bool		m_bServerWorking;
		int		m_iSearchSortby;
		bool		m_bSearchAsc;
		unsigned int	m_nRefresh;

};

#endif // WEBSERVER_H
