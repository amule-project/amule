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

/*
 * EC types
 */

typedef uint8 ec_opcode_t;
typedef uint16 ec_tagname_t;
typedef uint32 ec_taglen_t;


/**
 * Type to hold IPv4 address.
 */
typedef struct {
	uint8 ip[4];
	uint16 port;
} EC_IPv4_t;


/**
 * <b>EC opcodes</b>
 *
 * OpCodes tell the receiver what to do (with the provided data).
 */

// If you plan to use opcodes greater than 0x00ff, please
// change the size of ec_opcode_t to uint16.

enum {

		/*!
		 * \brief Perform no operation.
		 *
		 * \par Tags:
		 *	(none)
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
	// general commands - both way
	//

		/*!
		 * \brief Used when need to transfer text message without
		 * particular meaning - like logging message in reply to command
		 *
		 * \par Tags:
		 *	::EC_TAG_STRING (1) string message
		 */
	EC_OP_STRINGS,

		/*!
		 * \brief Miscanelous data.
		 *
		 * \par Tags:
		 *	(any tag)
		 */
	EC_OP_MISC_DATA,

	//
	// client -> server commands
	//

		/*!
		 * \brief Request to shut down aMule.
		 *
		 * \par Tags:
		 *	(none)
		 */
	EC_OP_SHUTDOWN,

		/*!
		 * \brief Request statistics.
		 *
		 * Request from client, server must reply with statistics string.
		 *
		 * \par Tags:
		 *	(none)
		 */
	EC_OP_STAT_REQ,
	
		/*!
		 * \brief Reply to stats request
		 * 
		 * Status information about files/users/limits/speeds etc
		 * 
		 * \par Tags:
		 *  \b ::EC_TAG_STATS_*
		 */
	EC_OP_STATS,

		/*!
		 * \brief Handle ED2k link.
		 *
		 * \par Tags:
		 *	\b ::EC_TAG_STRING (1+) holding ED2k link.
		 */
	EC_OP_ED2K_LINK,

		/*!
		 * \brief Request for download queue.
		 *
		 * \par Tags:
		 *	(none)
		 */
	EC_OP_GET_DLOAD_QUEUE,

		/*!
		 * \brief Request for upload queue - currect uploads
		 *
		 * \par Tags:
		 *	(none)
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

	//
	// server -> client response
	//

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

	/*!
	 * \brief Request list of servers
	 */
	EC_OP_GET_SERVER_LIST,
	
	/*!
	 * \brief List of servers, reply to EC_OP_GET_SERVER_LIST
	 * 
	 * \par Tags:
	 *  ::EC_TAG_SERVER (1+)
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
	 *  ::EC_TAG_ITEM_ID (0-1) identifies server to use
	 */
	EC_OP_SERVER_CONNECT,

	EC_OP_SERVER_REMOVE,

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
	 * \brief Tags for status (or stats) info
	 */
	/*!
	 * \brief Connection status: high/low id or disconnected
	 * 
	 * Values (uin32): ed2k ID, 0 if disconnected, 0xffffffff if connection in progress
	 */
	EC_TAG_STATS_ED2K_ID,
	EC_TAG_STATS_SERVER,

	/*!
	 * \brief Connection state.
	 *
	 * Possible values (uint8):
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
	EC_TAG_STATS_CONNSTATE,
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
	
	/*!
	 * \brief Info about CPartFile
	 * 
	 * Value (string): file name (the full one)
	 * 
	 * \par Chld TAGs:
	 *  ::EC_TAG_PARTFILE_*
	 */
	EC_TAG_PARTFILE,

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
	 * \brief Info about CKnownFile (shared file)
	 * 
	 * Value (string): (the full one)
	 * 
	 * \par Chld TAGs:
	 *  ::EC_TAG_KNOWNFILE_*
	 */
	EC_TAG_KNOWNFILE,
	EC_TAG_KNOWNFILE_ED2K_LINK,
	EC_TAG_KNOWNFILE_PRIO,
	EC_TAG_KNOWNFILE_XFERRED,
	EC_TAG_KNOWNFILE_REQ_COUNT,
	EC_TAG_KNOWNFILE_ACCEPT_COUNT,
	
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
	 *	::EC_TAG_SERVER_PREF\n
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
	EC_TAG_SERVER_PREF,		///< (\c uint8) Server preference (priority). Default: 1 (normal)
					/*!< <ul><li>0 - Low Priority</li>
						<li>1 - Normal</li>
						<li>2 - High Priority</li></ul>
					*/
	EC_TAG_SERVER_FAILED,		///< (\c uint8) Fail count. Default: 0
	EC_TAG_SERVER_STATIC,		///< (\c uint8) Nonzero, when server is static. Default: 0 (not static)
	EC_TAG_SERVER_VERSION,		///< (\c string) Server version. Default: unknown.

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

};

#endif	/* ECCODES_H */
