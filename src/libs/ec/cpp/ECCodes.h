// 
//  This file is part of the aMule Project.
// 
//  Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// 
//  Any parts of this program derived from the xMule, lMule or eMule project,
//  or contributed by third-party developers are copyrighted by their
//  respective authors.
// 
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
// 
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

// Purpose:
// EC codes and type definition.

#ifndef __ECCODES_H__
#define __ECCODES_H__

typedef uint8_t ec_opcode_t;
typedef uint16_t ec_tagname_t;
typedef uint8_t ec_tagtype_t;
typedef uint32_t ec_taglen_t;

enum ProtocolVersion {
	EC_CURRENT_PROTOCOL_VERSION = 0x0203
};

enum ECFlags {
	EC_FLAG_ZLIB	 = 0x00000001,
	EC_FLAG_UTF8_NUMBERS = 0x00000002,
	EC_FLAG_UNKNOWN_MASK = 0xff7f7f08
};

enum ECOpCodes {
	EC_OP_NOOP                          = 0x01,
	EC_OP_AUTH_REQ                      = 0x02,
	EC_OP_AUTH_FAIL                     = 0x03,
	EC_OP_AUTH_OK                       = 0x04,
	EC_OP_FAILED                        = 0x05,
	EC_OP_STRINGS                       = 0x06,
	EC_OP_MISC_DATA                     = 0x07,
	EC_OP_SHUTDOWN                      = 0x08,
	EC_OP_ADD_LINK                      = 0x09,
	EC_OP_STAT_REQ                      = 0x0A,
	EC_OP_GET_CONNSTATE                 = 0x0B,
	EC_OP_STATS                         = 0x0C,
	EC_OP_GET_DLOAD_QUEUE               = 0x0D,
	EC_OP_GET_ULOAD_QUEUE               = 0x0E,
	EC_OP_GET_SHARED_FILES              = 0x10,
	EC_OP_SHARED_SET_PRIO               = 0x11,
	EC_OP_PARTFILE_REMOVE_NO_NEEDED     = 0x12,
	EC_OP_PARTFILE_REMOVE_FULL_QUEUE    = 0x13,
	EC_OP_PARTFILE_REMOVE_HIGH_QUEUE    = 0x14,
	EC_OP_PARTFILE_UNUSED               = 0x15,
	EC_OP_PARTFILE_SWAP_A4AF_THIS       = 0x16,
	EC_OP_PARTFILE_SWAP_A4AF_THIS_AUTO  = 0x17,
	EC_OP_PARTFILE_SWAP_A4AF_OTHERS     = 0x18,
	EC_OP_PARTFILE_PAUSE                = 0x19,
	EC_OP_PARTFILE_RESUME               = 0x1A,
	EC_OP_PARTFILE_STOP                 = 0x1B,
	EC_OP_PARTFILE_PRIO_SET             = 0x1C,
	EC_OP_PARTFILE_DELETE               = 0x1D,
	EC_OP_PARTFILE_SET_CAT              = 0x1E,
	EC_OP_DLOAD_QUEUE                   = 0x1F,
	EC_OP_ULOAD_QUEUE                   = 0x20,
	EC_OP_SHARED_FILES                  = 0x22,
	EC_OP_SHAREDFILES_RELOAD            = 0x23,
	EC_OP_SHAREDFILES_ADD_DIRECTORY     = 0x24,
	EC_OP_RENAME_FILE                   = 0x25,
	EC_OP_SEARCH_START                  = 0x26,
	EC_OP_SEARCH_STOP                   = 0x27,
	EC_OP_SEARCH_RESULTS                = 0x28,
	EC_OP_SEARCH_PROGRESS               = 0x29,
	EC_OP_DOWNLOAD_SEARCH_RESULT        = 0x2A,
	EC_OP_IPFILTER_RELOAD               = 0x2B,
	EC_OP_GET_SERVER_LIST               = 0x2C,
	EC_OP_SERVER_LIST                   = 0x2D,
	EC_OP_SERVER_DISCONNECT             = 0x2E,
	EC_OP_SERVER_CONNECT                = 0x2F,
	EC_OP_SERVER_REMOVE                 = 0x30,
	EC_OP_SERVER_ADD                    = 0x31,
	EC_OP_SERVER_UPDATE_FROM_URL        = 0x32,
	EC_OP_ADDLOGLINE                    = 0x33,
	EC_OP_ADDDEBUGLOGLINE               = 0x34,
	EC_OP_GET_LOG                       = 0x35,
	EC_OP_GET_DEBUGLOG                  = 0x36,
	EC_OP_GET_SERVERINFO                = 0x37,
	EC_OP_LOG                           = 0x38,
	EC_OP_DEBUGLOG                      = 0x39,
	EC_OP_SERVERINFO                    = 0x3A,
	EC_OP_RESET_LOG                     = 0x3B,
	EC_OP_RESET_DEBUGLOG                = 0x3C,
	EC_OP_CLEAR_SERVERINFO              = 0x3D,
	EC_OP_GET_LAST_LOG_ENTRY            = 0x3E,
	EC_OP_GET_PREFERENCES               = 0x3F,
	EC_OP_SET_PREFERENCES               = 0x40,
	EC_OP_CREATE_CATEGORY               = 0x41,
	EC_OP_UPDATE_CATEGORY               = 0x42,
	EC_OP_DELETE_CATEGORY               = 0x43,
	EC_OP_GET_STATSGRAPHS               = 0x44,
	EC_OP_STATSGRAPHS                   = 0x45,
	EC_OP_GET_STATSTREE                 = 0x46,
	EC_OP_STATSTREE                     = 0x47,
	EC_OP_KAD_START                     = 0x48,
	EC_OP_KAD_STOP                      = 0x49,
	EC_OP_CONNECT                       = 0x4A,
	EC_OP_DISCONNECT                    = 0x4B,
	EC_OP_KAD_UPDATE_FROM_URL           = 0x4D,
	EC_OP_KAD_BOOTSTRAP_FROM_IP         = 0x4E,
	EC_OP_AUTH_SALT                     = 0x4F,
	EC_OP_AUTH_PASSWD                   = 0x50,
	EC_OP_IPFILTER_UPDATE               = 0x51,
	EC_OP_GET_UPDATE                    = 0x52,
	EC_OP_CLEAR_COMPLETED               = 0x53,
	EC_OP_CLIENT_SWAP_TO_ANOTHER_FILE   = 0x54,
	EC_OP_SHARED_FILE_SET_COMMENT       = 0x55,
	EC_OP_SERVER_SET_STATIC_PRIO        = 0x56,
	EC_OP_FRIEND                        = 0x57
};

