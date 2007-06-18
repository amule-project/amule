//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef FILETAGS_H
#define FILETAGS_H

// ED2K search + known.met + .part.met
#define	FT_FILENAME			0x01	// <string>
#define	FT_FILESIZE			0x02	// <uint32>
#define	FT_FILESIZE_HI			0x3A	// <uint32>
#define	FT_FILETYPE			0x03	// <string> or <uint32>
#define	FT_FILEFORMAT			0x04	// <string>
#define	FT_LASTSEENCOMPLETE		0x05	// <uint32>
#define	FT_TRANSFERED			0x08	// <uint32>
#define	FT_GAPSTART			0x09	// <uint32>
#define	FT_GAPEND			0x0A	// <uint32>
#define	FT_PARTFILENAME			0x12	// <string>
#define	FT_OLDDLPRIORITY		0x13	// Not used anymore
#define	FT_STATUS			0x14	// <uint32>
#define	FT_SOURCES			0x15	// <uint32>
#define	FT_PERMISSIONS			0x16	// <uint32>
#define	FT_OLDULPRIORITY		0x17	// Not used anymore
#define	FT_DLPRIORITY			0x18	// Was 13
#define	FT_ULPRIORITY			0x19	// Was 17
#define	FT_KADLASTPUBLISHKEY		0x20	// <uint32>
#define	FT_KADLASTPUBLISHSRC		0x21	// <uint32>
#define	FT_FLAGS			0x22	// <uint32>
#define	FT_DL_ACTIVE_TIME		0x23	// <uint32>
#define	FT_CORRUPTEDPARTS		0x24	// <string>
#define	FT_DL_PREVIEW			0x25
#define	FT_KADLASTPUBLISHNOTES		0x26	// <uint32> 
#define	FT_AICH_HASH			0x27
#define	FT_COMPLETE_SOURCES		0x30	// nr. of sources which share a
						// complete version of the
						// associated file (supported
						// by eserver 16.46+) statistic

#define	FT_ATTRANSFERED			0x50	// <uint32>
#define	FT_ATREQUESTED			0x51	// <uint32>
#define	FT_ATACCEPTED			0x52	// <uint32>
#define	FT_CATEGORY			0x53	// <uint32>
#define	FT_ATTRANSFEREDHI		0x54	// <uint32>
#define	FT_MEDIA_ARTIST			0xD0	// <string>
#define	FT_MEDIA_ALBUM			0xD1	// <string>
#define	FT_MEDIA_TITLE			0xD2	// <string>
#define	FT_MEDIA_LENGTH			0xD3	// <uint32> !!!
#define	FT_MEDIA_BITRATE		0xD4	// <uint32>
#define	FT_MEDIA_CODEC			0xD5	// <string>
#define	FT_FILERATING			0xF7	// <uint8>


// Kad search + some unused tags to mirror the ed2k ones.
#define	TAG_FILENAME			wxT("\x01")	// <string>
#define	TAG_FILESIZE			wxT("\x02")	// <uint32>
#define	TAG_FILESIZE_HI			wxT("\x3A")	// <uint32>
#define	TAG_FILETYPE			wxT("\x03")	// <string>
#define	TAG_FILEFORMAT			wxT("\x04")	// <string>
#define	TAG_COLLECTION			wxT("\x05")
#define	TAG_PART_PATH			wxT("\x06")	// <string>
#define	TAG_PART_HASH			wxT("\x07")
#define	TAG_COPIED			wxT("\x08")	// <uint32>
#define	TAG_GAP_START			wxT("\x09")	// <uint32>
#define	TAG_GAP_END			wxT("\x0A")	// <uint32>
#define	TAG_DESCRIPTION			wxT("\x0B")	// <string>
#define	TAG_PING			wxT("\x0C")
#define	TAG_FAIL			wxT("\x0D")
#define	TAG_PREFERENCE			wxT("\x0E")
#define	TAG_PORT			wxT("\x0F")
#define	TAG_IP_ADDRESS			wxT("\x10")
#define	TAG_VERSION			wxT("\x11")	// <string>
#define	TAG_TEMPFILE			wxT("\x12")	// <string>
#define	TAG_PRIORITY			wxT("\x13")	// <uint32>
#define	TAG_STATUS			wxT("\x14")	// <uint32>
#define	TAG_SOURCES			wxT("\x15")	// <uint32>
#define	TAG_AVAILABILITY		wxT("\x15")	// <uint32>
#define	TAG_PERMISSIONS			wxT("\x16")
#define	TAG_QTIME			wxT("\x16")
#define	TAG_PARTS			wxT("\x17")
#define	TAG_MEDIA_ARTIST		wxT("\xD0")	// <string>
#define	TAG_MEDIA_ALBUM			wxT("\xD1")	// <string>
#define	TAG_MEDIA_TITLE			wxT("\xD2")	// <string>
#define	TAG_MEDIA_LENGTH		wxT("\xD3")	// <uint32> !!!
#define	TAG_MEDIA_BITRATE		wxT("\xD4")	// <uint32>
#define	TAG_MEDIA_CODEC			wxT("\xD5")	// <string>
#define	TAG_ENCRYPTION			wxT("\xF3")	// <uint8>
#define	TAG_FILERATING			wxT("\xF7")	// <uint8>
#define	TAG_BUDDYHASH			wxT("\xF8")	// <string>
#define	TAG_CLIENTLOWID			wxT("\xF9")	// <uint32>
#define	TAG_SERVERPORT			wxT("\xFA")	// <uint16>
#define	TAG_SERVERIP			wxT("\xFB")	// <uint32>
#define	TAG_SOURCEUPORT			wxT("\xFC")	// <uint16>
#define	TAG_SOURCEPORT			wxT("\xFD")	// <uint16>
#define	TAG_SOURCEIP			wxT("\xFE")	// <uint32>
#define	TAG_SOURCETYPE			wxT("\xFF")	// <uint8>

// Media values for FT_FILETYPE
#define	ED2KFTSTR_AUDIO			wxT("Audio")	
#define	ED2KFTSTR_VIDEO			wxT("Video")	
#define	ED2KFTSTR_IMAGE			wxT("Image")	
#define	ED2KFTSTR_DOCUMENT		wxT("Doc")	
#define	ED2KFTSTR_PROGRAM		wxT("Pro")	
#define	ED2KFTSTR_ARCHIVE		wxT("Arc")	// *Mule internal use only
#define	ED2KFTSTR_CDIMAGE		wxT("Iso")	// *Mule internal use only

// Additional media meta data tags from eDonkeyHybrid (note also the uppercase/lowercase)
#define	FT_ED2K_MEDIA_ARTIST		"Artist"	// <string>
#define	FT_ED2K_MEDIA_ALBUM		"Album"		// <string>
#define	FT_ED2K_MEDIA_TITLE		"Title"		// <string>
#define	FT_ED2K_MEDIA_LENGTH		"length"	// <string> !!!
#define	FT_ED2K_MEDIA_BITRATE		"bitrate"	// <uint32>
#define	FT_ED2K_MEDIA_CODEC		"codec"		// <string>

#endif // FILETAGS_H
