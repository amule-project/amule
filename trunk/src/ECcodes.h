//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

/*!
 * \file ECcodes.h
 *
 * \brief EC types, OPCODEs, TAGNAMEs.
 */

#ifndef	ECCODES_H
#define	ECCODES_H

#include <inttypes.h>		// Needed for uint* types


/*
 * EC types
 */

typedef uint8_t ec_opcode_t;
typedef uint16_t ec_tagname_t;
typedef uint32_t ec_taglen_t;


/**
 * Current EC protocol version
 */
#define	EC_CURRENT_PROTOCOL_VERSION	0x01f3


/*
 * EC FLAG values
 */

#define EC_FLAG_ZLIB		0x00000001
#define EC_FLAG_UTF8_NUMBERS	0x00000002
#define EC_FLAG_ACCEPTS		0x00000010

#define EC_FLAG_UNKNOWN_MASK	0xff7f7f0c


/**
 * <b>EC opcodes</b>
 *
 * OpCodes tell the receiver what to do (with the provided data).
 */

// If you plan to use opcodes greater than 0x00ff, please
// change the size of ec_opcode_t to uint16.

enum {

		/*!
		 * \brief Perform no operation, just like do nothing.
		 *
		 * General server response when the requested operation was
		 * successful, and no reply needed.
		 */
	EC_OP_NOOP = 0x0001,

	//
	// Authentication
	//

		/*!
		 * \brief Authentication request.
		 *
		 * \par Tags:
		 *	::EC_TAG_PASSWD_HASH (required)\n
		 *	::EC_TAG_CLIENT_NAME\n
		 *	::EC_TAG_CLIENT_VERSION\n
		 *	::EC_TAG_CLIENT_MOD\n
		 *	::EC_TAG_PROTOCOL_VERSION (required)\n
		 *	::EC_TAG_VERSION_ID
		 */
	EC_OP_AUTH_REQ,

		/*!
		 * \brief Authentication failed.
		 *
		 * \par Tags:
		 *	::EC_TAG_STRING (optional) describing the reason why authentication was rejected.\n
		 *	::EC_TAG_SERVER_VERSION aMule version.
		 */
	EC_OP_AUTH_FAIL,

		/*!
		 * \brief Authentication succeeded.
		 *
		 * \par Tags:
		 *	::EC_TAG_SERVER_VERSION aMule version.
		 */
	EC_OP_AUTH_OK,

	//
	// Misc commands
	//

		/*!
		 * \brief General error reply.
		 *
		 * \par Tags:
		 *	::EC_TAG_STRING (0-1) An error message.
		 */
	EC_OP_FAILED,

		/*!
		 * \brief Used when need to transfer text message without any
		 * particular meaning - like logging message in reply to command
		 *
		 * \par Tags:
		 *	::EC_TAG_STRING (1+) string message
		 */
	EC_OP_STRINGS,

		/*!
		 * \brief Miscannelous data.
		 *
		 * \par Tags:
		 *	(any tag)
		 */
	EC_OP_MISC_DATA,

		/*!
		 * \brief Request to shut down aMule.
		 *
		 * \par Tags:
		 *	(none)
		 */
	EC_OP_SHUTDOWN,

		/*!
		 * \brief Handle ED2k link.
		 *
		 * \par Tags:
		 *	::EC_TAG_STRING (1+) holding ED2k link.
		 */
	EC_OP_ED2K_LINK,


	//
	// Statistics
	//

		/*!
		 * \brief Request statistics.
		 *
		 * Request from client, server must reply with statistics (::EC_OP_STATS).
		 *
		 * \par Tags:
		 *	(none)
		 */
	EC_OP_STAT_REQ,
	
		/*!
		 * \brief Request connection state.
		 *
		 * Request from client, server replies with an ::EC_OP_MISC_DATA
		 * packet containing an ::EC_TAG_CONNSTATE tag.
		 *
		 * \par Tags:
		 *	(none)
		 */
	EC_OP_GET_CONNSTATE,

		/*!
		 * \brief Reply to stats request
		 * 
		 * Status information about files/users/limits/speeds etc
		 * 
		 * \par Tags:
		 *	::EC_TAG_STATS_*
		 */
	EC_OP_STATS,

		/*!
		 * \brief Request for download queue.
		 *
		 * \par Tags:
		 *	::EC_TAG_PARTFILE (*) info can be requested for selected list only
		 */
	EC_OP_GET_DLOAD_QUEUE,
		
		/*!
		 * \brief Request for upload queue - currect uploads
		 *
		 * \par Tags:
		 *	::EC_TAG_PARTFILE (*) status can be requested for selected list only
		 */
	EC_OP_GET_ULOAD_QUEUE,
	EC_OP_GET_WAIT_QUEUE,

		/*!
		 * \brief Request for list of shared files
		 * 
		 */
	EC_OP_GET_SHARED_FILES,
	
		/*!
		 * \brief Set priority for shared file(s)
		 * 
		 * \par Tags:
		 *  ::EC_TAG_KNOWNFILE (1+) file to process
		 */
	EC_OP_SHARED_SET_PRIO,

		/*
		 * \brief Perform action on file in queue.
		 *
		 * \par Tags:
		 *	::EC_TAG_PARTFILE (1+) item (object) to perform command on
		 */
	EC_OP_PARTFILE_REMOVE_NO_NEEDED,
	EC_OP_PARTFILE_REMOVE_FULL_QUEUE,
	EC_OP_PARTFILE_REMOVE_HIGH_QUEUE,
	EC_OP_PARTFILE_CLEANUP_SOURCES,
	EC_OP_PARTFILE_SWAP_A4AF_THIS,
	EC_OP_PARTFILE_SWAP_A4AF_THIS_AUTO,
	EC_OP_PARTFILE_SWAP_A4AF_OTHERS,
	EC_OP_PARTFILE_PAUSE,
	EC_OP_PARTFILE_RESUME,
	EC_OP_PARTFILE_STOP,
	EC_OP_PARTFILE_PRIO_SET,
	EC_OP_PARTFILE_DELETE,
	EC_OP_PARTFILE_SET_CAT,

		/*!
		 * \brief Get download queue.
		 *
		 * \par Tags:
		 *	::EC_TAG_PARTFILE (1+) info about file in download queue
		 */
	EC_OP_DLOAD_QUEUE,
		
