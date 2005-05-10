//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef OPCODES_H
#define OPCODES_H

#define    SEC2MS(sec)             ((sec)*1000)
#define    MIN2MS(min)             SEC2MS((min)*60)

// Handlers

enum {

	// socket handlers

	LISTENSOCKET_HANDLER = wxID_HIGHEST+123,  // random safe ID
	CLIENTREQSOCKET_HANDLER,
	SERVERUDPSOCKET_HANDLER,
	SERVERSOCKET_HANDLER,
	CLIENTUDPSOCKET_HANDLER,
	PROXY_SOCKET_HANDLER,
	
	// Custom Events

	ID_CORETIMER,
	ID_GUITIMER,

	TM_DNSDONE,
	TM_SOURCESDNSDONE,

	TM_TCPSOCKET,
	TM_UDPSOCKET

};

// MOD Note: Do not change this part - Merkur
#define	CURRENT_VERSION_SHORT			0x44
#define	EMULE_PROTOCOL				0x01
// MOD Note: end

// aMule version 

// No more Mod Version unless we're cvs

// RELEASERS: REMOVE THE DEFINE ON THE RELEASES, PLEASE
// AND FIX THE MOD_VERSION_LONG

#define __CVS__

#ifdef __CVS__
	#define	MOD_VERSION_LONG		wxT("aMule CVS")
#else
	#define	MOD_VERSION_LONG		wxT("aMule 2.0.1")
#endif

#define	VERSION_MJR		0x02
#define	VERSION_MIN		0
#define	VERSION_UPDATE		1 

#define	EDONKEYVERSION				0x3c
#define	PREFFILE_VERSION			0x14 //<<-- last change: reduced .dat, by using .ini
#define	PARTFILE_VERSION			0xe0
#define	PARTFILE_SPLITTEDVERSION		0xe1 // For edonkey part files importing.
#define	CREDITFILE_VERSION			0x12

#define RSAKEYSIZE				384             //384 bits

#define EMBLOCKSIZE				(uint32)184320

#define COMPILE_DATE				__DATE__
#define COMPILE_TIME				__TIME__

// MOD Note: Do not change this part - Merkur
#define	UDPSEARCHSPEED				1000	// if this value is too low you will miss sources
#define	MAX_RESULTS				100	// max global search results
#define	MAX_CLIENTCONNECTIONTRY			2
#define	CONNECTION_TIMEOUT			40000	// set this lower if you want less connections at once, set  it higher if you have enough sockets (edonkey has its own timout too, so a very high value won't effect this)
#define	FILEREASKTIME				1300000	// 1300000 <- original value ***
#define	SERVERREASKTIME				800000  // don't set this too low, it wont speed up anything, but it could kill amule or your internetconnection
#define	UDPSERVERREASKTIME			1300000	// 1300000 <- original value ***
//#define	SOURCECLIENTREASK			600000
#define	SOURCECLIENTREASKS			MIN2MS(40)	//40 mins
#define	SOURCECLIENTREASKF			MIN2MS(5)	//5 mins
#define	ED2KREPUBLISHTIME		MIN2MS(1)	//1 min
#define	MINCOMMONPENALTY			4
#define	UDPSERVERSTATTIME			5000
#define	UDPSERVSTATREASKTIME			14400
#define	UDPSERVERPORT				4665	// default udp port
#define	MIN_UP_CLIENTS_ALLOWED			2	// min. clients allowed to download regardless UPLOAD_CLIENT_DATARATE or any other factors. Don't set this too high
// MOD Note: end

#define	MAXCONPER5SEC				20
#define	MAXCON5WIN9X				10
#define	UPLOAD_CHECK_CLIENT_DR			1000
#define	UPLOAD_LOW_CLIENT_DR			2400	// uploadspeed per client in bytes - you may want to adjust
							// this if you have a slow connection or T1-T3 ;)
#define	UPLOAD_CLIENT_DATARATE			3072
#define	MAX_UP_CLIENTS_ALLOWED			250	// max. clients allowed regardless UPLOAD_CLIENT_DATARATE or any other factors. Don't set this too low, use DATARATE to adjust uploadspeed per client
#define	DOWNLOADTIMEOUT				100000
#define	CONSERVTIMEOUT				25000	// agelimit for pending connection attempts
#define	RARE_FILE				50
#define	BADCLIENTBAN				4
#define	MIN_REQUESTTIME				590000
#define	MAX_PURGEQUEUETIME			3600000
#define	PURGESOURCESWAPSTOP			900000	// (15 mins), how long forbid swapping a source to a certain file (NNP,...)
#define	CONNECTION_LATENCY			22050	// latency for responces
#define	SOURCESSLOTS				100
#define	MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE	1500
#define	MAXAVERAGETIME				40000 //millisecs
#define	CLIENTBANTIME				7200000 // 2h
#define	TRACKED_CLEANUP_TIME			3600000 // 1 hour
#define	KEEPTRACK_TIME				7200000 // 2h	//how long to keep track of clients which were once in the uploadqueue