enum ECTagNames {
	EC_TAG_STRING                             = 0x0000,
	EC_TAG_PASSWD_HASH                        = 0x0001,
	EC_TAG_PROTOCOL_VERSION                   = 0x0002,
	EC_TAG_VERSION_ID                         = 0x0003,
	EC_TAG_DETAIL_LEVEL                       = 0x0004,
	EC_TAG_CONNSTATE                          = 0x0005,
	EC_TAG_ED2K_ID                            = 0x0006,
	EC_TAG_LOG_TO_STATUS                      = 0x0007,
	EC_TAG_BOOTSTRAP_IP                       = 0x0008,
	EC_TAG_BOOTSTRAP_PORT                     = 0x0009,
	EC_TAG_CLIENT_ID                          = 0x000A,
	EC_TAG_PASSWD_SALT                        = 0x000B,
	EC_TAG_CAN_ZLIB                           = 0x000C,
	EC_TAG_CAN_UTF8_NUMBERS                   = 0x000D,
	EC_TAG_CAN_NOTIFY                         = 0x000E,
	EC_TAG_ECID                               = 0x000F,
	EC_TAG_CLIENT_NAME                        = 0x0100,
		EC_TAG_CLIENT_VERSION                     = 0x0101,
		EC_TAG_CLIENT_MOD                         = 0x0102,
	EC_TAG_STATS_UL_SPEED                     = 0x0200,
		EC_TAG_STATS_DL_SPEED                     = 0x0201,
		EC_TAG_STATS_UL_SPEED_LIMIT               = 0x0202,
		EC_TAG_STATS_DL_SPEED_LIMIT               = 0x0203,
		EC_TAG_STATS_UP_OVERHEAD                  = 0x0204,
		EC_TAG_STATS_DOWN_OVERHEAD                = 0x0205,
		EC_TAG_STATS_TOTAL_SRC_COUNT              = 0x0206,
		EC_TAG_STATS_BANNED_COUNT                 = 0x0207,
		EC_TAG_STATS_UL_QUEUE_LEN                 = 0x0208,
		EC_TAG_STATS_ED2K_USERS                   = 0x0209,
		EC_TAG_STATS_KAD_USERS                    = 0x020A,
		EC_TAG_STATS_ED2K_FILES                   = 0x020B,
		EC_TAG_STATS_KAD_FILES                    = 0x020C,
		EC_TAG_STATS_LOGGER_MESSAGE               = 0x020D,
		EC_TAG_STATS_KAD_FIREWALLED_UDP           = 0x020E,
		EC_TAG_STATS_KAD_INDEXED_SOURCES          = 0x020F,
		EC_TAG_STATS_KAD_INDEXED_KEYWORDS         = 0x0210,
		EC_TAG_STATS_KAD_INDEXED_NOTES            = 0x0211,
		EC_TAG_STATS_KAD_INDEXED_LOAD             = 0x0212,
		EC_TAG_STATS_KAD_IP_ADRESS                = 0x0213,
		EC_TAG_STATS_BUDDY_STATUS                 = 0x0214,
		EC_TAG_STATS_BUDDY_IP                     = 0x0215,
		EC_TAG_STATS_BUDDY_PORT                   = 0x0216,
		EC_TAG_STATS_KAD_IN_LAN_MODE              = 0x0217,
	EC_TAG_PARTFILE                           = 0x0300,
		EC_TAG_PARTFILE_NAME                      = 0x0301,
		EC_TAG_PARTFILE_PARTMETID                 = 0x0302,
		EC_TAG_PARTFILE_SIZE_FULL                 = 0x0303,
		EC_TAG_PARTFILE_SIZE_XFER                 = 0x0304,
		EC_TAG_PARTFILE_SIZE_XFER_UP              = 0x0305,
		EC_TAG_PARTFILE_SIZE_DONE                 = 0x0306,
		EC_TAG_PARTFILE_SPEED                     = 0x0307,
		EC_TAG_PARTFILE_STATUS                    = 0x0308,
		EC_TAG_PARTFILE_PRIO                      = 0x0309,
		EC_TAG_PARTFILE_SOURCE_COUNT              = 0x030A,
		EC_TAG_PARTFILE_SOURCE_COUNT_A4AF         = 0x030B,
		EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT  = 0x030C,
		EC_TAG_PARTFILE_SOURCE_COUNT_XFER         = 0x030D,
		EC_TAG_PARTFILE_ED2K_LINK                 = 0x030E,
		EC_TAG_PARTFILE_CAT                       = 0x030F,
		EC_TAG_PARTFILE_LAST_RECV                 = 0x0310,
		EC_TAG_PARTFILE_LAST_SEEN_COMP            = 0x0311,
		EC_TAG_PARTFILE_PART_STATUS               = 0x0312,
		EC_TAG_PARTFILE_GAP_STATUS                = 0x0313,
		EC_TAG_PARTFILE_REQ_STATUS                = 0x0314,
		EC_TAG_PARTFILE_SOURCE_NAMES              = 0x0315,
		EC_TAG_PARTFILE_COMMENTS                  = 0x0316,
		EC_TAG_PARTFILE_STOPPED                   = 0x0317,
		EC_TAG_PARTFILE_DOWNLOAD_ACTIVE           = 0x0318,
		EC_TAG_PARTFILE_LOST_CORRUPTION           = 0x0319,
		EC_TAG_PARTFILE_GAINED_COMPRESSION        = 0x031A,
		EC_TAG_PARTFILE_SAVED_ICH                 = 0x031B,
		EC_TAG_PARTFILE_SOURCE_NAMES_COUNTS       = 0x031C,
		EC_TAG_PARTFILE_AVAILABLE_PARTS           = 0x031D,
		EC_TAG_PARTFILE_HASH                      = 0x031E,
		EC_TAG_PARTFILE_SHARED                    = 0x031F,
		EC_TAG_PARTFILE_HASHED_PART_COUNT         = 0x0320,
		EC_TAG_PARTFILE_A4AFAUTO                  = 0x0321,
		EC_TAG_PARTFILE_A4AF_SOURCES              = 0x0322,
	EC_TAG_KNOWNFILE                          = 0x0400,
		EC_TAG_KNOWNFILE_XFERRED                  = 0x0401,
		EC_TAG_KNOWNFILE_XFERRED_ALL              = 0x0402,
		EC_TAG_KNOWNFILE_REQ_COUNT                = 0x0403,
		EC_TAG_KNOWNFILE_REQ_COUNT_ALL            = 0x0404,
		EC_TAG_KNOWNFILE_ACCEPT_COUNT             = 0x0405,
		EC_TAG_KNOWNFILE_ACCEPT_COUNT_ALL         = 0x0406,
		EC_TAG_KNOWNFILE_AICH_MASTERHASH          = 0x0407,
		EC_TAG_KNOWNFILE_FILENAME                 = 0x0408,
		EC_TAG_KNOWNFILE_COMPLETE_SOURCES_LOW     = 0x0409,
		EC_TAG_KNOWNFILE_COMPLETE_SOURCES_HIGH    = 0x040A,
		EC_TAG_KNOWNFILE_PRIO                     = 0x040B,
		EC_TAG_KNOWNFILE_ON_QUEUE                 = 0x040C,
		EC_TAG_KNOWNFILE_COMPLETE_SOURCES         = 0x040D,
		EC_TAG_KNOWNFILE_COMMENT                  = 0x040E,
		EC_TAG_KNOWNFILE_RATING                   = 0x040F,
	EC_TAG_SERVER                             = 0x0500,
		EC_TAG_SERVER_NAME                        = 0x0501,
		EC_TAG_SERVER_DESC                        = 0x0502,
		EC_TAG_SERVER_ADDRESS                     = 0x0503,
		EC_TAG_SERVER_PING                        = 0x0504,
		EC_TAG_SERVER_USERS                       = 0x0505,
		EC_TAG_SERVER_USERS_MAX                   = 0x0506,
		EC_TAG_SERVER_FILES                       = 0x0507,
		EC_TAG_SERVER_PRIO                        = 0x0508,
		EC_TAG_SERVER_FAILED                      = 0x0509,
		EC_TAG_SERVER_STATIC                      = 0x050A,
		EC_TAG_SERVER_VERSION                     = 0x050B,
		EC_TAG_SERVER_IP                          = 0x050C,
		EC_TAG_SERVER_PORT                        = 0x050D,
	EC_TAG_CLIENT                             = 0x0600,
		EC_TAG_CLIENT_SOFTWARE                    = 0x0601,
		EC_TAG_CLIENT_SCORE                       = 0x0602,
		EC_TAG_CLIENT_HASH                        = 0x0603,
		EC_TAG_CLIENT_FRIEND_SLOT                 = 0x0604,
		EC_TAG_CLIENT_WAIT_TIME                   = 0x0605,
		EC_TAG_CLIENT_XFER_TIME                   = 0x0606,
		EC_TAG_CLIENT_QUEUE_TIME                  = 0x0607,
		EC_TAG_CLIENT_LAST_TIME                   = 0x0608,
		EC_TAG_CLIENT_UPLOAD_SESSION              = 0x0609,
		EC_TAG_CLIENT_UPLOAD_TOTAL                = 0x060A,
		EC_TAG_CLIENT_DOWNLOAD_TOTAL              = 0x060B,
		EC_TAG_CLIENT_DOWNLOAD_STATE              = 0x060C,
		EC_TAG_CLIENT_UP_SPEED                    = 0x060D,
		EC_TAG_CLIENT_DOWN_SPEED                  = 0x060E,
		EC_TAG_CLIENT_FROM                        = 0x060F,
		EC_TAG_CLIENT_USER_IP                     = 0x0610,
		EC_TAG_CLIENT_USER_PORT                   = 0x0611,
		EC_TAG_CLIENT_SERVER_IP                   = 0x0612,
		EC_TAG_CLIENT_SERVER_PORT                 = 0x0613,
		EC_TAG_CLIENT_SERVER_NAME                 = 0x0614,
		EC_TAG_CLIENT_SOFT_VER_STR                = 0x0615,
		EC_TAG_CLIENT_WAITING_POSITION            = 0x0616,
		EC_TAG_CLIENT_IDENT_STATE                 = 0x0617,
		EC_TAG_CLIENT_OBFUSCATION_STATUS          = 0x0618,
		EC_TAG_CLIENT_CURRENTLYUNUSED1            = 0x0619,
		EC_TAG_CLIENT_REMOTE_QUEUE_RANK           = 0x061A,
		EC_TAG_CLIENT_DISABLE_VIEW_SHARED         = 0x061B,
		EC_TAG_CLIENT_UPLOAD_STATE                = 0x061C,
		EC_TAG_CLIENT_EXT_PROTOCOL                = 0x061D,
		EC_TAG_CLIENT_USER_ID                     = 0x061E,
		EC_TAG_CLIENT_UPLOAD_FILE                 = 0x061F,
		EC_TAG_CLIENT_REQUEST_FILE                = 0x0620,
		EC_TAG_CLIENT_A4AF_FILES                  = 0x0621,
		EC_TAG_CLIENT_OLD_REMOTE_QUEUE_RANK       = 0x0622,
		EC_TAG_CLIENT_KAD_PORT                    = 0x0623,
		EC_TAG_CLIENT_PART_STATUS                 = 0x0624,
		EC_TAG_CLIENT_NEXT_REQUESTED_PART         = 0x0625,
		EC_TAG_CLIENT_LAST_DOWNLOADING_PART       = 0x0626,
		EC_TAG_CLIENT_REMOTE_FILENAME             = 0x0627,
		EC_TAG_CLIENT_MOD_VERSION                 = 0x0628,
		EC_TAG_CLIENT_OS_INFO                     = 0x0629,
		EC_TAG_CLIENT_AVAILABLE_PARTS             = 0x062A,
		EC_TAG_CLIENT_UPLOAD_PART_STATUS          = 0x062B,
	EC_TAG_SEARCHFILE                         = 0x0700,
		EC_TAG_SEARCH_TYPE                        = 0x0701,
		EC_TAG_SEARCH_NAME                        = 0x0702,
		EC_TAG_SEARCH_MIN_SIZE                    = 0x0703,
		EC_TAG_SEARCH_MAX_SIZE                    = 0x0704,
		EC_TAG_SEARCH_FILE_TYPE                   = 0x0705,
		EC_TAG_SEARCH_EXTENSION                   = 0x0706,
		EC_TAG_SEARCH_AVAILABILITY                = 0x0707,
		EC_TAG_SEARCH_STATUS                      = 0x0708,
		EC_TAG_SEARCH_PARENT                      = 0x0709,
	EC_TAG_FRIEND                             = 0x0800,
		EC_TAG_FRIEND_NAME                        = 0x0801,
		EC_TAG_FRIEND_HASH                        = 0x0802,
		EC_TAG_FRIEND_IP                          = 0x0803,
		EC_TAG_FRIEND_PORT                        = 0x0804,
		EC_TAG_FRIEND_CLIENT                      = 0x0805,
		EC_TAG_FRIEND_ADD                         = 0x0806,
		EC_TAG_FRIEND_REMOVE                      = 0x0807,
		EC_TAG_FRIEND_FRIENDSLOT                  = 0x0808,
		EC_TAG_FRIEND_SHARED                      = 0x0809,
	EC_TAG_SELECT_PREFS                       = 0x1000,
		EC_TAG_PREFS_CATEGORIES                   = 0x1100,
			EC_TAG_CATEGORY                           = 0x1101,
			EC_TAG_CATEGORY_TITLE                     = 0x1102,
			EC_TAG_CATEGORY_PATH                      = 0x1103,
			EC_TAG_CATEGORY_COMMENT                   = 0x1104,
			EC_TAG_CATEGORY_COLOR                     = 0x1105,
			EC_TAG_CATEGORY_PRIO                      = 0x1106,
		EC_TAG_PREFS_GENERAL                      = 0x1200,
			EC_TAG_USER_NICK                          = 0x1201,
			EC_TAG_USER_HASH                          = 0x1202,
			EC_TAG_USER_HOST                          = 0x1203,
			EC_TAG_GENERAL_CHECK_NEW_VERSION          = 0x1204,
		EC_TAG_PREFS_CONNECTIONS                  = 0x1300,
			EC_TAG_CONN_DL_CAP                        = 0x1301,
			EC_TAG_CONN_UL_CAP                        = 0x1302,
			EC_TAG_CONN_MAX_DL                        = 0x1303,
			EC_TAG_CONN_MAX_UL                        = 0x1304,
			EC_TAG_CONN_SLOT_ALLOCATION               = 0x1305,
			EC_TAG_CONN_TCP_PORT                      = 0x1306,
			EC_TAG_CONN_UDP_PORT	                  = 0x1307,
			EC_TAG_CONN_UDP_DISABLE                   = 0x1308,
			EC_TAG_CONN_MAX_FILE_SOURCES              = 0x1309,
			EC_TAG_CONN_MAX_CONN                      = 0x130A,
			EC_TAG_CONN_AUTOCONNECT                   = 0x130B,
			EC_TAG_CONN_RECONNECT                     = 0x130C,
			EC_TAG_NETWORK_ED2K                       = 0x130D,
			EC_TAG_NETWORK_KADEMLIA                   = 0x130E,
		EC_TAG_PREFS_MESSAGEFILTER                = 0x1400,
			EC_TAG_MSGFILTER_ENABLED                  = 0x1401,
			EC_TAG_MSGFILTER_ALL                      = 0x1402,
			EC_TAG_MSGFILTER_FRIENDS                  = 0x1403,
			EC_TAG_MSGFILTER_SECURE                   = 0x1404,
			EC_TAG_MSGFILTER_BY_KEYWORD               = 0x1405,
			EC_TAG_MSGFILTER_KEYWORDS                 = 0x1406,
		EC_TAG_PREFS_REMOTECTRL                   = 0x1500,
			EC_TAG_WEBSERVER_AUTORUN                  = 0x1501,
			EC_TAG_WEBSERVER_PORT                     = 0x1502,
			EC_TAG_WEBSERVER_GUEST                    = 0x1503,
			EC_TAG_WEBSERVER_USEGZIP                  = 0x1504,
			EC_TAG_WEBSERVER_REFRESH                  = 0x1505,
			EC_TAG_WEBSERVER_TEMPLATE                 = 0x1506,
		EC_TAG_PREFS_ONLINESIG                    = 0x1600,
			EC_TAG_ONLINESIG_ENABLED                  = 0x1601,
		EC_TAG_PREFS_SERVERS                      = 0x1700,
			EC_TAG_SERVERS_REMOVE_DEAD                = 0x1701,
			EC_TAG_SERVERS_DEAD_SERVER_RETRIES        = 0x1702,
			EC_TAG_SERVERS_AUTO_UPDATE                = 0x1703,
			EC_TAG_SERVERS_URL_LIST                   = 0x1704,
			EC_TAG_SERVERS_ADD_FROM_SERVER            = 0x1705,
			EC_TAG_SERVERS_ADD_FROM_CLIENT            = 0x1706,
			EC_TAG_SERVERS_USE_SCORE_SYSTEM           = 0x1707,
			EC_TAG_SERVERS_SMART_ID_CHECK             = 0x1708,
			EC_TAG_SERVERS_SAFE_SERVER_CONNECT        = 0x1709,
			EC_TAG_SERVERS_AUTOCONN_STATIC_ONLY       = 0x170A,
			EC_TAG_SERVERS_MANUAL_HIGH_PRIO           = 0x170B,
			EC_TAG_SERVERS_UPDATE_URL                 = 0x170C,
		EC_TAG_PREFS_FILES                        = 0x1800,
			EC_TAG_FILES_ICH_ENABLED                  = 0x1801,
			EC_TAG_FILES_AICH_TRUST                   = 0x1802,
			EC_TAG_FILES_NEW_PAUSED                   = 0x1803,
			EC_TAG_FILES_NEW_AUTO_DL_PRIO             = 0x1804,
			EC_TAG_FILES_PREVIEW_PRIO                 = 0x1805,
			EC_TAG_FILES_NEW_AUTO_UL_PRIO             = 0x1806,
			EC_TAG_FILES_UL_FULL_CHUNKS               = 0x1807,
			EC_TAG_FILES_START_NEXT_PAUSED            = 0x1808,
			EC_TAG_FILES_RESUME_SAME_CAT              = 0x1809,
			EC_TAG_FILES_SAVE_SOURCES                 = 0x180A,
			EC_TAG_FILES_EXTRACT_METADATA             = 0x180B,
			EC_TAG_FILES_ALLOC_FULL_SIZE              = 0x180C,
			EC_TAG_FILES_CHECK_FREE_SPACE             = 0x180D,
			EC_TAG_FILES_MIN_FREE_SPACE               = 0x180E,
		EC_TAG_PREFS_SRCDROP                      = 0x1900,
			EC_TAG_SRCDROP_NONEEDED                   = 0x1901,
			EC_TAG_SRCDROP_DROP_FQS                   = 0x1902,
			EC_TAG_SRCDROP_DROP_HQRS                  = 0x1903,
			EC_TAG_SRCDROP_HQRS_VALUE                 = 0x1904,
			EC_TAG_SRCDROP_AUTODROP_TIMER             = 0x1905,
		EC_TAG_PREFS_DIRECTORIES                  = 0x1A00,
			EC_TAG_DIRECTORIES_INCOMING               = 0x1A01,
			EC_TAG_DIRECTORIES_TEMP                   = 0x1A02,
			EC_TAG_DIRECTORIES_SHARED                 = 0x1A03,
			EC_TAG_DIRECTORIES_SHARE_HIDDEN           = 0x1A04,
		EC_TAG_PREFS_STATISTICS                   = 0x1B00,
			EC_TAG_STATSGRAPH_WIDTH                   = 0x1B01,
			EC_TAG_STATSGRAPH_SCALE                   = 0x1B02,
			EC_TAG_STATSGRAPH_LAST                    = 0x1B03,
			EC_TAG_STATSGRAPH_DATA                    = 0x1B04,
			EC_TAG_STATTREE_CAPPING                   = 0x1B05,
			EC_TAG_STATTREE_NODE                      = 0x1B06,
			EC_TAG_STAT_NODE_VALUE                    = 0x1B07,
			EC_TAG_STAT_VALUE_TYPE                    = 0x1B08,
			EC_TAG_STATTREE_NODEID                    = 0x1B09,
		EC_TAG_PREFS_SECURITY                     = 0x1C00,
			EC_TAG_SECURITY_CAN_SEE_SHARES            = 0x1C01,
			EC_TAG_IPFILTER_CLIENTS                   = 0x1C02,
			EC_TAG_IPFILTER_SERVERS                   = 0x1C03,
			EC_TAG_IPFILTER_AUTO_UPDATE               = 0x1C04,
			EC_TAG_IPFILTER_UPDATE_URL                = 0x1C05,
			EC_TAG_IPFILTER_LEVEL                     = 0x1C06,
			EC_TAG_IPFILTER_FILTER_LAN                = 0x1C07,
			EC_TAG_SECURITY_USE_SECIDENT              = 0x1C08,
			EC_TAG_SECURITY_OBFUSCATION_SUPPORTED     = 0x1C09,
			EC_TAG_SECURITY_OBFUSCATION_REQUESTED     = 0x1C0A,
			EC_TAG_SECURITY_OBFUSCATION_REQUIRED      = 0x1C0B,
		EC_TAG_PREFS_CORETWEAKS                   = 0x1D00,
			EC_TAG_CORETW_MAX_CONN_PER_FIVE           = 0x1D01,
			EC_TAG_CORETW_VERBOSE                     = 0x1D02,
			EC_TAG_CORETW_FILEBUFFER                  = 0x1D03,
			EC_TAG_CORETW_UL_QUEUE                    = 0x1D04,
			EC_TAG_CORETW_SRV_KEEPALIVE_TIMEOUT       = 0x1D05,
		EC_TAG_PREFS_KADEMLIA                     = 0x1E00,
			EC_TAG_KADEMLIA_UPDATE_URL                = 0x1E01
};