		/*!
		 * \brief Get upload/waiting queue.
		 *
		 * \par Tags:
		 *	::EC_TAG_UPDOWN_CLIENT (1+) info about client in queue
		 */
	EC_OP_ULOAD_QUEUE,
	EC_OP_WAIT_QUEUE,

		/*!
		 * \brief Get shared files
		 * 
		 * \par Tags:
		 *	::EC_TAG_KNOWNFILE (*) info about each file
		 */
	EC_OP_SHARED_FILES,

		/*!
		 * \brief Reloads the shared files list.
		 */
	EC_OP_SHAREDFILES_RELOAD,

		/*!
		 * \brief Add specified directory to the shared files list.
		 */
	EC_OP_SHAREDFILES_ADD_DIRECTORY,

		/*!
		 * \brief Rename a known file.
		 *
		 * \par Tags:
		 *	::EC_TAG_KNOWNFILE,\n
		 *	::EC_TAG_PARTFILE_NAME
		 */
	EC_OP_RENAME_FILE,


	//
	// Search
	//

		/*!
		 * \brief Command to start new search
		 */
	EC_OP_SEARCH_START,
	
		/*!
		 * \brief Command to stop current search
		 */
	EC_OP_SEARCH_STOP,

		/*!
		 * \brief Search results returned to client
		 */
	EC_OP_SEARCH_RESULTS,

		/*!
		 * \brief Search results, search is terminated
		 */
	EC_OP_SEARCH_RESULTS_DONE,

		/*!
		 * \brief Add 1 or more of found files to download queue
		 * 
		 * \par Tags:
		 *  ::EC_TAG_SEARCHFILE (1+) file to download
		 */
	EC_OP_DOWNLOAD_SEARCH_RESULT,
	

	//
	// IPFilter
	//

		/*!
		 * \brief Reloads IPFilter.
		 */
	EC_OP_IPFILTER_RELOAD,


	//
	// Server commands
	//

		/*!
		 * \brief Request list of servers
		 */
	EC_OP_GET_SERVER_LIST,
	
		/*!
		 * \brief List of servers, reply to ::EC_OP_GET_SERVER_LIST
		 *
		 * \par Tags:
		 *	::EC_TAG_SERVER (1+)
		 */
	EC_OP_SERVER_LIST,
	
		/*!
		 *  \brief Disconnect from current server
		 * 
		 * No tags
		 */
	EC_OP_SERVER_DISCONNECT,
	
		/*!
		 * \brief Connect to server
		 *
		 * Connect to server - when tag is present it identifies server to use,
		 * otherwise it will be "any server"
		 *
		 * \par Tags:
		 *	::EC_TAG_SERVER (0-1) identifies server to use
		 */
	EC_OP_SERVER_CONNECT,

		/*
		 * \brief Remove server from list.
		 *
		 * \par Tags:
		 *	::EC_TAG_SERVER (1) identifies the server to be removed
		 */
	EC_OP_SERVER_REMOVE,

		/*
		 * \brief Update server.met from given URL
		 *
		 * \par Tags:
		 *	::EC_TAG_STRING containing the URL
		 */
	EC_OP_SERVER_UPDATE_FROM_URL,


	//
	// Logging
	//

		/*!
		 * \brief Adds a new LogLine.
		 *
		 * \par Tags:
		 *	::EC_TAG_LOG_TO_STATUS
		 *	::EC_TAG_STRING
		 */
	EC_OP_ADDLOGLINE,

		/*!
		 * \brief Adds a new debog log line.
		 *
		 * \par Tags:
		 *	::EC_TAG_LOG_TO_STATUS
		 *	::EC_TAG_STRING
		 */
	EC_OP_ADDDEBUGLOGLINE,

		/*!
		 * \brief Retrieves the log.
		 *
		 * Server replies with an ::EC_OP_LOG packet.
		 */
	EC_OP_GET_LOG,

		/*!
		 * \brief Retrieves the debug log.
		 *
		 * Server replies with an ::EC_OP_DEBUGLOG packet.
		 */
	EC_OP_GET_DEBUGLOG,

		/*!
		 * \brief Retrieves the server info log.
		 *
		 * Server replies with an ::EC_OP_SERVERINFO packet.
		 */
	EC_OP_GET_SERVERINFO,

		/*!
		 * \brief The log.
		 *
		 * Server replies with this to an ::EC_OP_GET_LOG packet.
		 *
		 * \par Tags:
		 *	::EC_TAG_STRING (1+)	The log lines.
		 */
	EC_OP_LOG,

		/*!
		 * \brief The debug log.
		 *
		 * Server replies with this to an ::EC_OP_GET_DEBUGLOG packet.
		 *
		 * \par Tags:
		 *	::EC_TAG_STRING (1+)	The log lines.
		 */
	EC_OP_DEBUGLOG,

		/*!
		 * \brief The server info log.
		 *
		 * Server replies with this to an ::EC_OP_GET_SERVERINFO packet.
		 *
		 * \par Tags:
		 *	::EC_TAG_STRING (1+)	The log lines.
		 */
	EC_OP_SERVERINFO,

	EC_OP_RESET_LOG,	///< Clears the log.
	EC_OP_RESET_DEBUGLOG,	///< Clears the debug log.
	EC_OP_CLEAR_SERVERINFO,	///< Clears server info log.

		/*!
		 * \brief Returns the last log line.
		 *
		 * Response is an ::EC_OP_LOG packet, conatining only
		 * the last log line.
		 */
	EC_OP_GET_LAST_LOG_ENTRY,


	//
	// Preferences
	//

		/*!
		 * \brief Request for Preferences.
		 *
		 * This request must hold an ::EC_TAG_SELECT_PREFS tag,
		 * to select which kinds of preferences are required.
		 *
		 * \note Tags marked as (boolean) contain no data, their
		 * presence means \c true, their absence means \c false.
		 */
	EC_OP_GET_PREFERENCES,

