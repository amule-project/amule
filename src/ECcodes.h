/*
 * Copyright (C) 2004 aMule Team (http://www.amule.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/*!
 * \file ECcodes.h
 *
 * \brief EC types, OPCODEs, TAGNAMEs.
 */

#ifndef	ECCODES_H
#define	ECCODES_H

#pragma interface

/*
 * EC types
 */

typedef uint8 ec_opcode_t;
typedef uint16 ec_tagname_t;
typedef uint32 ec_taglen_t;


/**
 * Class to hold IPv4 address.
 */
class EC_IPv4_t {
	public:
		EC_IPv4_t() { }
		EC_IPv4_t(uint32 ip, uint16 port)
		{
			EC_IPv4_t::ip[0] = ip & 0xff;
			EC_IPv4_t::ip[1] = (ip >> 8) & 0xff;
			EC_IPv4_t::ip[2] = (ip >> 16) & 0xff;
			EC_IPv4_t::ip[3] = (ip >> 24) & 0xff;
			EC_IPv4_t::port = port;
		}
		
		uint32 IP()
		{
			return ip[0] | (ip[1] << 8) | (ip[2] << 16) | (ip[3] << 24);
		}

		wxString StringIP(bool brackets = true)
		{
			return wxString::Format(brackets ? wxT("[%d.%d.%d.%d:%d]") : wxT("%d.%d.%d.%d : %d"), ip[0], ip[1], ip[2], ip[3], port);
		}
		
		uint8 ip[4];
		uint16 port;
};


/**
 * <b>EC opcodes</b>
 *
 * OpCodes tell the receiver what to do (with the provided data).
 */

// If you plan to use opcodes greater than 0x00ff, please
// change the size of ec_opcode_t to uint16.

enum {

		/*!
		 * \brief Perform no operation, aka do nothing.
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
		 *	::EC_TAG_PROTOCOL_VERSION (required)
		 */
	EC_OP_AUTH_REQ,

		/*!
		 * \brief Authentication failed.
		 *
		 * \par Tags:
		 *	::EC_TAG_STRING (optional) describing the reason why authentication was rejected.
		 */
	EC_OP_AUTH_FAIL,

		/*!
		 * \brief Authentication succeeded.
		 *
		 * \par Tags:
		 *	(none)
		 */
	EC_OP_AUTH_OK,

	//
	// Misc commands
	//

		/*!
		 * \brief General error reply.
		 */
	EC_OP_FAILED,

		/*!
		 * \brief Used when need to transfer text message without
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
		 *	\b ::EC_TAG_STRING (1+) holding ED2k link.
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
		 *  \b ::EC_TAG_STATS_*
		 */
	EC_OP_STATS,


	//
	//
	//

		/*!
		 * \brief Request for download queue.
		 *
		 * \par Tags:
		 *  ::EC_TAG_PARTFILE (*) info can be requested for selected list only
		 */
	EC_OP_GET_DLOAD_QUEUE,
	
		/*!
		 * Same as EC_OP_GET_DLOAD_QUEUE but only status and parts status are returned
		 */
	EC_OP_GET_DLOAD_QUEUE_STATUS,

		/*!
		 * \brief Same as EC_OP_GET_DLOAD_QUEUE, but parts status returned for every file
		 * 
		 */
	EC_OP_GET_DLOAD_QUEUE_PARTS_STATUS,
	
		/*!
		 * \brief Request for upload queue - currect uploads
		 *
		 * \par Tags:
		 *  ::EC_TAG_PARTFILE (*) status can be requested for selected list only
		 */
	EC_OP_GET_ULOAD_QUEUE,

		/*
		 * \brief Perform action on file in queue.
		 *
		 * \par Tags:
		 *	\b ::EC_TAG_ITEM_ID (1+) item (object) to perform command on
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
	EC_OP_PARTFILE_PRIO_AUTO,
	EC_OP_PARTFILE_PRIO_SET,
	EC_OP_PARTFILE_DELETE,
	EC_OP_PARTFILE_SET_CAT,
	EC_OP_KNOWNFILE_SET_UP_PRIO,
	EC_OP_KNOWNFILE_SET_UP_PRIO_AUTO,
	EC_OP_KNOWNFILE_SET_PERM,
	EC_OP_KNOWNFILE_SET_COMMENT,

		/*!
		 * \brief Get download queue.
		 *
		 * \par Tags:
		 *	\b ::EC_TAG_PARTFILE (1+) info about file in download queue
		 */
	EC_OP_DLOAD_QUEUE,
		