enum EC_DETAIL_LEVEL {
	EC_DETAIL_CMD           = 0x00,
	EC_DETAIL_WEB           = 0x01,
	EC_DETAIL_FULL          = 0x02,
	EC_DETAIL_UPDATE        = 0x03,
	EC_DETAIL_INC_UPDATE    = 0x04
};

enum EC_SEARCH_TYPE {
	EC_SEARCH_LOCAL         = 0x00,
	EC_SEARCH_GLOBAL        = 0x01,
	EC_SEARCH_KAD           = 0x02,
	EC_SEARCH_WEB           = 0x03
};

enum EC_STATTREE_NODE_VALUE_TYPE {
	EC_VALUE_INTEGER        = 0x00,
	EC_VALUE_ISTRING        = 0x01,
	EC_VALUE_BYTES          = 0x02,
	EC_VALUE_ISHORT         = 0x03,
	EC_VALUE_TIME           = 0x04,
	EC_VALUE_SPEED          = 0x05,
	EC_VALUE_STRING         = 0x06,
	EC_VALUE_DOUBLE         = 0x07
};

enum EcPrefs {
	EC_PREFS_CATEGORIES     = 0x00000001,
	EC_PREFS_GENERAL        = 0x00000002,
	EC_PREFS_CONNECTIONS    = 0x00000004,
	EC_PREFS_MESSAGEFILTER  = 0x00000008,
	EC_PREFS_REMOTECONTROLS = 0x00000010,
	EC_PREFS_ONLINESIG      = 0x00000020,
	EC_PREFS_SERVERS        = 0x00000040,
	EC_PREFS_FILES          = 0x00000080,
	EC_PREFS_SRCDROP        = 0x00000100,
	EC_PREFS_DIRECTORIES    = 0x00000200,
	EC_PREFS_STATISTICS     = 0x00000400,
	EC_PREFS_SECURITY       = 0x00000800,
	EC_PREFS_CORETWEAKS     = 0x00001000,
	EC_PREFS_KADEMLIA       = 0x00002000
};