		/*!
		 * \brief Setting the preferences or reply to EC_OP_GET_PREFERENCES
		 *
		 * Setting preferences values to the provided ones.
		 * If a tag is omitted, the respective preference value
		 * will be left unchanged.
		 *
		 * \par Possible tags:
		 *	::EC_TAG_PREFS_CATEGORIES\n
		 *	::EC_TAG_PREFS_GENERAL\n
		 *	::EC_TAG_PREFS_CONNECTIONS\n
		 *	::EC_TAG_PREFS_MESSAGEFILTER\n
		 *	::EC_TAG_PREFS_REMOTECTRL\n
		 *	::EC_TAG_PREFS_ONLINESIG\n
		 *	::EC_TAG_PREFS_SERVERS\n
		 *	::EC_TAG_PREFS_FILES\n
		 *	::EC_TAG_PREFS_SRCDROP\n
		 *	::EC_TAG_PREFS_DIRECTORIES\n
		 *	::EC_TAG_PREFS_STATISTICS\n
		 *	::EC_TAG_PREFS_SECURITY\n
		 *	::EC_TAG_PREFS_CORETWEAKS
		 *
		 * \note Tags marked as (boolean) are set the following way:
		 * not present, they're left unchanged. If they are present,
		 * they should countain an \c uint8 value, which means \c true
		 * non-zero.
		 */
	EC_OP_SET_PREFERENCES,

		/*!
		 * \brief Command to create new category
		 *
		 *
		 * \par Child tags:
		 *	::EC_TAG_CATEGORY\n
		 */
	EC_OP_CREATE_CATEGORY,
	EC_OP_UPDATE_CATEGORY,
	EC_OP_DELETE_CATEGORY,

		/*!
		 * \brief Retrieves the statistics graphs
		 *
		 * Server replies with an ::EC_OP_STATSGRAPHS packet.
		 *
		 * \par Child Tags:
		 *	::EC_TAG_STATSGRAPH_WIDTH (required for webserver only)\n
		 *	::EC_TAG_STATSGRAPH_SCALE (required for webserver only)\n
		 *	::EC_TAG_STATSGRAPH_LAST
		 */
	EC_OP_GET_STATSGRAPHS,

		/*!
		 * \brief Statistics graphs reply
		 *
		 * \par Child Tags:
		 *	::EC_TAG_STATSGRAPH_LAST
		 *	::EC_TAG_STATSGRAPH_DATA
		 */	
	EC_OP_STATSGRAPHS,

		/*!
		 * \brief Retrieves the statistics tree
		 *
		 * Server replies with an ::EC_OP_STATSTREE packet.
		 *
		 * Might contain an ::EC_TAG_STATTREE_CAPPING tag, to set how tree capping should be done.
		 */
	EC_OP_GET_STATSTREE,

		/*!
		 * \brief Statistics tree reply
		 *
		 * \par Child Tags:
		 *	::EC_TAG_STATTREE_NODE\n
		 *	::EC_TAG_USER_NICK and ::EC_TAG_SERVER_VERSION only if
		 *	detail level is ::EC_DETAIL_WEB (ie. for webserver only)
		 */
	EC_OP_STATSTREE,

		/*!
		 * \brief Connects to Kad
		 */
	EC_OP_KAD_START,
	
		/*!
		 * \brief Stops Kad
		 */
	EC_OP_KAD_STOP,

		/*!
		 * \brief Connect to the network.
		 *
		 * Connects to those networks that are enabled in preferences (ed2k, kad).
		 */
	EC_OP_CONNECT,

		/*!
		 * \brief Disconnect from networks.
		 *
		 * Disconnects from all connected networks.
		 */
	EC_OP_DISCONNECT
	
};


/******************************************************************************/

/**
 * <b>EC TAG names</b>
 *
 * TAGs hold the data, on/with the requested operation is to be performed.
 */

enum {
		/*!
		 * \brief General TAG with string type content.
		 *
		 * An EC_TAG_STRING holds a UTF-8 encoded string, with terminating zero byte.
		 *
		 * \par Child TAGs:
		 *	(none)
		 *
		 * \note <tt>TAGLEN := strlen(</tt>string<tt>) + 1;</tt>
		 */
	EC_TAG_STRING		= 0x0001,

		/*!
		 * \brief MD4 password hash.
		 *
		 * A \c hash type tag, dedicated to hold password hashes.
		 *
		 * \par Child TAGs:
		 *	(none)
		 */
	EC_TAG_PASSWD_HASH,

		/*!
		 * \brief Client name.
		 *
		 * An ::EC_TAG_STRING, dedicated to hold the client name.
		 *
		 * \par Child TAGs:
		 *	(none)
		 */
	EC_TAG_CLIENT_NAME,

		/*!
		 * \brief Client version string.
		 *
		 * An ::EC_TAG_STRING, dedicated to hold the client version.
		 *
		 * \par Child TAGs:
		 *	(none)
		 */
	EC_TAG_CLIENT_VERSION,

		/*!
		 * \brief Client mod-name.
		 *
		 * An ::EC_TAG_STRING, dedicated to hold the client mod-name.
		 *
		 * \par Child TAGs:
		 *	(none)
		 */
	EC_TAG_CLIENT_MOD,

		/*!
		 * \brief EC protocol version.
		 *
		 * An uint16 value, MSB = major, LSB = minor.
		 * Defaults to \c 0x0200 for v2.0.
		 *
		 * \par Child TAGs:
		 *	(none)
		 */
	EC_TAG_PROTOCOL_VERSION,

		/*!
		 * \brief EC binary version ID
		 *
		 * This ID is different for each change in sensitive
		 * source files, that may cause binary incompatibility.
		 *
		 * This tag is not sent nor accepted on release versions,
		 * thus we can ensure that releases which are binary compatible
		 * can still communicate with each other.
		 *
		 * Data type: \c hash
		 */
	EC_TAG_VERSION_ID,

		/*!
		 * \brief Selects response detail level.
		 *
		 * Selects response detail level in all packets that contain
		 * a server, part/knownfile or updownclient tag. Value type is
		 * \c uint8, and possible values are the EC_DETAIL_* enum values.
		 *
		 * May be included in any request packet, its default value is
		 * highest detail - to save some bandwidth on GUI transfers.
		 */
	EC_TAG_DETAIL_LEVEL,


	//
	// Tags for status (or stats) info
	//

		/*!
		 * \brief Connection state.
		 *
		 * Possible values (\c uint32):
		 *	<ul>
		 *		<li>0 - Not Connected</li>
		 *		<li>0xffffffff - Connecting</li>
		 *		<li>Any other - Client ID</li>
		 *	</ul>
		 *
		 * When connected, it contains an ::EC_TAG_SERVER child, describing the
		 * server we're connected to.
		 */
	EC_TAG_CONNSTATE,