		/*!
		 * \brief Get upload queue.
		 *
		 * \par Tags:
		 *	\b ::EC_TAG_UPDOWN_CLIENT (1+) info about client in queue
		 */
	EC_OP_ULOAD_QUEUE,

		/*
		 * \brief Reloads the shared files list.
		 */
	EC_OP_SHAREDFILES_RELOAD,


	//
	// IPFilter
	//

		/*!
		 * \brief Change/query IPFilter settings: on, off, level, reload
		 *
		 * Set/get IPFilter settings, both TAGs may be present or omitted.
		 * When used without TAGs, just query current IPFilter level.
		 * The server will reply with an ::EC_OP_MISC_DATA packet, with
		 * EC_TAG_IPFILTER_* tags, that contain the current IPFilter
		 * values.
		 *
		 * \par Tags:
		 *	::EC_TAG_IPFILTER_STATUS (0-1) Sets IPFilter ON, OFF or RELOAD.\n
		 *	::EC_TAG_IPFILTER_LEVEL (0-1) Sets IPFilter level.
		 */
	EC_OP_IPFILTER_CMD,


	//
	// Server commands
	//

		/*!
		 * \brief Request list of servers
		 */
	EC_OP_GET_SERVER_LIST,
	
		/*!
		 * \brief List of servers, reply to EC_OP_GET_SERVER_LIST
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
		 *	::EC_TAG_SERVER (1) identified the server to be removed
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
		 * \brief Setting the Preferences.
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
		 *	::EC_TAG_PREFS_REMOTECONTROLS\n
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
		 * \brief Preference values.
		 *
		 * It's tags and contents are depending on
		 * what preferences were actually required.
		 *
		 * \par Possible tags:
		 *	::EC_TAG_PREFS_CATEGORIES\n
		 *	::EC_TAG_PREFS_GENERAL\n
		 *	::EC_TAG_PREFS_CONNECTIONS\n
		 *	::EC_TAG_PREFS_MESSAGEFILTER\n
		 *	::EC_TAG_PREFS_REMOTECONTROLS\n
		 *	::EC_TAG_PREFS_ONLINESIG\n
		 *	::EC_TAG_PREFS_SERVERS\n
		 *	::EC_TAG_PREFS_FILES\n
		 *	::EC_TAG_PREFS_SRCDROP\n
		 *	::EC_TAG_PREFS_DIRECTORIES\n
		 *	::EC_TAG_PREFS_STATISTICS\n
		 *	::EC_TAG_PREFS_SECURITY\n
		 *	::EC_TAG_PREFS_CORETWEAKS
		 */
	EC_OP_PREFERENCES,


	EC_OP_COMPAT	= 0x00ff	// compatibility opcode, for testing purposes only
					// tags: EC_TAG_STRING: v1.0 message
};

/*!
 * Turn pointer into 32-bit thingie int aka ID
 */
#define PTR_2_ID(ptr) ((uint32)ptr)

/******************************************************************************/

/**
 * <b>EC TAG names</b>
 *
 * TAGs hold the data, on/with the requested operation is to be performed.
 */