// you shouldn't change anything here if you are not really sure, or amule will probaly not work
#define	MAXFRAGSIZE				1300
#define	PARTSIZE				(uint32)9728000
const int BLOCKSIZE =				184320;
#define	OP_EDONKEYHEADER			0xE3
#define	OP_EDONKEYPROT				OP_EDONKEYHEADER
#define	OP_PACKEDPROT				0xD4
#define	OP_EMULEPROT				0xC5
#define	OP_MLDONKEYPROT				0x00
#define	MET_HEADER				0x0E
	
const unsigned int UNLIMITED =	 		0;

// client <-> server
#define	OP_LOGINREQUEST				0x01	//<HASH 16><ID 4><PORT 2><1 Tag_set>
#define	OP_REJECT				0x05	//(null)
#define	OP_GETSERVERLIST			0x14	//(null)client->server
#define	OP_OFFERFILES				0x15	// <count 4>(<HASH 16><ID 4><PORT 2><1 Tag_set>)[count]
#define	OP_SEARCHREQUEST			0x16	// <Query_Tree>
#define	OP_DISCONNECT				0x18	// (not verified)
#define	OP_GETSOURCES				0x19	// <HASH 16> /v2 <HASH 16><SIZE_4> (17.3)
#define	OP_SEARCH_USER				0x1A	// <Query_Tree>
#define	OP_CALLBACKREQUEST			0x1C	// <ID 4>
#define	OP_QUERY_CHATS				0x1D	// (deprecated not supported by server any longer)
#define	OP_CHAT_MESSAGE 		       	0x1E    // (deprecated not supported by server any longer)
#define	OP_JOIN_ROOM    		        0x1F    // (deprecated not supported by server any longer)
#define	OP_QUERY_MORE_RESULT    		0x21    // (null)
#define	OP_SERVERLIST				0x32	// <count 1>(<IP 4><PORT 2>)[count] server->client
#define	OP_SEARCHRESULT				0x33	// <count 4>(<HASH 16><ID 4><PORT 2><1 Tag_set>)[count]
#define	OP_SERVERSTATUS				0x34	// <USER 4><FILES 4>
#define	OP_CALLBACKREQUESTED			0x35	// <IP 4><PORT 2>
#define	OP_CALLBACK_FAIL			0x36	// (null notverified)
#define	OP_SERVERMESSAGE			0x38	// <len 2><Message len>
#define	OP_CHAT_ROOM_REQUEST    		0x39    // (deprecated not supported by server any longer)
#define	OP_CHAT_BROADCAST       		0x3A    // (deprecated not supported by server any longer)
#define	OP_CHAT_USER_JOIN       		0x3B    // (deprecated not supported by server any longer)
#define	OP_CHAT_USER_LEAVE      		0x3C    // (deprecated not supported by server any longer)
#define	OP_CHAT_USER            		0x3D    // (deprecated not supported by server any longer)
#define	OP_IDCHANGE				0x40	// <NEW_ID 4>
#define	OP_SERVERIDENT				0x41	// <HASH 16><IP 4><PORT 2>{1 TAG_SET}
#define	OP_FOUNDSOURCES				0x42	// <HASH 16><count 1>(<ID 4><PORT 2>)[count]
#define	OP_USERS_LIST           		0x43    // <count 4>(<HASH 16><ID 4><PORT 2><1 Tag_set>)[count]

//client <-> UDP server
#define	OP_GLOBGETSOURCES2		0x94	// <HASH 16><SIZE_4>*
#define	OP_GLOBSERVSTATREQ			0x96	// (null)
#define	OP_GLOBSERVSTATRES			0x97	// <USER 4><FILES 4>
#define	OP_GLOBSEARCHREQ			0x98	// <search_tree>
#define	OP_GLOBSEARCHRES			0x99	// 
#define	OP_GLOBGETSOURCES			0x9A	// <HASH 16>
#define	OP_GLOBFOUNDSOURCES			0x9B	//
#define	OP_GLOBCALLBACKREQ			0x9C	// <IP 4><PORT 2><client_ID 4>
#define	OP_INVALID_LOWID			0x9E	// <ID 4>
#define	OP_SERVER_LIST_REQ			0xA0	// <IP 4><PORT 2>
#define	OP_SERVER_LIST_RES			0xA1	// <count 1> (<ip 4><port 2>)[count]
#define	OP_SERVER_DESC_REQ			0xA2	// (null)
#define	OP_SERVER_DESC_RES			0xA3	// <name_len 2><name name_len><desc_len 2 desc_en>
#define	OP_SERVER_LIST_REQ2			0xA4	// (null)