	EC_TAG_STATS_UL_SPEED,		///< (\c uint32) Current upload speed in bytes/sec.
	EC_TAG_STATS_DL_SPEED,		///< (\c uint32) Current download speed in bytes/sec.
	EC_TAG_STATS_UL_SPEED_LIMIT,	///< (\c uint32) Upload speed limit in bytes/sec.
	EC_TAG_STATS_DL_SPEED_LIMIT,	///< (\c uint32) Download speed limit in bytes/sec.
	EC_TAG_STATS_UP_OVERHEAD,	///< (\c uint32) Current upload overhead in bytes/sec.
	EC_TAG_STATS_DOWN_OVERHEAD,	///< (\c uint32) Current download overhead in bytes/sec.
	EC_TAG_STATS_TOTAL_SRC_COUNT,	///< (\c uint32) Total number of sources found.
	EC_TAG_STATS_BANNED_COUNT,	///< (\c uint32) Number of banned clients.
	EC_TAG_STATS_UL_QUEUE_LEN,	///< (\c uint32) Number of waiting clients in the upload queue.
	

	//
	// Partfile
	//
	
	/*!
	 * \brief Info about CPartFile
	 * 
	 * Value: MD4 hash (16 bytes)
	 * 
	 * \par Chld TAGs:
	 *  ::EC_TAG_PARTFILE_*
	 */
	EC_TAG_PARTFILE,

	EC_TAG_PARTFILE_NAME,
	EC_TAG_PARTFILE_PARTMETID,	///< (\c uint16) Partfile number. (nnn.part.met)
	EC_TAG_PARTFILE_SIZE_FULL,
	EC_TAG_PARTFILE_SIZE_XFER,    // downloaded
	EC_TAG_PARTFILE_SIZE_XFER_UP, // uploaded
	EC_TAG_PARTFILE_SIZE_DONE,
	EC_TAG_PARTFILE_SPEED,
	EC_TAG_PARTFILE_STATUS,
	EC_TAG_PARTFILE_PRIO,
	EC_TAG_PARTFILE_SOURCE_COUNT,
	EC_TAG_PARTFILE_SOURCE_COUNT_A4AF,
	EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT,
	EC_TAG_PARTFILE_SOURCE_COUNT_XFER,
	EC_TAG_PARTFILE_ED2K_LINK,
	EC_TAG_PARTFILE_CAT,
	EC_TAG_PARTFILE_LAST_RECV,
	EC_TAG_PARTFILE_LAST_SEEN_COMP,

	/*!
	 * This tag contain info about status of gaps in PartFile and availability of each part.
	 * 
	 * Value: RLE encoded differential info about availability of each part
	 */	
	EC_TAG_PARTFILE_PART_STATUS,
	
	/*!
	 * Info about gaps in CPartFile: data inside contains 2 things
	 * 1. RLE encoded list of differences between previously transmitted gap list and current one
	 */
	EC_TAG_PARTFILE_GAP_STATUS,

	/*!
	 * Info about requested parts: data inside consist of 
	 * pairs of dwords [ start_offset, end_offset]. This data
	 * is not encoded, since it's expected to change quite often
	 */
	 EC_TAG_PARTFILE_REQ_STATUS,

	//
	// KnownFile
	//

	/*!
	 * \brief Info about CKnownFile (shared file)
	 * 
	 * Value (string): (the full one)
	 * 
	 * \par Chld TAGs:
	 *  ::EC_TAG_KNOWNFILE_*
	 */
	EC_TAG_KNOWNFILE,
	EC_TAG_KNOWNFILE_XFERRED,
	EC_TAG_KNOWNFILE_XFERRED_ALL,
	EC_TAG_KNOWNFILE_REQ_COUNT,
	EC_TAG_KNOWNFILE_REQ_COUNT_ALL,
	EC_TAG_KNOWNFILE_ACCEPT_COUNT,
	EC_TAG_KNOWNFILE_ACCEPT_COUNT_ALL,


	//
	// Server
	//

		/*!
		 * \brief Info about server
		 *
		 * Value: (EC_IPv4_t) IP:port of the server
		 *
		 * When any of the childs are missing, their default value should be used
		 *
		 * \par Child TAGs:
		 *	::EC_TAG_SERVER_NAME\n
		 *	::EC_TAG_SERVER_DESC\n
		 *	::EC_TAG_SERVER_PING\n
		 *	::EC_TAG_SERVER_USERS\n
		 *	::EC_TAG_SERVER_USERS_MAX\n
		 *	::EC_TAG_SERVER_FILES\n
		 *	::EC_TAG_SERVER_PRIO\n
		 *	::EC_TAG_SERVER_FAILED\n
		 *	::EC_TAG_SERVER_STATIC\n
		 *	::EC_TAG_SERVER_VERSION
		 */
	EC_TAG_SERVER,

	EC_TAG_SERVER_NAME,		///< (\c string) Server name. Default: none.
	EC_TAG_SERVER_DESC,		///< (\c string) Server description. Default: none.
	EC_TAG_SERVER_ADDRESS,
	EC_TAG_SERVER_PING,		///< (\c uint32) Server ping time. Default: N/A.
	EC_TAG_SERVER_USERS,		///< (\c uint32) User count. Default: N/A.
	EC_TAG_SERVER_USERS_MAX,	///< (\c uint32) Max. user count. Default: N/A.
	EC_TAG_SERVER_FILES,		///< (\c uint32) File count. Default: N/A.
	EC_TAG_SERVER_PRIO,		///< (\c uint8) Server priority. Default: SRV_PR_NORMAL (normal)
					/*!< <ul>
						<li>SRV_PR_LOW - Low Priority</li>
						<li>SRV_PR_NORMAL - Normal</li>
						<li>SRV_PR_HIGH - High Priority</li>
						</ul>

						Look into KnownFile.h for the SRV_PR_* values.
					*/
	EC_TAG_SERVER_FAILED,		///< (\c uint8) Fail count. Default: 0
	EC_TAG_SERVER_STATIC,		///< (\c uint8) Nonzero, when server is static. Default: 0 (not static)
	EC_TAG_SERVER_VERSION,		///< (\c string) Server version. Default: unknown.