// Please do not use tagnames greater than 0x7fff, beacuse that
// would mess up the protocol.

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
		 * An ::EC_TAG_STRING, dedicated to hold the password hash.
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

	EC_TAG_ITEM_ID,

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
		 * Possible values (\c uint8):
		 *	<ul>
		 *		<li>0 - Not Connected</li>
		 *		<li>1 - Connecting</li>
		 *		<li>2 - Connected with LowID</li>
		 *		<li>3 - Connected with HighID</li>
		 *	</ul>
		 *
		 * When connected, it contains an ::EC_TAG_SERVER child, describing the
		 * server we're connected to.
		 */
	EC_TAG_CONNSTATE,
	EC_TAG_STATS_UL_SPEED,
	EC_TAG_STATS_DL_SPEED,
	/*!
	 *  \brief Up/Down speed/limit
	 * 
	 * Values (uint32): speed in bps
	 */
	EC_TAG_STATS_UL_SPEED_LIMIT,
	EC_TAG_STATS_DL_SPEED_LIMIT,
	/*!
	 * \brief Number of activel up/downloads
	 * 
	 */
	EC_TAG_STATS_CURR_UL_COUNT,
	EC_TAG_STATS_CURR_DL_COUNT,
	/*!
	 * \brief Total number of file in download queue
	 */
	EC_TAG_STATS_TOTAL_DL_COUNT,
	/*!
	 * \brief Total number of sources found
	 */
	EC_TAG_STATS_TOTAL_SRC_COUNT,
	/*!
	 * \brief Number of users in upload queue
	 */
	EC_TAG_STATS_UL_QUEUE_LEN,
	
	/*!
	 * \brief Number of users on currectly conected server
	 */
	EC_TAG_STATS_USERS_ON_SERVER,


	//
	// Partfile
	//
	
	/*!
	 * \brief Info about CPartFile
	 * 
	 * Value (string): file name (the full one)
	 * 
	 * \par Chld TAGs:
	 *  ::EC_TAG_PARTFILE_*
	 */
	EC_TAG_PARTFILE,

	EC_TAG_PARTFILE_NAME,
	EC_TAG_PARTFILE_SIZE_FULL,
	EC_TAG_PARTFILE_SIZE_XFER,
	EC_TAG_PARTFILE_SIZE_DONE,
	EC_TAG_PARTFILE_SPEED,
	EC_TAG_PARTFILE_STATUS,
	EC_TAG_PARTFILE_PRIO,
	EC_TAG_PARTFILE_SOURCE_COUNT,
	EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT,
	EC_TAG_PARTFILE_SOURCE_COUNT_XFER,
	EC_TAG_PARTFILE_ED2K_LINK,

	/*!
	 * This tag contain info about status of gaps in PartFile and availability of each part.
	 * 
	 * Value (string): status of each part
	 */	
	EC_TAG_PARTFILE_PART_STATUS,


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
	 * Value (string): name,id of connected user
	 * 
	 * \par Child TAGs:
	 *  ::EC_TAG_PARTFILE
	 *  ::EC_TAG_PARTFILE_SIZE_XFER
	 *  ::EC_TAG_PARTFILE_SPEED
	 */
	EC_TAG_UPDOWN_CLIENT,


	//
	// IPFilter
	//

		/*!
		 * \brief Status of IPFilter (current/desired)
		 *
		 * Values (uint8):
		 * <ul>
		 *	<li>0 - OFF</li>
		 *	<li>1 - ON</li>
		 *	<li>2 - RELOAD</li>
		 * </ul>
		 *
		 * \par Child TAGs:
		 *	(none)
		 */
	EC_TAG_IPFILTER_STATUS,
		/*!
		 * \brief IPFilter level
		 *
		 * In an ::EC_OP_IPFILTER_CMD packet, it sets the iplevel
		 * to its contents (uint8), in an ::EC_OP_MISC_DATA packet
		 * it holds the current ip level.
		 *
		 * \par Child TAGs:
		 *	(none)
		 */
	EC_TAG_IPFILTER_LEVEL,


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

		/*
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

		/*
		* \brief Preferences - General.
		*
		* \par Child TAGs:
		*/
	EC_TAG_PREFS_GENERAL,
/* <---> */


	//
	// Preferences - Connections
	//

		/*
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
		*	::EC_TAG_CONN_RECONNECT
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


	//
	// Preferences - Message Filter
	//

		/*
		* \brief Preferences - Message Filter.
		*
		* \par Child TAGs:
		*/
	EC_TAG_PREFS_MESSAGEFILTER,