#define INV_SERV_DESC_LEN			0xF0FF	// used as an 'invalid' string len for OP_SERVER_DESC_REQ/RES

// client <-> client
#define	OP_HELLO				0x01	// 0x10<HASH 16><ID 4><PORT 2><1 Tag_set>
#define	OP_SENDINGPART				0x46	// <HASH 16><von 4><bis 4><Daten len:(von-bis)>
#define	OP_REQUESTPARTS				0x47	// <HASH 16><von[3] 4*3><bis[3] 4*3>
#define	OP_FILEREQANSNOFIL			0x48	// <HASH 16>
#define	OP_END_OF_DOWNLOAD     			0x49    // <HASH 16>
#define	OP_ASKSHAREDFILES			0x4A	// (null)
#define	OP_ASKSHAREDFILESANSWER 		0x4B	// <count 4>(<HASH 16><ID 4><PORT 2><1 Tag_set>)[count]
#define	OP_HELLOANSWER				0x4C	// <HASH 16><ID 4><PORT 2><1 Tag_set><SERVER_IP 4><SERVER_PORT 2>
#define	OP_CHANGE_CLIENT_ID 			0x4D	// <ID_old 4><ID_new 4>
#define	OP_MESSAGE				0x4E	// <len 2><Message len>
#define	OP_SETREQFILEID				0x4F	// <HASH 16>
#define	OP_FILESTATUS				0x50	// <HASH 16><count 2><status(bit array) len:((count+7)/8)>
#define	OP_HASHSETREQUEST			0x51	// <HASH 16>
#define	OP_HASHSETANSWER			0x52	// <count 2><HASH[count] 16*count>
#define	OP_STARTUPLOADREQ			0x54	// <HASH 16>
#define	OP_ACCEPTUPLOADREQ			0x55	// (null)
#define	OP_CANCELTRANSFER			0x56	// (null)	
#define	OP_OUTOFPARTREQS			0x57	// (null)
#define	OP_REQUESTFILENAME			0x58	// <HASH 16>	(more correctly file_name_request)
#define	OP_REQFILENAMEANSWER			0x59	// <HASH 16><len 4><NAME len>
#define	OP_CHANGE_SLOT				0x5B	// <HASH 16>
#define	OP_QUEUERANK				0x5C	// <wert  4> (slot index of the request)
#define	OP_ASKSHAREDDIRS			0x5D	// (null)
#define	OP_ASKSHAREDFILESDIR			0x5E	// <len 2><Directory len>
#define	OP_ASKSHAREDDIRSANS			0x5F	// <count 4>(<len 2><Directory len>)[count]
#define	OP_ASKSHAREDFILESDIRANS			0x60	// <len 2><Directory len><count 4>(<HASH 16><ID 4><PORT 2><1 T
#define	OP_ASKSHAREDDENIEDANS			0x61	// (null)

// this 'identifier' is used for referencing shared part (incomplete) files with the OP_ASKSHAREDDIRS and related opcodes
// it was introduced with eDonkeyHybrid and is considered as part of the protocol.
#define OP_INCOMPLETE_SHARED_FILES wxT("!Incomplete Files")

// extened prot client <-> extened prot client
#define	OP_EMULEINFO				0x01	//
#define	OP_EMULEINFOANSWER			0x02	//
#define	OP_COMPRESSEDPART			0x40	//
#define	OP_QUEUERANKING				0x60	// <RANG 2>
#define	OP_FILEDESC				0x61	// <len 2><NAME len>
#define	OP_VERIFYUPSREQ				0x71	// (never used)
#define	OP_VERIFYUPSANSWER			0x72	// (never used)
#define	OP_UDPVERIFYUPREQ			0x73	// (never used)
#define	OP_UDPVERIFYUPA				0x74	// (never used)
#define	OP_REQUESTSOURCES			0x81	// <HASH 16>
#define	OP_ANSWERSOURCES			0x82	//
#define OP_PUBLICKEY				0x85	// <len 1><pubkey len>
#define OP_SIGNATURE				0x86	// v1: <len 1><signature len>
							// v2:<len 1><signature len><sigIPused 1>