#ifdef DEBUG_EC_IMPLEMENTATION

wxString GetDebugNameProtocolVersion(uint16 arg)
{
	switch (arg) {
		case 0x0203: return wxT("EC_CURRENT_PROTOCOL_VERSION");
		default: return CFormat(wxT("unknown %d 0x%x")) % arg % arg;
	}
}

wxString GetDebugNameECFlags(uint32 arg)
{
	switch (arg) {
		case 0x00000001: return wxT("EC_FLAG_ZLIB");
		case 0x00000002: return wxT("EC_FLAG_UTF8_NUMBERS");
		case 0xff7f7f08: return wxT("EC_FLAG_UNKNOWN_MASK");
		default: return CFormat(wxT("unknown %d 0x%x")) % arg % arg;
	}
}

wxString GetDebugNameECOpCodes(uint8 arg)
{
	switch (arg) {
		case 0x01: return wxT("EC_OP_NOOP");
		case 0x02: return wxT("EC_OP_AUTH_REQ");
		case 0x03: return wxT("EC_OP_AUTH_FAIL");
		case 0x04: return wxT("EC_OP_AUTH_OK");
		case 0x05: return wxT("EC_OP_FAILED");
		case 0x06: return wxT("EC_OP_STRINGS");
		case 0x07: return wxT("EC_OP_MISC_DATA");
		case 0x08: return wxT("EC_OP_SHUTDOWN");
		case 0x09: return wxT("EC_OP_ADD_LINK");
		case 0x0A: return wxT("EC_OP_STAT_REQ");
		case 0x0B: return wxT("EC_OP_GET_CONNSTATE");
		case 0x0C: return wxT("EC_OP_STATS");
		case 0x0D: return wxT("EC_OP_GET_DLOAD_QUEUE");
		case 0x0E: return wxT("EC_OP_GET_ULOAD_QUEUE");
		case 0x10: return wxT("EC_OP_GET_SHARED_FILES");
		case 0x11: return wxT("EC_OP_SHARED_SET_PRIO");
		case 0x12: return wxT("EC_OP_PARTFILE_REMOVE_NO_NEEDED");
		case 0x13: return wxT("EC_OP_PARTFILE_REMOVE_FULL_QUEUE");
		case 0x14: return wxT("EC_OP_PARTFILE_REMOVE_HIGH_QUEUE");
		case 0x15: return wxT("EC_OP_PARTFILE_UNUSED");
		case 0x16: return wxT("EC_OP_PARTFILE_SWAP_A4AF_THIS");
		case 0x17: return wxT("EC_OP_PARTFILE_SWAP_A4AF_THIS_AUTO");
		case 0x18: return wxT("EC_OP_PARTFILE_SWAP_A4AF_OTHERS");
		case 0x19: return wxT("EC_OP_PARTFILE_PAUSE");
		case 0x1A: return wxT("EC_OP_PARTFILE_RESUME");
		case 0x1B: return wxT("EC_OP_PARTFILE_STOP");
		case 0x1C: return wxT("EC_OP_PARTFILE_PRIO_SET");
		case 0x1D: return wxT("EC_OP_PARTFILE_DELETE");
		case 0x1E: return wxT("EC_OP_PARTFILE_SET_CAT");
		case 0x1F: return wxT("EC_OP_DLOAD_QUEUE");
		case 0x20: return wxT("EC_OP_ULOAD_QUEUE");
		case 0x22: return wxT("EC_OP_SHARED_FILES");
		case 0x23: return wxT("EC_OP_SHAREDFILES_RELOAD");
		case 0x24: return wxT("EC_OP_SHAREDFILES_ADD_DIRECTORY");
		case 0x25: return wxT("EC_OP_RENAME_FILE");
		case 0x26: return wxT("EC_OP_SEARCH_START");
		case 0x27: return wxT("EC_OP_SEARCH_STOP");
		case 0x28: return wxT("EC_OP_SEARCH_RESULTS");
		case 0x29: return wxT("EC_OP_SEARCH_PROGRESS");
		case 0x2A: return wxT("EC_OP_DOWNLOAD_SEARCH_RESULT");
		case 0x2B: return wxT("EC_OP_IPFILTER_RELOAD");
		case 0x2C: return wxT("EC_OP_GET_SERVER_LIST");
		case 0x2D: return wxT("EC_OP_SERVER_LIST");
		case 0x2E: return wxT("EC_OP_SERVER_DISCONNECT");
		case 0x2F: return wxT("EC_OP_SERVER_CONNECT");
		case 0x30: return wxT("EC_OP_SERVER_REMOVE");
		case 0x31: return wxT("EC_OP_SERVER_ADD");
		case 0x32: return wxT("EC_OP_SERVER_UPDATE_FROM_URL");
		case 0x33: return wxT("EC_OP_ADDLOGLINE");
		case 0x34: return wxT("EC_OP_ADDDEBUGLOGLINE");
		case 0x35: return wxT("EC_OP_GET_LOG");
		case 0x36: return wxT("EC_OP_GET_DEBUGLOG");
		case 0x37: return wxT("EC_OP_GET_SERVERINFO");
		case 0x38: return wxT("EC_OP_LOG");
		case 0x39: return wxT("EC_OP_DEBUGLOG");
		case 0x3A: return wxT("EC_OP_SERVERINFO");
		case 0x3B: return wxT("EC_OP_RESET_LOG");
		case 0x3C: return wxT("EC_OP_RESET_DEBUGLOG");
		case 0x3D: return wxT("EC_OP_CLEAR_SERVERINFO");
		case 0x3E: return wxT("EC_OP_GET_LAST_LOG_ENTRY");
		case 0x3F: return wxT("EC_OP_GET_PREFERENCES");
		case 0x40: return wxT("EC_OP_SET_PREFERENCES");
		case 0x41: return wxT("EC_OP_CREATE_CATEGORY");
		case 0x42: return wxT("EC_OP_UPDATE_CATEGORY");
		case 0x43: return wxT("EC_OP_DELETE_CATEGORY");
		case 0x44: return wxT("EC_OP_GET_STATSGRAPHS");
		case 0x45: return wxT("EC_OP_STATSGRAPHS");
		case 0x46: return wxT("EC_OP_GET_STATSTREE");
		case 0x47: return wxT("EC_OP_STATSTREE");
		case 0x48: return wxT("EC_OP_KAD_START");
		case 0x49: return wxT("EC_OP_KAD_STOP");
		case 0x4A: return wxT("EC_OP_CONNECT");
		case 0x4B: return wxT("EC_OP_DISCONNECT");
		case 0x4D: return wxT("EC_OP_KAD_UPDATE_FROM_URL");
		case 0x4E: return wxT("EC_OP_KAD_BOOTSTRAP_FROM_IP");
		case 0x4F: return wxT("EC_OP_AUTH_SALT");
		case 0x50: return wxT("EC_OP_AUTH_PASSWD");
		case 0x51: return wxT("EC_OP_IPFILTER_UPDATE");
		case 0x52: return wxT("EC_OP_GET_UPDATE");
		case 0x53: return wxT("EC_OP_CLEAR_COMPLETED");
		case 0x54: return wxT("EC_OP_CLIENT_SWAP_TO_ANOTHER_FILE");
		case 0x55: return wxT("EC_OP_SHARED_FILE_SET_COMMENT");
		case 0x56: return wxT("EC_OP_SERVER_SET_STATIC_PRIO");
		case 0x57: return wxT("EC_OP_FRIEND");
		default: return CFormat(wxT("unknown %d 0x%x")) % arg % arg;
	}
}