	//
	// Up-Down Client
	//

	/*!
	 * \brief Info about up-down client
	 * 
	 * Value (uint32): id of connected user
	 * 
	 * \par Child TAGs: all below
	 */
	EC_TAG_UPDOWN_CLIENT,
	EC_TAG_CLIENT_SOFTWARE,
	EC_TAG_CLIENT_SCORE,
	EC_TAG_CLIENT_HASH,
	EC_TAG_CLIENT_FRIEND,
	EC_TAG_CLIENT_WAIT_TIME,
	EC_TAG_CLIENT_XFER_TIME,
	EC_TAG_CLIENT_QUEUE_TIME,
	EC_TAG_CLIENT_LAST_TIME,
	EC_TAG_CLIENT_UPLOAD_SESSION,
	EC_TAG_CLIENT_UPLOAD_TOTAL,
	EC_TAG_CLIENT_DOWNLOAD_TOTAL,
	EC_TAG_CLIENT_STATE,
	EC_TAG_CLIENT_UP_SPEED,
	EC_TAG_CLIENT_DOWN_SPEED,
	EC_TAG_CLIENT_FROM,
	

	//
	// Search
	//
		/*!
		 * \brief Info about file found in search
		 */
	EC_TAG_SEARCHFILE,
		 
		/*!
		 * \brief Type of search requested
		 * 
		 * Value: (\c uint32) EC_SEARCH_TYPE
		 */
	EC_TAG_SEARCH_TYPE,
	
		/*!
		 * \brief Search parameters
		 */
	EC_TAG_SEARCH_NAME,
	EC_TAG_SEARCH_MIN_SIZE,
	EC_TAG_SEARCH_MAX_SIZE,
	EC_TAG_SEARCH_FILE_TYPE,
	EC_TAG_SEARCH_EXTENSION,
	EC_TAG_SEARCH_AVAILABILITY,


	/*!
	 * \brief Status of search in core
	 */
	 EC_TAG_SEARCH_STATUS,


	//
	// Preferences
	//

		/*!
		 * \brief Selects which preference tables are to be returned.
		 *
		 * Value is (\c uint32) bitmask, OR-ing the EC_PREFS_* values.
		 */
	EC_TAG_SELECT_PREFS,


	//
	// Preferences - Categories
	//

		/*!
		 * \brief Categories
		 *
		 * Holds one or more ::EC_TAG_CATEGORY tags, depending on
		 * the current category count.
		 *
		 * \note When there are no categories, even the
		 * EC_TAG_PREFS_CATEGORIES tag is omitted from the response!
		 */
	EC_TAG_PREFS_CATEGORIES,

		/*!
		 * \brief A Category.
		 *
		 * Value: (\c uint32) Category index.
		 *
		 * \par Child TAGs:
		 *	::EC_TAG_CATEGORY_TITLE\n
		 *	::EC_TAG_CATEGORY_PATH\n
		 *	::EC_TAG_CATEGORY_COMMENT\n
		 *	::EC_TAG_CATEGORY_COLOR\n
		 *	::EC_TAG_CATEGORY_PRIO
		 */
	EC_TAG_CATEGORY,

	EC_TAG_CATEGORY_TITLE,		///< (\c string) Category title.
	EC_TAG_CATEGORY_PATH,		///< (\c string) Incoming files folder.
	EC_TAG_CATEGORY_COMMENT,	///< (\c string) Category comment.
	EC_TAG_CATEGORY_COLOR,		///< (\c uint32) Category color.
	EC_TAG_CATEGORY_PRIO,		///< (\c uint8) Category priority.


	//
	// Preferences - General
	//

		/*!
		* \brief Preferences - General.
		*
		* \par Child TAGs:
		*	::EC_TAG_USER_NICK\n
		*	::EC_TAG_USER_ID
		*/
	EC_TAG_PREFS_GENERAL,

	EC_TAG_USER_NICK,		///< (\c string) Nickname of the user.
	EC_TAG_USER_USERHASH,		///< (\c hash) Our userhash. Read only! (i.e. in a SET operation it will be ignored)


	//
	// Preferences - Connections
	//

		/*!
		* \brief Preferences - Connections.
		*
		* \par Child TAGs:
		*	::EC_TAG_CONN_DL_CAP\n
		*	::EC_TAG_CONN_UL_CAP\n
		*	::EC_TAG_CONN_MAX_DL\n
		*	::EC_TAG_CONN_MAX_UL\n
		*	::EC_TAG_CONN_SLOT_ALLOCATION\n
		*	::EC_TAG_CONN_TCP_PORT\n
		*	::EC_TAG_CONN_UDP_PORT\n
		*	::EC_TAG_CONN_UDP_DISABLE\n
		*	::EC_TAG_CONN_MAX_FILE_SOURCES\n
		*	::EC_TAG_CONN_MAX_CONN\n
		*	::EC_TAG_CONN_AUTOCONNECT\n
		*	::EC_TAG_CONN_RECONNECT\n
		*	::EC_TAG_NETWORK_ED2K\n
		*	::EC_TAG_NETWORK_KADEMLIA
		*/
	EC_TAG_PREFS_CONNECTIONS,

	EC_TAG_CONN_DL_CAP,		///< (\c uint32) Line download capacity.
	EC_TAG_CONN_UL_CAP,		///< (\c uint32) Line upload capacity.
	EC_TAG_CONN_MAX_DL,		///< (\c uint16) Maximum download speed.
	EC_TAG_CONN_MAX_UL,		///< (\c uint16) Maximum upload speed.
	EC_TAG_CONN_SLOT_ALLOCATION,	///< (\c uint16) Upload slot allocation.
	EC_TAG_CONN_TCP_PORT,		///< (\c uint16) TCP listening port number.
	EC_TAG_CONN_UDP_PORT,		///< (\c uint16) UDP listening port number.
	EC_TAG_CONN_UDP_DISABLE,	///< (boolean) Whether to disable UDP port or not.
	EC_TAG_CONN_MAX_FILE_SOURCES,	///< (\c uint16) Maximum sources per file allowed.
	EC_TAG_CONN_MAX_CONN,		///< (\c uint16) Maximum total connections allowed.
	EC_TAG_CONN_AUTOCONNECT,	///< (boolean) Whether to automatically connect at startup or not.
	EC_TAG_CONN_RECONNECT,		///< (boolean) Whether to reconnect to server upon connection loss or not.

