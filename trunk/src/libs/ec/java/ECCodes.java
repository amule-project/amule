// 
//  This file is part of the aMule Project.
// 
//  Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

public interface ECCodes {


public final static short EC_CURRENT_PROTOCOL_VERSION = 0x0200;

public final static int EC_FLAG_ZLIB	 = 0x00000001;
public final static int EC_FLAG_UTF8_NUMBERS = 0x00000002;
public final static int EC_FLAG_HAS_ID	 = 0x00000004;
public final static int EC_FLAG_ACCEPTS	 = 0x00000010;
public final static int EC_FLAG_UNKNOWN_MASK = 0xff7f7f08;

public final static byte EC_OP_NOOP                          = 0x01;
public final static byte EC_OP_AUTH_REQ                      = 0x02;
public final static byte EC_OP_AUTH_FAIL                     = 0x03;
public final static byte EC_OP_AUTH_OK                       = 0x04;
public final static byte EC_OP_FAILED                        = 0x05;
public final static byte EC_OP_STRINGS                       = 0x06;
public final static byte EC_OP_MISC_DATA                     = 0x07;
public final static byte EC_OP_SHUTDOWN                      = 0x08;
public final static byte EC_OP_ED2K_LINK                     = 0x09;
public final static byte EC_OP_STAT_REQ                      = 0x0A;
public final static byte EC_OP_GET_CONNSTATE                 = 0x0B;
public final static byte EC_OP_STATS                         = 0x0C;
public final static byte EC_OP_GET_DLOAD_QUEUE               = 0x0D;
public final static byte EC_OP_GET_ULOAD_QUEUE               = 0x0E;
public final static byte EC_OP_GET_WAIT_QUEUE                = 0x0F;
public final static byte EC_OP_GET_SHARED_FILES              = 0x10;
public final static byte EC_OP_SHARED_SET_PRIO               = 0x11;
public final static byte EC_OP_PARTFILE_REMOVE_NO_NEEDED     = 0x12;
public final static byte EC_OP_PARTFILE_REMOVE_FULL_QUEUE    = 0x13;
public final static byte EC_OP_PARTFILE_REMOVE_HIGH_QUEUE    = 0x14;
public final static byte EC_OP_PARTFILE_CLEANUP_SOURCES      = 0x15;
public final static byte EC_OP_PARTFILE_SWAP_A4AF_THIS       = 0x16;
public final static byte EC_OP_PARTFILE_SWAP_A4AF_THIS_AUTO  = 0x17;
public final static byte EC_OP_PARTFILE_SWAP_A4AF_OTHERS     = 0x18;
public final static byte EC_OP_PARTFILE_PAUSE                = 0x19;
public final static byte EC_OP_PARTFILE_RESUME               = 0x1A;
public final static byte EC_OP_PARTFILE_STOP                 = 0x1B;
public final static byte EC_OP_PARTFILE_PRIO_SET             = 0x1C;
public final static byte EC_OP_PARTFILE_DELETE               = 0x1D;
public final static byte EC_OP_PARTFILE_SET_CAT              = 0x1E;
public final static byte EC_OP_DLOAD_QUEUE                   = 0x1F;
public final static byte EC_OP_ULOAD_QUEUE                   = 0x20;
public final static byte EC_OP_WAIT_QUEUE                    = 0x21;
public final static byte EC_OP_SHARED_FILES                  = 0x22;
public final static byte EC_OP_SHAREDFILES_RELOAD            = 0x23;
public final static byte EC_OP_SHAREDFILES_ADD_DIRECTORY     = 0x24;
public final static byte EC_OP_RENAME_FILE                   = 0x25;
public final static byte EC_OP_SEARCH_START                  = 0x26;
public final static byte EC_OP_SEARCH_STOP                   = 0x27;
public final static byte EC_OP_SEARCH_RESULTS                = 0x28;
public final static byte EC_OP_SEARCH_PROGRESS               = 0x29;
public final static byte EC_OP_DOWNLOAD_SEARCH_RESULT        = 0x2A;
public final static byte EC_OP_IPFILTER_RELOAD               = 0x2B;
public final static byte EC_OP_GET_SERVER_LIST               = 0x2C;
public final static byte EC_OP_SERVER_LIST                   = 0x2D;
public final static byte EC_OP_SERVER_DISCONNECT             = 0x2E;
public final static byte EC_OP_SERVER_CONNECT                = 0x2F;
public final static byte EC_OP_SERVER_REMOVE                 = 0x30;
public final static byte EC_OP_SERVER_ADD                    = 0x31;
public final static byte EC_OP_SERVER_UPDATE_FROM_URL        = 0x32;
public final static byte EC_OP_ADDLOGLINE                    = 0x33;
public final static byte EC_OP_ADDDEBUGLOGLINE               = 0x34;
public final static byte EC_OP_GET_LOG                       = 0x35;
public final static byte EC_OP_GET_DEBUGLOG                  = 0x36;
public final static byte EC_OP_GET_SERVERINFO                = 0x37;
public final static byte EC_OP_LOG                           = 0x38;
public final static byte EC_OP_DEBUGLOG                      = 0x39;
public final static byte EC_OP_SERVERINFO                    = 0x3A;
public final static byte EC_OP_RESET_LOG                     = 0x3B;
public final static byte EC_OP_RESET_DEBUGLOG                = 0x3C;
public final static byte EC_OP_CLEAR_SERVERINFO              = 0x3D;
public final static byte EC_OP_GET_LAST_LOG_ENTRY            = 0x3E;
public final static byte EC_OP_GET_PREFERENCES               = 0x3F;
public final static byte EC_OP_SET_PREFERENCES               = 0x40;
public final static byte EC_OP_CREATE_CATEGORY               = 0x41;
public final static byte EC_OP_UPDATE_CATEGORY               = 0x42;
public final static byte EC_OP_DELETE_CATEGORY               = 0x43;
public final static byte EC_OP_GET_STATSGRAPHS               = 0x44;
public final static byte EC_OP_STATSGRAPHS                   = 0x45;
public final static byte EC_OP_GET_STATSTREE                 = 0x46;
public final static byte EC_OP_STATSTREE                     = 0x47;
public final static byte EC_OP_KAD_START                     = 0x48;
public final static byte EC_OP_KAD_STOP                      = 0x49;
public final static byte EC_OP_CONNECT                       = 0x4A;
public final static byte EC_OP_DISCONNECT                    = 0x4B;

public final static short EC_TAG_STRING                             = 0x0000;
public final static short EC_TAG_PASSWD_HASH                        = 0x0001;
public final static short EC_TAG_PROTOCOL_VERSION                   = 0x0002;
public final static short EC_TAG_VERSION_ID                         = 0x0003;
public final static short EC_TAG_DETAIL_LEVEL                       = 0x0004;
public final static short EC_TAG_CONNSTATE                          = 0x0005;
public final static short EC_TAG_ED2K_ID                            = 0x0006;
public final static short EC_TAG_LOG_TO_STATUS                      = 0x0007;
public final static short EC_TAG_CLIENT_NAME                        = 0x0100;
public final static short 	EC_TAG_CLIENT_VERSION                     = 0x0101;
public final static short 	EC_TAG_CLIENT_MOD                         = 0x0102;
public final static short EC_TAG_STATS_UL_SPEED                     = 0x0200;
public final static short 	EC_TAG_STATS_DL_SPEED	                  = 0x0201;
public final static short 	EC_TAG_STATS_UL_SPEED_LIMIT               = 0x0202;
public final static short 	EC_TAG_STATS_DL_SPEED_LIMIT               = 0x0203;
public final static short 	EC_TAG_STATS_UP_OVERHEAD                  = 0x0204;
public final static short 	EC_TAG_STATS_DOWN_OVERHEAD                = 0x0205;
public final static short 	EC_TAG_STATS_TOTAL_SRC_COUNT              = 0x0206;
public final static short 	EC_TAG_STATS_BANNED_COUNT                 = 0x0207;
public final static short 	EC_TAG_STATS_UL_QUEUE_LEN                 = 0x0208;
public final static short EC_TAG_PARTFILE                           = 0x0300;
public final static short 	EC_TAG_PARTFILE_NAME                      = 0x0301;
public final static short 	EC_TAG_PARTFILE_PARTMETID                 = 0x0302;
public final static short 	EC_TAG_PARTFILE_SIZE_FULL                 = 0x0303;
public final static short 	EC_TAG_PARTFILE_SIZE_XFER                 = 0x0304;
public final static short 	EC_TAG_PARTFILE_SIZE_XFER_UP              = 0x0305;
public final static short 	EC_TAG_PARTFILE_SIZE_DONE                 = 0x0306;
public final static short 	EC_TAG_PARTFILE_SPEED                     = 0x0307;
public final static short 	EC_TAG_PARTFILE_STATUS                    = 0x0308;
public final static short 	EC_TAG_PARTFILE_PRIO                      = 0x0309;
public final static short 	EC_TAG_PARTFILE_SOURCE_COUNT              = 0x030A;
public final static short 	EC_TAG_PARTFILE_SOURCE_COUNT_A4AF         = 0x030B;
public final static short 	EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT  = 0x030C;
public final static short 	EC_TAG_PARTFILE_SOURCE_COUNT_XFER         = 0x030D;
public final static short 	EC_TAG_PARTFILE_ED2K_LINK                 = 0x030E;
public final static short 	EC_TAG_PARTFILE_CAT                       = 0x030F;
public final static short 	EC_TAG_PARTFILE_LAST_RECV                 = 0x0310;
public final static short 	EC_TAG_PARTFILE_LAST_SEEN_COMP            = 0x0311;
public final static short 	EC_TAG_PARTFILE_PART_STATUS               = 0x0312;
public final static short 	EC_TAG_PARTFILE_GAP_STATUS                = 0x0313;
public final static short 	EC_TAG_PARTFILE_REQ_STATUS                = 0x0314;
public final static short EC_TAG_KNOWNFILE                          = 0x0400;
public final static short 	EC_TAG_KNOWNFILE_XFERRED                  = 0x0401;
public final static short 	EC_TAG_KNOWNFILE_XFERRED_ALL              = 0x0402;
public final static short 	EC_TAG_KNOWNFILE_REQ_COUNT                = 0x0403;
public final static short 	EC_TAG_KNOWNFILE_REQ_COUNT_ALL            = 0x0404;
public final static short 	EC_TAG_KNOWNFILE_ACCEPT_COUNT             = 0x0405;
public final static short 	EC_TAG_KNOWNFILE_ACCEPT_COUNT_ALL         = 0x0406;
public final static short EC_TAG_SERVER                             = 0x0500;
public final static short 	EC_TAG_SERVER_NAME                        = 0x0501;
public final static short 	EC_TAG_SERVER_DESC                        = 0x0502;
public final static short 	EC_TAG_SERVER_ADDRESS                     = 0x0503;
public final static short 	EC_TAG_SERVER_PING                        = 0x0504;
public final static short 	EC_TAG_SERVER_USERS                       = 0x0505;
public final static short 	EC_TAG_SERVER_USERS_MAX                   = 0x0506;
public final static short 	EC_TAG_SERVER_FILES		          = 0x0507;
public final static short 	EC_TAG_SERVER_PRIO	                  = 0x0508;
public final static short 	EC_TAG_SERVER_FAILED                      = 0x0509;
public final static short 	EC_TAG_SERVER_STATIC                      = 0x050A;
public final static short 	EC_TAG_SERVER_VERSION                     = 0x050B;
public final static short EC_TAG_CLIENT                             = 0x0600;
public final static short 	EC_TAG_CLIENT_SOFTWARE                    = 0x0601;
public final static short 	EC_TAG_CLIENT_SCORE                       = 0x0602;
public final static short 	EC_TAG_CLIENT_HASH                        = 0x0603;
public final static short 	EC_TAG_CLIENT_FRIEND                      = 0x0604;
public final static short 	EC_TAG_CLIENT_WAIT_TIME                   = 0x0605;
public final static short 	EC_TAG_CLIENT_XFER_TIME                   = 0x0606;
public final static short 	EC_TAG_CLIENT_QUEUE_TIME                  = 0x0607;
public final static short 	EC_TAG_CLIENT_LAST_TIME                   = 0x0608;
public final static short 	EC_TAG_CLIENT_UPLOAD_SESSION              = 0x0609;
public final static short 	EC_TAG_CLIENT_UPLOAD_TOTAL                = 0x060A;
public final static short 	EC_TAG_CLIENT_DOWNLOAD_TOTAL              = 0x060B;
public final static short 	EC_TAG_CLIENT_STATE                       = 0x060C;
public final static short 	EC_TAG_CLIENT_UP_SPEED                    = 0x060D;
public final static short 	EC_TAG_CLIENT_DOWN_SPEED                  = 0x060E;
public final static short 	EC_TAG_CLIENT_FROM                        = 0x060F;
public final static short 	EC_TAG_CLIENT_USER_IP                     = 0x0610;
public final static short 	EC_TAG_CLIENT_USER_PORT                   = 0x0611;
public final static short 	EC_TAG_CLIENT_SERVER_IP                   = 0x0612;
public final static short 	EC_TAG_CLIENT_SERVER_PORT                 = 0x0613;
public final static short 	EC_TAG_CLIENT_SERVER_NAME                 = 0x0614;
public final static short 	EC_TAG_CLIENT_SOFT_VER_STR                = 0x0615;
public final static short EC_TAG_SEARCHFILE                         = 0x0700;
public final static short 	EC_TAG_SEARCH_TYPE                        = 0x0701;
public final static short 	EC_TAG_SEARCH_NAME                        = 0x0702;
public final static short 	EC_TAG_SEARCH_MIN_SIZE                    = 0x0703;
public final static short 	EC_TAG_SEARCH_MAX_SIZE                    = 0x0704;
public final static short 	EC_TAG_SEARCH_FILE_TYPE                   = 0x0705;
public final static short 	EC_TAG_SEARCH_EXTENSION                   = 0x0706;
public final static short 	EC_TAG_SEARCH_AVAILABILITY                = 0x0707;
public final static short 	EC_TAG_SEARCH_STATUS                      = 0x0708;
public final static short EC_TAG_SELECT_PREFS                       = 0x1000;
public final static short 	EC_TAG_PREFS_CATEGORIES                   = 0x1100;
public final static short 		EC_TAG_CATEGORY                           = 0x1101;
public final static short 		EC_TAG_CATEGORY_TITLE                     = 0x1102;
public final static short 		EC_TAG_CATEGORY_PATH                      = 0x1103;
public final static short 		EC_TAG_CATEGORY_COMMENT                   = 0x1104;
public final static short 		EC_TAG_CATEGORY_COLOR                     = 0x1105;
public final static short 		EC_TAG_CATEGORY_PRIO                      = 0x1106;
public final static short 	EC_TAG_PREFS_GENERAL                      = 0x1200;
public final static short 		EC_TAG_USER_NICK                          = 0x1201;
public final static short 		EC_TAG_USER_USERHASH                      = 0x1202;
public final static short 	EC_TAG_PREFS_CONNECTIONS                  = 0x1300;
public final static short 		EC_TAG_CONN_DL_CAP                        = 0x1301;
public final static short 		EC_TAG_CONN_UL_CAP	                  = 0x1302;
public final static short 		EC_TAG_CONN_MAX_DL	                  = 0x1303;
public final static short 		EC_TAG_CONN_MAX_UL	                  = 0x1304;
public final static short 		EC_TAG_CONN_SLOT_ALLOCATION               = 0x1305;
public final static short 		EC_TAG_CONN_TCP_PORT                      = 0x1306;
public final static short 		EC_TAG_CONN_UDP_PORT	                  = 0x1307;
public final static short 		EC_TAG_CONN_UDP_DISABLE                   = 0x1308;
public final static short 		EC_TAG_CONN_MAX_FILE_SOURCES              = 0x1309;
public final static short 		EC_TAG_CONN_MAX_CONN	                  = 0x130A;
public final static short 		EC_TAG_CONN_AUTOCONNECT                   = 0x130B;
public final static short 		EC_TAG_CONN_RECONNECT	                  = 0x130C;
public final static short 		EC_TAG_NETWORK_ED2K		          = 0x130D;
public final static short 		EC_TAG_NETWORK_KADEMLIA                   = 0x130E;
public final static short 	EC_TAG_PREFS_MESSAGEFILTER                = 0x1400;
public final static short 		EC_TAG_MSGFILTER_ENABLED                  = 0x1401;
public final static short 		EC_TAG_MSGFILTER_ALL                      = 0x1402;
public final static short 		EC_TAG_MSGFILTER_FRIENDS                  = 0x1403;
public final static short 		EC_TAG_MSGFILTER_SECURE                   = 0x1404;
public final static short 		EC_TAG_MSGFILTER_BY_KEYWORD               = 0x1405;
public final static short 		EC_TAG_MSGFILTER_KEYWORDS                 = 0x1406;
public final static short 	EC_TAG_PREFS_REMOTECTRL                   = 0x1500;
public final static short 		EC_TAG_WEBSERVER_AUTORUN                  = 0x1501;
public final static short 		EC_TAG_WEBSERVER_PORT                     = 0x1502;
public final static short 		EC_TAG_WEBSERVER_GUEST                    = 0x1503;
public final static short 		EC_TAG_WEBSERVER_USEGZIP                  = 0x1504;
public final static short 		EC_TAG_WEBSERVER_REFRESH                  = 0x1505;
public final static short 		EC_TAG_WEBSERVER_TEMPLATE                 = 0x1506;
public final static short 	EC_TAG_PREFS_ONLINESIG                    = 0x1600;
public final static short 		EC_TAG_ONLINESIG_ENABLED                  = 0x1601;
public final static short 	EC_TAG_PREFS_SERVERS                      = 0x1700;
public final static short 		EC_TAG_SERVERS_REMOVE_DEAD                = 0x1701;
public final static short 		EC_TAG_SERVERS_DEAD_SERVER_RETRIES        = 0x1702;
public final static short 		EC_TAG_SERVERS_AUTO_UPDATE                = 0x1703;
public final static short 		EC_TAG_SERVERS_URL_LIST                   = 0x1704;
public final static short 		EC_TAG_SERVERS_ADD_FROM_SERVER            = 0x1705;
public final static short 		EC_TAG_SERVERS_ADD_FROM_CLIENT            = 0x1706;
public final static short 		EC_TAG_SERVERS_USE_SCORE_SYSTEM           = 0x1707;
public final static short 		EC_TAG_SERVERS_SMART_ID_CHECK             = 0x1708;
public final static short 		EC_TAG_SERVERS_SAFE_SERVER_CONNECT        = 0x1709;
public final static short 		EC_TAG_SERVERS_AUTOCONN_STATIC_ONLY       = 0x170A;
public final static short 		EC_TAG_SERVERS_MANUAL_HIGH_PRIO           = 0x170B;
public final static short 	EC_TAG_PREFS_FILES                        = 0x1800;
public final static short 		EC_TAG_FILES_ICH_ENABLED                  = 0x1801;
public final static short 		EC_TAG_FILES_AICH_TRUST                   = 0x1802;
public final static short 		EC_TAG_FILES_NEW_PAUSED                   = 0x1803;
public final static short 		EC_TAG_FILES_NEW_AUTO_DL_PRIO             = 0x1804;
public final static short 		EC_TAG_FILES_PREVIEW_PRIO                 = 0x1805;
public final static short 		EC_TAG_FILES_NEW_AUTO_UL_PRIO             = 0x1806;
public final static short 		EC_TAG_FILES_UL_FULL_CHUNKS               = 0x1807;
public final static short 		EC_TAG_FILES_START_NEXT_PAUSED            = 0x1808;
public final static short 		EC_TAG_FILES_RESUME_SAME_CAT              = 0x1809;
public final static short 		EC_TAG_FILES_SAVE_SOURCES                 = 0x180A;
public final static short 		EC_TAG_FILES_EXTRACT_METADATA             = 0x180B;
public final static short 		EC_TAG_FILES_ALLOC_FULL_CHUNKS            = 0x180C;
public final static short 		EC_TAG_FILES_ALLOC_FULL_SIZE              = 0x180D;
public final static short 		EC_TAG_FILES_CHECK_FREE_SPACE             = 0x180E;
public final static short 		EC_TAG_FILES_MIN_FREE_SPACE	          = 0x180F;
public final static short 	EC_TAG_PREFS_SRCDROP                      = 0x1900;
public final static short 		EC_TAG_SRCDROP_NONEEDED                   = 0x1901;
public final static short 		EC_TAG_SRCDROP_DROP_FQS                   = 0x1902;
public final static short 		EC_TAG_SRCDROP_DROP_HQRS                  = 0x1903;
public final static short 		EC_TAG_SRCDROP_HQRS_VALUE                 = 0x1904;
public final static short 		EC_TAG_SRCDROP_AUTODROP_TIMER             = 0x1905;
public final static short 	EC_TAG_PREFS_DIRECTORIES                  = 0x1A00;
public final static short 	EC_TAG_PREFS_STATISTICS                   = 0x1B00;
public final static short 		EC_TAG_STATSGRAPH_WIDTH                   = 0x1B01;
public final static short 		EC_TAG_STATSGRAPH_SCALE                   = 0x1B02;
public final static short 		EC_TAG_STATSGRAPH_LAST                    = 0x1B03;
public final static short 		EC_TAG_STATSGRAPH_DATA                    = 0x1B04;
public final static short 		EC_TAG_STATTREE_CAPPING                   = 0x1B05;
public final static short 		EC_TAG_STATTREE_NODE                      = 0x1B06;
public final static short 		EC_TAG_STAT_NODE_VALUE                    = 0x1B07;
public final static short 		EC_TAG_STAT_VALUE_TYPE                    = 0x1B08;
public final static short 	EC_TAG_PREFS_SECURITY                     = 0x1C00;
public final static short 		EC_TAG_SECURITY_CAN_SEE_SHARES            = 0x1C01;
public final static short 		EC_TAG_IPFILTER_CLIENTS                   = 0x1C02;
public final static short 		EC_TAG_IPFILTER_SERVERS                   = 0x1C03;
public final static short 		EC_TAG_IPFILTER_AUTO_UPDATE               = 0x1C04;
public final static short 		EC_TAG_IPFILTER_UPDATE_URL                = 0x1C05;
public final static short 		EC_TAG_IPFILTER_LEVEL                     = 0x1C06;
public final static short 		EC_TAG_IPFILTER_FILTER_LAN                = 0x1C07;
public final static short 		EC_TAG_SECURITY_USE_SECIDENT              = 0x1C08;
public final static short 	EC_TAG_PREFS_CORETWEAKS                   = 0x1D00;
public final static short 		EC_TAG_CORETW_MAX_CONN_PER_FIVE           = 0x1D01;
public final static short 		EC_TAG_CORETW_VERBOSE                     = 0x1D02;
public final static short 		EC_TAG_CORETW_FILEBUFFER                  = 0x1D03;
public final static short 		EC_TAG_CORETW_UL_QUEUE                    = 0x1D04;
public final static short 		EC_TAG_CORETW_SRV_KEEPALIVE_TIMEOUT       = 0x1D05;

public final static byte EC_DETAIL_CMD           = 0x00;
public final static byte EC_DETAIL_WEB           = 0x01;
public final static byte EC_DETAIL_FULL          = 0x02;
public final static byte EC_DETAIL_UPDATE        = 0x03;
public final static byte EC_DETAIL_INC_UPDATE    = 0x04;

public final static byte EC_SEARCH_LOCAL         = 0x00;
public final static byte EC_SEARCH_GLOBAL        = 0x01;
public final static byte EC_SEARCH_KAD           = 0x02;
public final static byte EC_SEARCH_WEB           = 0x03;

public final static byte EC_VALUE_INTEGER        = 0x00;
public final static byte EC_VALUE_ISTRING        = 0x01;
public final static byte EC_VALUE_BYTES          = 0x02;
public final static byte EC_VALUE_ISHORT         = 0x03;
public final static byte EC_VALUE_TIME           = 0x04;
public final static byte EC_VALUE_SPEED          = 0x05;
public final static byte EC_VALUE_STRING         = 0x06;
public final static byte EC_VALUE_DOUBLE         = 0x07;

public final static int EC_PREFS_CATEGORIES     = 0x00000001;
public final static int EC_PREFS_GENERAL        = 0x00000002;
public final static int EC_PREFS_CONNECTIONS    = 0x00000004;
public final static int EC_PREFS_MESSAGEFILTER  = 0x00000008;
public final static int EC_PREFS_REMOTECONTROLS = 0x00000010;
public final static int EC_PREFS_ONLINESIG      = 0x00000020;
public final static int EC_PREFS_SERVERS        = 0x00000040;
public final static int EC_PREFS_FILES          = 0x00000080;
public final static int EC_PREFS_SRCDROP        = 0x00000100;
public final static int EC_PREFS_DIRECTORIES    = 0x00000200;
public final static int EC_PREFS_STATISTICS     = 0x00000400;
public final static int EC_PREFS_SECURITY       = 0x00000800;
public final static int EC_PREFS_CORETWEAKS     = 0x00001000;

}