wxString GetDebugNameECTagNames(uint16 arg)
{
	switch (arg) {
		case 0x0000: return wxT("EC_TAG_STRING");
		case 0x0001: return wxT("EC_TAG_PASSWD_HASH");
		case 0x0002: return wxT("EC_TAG_PROTOCOL_VERSION");
		case 0x0003: return wxT("EC_TAG_VERSION_ID");
		case 0x0004: return wxT("EC_TAG_DETAIL_LEVEL");
		case 0x0005: return wxT("EC_TAG_CONNSTATE");
		case 0x0006: return wxT("EC_TAG_ED2K_ID");
		case 0x0007: return wxT("EC_TAG_LOG_TO_STATUS");
		case 0x0008: return wxT("EC_TAG_BOOTSTRAP_IP");
		case 0x0009: return wxT("EC_TAG_BOOTSTRAP_PORT");
		case 0x000A: return wxT("EC_TAG_CLIENT_ID");
		case 0x000B: return wxT("EC_TAG_PASSWD_SALT");
		case 0x000C: return wxT("EC_TAG_CAN_ZLIB");
		case 0x000D: return wxT("EC_TAG_CAN_UTF8_NUMBERS");
		case 0x000E: return wxT("EC_TAG_CAN_NOTIFY");
		case 0x000F: return wxT("EC_TAG_ECID");
		case 0x0100: return wxT("EC_TAG_CLIENT_NAME");
		case 0x0101: return wxT("EC_TAG_CLIENT_VERSION");
		case 0x0102: return wxT("EC_TAG_CLIENT_MOD");
		case 0x0200: return wxT("EC_TAG_STATS_UL_SPEED");
		case 0x0201: return wxT("EC_TAG_STATS_DL_SPEED");
		case 0x0202: return wxT("EC_TAG_STATS_UL_SPEED_LIMIT");
		case 0x0203: return wxT("EC_TAG_STATS_DL_SPEED_LIMIT");
		case 0x0204: return wxT("EC_TAG_STATS_UP_OVERHEAD");
		case 0x0205: return wxT("EC_TAG_STATS_DOWN_OVERHEAD");
		case 0x0206: return wxT("EC_TAG_STATS_TOTAL_SRC_COUNT");
		case 0x0207: return wxT("EC_TAG_STATS_BANNED_COUNT");
		case 0x0208: return wxT("EC_TAG_STATS_UL_QUEUE_LEN");
		case 0x0209: return wxT("EC_TAG_STATS_ED2K_USERS");
		case 0x020A: return wxT("EC_TAG_STATS_KAD_USERS");
		case 0x020B: return wxT("EC_TAG_STATS_ED2K_FILES");
		case 0x020C: return wxT("EC_TAG_STATS_KAD_FILES");
		case 0x020D: return wxT("EC_TAG_STATS_LOGGER_MESSAGE");
		case 0x020E: return wxT("EC_TAG_STATS_KAD_FIREWALLED_UDP");
		case 0x020F: return wxT("EC_TAG_STATS_KAD_INDEXED_SOURCES");
		case 0x0210: return wxT("EC_TAG_STATS_KAD_INDEXED_KEYWORDS");
		case 0x0211: return wxT("EC_TAG_STATS_KAD_INDEXED_NOTES");
		case 0x0212: return wxT("EC_TAG_STATS_KAD_INDEXED_LOAD");
		case 0x0213: return wxT("EC_TAG_STATS_KAD_IP_ADRESS");
		case 0x0214: return wxT("EC_TAG_STATS_BUDDY_STATUS");
		case 0x0215: return wxT("EC_TAG_STATS_BUDDY_IP");
		case 0x0216: return wxT("EC_TAG_STATS_BUDDY_PORT");
		case 0x0217: return wxT("EC_TAG_STATS_KAD_IN_LAN_MODE");
		case 0x0300: return wxT("EC_TAG_PARTFILE");
		case 0x0301: return wxT("EC_TAG_PARTFILE_NAME");
		case 0x0302: return wxT("EC_TAG_PARTFILE_PARTMETID");
		case 0x0303: return wxT("EC_TAG_PARTFILE_SIZE_FULL");
		case 0x0304: return wxT("EC_TAG_PARTFILE_SIZE_XFER");
		case 0x0305: return wxT("EC_TAG_PARTFILE_SIZE_XFER_UP");
		case 0x0306: return wxT("EC_TAG_PARTFILE_SIZE_DONE");
		case 0x0307: return wxT("EC_TAG_PARTFILE_SPEED");
		case 0x0308: return wxT("EC_TAG_PARTFILE_STATUS");
		case 0x0309: return wxT("EC_TAG_PARTFILE_PRIO");
		case 0x030A: return wxT("EC_TAG_PARTFILE_SOURCE_COUNT");
		case 0x030B: return wxT("EC_TAG_PARTFILE_SOURCE_COUNT_A4AF");
		case 0x030C: return wxT("EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT");
		case 0x030D: return wxT("EC_TAG_PARTFILE_SOURCE_COUNT_XFER");
		case 0x030E: return wxT("EC_TAG_PARTFILE_ED2K_LINK");
		case 0x030F: return wxT("EC_TAG_PARTFILE_CAT");
		case 0x0310: return wxT("EC_TAG_PARTFILE_LAST_RECV");
		case 0x0311: return wxT("EC_TAG_PARTFILE_LAST_SEEN_COMP");
		case 0x0312: return wxT("EC_TAG_PARTFILE_PART_STATUS");
		case 0x0313: return wxT("EC_TAG_PARTFILE_GAP_STATUS");
		case 0x0314: return wxT("EC_TAG_PARTFILE_REQ_STATUS");
		case 0x0315: return wxT("EC_TAG_PARTFILE_SOURCE_NAMES");
		case 0x0316: return wxT("EC_TAG_PARTFILE_COMMENTS");
		case 0x0317: return wxT("EC_TAG_PARTFILE_STOPPED");
		case 0x0318: return wxT("EC_TAG_PARTFILE_DOWNLOAD_ACTIVE");
		case 0x0319: return wxT("EC_TAG_PARTFILE_LOST_CORRUPTION");
		case 0x031A: return wxT("EC_TAG_PARTFILE_GAINED_COMPRESSION");
		case 0x031B: return wxT("EC_TAG_PARTFILE_SAVED_ICH");
		case 0x031C: return wxT("EC_TAG_PARTFILE_SOURCE_NAMES_COUNTS");
		case 0x031D: return wxT("EC_TAG_PARTFILE_AVAILABLE_PARTS");
		case 0x031E: return wxT("EC_TAG_PARTFILE_HASH");
		case 0x031F: return wxT("EC_TAG_PARTFILE_SHARED");
		case 0x0320: return wxT("EC_TAG_PARTFILE_HASHED_PART_COUNT");
		case 0x0321: return wxT("EC_TAG_PARTFILE_A4AFAUTO");
		case 0x0322: return wxT("EC_TAG_PARTFILE_A4AF_SOURCES");
		case 0x0400: return wxT("EC_TAG_KNOWNFILE");
		case 0x0401: return wxT("EC_TAG_KNOWNFILE_XFERRED");
		case 0x0402: return wxT("EC_TAG_KNOWNFILE_XFERRED_ALL");
		case 0x0403: return wxT("EC_TAG_KNOWNFILE_REQ_COUNT");
		case 0x0404: return wxT("EC_TAG_KNOWNFILE_REQ_COUNT_ALL");
		case 0x0405: return wxT("EC_TAG_KNOWNFILE_ACCEPT_COUNT");
		case 0x0406: return wxT("EC_TAG_KNOWNFILE_ACCEPT_COUNT_ALL");
		case 0x0407: return wxT("EC_TAG_KNOWNFILE_AICH_MASTERHASH");
		case 0x0408: return wxT("EC_TAG_KNOWNFILE_FILENAME");
		case 0x0409: return wxT("EC_TAG_KNOWNFILE_COMPLETE_SOURCES_LOW");
		case 0x040A: return wxT("EC_TAG_KNOWNFILE_COMPLETE_SOURCES_HIGH");
		case 0x040B: return wxT("EC_TAG_KNOWNFILE_PRIO");
		case 0x040C: return wxT("EC_TAG_KNOWNFILE_ON_QUEUE");
		case 0x040D: return wxT("EC_TAG_KNOWNFILE_COMPLETE_SOURCES");
		case 0x040E: return wxT("EC_TAG_KNOWNFILE_COMMENT");
		case 0x040F: return wxT("EC_TAG_KNOWNFILE_RATING");
		case 0x0500: return wxT("EC_TAG_SERVER");
		case 0x0501: return wxT("EC_TAG_SERVER_NAME");
		case 0x0502: return wxT("EC_TAG_SERVER_DESC");
		case 0x0503: return wxT("EC_TAG_SERVER_ADDRESS");
		case 0x0504: return wxT("EC_TAG_SERVER_PING");
		case 0x0505: return wxT("EC_TAG_SERVER_USERS");
		case 0x0506: return wxT("EC_TAG_SERVER_USERS_MAX");
		case 0x0507: return wxT("EC_TAG_SERVER_FILES");
		case 0x0508: return wxT("EC_TAG_SERVER_PRIO");
		case 0x0509: return wxT("EC_TAG_SERVER_FAILED");
		case 0x050A: return wxT("EC_TAG_SERVER_STATIC");
		case 0x050B: return wxT("EC_TAG_SERVER_VERSION");
		case 0x050C: return wxT("EC_TAG_SERVER_IP");
		case 0x050D: return wxT("EC_TAG_SERVER_PORT");
		case 0x0600: return wxT("EC_TAG_CLIENT");
		case 0x0601: return wxT("EC_TAG_CLIENT_SOFTWARE");
		case 0x0602: return wxT("EC_TAG_CLIENT_SCORE");
		case 0x0603: return wxT("EC_TAG_CLIENT_HASH");
		case 0x0604: return wxT("EC_TAG_CLIENT_FRIEND_SLOT");
		case 0x0605: return wxT("EC_TAG_CLIENT_WAIT_TIME");
		case 0x0606: return wxT("EC_TAG_CLIENT_XFER_TIME");
		case 0x0607: return wxT("EC_TAG_CLIENT_QUEUE_TIME");
		case 0x0608: return wxT("EC_TAG_CLIENT_LAST_TIME");
		case 0x0609: return wxT("EC_TAG_CLIENT_UPLOAD_SESSION");
		case 0x060A: return wxT("EC_TAG_CLIENT_UPLOAD_TOTAL");
		case 0x060B: return wxT("EC_TAG_CLIENT_DOWNLOAD_TOTAL");
		case 0x060C: return wxT("EC_TAG_CLIENT_DOWNLOAD_STATE");
		case 0x060D: return wxT("EC_TAG_CLIENT_UP_SPEED");
		case 0x060E: return wxT("EC_TAG_CLIENT_DOWN_SPEED");
		case 0x060F: return wxT("EC_TAG_CLIENT_FROM");
		case 0x0610: return wxT("EC_TAG_CLIENT_USER_IP");
		case 0x0611: return wxT("EC_TAG_CLIENT_USER_PORT");
		case 0x0612: return wxT("EC_TAG_CLIENT_SERVER_IP");
		case 0x0613: return wxT("EC_TAG_CLIENT_SERVER_PORT");
		case 0x0614: return wxT("EC_TAG_CLIENT_SERVER_NAME");
		case 0x0615: return wxT("EC_TAG_CLIENT_SOFT_VER_STR");
		case 0x0616: return wxT("EC_TAG_CLIENT_WAITING_POSITION");
		case 0x0617: return wxT("EC_TAG_CLIENT_IDENT_STATE");
		case 0x0618: return wxT("EC_TAG_CLIENT_OBFUSCATION_STATUS");
		case 0x0619: return wxT("EC_TAG_CLIENT_CURRENTLYUNUSED1");
		case 0x061A: return wxT("EC_TAG_CLIENT_REMOTE_QUEUE_RANK");
		case 0x061B: return wxT("EC_TAG_CLIENT_DISABLE_VIEW_SHARED");
		case 0x061C: return wxT("EC_TAG_CLIENT_UPLOAD_STATE");
		case 0x061D: return wxT("EC_TAG_CLIENT_EXT_PROTOCOL");
		case 0x061E: return wxT("EC_TAG_CLIENT_USER_ID");
		case 0x061F: return wxT("EC_TAG_CLIENT_UPLOAD_FILE");
		case 0x0620: return wxT("EC_TAG_CLIENT_REQUEST_FILE");
		case 0x0621: return wxT("EC_TAG_CLIENT_A4AF_FILES");
		case 0x0622: return wxT("EC_TAG_CLIENT_OLD_REMOTE_QUEUE_RANK");
		case 0x0623: return wxT("EC_TAG_CLIENT_KAD_PORT");
		case 0x0624: return wxT("EC_TAG_CLIENT_PART_STATUS");
		case 0x0625: return wxT("EC_TAG_CLIENT_NEXT_REQUESTED_PART");
		case 0x0626: return wxT("EC_TAG_CLIENT_LAST_DOWNLOADING_PART");
		case 0x0627: return wxT("EC_TAG_CLIENT_REMOTE_FILENAME");
		case 0x0628: return wxT("EC_TAG_CLIENT_MOD_VERSION");
		case 0x0629: return wxT("EC_TAG_CLIENT_OS_INFO");
		case 0x062A: return wxT("EC_TAG_CLIENT_AVAILABLE_PARTS");
		case 0x062B: return wxT("EC_TAG_CLIENT_UPLOAD_PART_STATUS");
		case 0x0700: return wxT("EC_TAG_SEARCHFILE");
		case 0x0701: return wxT("EC_TAG_SEARCH_TYPE");
		case 0x0702: return wxT("EC_TAG_SEARCH_NAME");
		case 0x0703: return wxT("EC_TAG_SEARCH_MIN_SIZE");
		case 0x0704: return wxT("EC_TAG_SEARCH_MAX_SIZE");
		case 0x0705: return wxT("EC_TAG_SEARCH_FILE_TYPE");
		case 0x0706: return wxT("EC_TAG_SEARCH_EXTENSION");
		case 0x0707: return wxT("EC_TAG_SEARCH_AVAILABILITY");
		case 0x0708: return wxT("EC_TAG_SEARCH_STATUS");
		case 0x0709: return wxT("EC_TAG_SEARCH_PARENT");
		case 0x0800: return wxT("EC_TAG_FRIEND");
		case 0x0801: return wxT("EC_TAG_FRIEND_NAME");
		case 0x0802: return wxT("EC_TAG_FRIEND_HASH");
		case 0x0803: return wxT("EC_TAG_FRIEND_IP");
		case 0x0804: return wxT("EC_TAG_FRIEND_PORT");
		case 0x0805: return wxT("EC_TAG_FRIEND_CLIENT");
		case 0x0806: return wxT("EC_TAG_FRIEND_ADD");
		case 0x0807: return wxT("EC_TAG_FRIEND_REMOVE");
		case 0x0808: return wxT("EC_TAG_FRIEND_FRIENDSLOT");
		case 0x0809: return wxT("EC_TAG_FRIEND_SHARED");
		case 0x1000: return wxT("EC_TAG_SELECT_PREFS");
		case 0x1100: return wxT("EC_TAG_PREFS_CATEGORIES");
		case 0x1101: return wxT("EC_TAG_CATEGORY");
		case 0x1102: return wxT("EC_TAG_CATEGORY_TITLE");
		case 0x1103: return wxT("EC_TAG_CATEGORY_PATH");
		case 0x1104: return wxT("EC_TAG_CATEGORY_COMMENT");
		case 0x1105: return wxT("EC_TAG_CATEGORY_COLOR");
		case 0x1106: return wxT("EC_TAG_CATEGORY_PRIO");
		case 0x1200: return wxT("EC_TAG_PREFS_GENERAL");
		case 0x1201: return wxT("EC_TAG_USER_NICK");
		case 0x1202: return wxT("EC_TAG_USER_HASH");
		case 0x1203: return wxT("EC_TAG_USER_HOST");
		case 0x1204: return wxT("EC_TAG_GENERAL_CHECK_NEW_VERSION");
		case 0x1300: return wxT("EC_TAG_PREFS_CONNECTIONS");
		case 0x1301: return wxT("EC_TAG_CONN_DL_CAP");
		case 0x1302: return wxT("EC_TAG_CONN_UL_CAP");
		case 0x1303: return wxT("EC_TAG_CONN_MAX_DL");
		case 0x1304: return wxT("EC_TAG_CONN_MAX_UL");
		case 0x1305: return wxT("EC_TAG_CONN_SLOT_ALLOCATION");
		case 0x1306: return wxT("EC_TAG_CONN_TCP_PORT");
		case 0x1307: return wxT("EC_TAG_CONN_UDP_PORT");
		case 0x1308: return wxT("EC_TAG_CONN_UDP_DISABLE");
		case 0x1309: return wxT("EC_TAG_CONN_MAX_FILE_SOURCES");
		case 0x130A: return wxT("EC_TAG_CONN_MAX_CONN");
		case 0x130B: return wxT("EC_TAG_CONN_AUTOCONNECT");
		case 0x130C: return wxT("EC_TAG_CONN_RECONNECT");
		case 0x130D: return wxT("EC_TAG_NETWORK_ED2K");
		case 0x130E: return wxT("EC_TAG_NETWORK_KADEMLIA");
		case 0x1400: return wxT("EC_TAG_PREFS_MESSAGEFILTER");
		case 0x1401: return wxT("EC_TAG_MSGFILTER_ENABLED");
		case 0x1402: return wxT("EC_TAG_MSGFILTER_ALL");
		case 0x1403: return wxT("EC_TAG_MSGFILTER_FRIENDS");
		case 0x1404: return wxT("EC_TAG_MSGFILTER_SECURE");
		case 0x1405: return wxT("EC_TAG_MSGFILTER_BY_KEYWORD");
		case 0x1406: return wxT("EC_TAG_MSGFILTER_KEYWORDS");
		case 0x1500: return wxT("EC_TAG_PREFS_REMOTECTRL");
		case 0x1501: return wxT("EC_TAG_WEBSERVER_AUTORUN");
		case 0x1502: return wxT("EC_TAG_WEBSERVER_PORT");
		case 0x1503: return wxT("EC_TAG_WEBSERVER_GUEST");
		case 0x1504: return wxT("EC_TAG_WEBSERVER_USEGZIP");
		case 0x1505: return wxT("EC_TAG_WEBSERVER_REFRESH");
		case 0x1506: return wxT("EC_TAG_WEBSERVER_TEMPLATE");
		case 0x1600: return wxT("EC_TAG_PREFS_ONLINESIG");
		case 0x1601: return wxT("EC_TAG_ONLINESIG_ENABLED");
		case 0x1700: return wxT("EC_TAG_PREFS_SERVERS");
		case 0x1701: return wxT("EC_TAG_SERVERS_REMOVE_DEAD");
		case 0x1702: return wxT("EC_TAG_SERVERS_DEAD_SERVER_RETRIES");
		case 0x1703: return wxT("EC_TAG_SERVERS_AUTO_UPDATE");
		case 0x1704: return wxT("EC_TAG_SERVERS_URL_LIST");
		case 0x1705: return wxT("EC_TAG_SERVERS_ADD_FROM_SERVER");
		case 0x1706: return wxT("EC_TAG_SERVERS_ADD_FROM_CLIENT");
		case 0x1707: return wxT("EC_TAG_SERVERS_USE_SCORE_SYSTEM");
		case 0x1708: return wxT("EC_TAG_SERVERS_SMART_ID_CHECK");
		case 0x1709: return wxT("EC_TAG_SERVERS_SAFE_SERVER_CONNECT");
		case 0x170A: return wxT("EC_TAG_SERVERS_AUTOCONN_STATIC_ONLY");
		case 0x170B: return wxT("EC_TAG_SERVERS_MANUAL_HIGH_PRIO");
		case 0x170C: return wxT("EC_TAG_SERVERS_UPDATE_URL");
		case 0x1800: return wxT("EC_TAG_PREFS_FILES");
		case 0x1801: return wxT("EC_TAG_FILES_ICH_ENABLED");
		case 0x1802: return wxT("EC_TAG_FILES_AICH_TRUST");
		case 0x1803: return wxT("EC_TAG_FILES_NEW_PAUSED");
		case 0x1804: return wxT("EC_TAG_FILES_NEW_AUTO_DL_PRIO");
		case 0x1805: return wxT("EC_TAG_FILES_PREVIEW_PRIO");
		case 0x1806: return wxT("EC_TAG_FILES_NEW_AUTO_UL_PRIO");
		case 0x1807: return wxT("EC_TAG_FILES_UL_FULL_CHUNKS");
		case 0x1808: return wxT("EC_TAG_FILES_START_NEXT_PAUSED");
		case 0x1809: return wxT("EC_TAG_FILES_RESUME_SAME_CAT");
		case 0x180A: return wxT("EC_TAG_FILES_SAVE_SOURCES");
		case 0x180B: return wxT("EC_TAG_FILES_EXTRACT_METADATA");
		case 0x180C: return wxT("EC_TAG_FILES_ALLOC_FULL_SIZE");
		case 0x180D: return wxT("EC_TAG_FILES_CHECK_FREE_SPACE");
		case 0x180E: return wxT("EC_TAG_FILES_MIN_FREE_SPACE");
		case 0x1900: return wxT("EC_TAG_PREFS_SRCDROP");
		case 0x1901: return wxT("EC_TAG_SRCDROP_NONEEDED");
		case 0x1902: return wxT("EC_TAG_SRCDROP_DROP_FQS");
		case 0x1903: return wxT("EC_TAG_SRCDROP_DROP_HQRS");
		case 0x1904: return wxT("EC_TAG_SRCDROP_HQRS_VALUE");
		case 0x1905: return wxT("EC_TAG_SRCDROP_AUTODROP_TIMER");
		case 0x1A00: return wxT("EC_TAG_PREFS_DIRECTORIES");
		case 0x1A01: return wxT("EC_TAG_DIRECTORIES_INCOMING");
		case 0x1A02: return wxT("EC_TAG_DIRECTORIES_TEMP");
		case 0x1A03: return wxT("EC_TAG_DIRECTORIES_SHARED");
		case 0x1A04: return wxT("EC_TAG_DIRECTORIES_SHARE_HIDDEN");
		case 0x1B00: return wxT("EC_TAG_PREFS_STATISTICS");
		case 0x1B01: return wxT("EC_TAG_STATSGRAPH_WIDTH");
		case 0x1B02: return wxT("EC_TAG_STATSGRAPH_SCALE");
		case 0x1B03: return wxT("EC_TAG_STATSGRAPH_LAST");
		case 0x1B04: return wxT("EC_TAG_STATSGRAPH_DATA");
		case 0x1B05: return wxT("EC_TAG_STATTREE_CAPPING");
		case 0x1B06: return wxT("EC_TAG_STATTREE_NODE");
		case 0x1B07: return wxT("EC_TAG_STAT_NODE_VALUE");
		case 0x1B08: return wxT("EC_TAG_STAT_VALUE_TYPE");
		case 0x1B09: return wxT("EC_TAG_STATTREE_NODEID");
		case 0x1C00: return wxT("EC_TAG_PREFS_SECURITY");
		case 0x1C01: return wxT("EC_TAG_SECURITY_CAN_SEE_SHARES");
		case 0x1C02: return wxT("EC_TAG_IPFILTER_CLIENTS");
		case 0x1C03: return wxT("EC_TAG_IPFILTER_SERVERS");
		case 0x1C04: return wxT("EC_TAG_IPFILTER_AUTO_UPDATE");
		case 0x1C05: return wxT("EC_TAG_IPFILTER_UPDATE_URL");
		case 0x1C06: return wxT("EC_TAG_IPFILTER_LEVEL");
		case 0x1C07: return wxT("EC_TAG_IPFILTER_FILTER_LAN");
		case 0x1C08: return wxT("EC_TAG_SECURITY_USE_SECIDENT");
		case 0x1C09: return wxT("EC_TAG_SECURITY_OBFUSCATION_SUPPORTED");
		case 0x1C0A: return wxT("EC_TAG_SECURITY_OBFUSCATION_REQUESTED");
		case 0x1C0B: return wxT("EC_TAG_SECURITY_OBFUSCATION_REQUIRED");
		case 0x1D00: return wxT("EC_TAG_PREFS_CORETWEAKS");
		case 0x1D01: return wxT("EC_TAG_CORETW_MAX_CONN_PER_FIVE");
		case 0x1D02: return wxT("EC_TAG_CORETW_VERBOSE");
		case 0x1D03: return wxT("EC_TAG_CORETW_FILEBUFFER");
		case 0x1D04: return wxT("EC_TAG_CORETW_UL_QUEUE");
		case 0x1D05: return wxT("EC_TAG_CORETW_SRV_KEEPALIVE_TIMEOUT");
		case 0x1E00: return wxT("EC_TAG_PREFS_KADEMLIA");
		case 0x1E01: return wxT("EC_TAG_KADEMLIA_UPDATE_URL");
		default: return CFormat(wxT("unknown %d 0x%x")) % arg % arg;
	}
}