#define OP_SECIDENTSTATE			0x87	// <state 1><rndchallenge 4>
#define OP_REQUESTPREVIEW			0x90	// <HASH 16>
#define OP_PREVIEWANSWER			0x91	// <HASH 16><frames 1>{frames * <len 4><frame len>}
#define OP_MULTIPACKET				0x92
#define OP_MULTIPACKETANSWER			0x93
#define	OP_PEERCACHE_QUERY			0x94
#define	OP_PEERCACHE_ANSWER			0x95
#define	OP_PEERCACHE_ACK			0x96
#define	OP_PUBLICIP_REQ				0x97
#define	OP_PUBLICIP_ANSWER			0x98
#define OP_CALLBACK				0x99	// <HASH 16><HASH 16><uint 16>
#define OP_REASKCALLBACKTCP			0x9A
#define OP_AICHREQUEST				0x9B	// <HASH 16><uint16><HASH aichhashlen>
#define OP_AICHANSWER				0x9C	// <HASH 16><uint16><HASH aichhashlen> <data>
#define OP_AICHFILEHASHANS			0x9D	  
#define OP_AICHFILEHASHREQ			0x9E

// extened prot client <-> extened prot client UDP
#define	OP_REASKFILEPING			0x90	// <HASH 16>
#define	OP_REASKACK				0x91	// <RANG 2>
#define	OP_FILENOTFOUND				0x92	// (null)
#define	OP_QUEUEFULL				0x93	// (null)
	
// server.met
#define	ST_SERVERNAME				0x01	// <string>
#define	ST_DESCRIPTION				0x0B	// <string>
#define	ST_PING					0x0C	// <uint32>
#define	ST_PREFERENCE				0x0E	// <uint32>
#define	ST_FAIL					0x0D	// <uint32>
#define	ST_DYNIP				0x85
// #define ST_LASTPING				0x86	// <uint32>
#define 	ST_MAXUSERS			0x87
#define	ST_SOFTFILES				0x88
#define	ST_HARDFILES				0x89
#define	ST_LASTPING				0x90	// <uint32>
#define	ST_VERSION				0x91	// <string>
#define	ST_UDPFLAGS				0x92	// <uint32>
#define	ST_AUXPORTSLIST				0x93	// <string>
#define	ST_LOWIDUSERS				0x94	// <uint32>

//file tags

//file tags
#define FT_FILENAME				 0x01	// <string>
#define TAG_NAME				"\x01"	// <string>
#define FT_FILESIZE				 0x02	// <uint32>
#define TAG_SIZE				"\x02"	// <uint32>
#define FT_FILETYPE				 0x03	// <string>
#define TAG_TYPE				"\x03"	// <string>
#define FT_FILEFORMAT				 0x04	// <string>
#define TAG_FORMAT				"\x04"	// <string>
#define FT_LASTSEENCOMPLETE			 0x05	// <uint32>
#define TAG_COLLECTION				"\x05"
#define	TAG_PART_PATH				"\x06"	// <string>
#define	TAG_PART_HASH				"\x07"
#define FT_TRANSFERED				 0x08	// <uint32>
#define	TAG_COPIED				"\x08"	// <uint32>
#define FT_GAPSTART				 0x09	// <uint32>
#define	TAG_GAP_START				"\x09"	// <uint32>
#define FT_GAPEND				 0x0A	// <uint32>
#define	TAG_GAP_END				"\x0A"	// <uint32>
#define	TAG_DESCRIPTION				"\x0B"	// <string>
#define	TAG_PING				"\x0C"
#define	TAG_FAIL				"\x0D"
#define	TAG_PREFERENCE				"\x0E"
#define TAG_PORT				"\x0F"
#define TAG_IP_ADDRESS				"\x10"
#define TAG_VERSION				"\x11"	// <string>
#define FT_PARTFILENAME				 0x12	// <string>
#define TAG_TEMPFILE				"\x12"	// <string>
#define FT_OLDDLPRIORITY			 0x13	// Not used anymore
#define TAG_PRIORITY				"\x13"	// <uint32>
#define FT_STATUS				 0x14	// <uint32>
#define TAG_STATUS				"\x14"	// <uint32>
#define FT_SOURCES				 0x15	// <uint32>
#define TAG_AVAILABILITY			"\x15"	// <uint32>
#define FT_PERMISSIONS				 0x16	// <uint32>
#define TAG_QTIME				"\x16"
#define FT_OLDULPRIORITY			 0x17	// Not used anymore
#define TAG_PARTS				"\x17"
#define FT_DLPRIORITY				 0x18	// Was 13
#define FT_ULPRIORITY				 0x19	// Was 17
#define FT_KADLASTPUBLISHKEY			 0x20	// <uint32>
#define FT_KADLASTPUBLISHSRC			 0x21	// <uint32>
#define	FT_FLAGS				 0x22	// <uint32>
#define	FT_DL_ACTIVE_TIME			 0x23	// <uint32>
#define	FT_CORRUPTEDPARTS			 0x24	// <string>
#define FT_DL_PREVIEW				 0x25
#define  FT_KADLASTPUBLISHNOTES	 0x26	// <uint32> 
#define FT_AICH_HASH				 0x27
#define	FT_COMPLETE_SOURCES			 0x30	// nr. of sources which share a complete version
							 //of the associated file (supported by eserver 16.46+)