	// Maybe these two could get a new category...
	EC_TAG_NETWORK_ED2K,		///< (boolean) Whether network ED2K is enabled or not.
	EC_TAG_NETWORK_KADEMLIA,	///< (boolean) Whether network Kademlia is enabled or not.


	//
	// Preferences - Message Filter
	//

		/*!
		* \brief Preferences - Message Filter.
		*
		* \par Child TAGs:
		*	::EC_TAG_MSGFILTER_ENABLED\n
		*	::EC_TAG_MSGFILTER_ALL\n
		*	::EC_TAG_MSGFILTER_FRIENDS\n
		*	::EC_TAG_MSGFILTER_SECURE\n
		*	::EC_TAG_MSGFILTER_BY_KEYWORD\n
		*	::EC_TAG_MSGFILTER_KEYWORDS
		*/
	EC_TAG_PREFS_MESSAGEFILTER,

	EC_TAG_MSGFILTER_ENABLED,	///< (boolean) Whether the Message Filter is enabled.
	EC_TAG_MSGFILTER_ALL,		///< (boolean) Whether to filter all messages.
	EC_TAG_MSGFILTER_FRIENDS,	///< (boolean) Whether to allow messages only from friends.
	EC_TAG_MSGFILTER_SECURE,	///< (boolean) Whether to filter messages from unknown clients.
	EC_TAG_MSGFILTER_BY_KEYWORD,	///< (boolean) Whether to filter messages by keywords.
	EC_TAG_MSGFILTER_KEYWORDS,	///< (\c string) Keyword list (comma separated).


	//
	// Preferences - Remote Controls
	//

		/*!
		* \brief WebServer Preferences (aka Remote Controls).
		*
		* \par Child TAGs:
		*	::EC_TAG_WEBSERVER_AUTORUN\n
		*	::EC_TAG_WEBSERVER_PORT (required)\n
		*	::EC_TAG_PASSWD_HASH (0-1) Admin Password. Omitted, when admin password is blank.\n
		*	::EC_TAG_WEBSERVER_GUEST (0-1)
		*/
	EC_TAG_PREFS_REMOTECTRL,

	EC_TAG_WEBSERVER_AUTORUN,	///< (boolean) Whether to run webserver on startup.
	EC_TAG_WEBSERVER_PORT,		///< (\c uint16) WebServer listening port
	EC_TAG_WEBSERVER_GUEST,		///< WebServer Guest account information
					/*!< (boolean) The presence of this tag means that the guest account is enabled.
						It contains an ::EC_TAG_PASSWD_HASH tag, if the guest
						password is not empty.
					*/
	EC_TAG_WEBSERVER_USEGZIP,	///< (boolean) When present, webserver uses gzip compression.
	EC_TAG_WEBSERVER_REFRESH,	///< (\c uint32) Refresh interval of web pages in seconds.


	//
	// Preferences - Online Signature
	//

		/*!
		* \brief Preferences - Online Signature.
		*
		* \par Child TAGs:
		*	::EC_TAG_ONLINESIG_ENABLED
		*/
	EC_TAG_PREFS_ONLINESIG,

	EC_TAG_ONLINESIG_ENABLED,	///< (boolean) Whether Online Signature is enabled.


	//
	// Preferences - Servers
	//

		/*!
		* \brief Preferences - Servers.
		*
		* \par Child TAGs:
		*	::EC_TAG_SERVERS_REMOVE_DEAD\n
		*	::EC_TAG_SERVERS_DEAD_SERVER_RETRIES\n
		*	::EC_TAG_SERVERS_AUTO_UPDATE\n
		*	::EC_TAG_SERVERS_URL_LIST\n
		*	::EC_TAG_SERVERS_ADD_FROM_SERVER\n
		*	::EC_TAG_SERVERS_ADD_FROM_CLIENT\n
		*	::EC_TAG_SERVERS_USE_SCORE_SYSTEM\n
		*	::EC_TAG_SERVERS_SMART_ID_CHECK\n
		*	::EC_TAG_SERVERS_SAFE_SERVER_CONNECT\n
		*	::EC_TAG_SERVERS_AUTOCONN_STATIC_ONLY\n
		*	::EC_TAG_SERVERS_MANUAL_HIGH_PRIO
		*/
	EC_TAG_PREFS_SERVERS,

	EC_TAG_SERVERS_REMOVE_DEAD,		///< (boolean) Remove dead servers.
	EC_TAG_SERVERS_DEAD_SERVER_RETRIES,	///< (\c uint16) Retries before removing dead servers.
	EC_TAG_SERVERS_AUTO_UPDATE,		///< (boolean) Auto-update server list on startup.
	EC_TAG_SERVERS_URL_LIST,		///< TODO: Implement this!
	EC_TAG_SERVERS_ADD_FROM_SERVER,		///< (boolean) Update server list on connecting to server.
	EC_TAG_SERVERS_ADD_FROM_CLIENT,		///< (boolean) Update server list on connecting to a client.
	EC_TAG_SERVERS_USE_SCORE_SYSTEM,	///< (boolean) Use priority system.
	EC_TAG_SERVERS_SMART_ID_CHECK,		///< (boolean) Smart LowID check.
	EC_TAG_SERVERS_SAFE_SERVER_CONNECT,	///< (boolean) Safe server connection.
	EC_TAG_SERVERS_AUTOCONN_STATIC_ONLY,	///< (boolean) Auto-connect to static servers only.
	EC_TAG_SERVERS_MANUAL_HIGH_PRIO,	///< (boolean) Set manually added servers to High Priority.


	//
	// Preferences - Files
	//

