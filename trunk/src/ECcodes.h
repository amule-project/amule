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
 * EC opcodes
 */

enum {
    EC_OP_AUTH_REQ	= 0x0001,
    EC_OP_AUTH_FAIL	= 0x0002,
    EC_OP_AUTH_OK	= 0x0003,
    EC_OP_STRINGS	= 0x0004,
    EC_OP_MISC_DATA	= 0x0005,

    EC_OP_COMPAT	= 0x00ff	// compatibility opcode, for testing purposes only
					// tags: EC_TAG_STRING: v1.0 message
};


/*
 * EC TAG names
 */

enum {
    EC_TAG_STRING		= 0x0001,
    EC_TAG_PASSWD_HASH		= 0x0002,
    EC_TAG_CLIENT_NAME		= 0x0003,
    EC_TAG_PROTOCOL_VERSION	= 0x0004
};

#endif	/* ECCODES_H */
