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


#ifndef	ECCODES_H
#define	ECCODES_H

/*
 * EC types
 */

typedef uint8 ec_opcode_t;
typedef uint16 ec_tagname_t;
typedef uint16 ec_taglen_t;

/**
 * Type to hold EC Protocol Version information
 */
typedef struct {
	uint8	major;
	uint8	minor;
} EC_Version_t;


/*
 * EC opcodes
 *
 * If you plan to use opcodes greater than 0x00ff, please
 * change the size of ec_opcode_t to uint16.
 */

enum {
	// authentication
	EC_OP_AUTH_REQ	= 0x0001,
	EC_OP_AUTH_FAIL,
	EC_OP_AUTH_OK,
	
	// generic commands - both way
	EC_OP_STRINGS,
	EC_OP_MISC_DATA,

	// client -> server commands
	
	EC_OP_SHUTDOWN, // stop server
	EC_OP_STAT_REQ, // request statistics
	EC_OP_ED2K_LINK, // handle ed2k link
	
	EC_OP_GET_DLOAD_QUEUE, // request download queue contents
	
	EC_OP_Q_FILE_CMD, // perform action on file in queue

	// server -> client response
	EC_OP_DLOAD_QUEUE, // download queue contents
	
	EC_OP_COMPAT	= 0x00ff	// compatibility opcode, for testing purposes only
					// tags: EC_TAG_STRING: v1.0 message
};


/*
 * EC TAG names
 *
 * Please do not use tagnames greater than 0x7fff, beacuse that
 * would mess up the protocol.
 */

enum {
	EC_TAG_STRING		= 0x0001,
	EC_TAG_PASSWD_HASH	= 0x0002,
	EC_TAG_CLIENT_NAME	= 0x0003,
	EC_TAG_CLIENT_VERSION	= 0x0004,
	EC_TAG_CLIENT_MOD	= 0x0005,
	EC_TAG_PROTOCOL_VERSION	= 0x0006,
	
	// tags for partfile info
	EC_TAG_PARTFILE = 0x0007,
	EC_TAG_PARTFILE_SIZE_FULL,
	EC_TAG_PARTFILE_SIZE_XFER,
	EC_TAG_PARTFILE_SIZE_DONE,
	EC_TAG_PARTFILE_DOWN_SPEED,
	EC_TAG_PARTFILE_STATUS,
	EC_TAG_PARTFILE_PRIO,
	EC_TAG_PARTFILE_SOURCE_COUNT,
	EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT,
	EC_TAG_PARTFILE_SOURCE_COUNT_XFER,
	EC_TAG_PARTFILE_ED2K_LINK,
};

#endif	/* ECCODES_H */