		/*!
		* \brief Preferences - Files.
		*
		* \par Child TAGs:
		*	::EC_TAG_FILES_ICH_ENABLED\n
		*	::EC_TAG_FILES_AICH_TRUST\n
		*	::EC_TAG_FILES_NEW_PAUSED\n
		*	::EC_TAG_FILES_NEW_AUTO_DL_PRIO\n
		*	::EC_TAG_FILES_PREVIEW_PRIO\n
		*	::EC_TAG_FILES_NEW_AUTO_UL_PRIO\n
		*	::EC_TAG_FILES_UL_FULL_CHUNKS\n
		*	::EC_TAG_FILES_START_NEXT_PAUSED\n
		*	::EC_TAG_FILES_RESUME_SAME_CAT\n
		*	::EC_TAG_FILES_SAVE_SOURCES\n
		*	::EC_TAG_FILES_EXTRACT_METADATA\n
		*	::EC_TAG_FILES_ALLOC_FULL_CHUNKS\n
		*	::EC_TAG_FILES_ALLOC_FULL_SIZE\n
		*	::EC_TAG_FILES_CHECK_FREE_SPACE\n
		*	::EC_TAG_FILES_MIN_FREE_SPACE
		*/
	EC_TAG_PREFS_FILES,

	EC_TAG_FILES_ICH_ENABLED,	///< (boolean) Is ICH enabled?
	EC_TAG_FILES_AICH_TRUST,	///< (boolean) Does AICH trust every hash?
	EC_TAG_FILES_NEW_PAUSED,	///< (boolean) Add new files paused.
	EC_TAG_FILES_NEW_AUTO_DL_PRIO,	///< (boolean) Auto-priority on new downloads.
	EC_TAG_FILES_PREVIEW_PRIO,	///< (boolean) Try to download first and last chunk first.
	EC_TAG_FILES_NEW_AUTO_UL_PRIO,	///< (boolean) Add new shared files with auto-priority.
	EC_TAG_FILES_UL_FULL_CHUNKS,	///< (boolean) Transfer full chunks in uploads.
	EC_TAG_FILES_START_NEXT_PAUSED,	///< (boolean) Start next paused file when a download completes.
	EC_TAG_FILES_RESUME_SAME_CAT,	///< (boolean) Start next paused file from the same category.
	EC_TAG_FILES_SAVE_SOURCES,	///< (boolean) Save 5 sources on rare files.
	EC_TAG_FILES_EXTRACT_METADATA,	///< (boolean) Extract metadata tags.
	EC_TAG_FILES_ALLOC_FULL_CHUNKS,	///< (boolean) Allocate full chunks.
	EC_TAG_FILES_ALLOC_FULL_SIZE,	///< (boolean) Aloocate full filesize.
	EC_TAG_FILES_CHECK_FREE_SPACE,	///< (boolean) Check disk free space.
	EC_TAG_FILES_MIN_FREE_SPACE,	///< (\c uint32) Minimum disk free space.


	//
	// Preferences - Source Dropping
	//

		/*!
		* \brief Preferences - Source Dropping.
		*
		* \par Child TAGs:
		*	::EC_TAG_SRCDROP_NONEEDED\n
		*	::EC_TAG_SRCDROP_DROP_FQS\n
		*	::EC_TAG_SRCDROP_DROP_HQRS\n
		*	::EC_TAG_SRCDROP_HQRS_VALUE\n
		*	::EC_TAG_SRCDROP_AUTODROP_TIMER
		*/
	EC_TAG_PREFS_SRCDROP,

	EC_TAG_SRCDROP_NONEEDED,	///< (\c uint8) How to handle no-needed-sources: 0: keep, 1: drop, 2: swap
	EC_TAG_SRCDROP_DROP_FQS,	///< (boolean) Whether to auto-drop Full Queue Sources
	EC_TAG_SRCDROP_DROP_HQRS,	///< (boolean) Whether to auto-drop High Queue Rating Sources
	EC_TAG_SRCDROP_HQRS_VALUE,	///< (\c uint16) When to count as *High* queue rating
	EC_TAG_SRCDROP_AUTODROP_TIMER,	///< (\c uint16) Timer for auto-dropping sources (in seconds)


	//
	// Preferences - Directories
	//

		/*!
		* \brief Preferences - Directories.
		*
		* \par Child TAGs:
		*/
	EC_TAG_PREFS_DIRECTORIES,
/* <---> */


	//
	// Preferences - Statistics
	//

		/*!
		* \brief Preferences - Statistics.
		*
		* \par Child TAGs:
		*/
	EC_TAG_PREFS_STATISTICS,
/* <---> */


	//
	// Preferences - Security
	//

		/*!
		* \brief Preferences - Security.
		*
		* \par Child TAGs:
		*	::EC_TAG_PREFS_SECURITY\n
		*	::EC_TAG_SECURITY_CAN_SEE_SHARES\n
		*	::EC_TAG_SECURITY_FILE_PERMISSIONS\n
		*	::EC_TAG_SECURITY_DIR_PERMISSIONS\n
		*	::EC_TAG_IPFILTER_ENABLED\n
		*	::EC_TAG_IPFILTER_AUTO_UPDATE\n
		*	::EC_TAG_IPFILTER_UPDATE_URL\n
		*	::EC_TAG_IPFILTER_LEVEL\n
		*	::EC_TAG_IPFILTER_FILTER_LAN\n
		*	::EC_TAG_SECURITY_USE_SECIDENT
		*/
	EC_TAG_PREFS_SECURITY,

	EC_TAG_SECURITY_CAN_SEE_SHARES,		///< (\c uint8) Who can see shared files?
						/*!< <ul>
							<li>0 - Everybody</li>
							<li>1 - Friends only</li>
							<li>2 - Nobody</li>
							</ul>
						*/
	EC_TAG_SECURITY_FILE_PERMISSIONS,	///< (\c uint32) Permissions on newly created files.
	EC_TAG_SECURITY_DIR_PERMISSIONS,	///< (\c uint32) Permissions on newly created directories.
	EC_TAG_IPFILTER_ENABLED,		///< (boolean) IPFilter enabled.
	EC_TAG_IPFILTER_AUTO_UPDATE,		///< (boolean) Auto-update IPFilter from URL at startup.
	EC_TAG_IPFILTER_UPDATE_URL,		///< (\c string) IPFilter auto-update URL.
	EC_TAG_IPFILTER_LEVEL,			///< (\c uint8) IPFilter filtering level.
	EC_TAG_IPFILTER_FILTER_LAN,		///< (boolean) Always filter LAN IPs.
	EC_TAG_SECURITY_USE_SECIDENT,		///< (boolean) Use Secure Identification.


	//
	// Preferences - Core Tweaks
	//

