//
// This file is part of the aMule Project.
//
// Copyright (c) 2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef CONSTANTS_H
#define CONSTANTS_H

//! This enum lists the views currently supported by the CClientListCtrl class.
enum ViewType
{ 
	//! The "neutral" state, where nothing is displayed.
	vtNone = 0,
	
	//! Clients which are recieved files are displayed.
	vtUploading = 1,
	
	//! Clients which are queued for files are displayed.
	vtQueued = 2,
	
	//! All clients are displayed.
	vtClients = 3
};


//! These types specifies the char. of a given item on the CDownloadListCtrl
enum DownloadItemType
{
	//! A CPartFile object
	FILE_TYPE,
	//! A source which is currently queued for another file.
	A4AF_SOURCE,
	//! A source which has not yet been contacted.
	UNAVAILABLE_SOURCE,
	//! A source which is currently queued for this file.
	AVAILABLE_SOURCE
};


// lfroen : custom events for core internal messages
// 'cause - there's no wxCommand etc in wxBase
enum Core_Event_ID 
{
	HTTP_DOWNLOAD_FINISHED = 1,
	
	SOURCE_DNS_DONE,
	UDP_DNS_DONE,
	SERVER_DNS_DONE
};


//! These IDs are used when a download is initiated via the HTTPThread.
enum HTTP_Download_File 
{
	//! The download is a ipfilter.dat file (possibly compressed).
	HTTP_IPFilter = 1,
	//! The download is a server.met file (possibly compressed).
	HTTP_ServerMet,
	// Auto-updating server.met has a different callback.
	HTTP_ServerMetAuto,
	//! The download is the version check file.
	HTTP_VersionCheck,
	//! The download is a notes.dat file (possibly compressed).
	HTTP_NodesDat
};


#endif
