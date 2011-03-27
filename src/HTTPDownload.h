//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Timo Kujala ( tiku@users.sourceforge.net )
// Copyright (c) 2002-2011 Patrizio Bassi ( hetfield@amule.org )
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

#ifndef HTTPDOWNLOAD_H
#define HTTPDOWNLOAD_H

#include "GuiEvents.h"		// Needed for HTTP_Download_File
#include "MuleThread.h"		// Needed for CMuleThread
#include <wx/datetime.h>	// Needed for wxDateTime
#include <set>

class wxEvtHandler;
class wxHTTP;
class wxInputStream;

enum HTTPDownloadResult {
	HTTP_Success = 0,
	HTTP_Error,
	HTTP_Skipped
};

class CHTTPDownloadThread : public CMuleThread
{
public:
	/** Note: wxChar* is used to circumvent the thread-unsafe wxString reference counting. */
	CHTTPDownloadThread(const wxString& url, const wxString& filename, const wxString& oldfilename, HTTP_Download_File file_id, 
						bool showDialog, bool checkDownloadNewer);

	static void StopAll();
private:
	ExitCode		Entry();
	virtual void 		OnExit();

	wxString		m_url;
	wxString		m_tempfile;
	wxDateTime		m_lastmodified;	//! Date on which the file being updated was last modified.
	int			m_result;
	int			m_response;	//! HTTP response code (e.g. 200)
	int			m_error;	//! Additional error code (@see wxProtocol class)
	HTTP_Download_File	m_file_id;
	wxEvtHandler*		m_companion;
	typedef std::set<CHTTPDownloadThread *>	ThreadSet;
	static ThreadSet	s_allThreads;
	static wxMutex		s_allThreadsMutex;

	wxInputStream* GetInputStream(wxHTTP * & url_handler, const wxString& location, bool proxy);
	static wxString FormatDateHTTP(const wxDateTime& date);
};

#endif // HTTPDOWNLOAD_H
// File_checked_for_headers