		/*!
		* \brief Preferences - Core Tweaks.
		*
		* \par Child TAGs:
		*	::EC_TAG_CORETW_MAX_CONN_PER_FIVE\n
		*	::EC_TAG_CORETW_VERBOSE\n
		*	::EC_TAG_CORETW_FILEBUFFER\n
		*	::EC_TAG_CORETW_UL_QUEUE\n
		*	::EC_TAG_CORETW_SRV_KEEPALIVE_TIMEOUT
		*/
	EC_TAG_PREFS_CORETWEAKS,

	EC_TAG_CORETW_MAX_CONN_PER_FIVE,	///< (\c uint16) Max connections per 5 seconds.
	EC_TAG_CORETW_VERBOSE,			///< (boolean) Verbose debug messages.
	EC_TAG_CORETW_FILEBUFFER,		///< (\c uint32) File buffer size.
	EC_TAG_CORETW_UL_QUEUE,			///< (\c uint32) Upload queue size.
	EC_TAG_CORETW_SRV_KEEPALIVE_TIMEOUT,	///< (\c uint32) Server connection keepalive timeout.


	//
	// Logging
	//

		/*!
		 * \brief Whether to add logline to statusline or not.
		 *
		 * The presence of this tag means (true), to add logline also
		 * to statusline.
		 */
	EC_TAG_LOG_TO_STATUS,

	//
	// Statistics
	//

	EC_TAG_STATSGRAPH_WIDTH,	///< (\c uint16) Maximum number of sample points (for webserver only)
	EC_TAG_STATSGRAPH_SCALE,	///< (\c uint16) Time between sample point (in seconds) (for webserver only)
	EC_TAG_STATSGRAPH_LAST,		///< (\c double) Timestamp of last acquired/sent history item. Default: 0.0
	EC_TAG_STATSGRAPH_DATA,		///< sequence of uint32 triplets for the webserver (dl,ul,conn); ...

		/*!
		 * \brief Describes how stattree capping should be done.
		 *
		 * Holds an \c uint8 value, and defaults to 0 (unlimited).
		 *
		 * Its value is passed to CStatTreItemBase::CreateECTag(), so tree items
		 * which support capping know how many children should they provide at most.
		 * (It is currently used only on client versions.)
		 */
	EC_TAG_STATTREE_CAPPING,

		/*!
		 * \brief A node (tree item/subtree root) for statistics.
		 *
		 * Value: (\c string) Stats label.
		 *
		 * \par Possible children:
		 *	::EC_TAG_STAT_NODE_VALUE\n
		 *	::EC_TAG_STATTREE_NODE
		 */
	EC_TAG_STATTREE_NODE,
	EC_TAG_STAT_NODE_VALUE,		///< A value for this node.
					/*!< May conatin an ::EC_TAG_STAT_VALUE_TYPE tag, describing its contents, otherwise it is interpreted as an \c uint64 integer.
					 *
					 *   May also contain a ::EC_TAG_STAT_NODE_VALUE child, and that will be displayed in brackets after this value.
					 *   This only works for value types that require a \c %s format string.
					 */
	EC_TAG_STAT_VALUE_TYPE		///< \c uint8 value describing the value of its parent (::EC_TAG_STAT_NODE_VALUE). See also the ::EC_STATTREE_NODE_VALUE_TYPE enum.
};


/*!
 * EC detail level values
 *
 * All higher levels contain all the lower level information aswell.
 */
enum EC_DETAIL_LEVEL {
	EC_DETAIL_CMD,		///< aMuleCmd uses this level to obtain only basic information
	EC_DETAIL_WEB,		///< aMuleWeb uses this level to obtain specific information/specific format
	
	/*
	 * Send either full info or fields that usually changes (speed, source count, etc).
	 * This algorithm doesn't use match memory, but consumes more bandwidth.
	 */
	EC_DETAIL_FULL,
	EC_DETAIL_UPDATE,
	
	/*
	 * Send only tags that has been changed since last request. This algorithm
	 * uses twice memory as previous, but saves bandwidth. Good for remote gui
	 */
	EC_DETAIL_INC_UPDATE
};


/*!
 * Search type
 */
 
enum EC_SEARCH_TYPE {
	EC_SEARCH_LOCAL,
	EC_SEARCH_GLOBAL,
	EC_SEARCH_KAD,
	EC_SEARCH_WEB
};

/**
 * EC value types for stattree nodes.
 */
enum EC_STATTREE_NODE_VALUE_TYPE {
	EC_VALUE_INTEGER,	///< value is an \c uint64 integer. The ::EC_TAG_STAT_VALUE_TYPE node may be omitted in this case. Format code is \c %u or similar.
	EC_VALUE_ISTRING,	///< value is an \c uint64 integer. The only difference from ::EC_VALUE_INTEGER is that this reqires a \c %s format code.
	EC_VALUE_BYTES,		///< value is an \c uint64 representing bytes, format is \c %s.
	EC_VALUE_ISHORT,	///< value is an \c uint64 integer, format is \c %s. Value will be displayed in a human-readable short form.
	EC_VALUE_TIME,		///< value is an \c uint32 representing time in seconds, format is \c %s.
	EC_VALUE_SPEED,		///< value is an \c uint32 representing speed in bytes/second, format is \c %s.
	EC_VALUE_STRING,	///< \c string type value, with \c %s format
	EC_VALUE_DOUBLE		///< floating-point (\c double) type value. Format code is \c %g or similar.
};


/*
 * EC Preferences selection bit values.
 */

#define EC_PREFS_CATEGORIES	0x00000001
#define EC_PREFS_GENERAL	0x00000002
#define EC_PREFS_CONNECTIONS	0x00000004
#define EC_PREFS_MESSAGEFILTER	0x00000008
#define EC_PREFS_REMOTECONTROLS	0x00000010
#define EC_PREFS_ONLINESIG	0x00000020
#define EC_PREFS_SERVERS	0x00000040
#define EC_PREFS_FILES		0x00000080
#define EC_PREFS_SRCDROP	0x00000100
#define EC_PREFS_DIRECTORIES	0x00000200
#define EC_PREFS_STATISTICS	0x00000400
#define EC_PREFS_SECURITY	0x00000800
// there's no need to do GUI Tweaks on EC :)
#define EC_PREFS_CORETWEAKS	0x00001000
// Is there any need to change Debug prefs via EC?

#endif	/* ECCODES_H */