wxString GetDebugNameEC_DETAIL_LEVEL(uint8 arg)
{
	switch (arg) {
		case 0x00: return wxT("EC_DETAIL_CMD");
		case 0x01: return wxT("EC_DETAIL_WEB");
		case 0x02: return wxT("EC_DETAIL_FULL");
		case 0x03: return wxT("EC_DETAIL_UPDATE");
		case 0x04: return wxT("EC_DETAIL_INC_UPDATE");
		default: return CFormat(wxT("unknown %d 0x%x")) % arg % arg;
	}
}

wxString GetDebugNameEC_SEARCH_TYPE(uint8 arg)
{
	switch (arg) {
		case 0x00: return wxT("EC_SEARCH_LOCAL");
		case 0x01: return wxT("EC_SEARCH_GLOBAL");
		case 0x02: return wxT("EC_SEARCH_KAD");
		case 0x03: return wxT("EC_SEARCH_WEB");
		default: return CFormat(wxT("unknown %d 0x%x")) % arg % arg;
	}
}

wxString GetDebugNameEC_STATTREE_NODE_VALUE_TYPE(uint8 arg)
{
	switch (arg) {
		case 0x00: return wxT("EC_VALUE_INTEGER");
		case 0x01: return wxT("EC_VALUE_ISTRING");
		case 0x02: return wxT("EC_VALUE_BYTES");
		case 0x03: return wxT("EC_VALUE_ISHORT");
		case 0x04: return wxT("EC_VALUE_TIME");
		case 0x05: return wxT("EC_VALUE_SPEED");
		case 0x06: return wxT("EC_VALUE_STRING");
		case 0x07: return wxT("EC_VALUE_DOUBLE");
		default: return CFormat(wxT("unknown %d 0x%x")) % arg % arg;
	}
}