// statistic
#define FT_ATTRANSFERED			 0x50	// <uint32>
#define FT_ATREQUESTED			 0x51	// <uint32>
#define FT_ATACCEPTED			 0x52	// <uint32>
#define FT_CATEGORY			 0x53	// <uint32>
#define	FT_ATTRANSFEREDHI		 0x54	// <uint32>
#define	TAG_MEDIA_ARTIST		"\xD0"	// <string>
#define	 FT_MEDIA_ARTIST		 0xD0	// <string>
#define	TAG_MEDIA_ALBUM			"\xD1"	// <string>
#define	 FT_MEDIA_ALBUM			 0xD1	// <string>
#define	TAG_MEDIA_TITLE			"\xD2"	// <string>
#define	 FT_MEDIA_TITLE			 0xD2	// <string>
#define	TAG_MEDIA_LENGTH		"\xD3"	// <uint32> !!!
#define	 FT_MEDIA_LENGTH		 0xD3	// <uint32> !!!
#define	TAG_MEDIA_BITRATE		"\xD4"	// <uint32>
#define	 FT_MEDIA_BITRATE		 0xD4	// <uint32>
#define	TAG_MEDIA_CODEC			"\xD5"	// <string>
#define	 FT_MEDIA_CODEC			 0xD5	// <string>
#define TAG_BUDDYHASH			"\xF8"	// <string>
#define TAG_CLIENTLOWID			"\xF9"	// <uint32>
#define TAG_SERVERPORT			"\xFA"	// <uint16>
#define TAG_SERVERIP			"\xFB"	// <uint32>
#define TAG_SOURCEUPORT			"\xFC"	// <uint16>
#define TAG_SOURCEPORT			"\xFD"	// <uint16>
#define TAG_SOURCEIP			"\xFE"	// <uint32>
#define TAG_SOURCETYPE			"\xFF"	// <uint8>

#define	TAGTYPE_HASH			0x01
#define	TAGTYPE_STRING			0x02
#define	TAGTYPE_UINT32			0x03
#define	TAGTYPE_FLOAT32			0x04
#define	TAGTYPE_BOOL			0x05
#define	TAGTYPE_BOOLARRAY		0x06
#define	TAGTYPE_BLOB			0x07
#define	TAGTYPE_UINT16			0x08
#define	TAGTYPE_UINT8			0x09
#define	TAGTYPE_BSOB			0x0A

#define TAGTYPE_STR1			0x11
#define TAGTYPE_STR2			0x12
#define TAGTYPE_STR3			0x13
#define TAGTYPE_STR4			0x14
#define TAGTYPE_STR5			0x15
#define TAGTYPE_STR6			0x16
#define TAGTYPE_STR7			0x17
#define TAGTYPE_STR8			0x18
#define TAGTYPE_STR9			0x19
#define TAGTYPE_STR10			0x1A
#define TAGTYPE_STR11			0x1B
#define TAGTYPE_STR12			0x1C
#define TAGTYPE_STR13			0x1D
#define TAGTYPE_STR14			0x1E
#define TAGTYPE_STR15			0x1F
#define TAGTYPE_STR16			0x20
#define TAGTYPE_STR17			0x21	// accepted by eMule 0.42f (02-Mai-2004) in receiving code
						// only because of a flaw, those tags are handled correctly,
						// but should not be handled at all
#define TAGTYPE_STR18			0x22	// accepted by eMule 0.42f (02-Mai-2004) in receiving code
						//  only because of a flaw, those tags are handled correctly,
						// but should not be handled at all
#define TAGTYPE_STR19			0x23	// accepted by eMule 0.42f (02-Mai-2004) in receiving code
						// only because of a flaw, those tags are handled correctly,
						// but should not be handled at all