/* <---> */


	//
	// Preferences - Remote Controls
	//

		/*
		* \brief WebServer Preferences (aka Remote Controls).
		*
		* \par Child TAGs:
		*	::EC_TAG_WEBSERVER_PORT (required)\n
		*	::EC_TAG_PASSWD_HASH (0-1) Admin Password. Omitted, when admin password is blank.\n
		*	::EC_TAG_WEBSERVER_GUEST (0-1)
		*/
	EC_TAG_PREFS_REMOTECTRL,
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

		/*
		* \brief Preferences - Online Signature.
		*
		* \par Child TAGs:
		*/
	EC_TAG_PREFS_ONLINESIG,
/* <---> */


	//
	// Preferences - Servers
	//

		/*
		* \brief Preferences - Servers.
		*
		* \par Child TAGs:
		*/
	EC_TAG_PREFS_SERVERS,
/* <---> */


	//
	// Preferences - Files
	//

		/*
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
	EC_TAG_FILES_SAVE_SOURCES,	///< (boolean) Save 5 sources on rare files.
	EC_TAG_FILES_EXTRACT_METADATA,	///< (boolean) Extract metadata tags.
	EC_TAG_FILES_ALLOC_FULL_CHUNKS,	///< (boolean) Allocate full chunks.
	EC_TAG_FILES_ALLOC_FULL_SIZE,	///< (boolean) Aloocate full filesize.
	EC_TAG_FILES_CHECK_FREE_SPACE,	///< (boolean) Check disk free space.
	EC_TAG_FILES_MIN_FREE_SPACE,	///< (\c uint32) Minimum disk free space. Omitted, when CHECK_FREE_SPACE is false.


	//
	// Preferences - Source Dropping
	//

		/*
		* \brief Preferences - Source Dropping.
		*
		* \par Child TAGs:
		*/
	EC_TAG_PREFS_SRCDROP,
/* <---> */


	//
	// Preferences - Directories
	//

		/*
		* \brief Preferences - Directories.
		*
		* \par Child TAGs:
		*/
	EC_TAG_PREFS_DIRECTORIES,
/* <---> */


	//
	// Preferences - Statistics
	//

		/*
		* \brief Preferences - Statistics.
		*
		* \par Child TAGs:
		*/
	EC_TAG_PREFS_STATISTICS,
/* <---> */


	//
	// Preferences - Security
	//

		/*
		* \brief Preferences - Security.
		*
		* \par Child TAGs:
		*/
	EC_TAG_PREFS_SECURITY,
/* <---> */


	//
	// Preferences - Core Tweaks
	//

		/*
		* \brief Preferences - Core Tweaks.
		*
		* \par Child TAGs:
		*	::EC_TAG_CORETW_MAX_CONN_PER_FIVE\n
		*	::EC_TAG_CORETW_SAFE_MAXCONN\n
		*	::EC_TAG_CORETW_VERBOSE\n
		*	::EC_TAG_CORETW_VERBOSE_PACKET\n
		*	::EC_TAG_CORETW_FILEBUFFER\n
		*	::EC_TAG_CORETW_UL_QUEUE\n
		*	::EC_TAG_CORETW_SRV_KEEPALIVE_TIMEOUT
		*/
	EC_TAG_PREFS_CORETWEAKS,
	EC_TAG_CORETW_MAX_CONN_PER_FIVE,	///< (\c uint16) Max connections per 5 seconds.
	EC_TAG_CORETW_SAFE_MAXCONN,		///< (boolean) Safe max connections.
	EC_TAG_CORETW_VERBOSE,			///< (boolean) Verbose debug messages.
	EC_TAG_CORETW_VERBOSE_PACKET,		///< (boolean) Verbose packet debug messages.
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
};


/*!
 * EC detail level values
 *
 * All higher levels contain all the lower level information aswell.
 */
enum EC_DETAIL_LEVEL {
	EC_DETAIL_CMD,		///< aMuleCmd uses this level to obtain only basic information
	EC_DETAIL_WEB,		///< aMuleWeb uses this level to obtain represenative (but not full) information
	EC_DETAIL_GUI,		///< the (upcoming) remote gui will use this (default) value, to obtain full information
	EC_DETAIL_UPDATE,   ///< return only fields that constatly change (rate, ping, part status)
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

#endif	/* ECCODES_H */
