//
// This file is part of the aMule Project.
//
// Copyright (c) 2006-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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
	
	//! Clients which are received files are displayed.
	vtUploading = 1,
	
	//! Clients which are queued for files are displayed.
	vtQueued = 2,
	
	//! All clients are displayed.
	vtClients = 3
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
	HTTP_NodesDat,
	//! The download is a GeoIP database file (possibly compressed).
	HTTP_GeoIP
};

//! Source types for source showing list.
enum SourceItemType
{
	//! A source which is currently queued for another file.
	A4AF_SOURCE,
	//! A source which has not yet been contacted.
	UNAVAILABLE_SOURCE,
	//! A source which is currently queued for this file.
	AVAILABLE_SOURCE
};

//! Types of stat graphs
enum StatsGraphType {
	GRAPH_INVALID = 0,
	GRAPH_DOWN,
	GRAPH_UP,
	GRAPH_CONN,
	GRAPH_KAD
};

// KnownFile constants

#define	PS_READY			0
#define	PS_EMPTY			1
#define PS_WAITINGFORHASH		2
#define PS_HASHING			3
#define PS_ERROR			4
#define	PS_INSUFFICIENT			5
#define	PS_UNKNOWN			6
#define PS_PAUSED			7
#define PS_COMPLETING			8
#define PS_COMPLETE			9
#define PS_ALLOCATING			10


#define PR_VERYLOW			4 // I Had to change this because
					  // it didn't save negative number
					  // correctly.. Had to modify the
					  // sort function for this change..
#define PR_LOW				0 //*
#define PR_NORMAL			1 // Don't change this - needed for
					  // edonkey clients and server!
#define PR_HIGH				2 //*
#define PR_VERYHIGH			3
#define PR_AUTO				5
#define PR_POWERSHARE			6 //added for powershare (deltaHF)


// CUpDownClient constants

enum ESourceFrom {
	SF_NONE,
	SF_LOCAL_SERVER,
	SF_REMOTE_SERVER,
	SF_KADEMLIA,
	SF_SOURCE_EXCHANGE,
	SF_PASSIVE,
	SF_LINK,
	SF_SOURCE_SEEDS,
	SF_SEARCH_RESULT
};

// downloadstate
enum EDownloadState {
	DS_DOWNLOADING = 0,
	DS_ONQUEUE,
	DS_CONNECTED,
	DS_CONNECTING,
	DS_WAITCALLBACK,
	DS_WAITCALLBACKKAD,
	DS_REQHASHSET,
	DS_NONEEDEDPARTS,
	DS_TOOMANYCONNS,
	DS_TOOMANYCONNSKAD,
	DS_LOWTOLOWIP,
	DS_BANNED,
	DS_ERROR,
	DS_NONE,
	DS_REMOTEQUEUEFULL  // not used yet, except in statistics
};

// uploadstate
enum EUploadState {
	US_UPLOADING = 0,
	US_ONUPLOADQUEUE,
	US_WAITCALLBACK,
	US_CONNECTING,
	US_PENDING,
	US_LOWTOLOWIP,
	US_BANNED,
	US_ERROR,
	US_NONE
};

// Obfuscation status
enum EObfuscationState {
	OBST_UNDEFINED = 0,
	OBST_ENABLED,
	OBST_SUPPORTED,
	OBST_NOT_SUPPORTED,
	OBST_DISABLED
};

// m_byChatstate
enum {
	MS_NONE = 0,
	MS_CHATTING,
	MS_CONNECTING,
	MS_UNABLETOCONNECT
};



#endif
// File_checked_for_headers