#define TAGTYPE_STR20			0x24	// accepted by eMule 0.42f (02-Mai-2004) in receiving code
						// only because of a flaw, those tags are handled correctly,
						// but should not be handled at all
#define TAGTYPE_STR21			0x25	// accepted by eMule 0.42f (02-Mai-2004) in receiving code
						// only because of a flaw, those tags are handled correctly,
						// but should not be handled at all
#define TAGTYPE_STR22			0x26	// accepted by eMule 0.42f (02-Mai-2004) in receiving code
						// only because of a flaw, those tags are handled correctly,
						// but should not be handled at all


#define	ED2KFTSTR_AUDIO			wxT("Audio")	// value for eD2K tag FT_FILETYPE
#define	ED2KFTSTR_VIDEO			wxT("Video")	// value for eD2K tag FT_FILETYPE
#define	ED2KFTSTR_IMAGE			wxT("Image")	// value for eD2K tag FT_FILETYPE
#define	ED2KFTSTR_DOCUMENT		wxT("Doc")	// value for eD2K tag FT_FILETYPE
#define	ED2KFTSTR_PROGRAM		wxT("Pro")	// value for eD2K tag FT_FILETYPE
#define	ED2KFTSTR_ARCHIVE		wxT("Arc")	// eMule internal use only
#define	ED2KFTSTR_CDIMAGE		wxT("Iso")	// eMule internal use only

// additional media meta data tags from eDonkeyHybrid (note also the uppercase/lowercase)
#define	FT_ED2K_MEDIA_ARTIST		"Artist"	// <string>
#define	FT_ED2K_MEDIA_ALBUM		"Album"		// <string>
#define	FT_ED2K_MEDIA_TITLE		"Title"		// <string>
#define	FT_ED2K_MEDIA_LENGTH		"length"	// <string> !!!
#define	FT_ED2K_MEDIA_BITRATE		"bitrate"	// <uint32>
#define	FT_ED2K_MEDIA_CODEC		"codec"		// <string>
#define TAG_NSENT			"# Sent"
#define TAG_ONIP			"ip"
#define TAG_ONPORT			"port"

// ed2k search expression comparison operators
#define ED2K_SEARCH_OP_EQUAL         0 // eserver 16.45+
#define ED2K_SEARCH_OP_GREATER       1 // dserver
#define ED2K_SEARCH_OP_LESS          2 // dserver
#define ED2K_SEARCH_OP_GREATER_EQUAL 3 // eserver 16.45+
#define ED2K_SEARCH_OP_LESS_EQUAL    4 // eserver 16.45+
#define ED2K_SEARCH_OP_NOTEQUAL      5 // eserver 16.45+

// Kad search expression comparison operators
#define KAD_SEARCH_OP_EQUAL         0 // eMule 0.43+
#define KAD_SEARCH_OP_GREATER_EQUAL 1 // eMule 0.40+; NOTE: this different than ED2K!
#define KAD_SEARCH_OP_LESS_EQUAL    2 // eMule 0.40+; NOTE: this different than ED2K!
#define KAD_SEARCH_OP_GREATER       3 // eMule 0.43+; NOTE: this different than ED2K!
#define KAD_SEARCH_OP_LESS          4 // eMule 0.43+; NOTE: this different than ED2K!
#define KAD_SEARCH_OP_NOTEQUAL      5 // eMule 0.43+

#define	CT_NAME				0x01
#define	CT_PORT				0x0f
#define	CT_VERSION			0x11
#define	CT_SERVER_FLAGS			0x20	// currently only used to inform a server about supported features
#define	CT_EMULECOMPAT_OPTIONS		0xef
#define	CT_EMULE_RESERVED1		0xf0
#define	CT_EMULE_RESERVED2		0xf1
#define	CT_EMULE_RESERVED3		0xf2
#define	CT_EMULE_RESERVED4		0xf3
#define	CT_EMULE_RESERVED5		0xf4
#define	CT_EMULE_RESERVED6		0xf5
#define	CT_EMULE_RESERVED7		0xf6
#define	CT_EMULE_RESERVED8		0xf7
#define	CT_EMULE_RESERVED9		0xf8
#define	CT_EMULE_UDPPORTS		0xf9
#define	CT_EMULE_MISCOPTIONS1		0xfa
#define	CT_EMULE_VERSION		0xfb
#define CT_EMULE_BUDDYIP		0xfc
#define CT_EMULE_BUDDYUDP		0xfd
#define CT_EMULE_MISCOPTIONS2	0xfe
#define CT_EMULE_RESERVED13		0xff