wxString GetDebugNameEcPrefs(uint32 arg)
{
	switch (arg) {
		case 0x00000001: return wxT("EC_PREFS_CATEGORIES");
		case 0x00000002: return wxT("EC_PREFS_GENERAL");
		case 0x00000004: return wxT("EC_PREFS_CONNECTIONS");
		case 0x00000008: return wxT("EC_PREFS_MESSAGEFILTER");
		case 0x00000010: return wxT("EC_PREFS_REMOTECONTROLS");
		case 0x00000020: return wxT("EC_PREFS_ONLINESIG");
		case 0x00000040: return wxT("EC_PREFS_SERVERS");
		case 0x00000080: return wxT("EC_PREFS_FILES");
		case 0x00000100: return wxT("EC_PREFS_SRCDROP");
		case 0x00000200: return wxT("EC_PREFS_DIRECTORIES");
		case 0x00000400: return wxT("EC_PREFS_STATISTICS");
		case 0x00000800: return wxT("EC_PREFS_SECURITY");
		case 0x00001000: return wxT("EC_PREFS_CORETWEAKS");
		case 0x00002000: return wxT("EC_PREFS_KADEMLIA");
		default: return CFormat(wxT("unknown %d 0x%x")) % arg % arg;
	}
}

#endif	// DEBUG_EC_IMPLEMENTATION

#endif // __ECCODES_H__