#define	MP_MESSAGE			10102
#define	MP_DETAIL			10103
#define	MP_ADDFRIEND			10104
#define	MP_REMOVEFRIEND			10105
#define	MP_SHOWLIST			10106
#define	MP_FRIENDSLOT			10107
#define MP_CHANGE2FILE			10108
#define	MP_CANCEL			10201
#define	MP_STOP				10202
#define	MP_RESUME			10204
#define	MP_PAUSE			10203
#define	MP_CLEARCOMPLETED		10205
#define	MP_VIEW				30207
#define	MP_SENDMESSAGE			30208
#define	MP_FAKECHECK1			10210   // deltaHF -> fakecheck
#define	MP_FAKECHECK2			10211
#define	MP_WS				10212
#define	MP_RAZORSTATS			10213
#define MP_GETCOMMENTS			10214
//For comments 
#define	MP_CMT				10208
#define	MP_SWITCHCTRL_0			10250
#define	MP_SWITCHCTRL_9			10259


#define	MP_PRIOVERYLOW				10300
#define	MP_PRIOLOW				10301
#define	MP_PRIONORMAL				10302
#define	MP_PRIOHIGH				10303
#define	MP_PRIOVERYHIGH				10304
#define	MP_POWERSHARE                           10331
#define	MP_PRIOAUTO				10317
#define	MP_GETED2KLINK				10305
#define	MP_GETHTMLED2KLINK			10306
#define	MP_GETSOURCEED2KLINK			10299
#define	MP_GETHOSTNAMESOURCEED2KLINK		10361
#define	MP_METINFO				10307
#define	MP_CONNECTTO				10311
#define	MP_REMOVE				10312
#define	MP_REMOVEALL				10313
#define	MP_UNBAN				10315
#define	MP_ADDTOSTATIC				10316
#define	MP_REMOVEFROMSTATIC			10400
#define	MP_VIEWFILECOMMENTS			10401
#define	MP_CAT_ADD				10321
#define	MP_CAT_EDIT				10322
#define	MP_CAT_REMOVE				10323
#define MP_TOOGLELIST				10324
#define MP_CLOSE_TAB				10325
#define MP_CLOSE_ALL_TABS			10326
#define MP_CLOSE_OTHER_TABS			10327

/* Razor 1a - Modif by MikaelB
     Opcodes for :
      - Drop No Needed Sources now
      - Drop Full Queue Sources now
      - Drop High Queue Rating Sources now
      - Clean Up Sources now ( drop NNS, FQS and HQRS )
      - Swap every A4AF to this file now
      - Swap every A4AF to this file ( AUTO )
      - Swap every A4AF to any other file now   */
#define	MP_DROP_NO_NEEDED_SOURCES		57841
#define	MP_DROP_FULL_QUEUE_SOURCES		57842
#define	MP_DROP_HIGH_QUEUE_RATING_SOURCES	57843
#define	MP_CLEAN_UP_SOURCES			57844
#define	MP_SWAP_A4AF_TO_THIS			57845
#define	MP_SWAP_A4AF_TO_THIS_AUTO		57846
#define	MP_SWAP_A4AF_TO_ANY_OTHER		57847

//menus
#define MP_MENU_PRIO				999989
#define MP_MENU_EXTD				999988
#define MP_MENU_FAKE				999987
#define MP_MENU_CATS				432843

#define	MP_ASSIGNCAT				10700
// reserve some for categories!
#define	MP_CAT_SET0				10900
// reserve some for change all-cats (about 20)
	
// amule tagnames
#define 	ET_COMPRESSION			0x20
#define	ET_UDPPORT				0x21
#define	ET_UDPVER				0x22
#define	ET_SOURCEEXCHANGE			0x23
#define	ET_COMMENTS				0x24
#define	ET_EXTENDEDREQUEST			0x25
#define	ET_COMPATIBLECLIENT			0x26
#define	ET_BAD_COMPATABLECLIENT			0x30
#define	ET_FEATURES				0x27	// bit 0: SecIdent v1 - bit 1: SecIdent v2
#define	ET_MOD_VERSION				0x55

// tags by various mods	
#define	ET_TAROD				0x77	// int - flag - Tarod mod
#define	ET_TAROD_VERSION			0x78	// Tarod mod version number
#define	ET_L2HAC				0x3E	// does send FILEREASKTIME
							// seems to synchronize connecting between low and high ID users

#define	ET_MOD_LSD				0x87	// int - flag
#define	ET_MOD_LSD_VERSION			0x88	// int
#define	ET_MOD_SECURE_COMMUNITY			0x5C	// string
#define	ET_MOD_PROTOCOL				0x56	// string
#define	ET_MOD_Morph				0x79	// int
#define	ET_MOD_Morph_VERSION			0x80	// int
#define	ET_MOD_FUSION				0x66	// int
#define	ET_MOD_FUSION_VERSION			0x67	// string
#define	ET_MOD_MorTillo				0x82	// int
#define	ET_MOD_MorTillo_VERSION			0x83	// string
#define	ET_MOD_LOVELACE_VERSION			0x90	// string
#define	ET_MOD_OXY				0x94	// unspecified type
#define	ET_PLUS					0x99	// int - eMule Plus version
#define	ET_FEATURESET				0x54	// int - [Bloodymad Featureset]

// Reuse a random tag for aMule OS_INFO

#define	ET_OS_INFO				ET_MOD_OXY // We use MOD_OXY because the type is unknown

// KADEMLIA (opcodes) (udp)
#define KADEMLIA_BOOTSTRAP_REQ	0x00	// <PEER (sender) [25]>
#define KADEMLIA_BOOTSTRAP_RES	0x08	// <CNT [2]> <PEER [25]>*(CNT)

#define KADEMLIA_HELLO_REQ	 	0x10	// <PEER (sender) [25]>
#define KADEMLIA_HELLO_RES     	0x18	// <PEER (receiver) [25]>

#define KADEMLIA_REQ		   	0x20	// <TYPE [1]> <HASH (target) [16]> <HASH (receiver) 16>
#define KADEMLIA_RES			0x28	// <HASH (target) [16]> <CNT> <PEER [25]>*(CNT)

#define KADEMLIA_SEARCH_REQ		0x30	// <HASH (key) [16]> <ext 0/1 [1]> <SEARCH_TREE>[ext]
//#define UNUSED				0x31	// Old Opcode, don't use.
#define KADEMLIA_SRC_NOTES_REQ	0x32	// <HASH (key) [16]>
#define KADEMLIA_SEARCH_RES		0x38	// <HASH (key) [16]> <CNT1 [2]> (<HASH (answer) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)
//#define UNUSED				0x39	// Old Opcode, don't use.
#define KADEMLIA_SRC_NOTES_RES	0x3A	// <HASH (key) [16]> <CNT1 [2]> (<HASH (answer) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)

#define KADEMLIA_PUBLISH_REQ	0x40	// <HASH (key) [16]> <CNT1 [2]> (<HASH (target) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)
//#define UNUSED				0x41	// Old Opcode, don't use.
#define KADEMLIA_PUB_NOTES_REQ	0x42	// <HASH (key) [16]> <HASH (target) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)
#define KADEMLIA_PUBLISH_RES	0x48	// <HASH (key) [16]>
//#define UNUSED				0x49	// Old Opcode, don't use.
#define KADEMLIA_PUB_NOTES_RES	0x4A	// <HASH (key) [16]>

#define KADEMLIA_FIREWALLED_REQ	0x50	// <TCPPORT (sender) [2]>
#define KADEMLIA_FINDBUDDY_REQ	0x51	// <TCPPORT (sender) [2]>
#define KADEMLIA_FINDSOURCE_REQ	0x52	// <TCPPORT (sender) [2]>
#define KADEMLIA_FIREWALLED_RES	0x58	// <IP (sender) [4]>
#define KADEMLIA_FIREWALLED_ACK	0x59	// (null)
#define KADEMLIA_FINDBUDDY_RES	0x5A	// <TCPPORT (sender) [2]>

// KADEMLIA (parameter)
#define KADEMLIA_FIND_VALUE		0x02
#define KADEMLIA_STORE			0x04
#define KADEMLIA_FIND_NODE		0x0B

// CMuleListCtrl tabs.
#define	MP_LISTCOL_1				10601
#define	MP_LISTCOL_2				10602
#define	MP_LISTCOL_3				10603
#define	MP_LISTCOL_4				10604
#define	MP_LISTCOL_5				10605
#define	MP_LISTCOL_6				10606
#define	MP_LISTCOL_7				10607
#define	MP_LISTCOL_8				10608
#define	MP_LISTCOL_9				10609
#define	MP_LISTCOL_10				10610
#define	MP_LISTCOL_11				10611
#define	MP_LISTCOL_12				10612
#define	MP_LISTCOL_13				10613
#define	MP_LISTCOL_14				10614
#define	MP_LISTCOL_15				10615

#define	DISKSPACERECHECKTIME			900000	// checkDiskspace

#endif // OPCODES_H
