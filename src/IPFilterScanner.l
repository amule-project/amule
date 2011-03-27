%{
//
// This file is part of the aMule Project.
//
// Copyright (c) 2009-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2009-2011 Stu Redman ( sturedman@amule.org )
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

//
// Read IP filters
//
// These have a silly "grammar". For example:
// 1.2.3.4-1.2.3.5,100,Duh:2.3.4.5-2.3.4.6
// Now - which format is it, and what range should it take?
//
// So just use the lexer to read the file and assure the line format.
// The actual IP reading is done with the ScanIP() function.
//

#include <stdio.h>
#include "Types.h"
#include "Logger.h"
#define IPFS_EXTERN
#include "IPFilterScanner.h"
#include <common/Format.h>
#include <common/StringFunctions.h>

#ifdef _MSC_VER
#define isatty(DUMMY) 0
#define YY_NO_UNISTD_H
#pragma warning(disable:4003) 
#endif

#define YY_NEVER_INTERACTIVE 1

// When we get here the IP has been lexed nicely,
// so we can blaze through without any checks.
// The total lexing time is more than twice as fast
// with this than when using sscanf.
static bool ScanIP(const char * buf, uint32 & ip)
{
	while (*buf < '0') {
		buf++;	// skip whitespace
	}
	ip = 0;
	int a = 0;
	for (int i = 0; i < 4; buf++) {
		if (*buf < '0' || *buf > '9') {
			// finished a number, check it and add to the ip
			if (a > 255) {
				return false;
			}
			ip = (ip << 8) | a;
			a = 0;
			i++;
		} else {
			// build number
			a = a * 10 + *buf - '0';
		}
	}
	return true;
}

static bool ScanInt(const char * buf, uint32 & a)
{
	while (*buf < '0') {
		buf++;	// skip whitespace
	}
	a = 0;
	while (*buf >= '0' && *buf <= '9') {
		a = a * 10 + *buf - '0';
		if (a > 255) {
			return false;
		}
		buf++;
	}
	return true;
}

%}

%option noyywrap

NO		[0-9]{1,3}
IP		{NO}"."{NO}"."{NO}"."{NO}
WS		[ \t\r]*

%%

^{WS}{IP}{WS}-{WS}{IP}{WS},{WS}{NO}{WS},.*		{
		/* PeerGuardian filter line
		   <IPStart> - <IPEnd> , <AccessLevel> , <Description> 
		*/
		char * ip1 = yytext;
		char * ip2 = strchr(ip1 + 7, '-') + 1;
		char * acc = strchr(ip2 + 7, ',') + 1;
		char * dsc = strchr(acc + 1, ',') + 1;
		if (!ScanIP(ip1, IPStart) || !ScanIP(ip2, IPEnd)
			|| !ScanInt(acc, IPLevel)) {
			yyip_Bad++;
		} else {
			IPDescription = dsc;
			return 1;
		}
	}

^{WS}#.*	{
		/* Comment */
	}

^.*:{WS}{IP}{WS}-{WS}{IP}{WS}		{
		/* AntiP2P filter line
		   <Description> : <IPStart> - <IPEnd>
		*/
		char * ip1 = strrchr(yytext, ':');
		*ip1++ = 0;		// remove : and terminate comment
		char * ip2 = strchr(ip1 + 7, '-') + 1;
		if (!ScanIP(ip1, IPStart) || !ScanIP(ip2, IPEnd)) {
			yyip_Bad++;
		} else {
			IPLevel = 0;
			IPDescription = yytext;
			return 1;
		}
	}

{WS}"\n"	{
		yyip_Line++;
	}

^.			{ 
		/* Bad line */
		yyip_Bad++;
		AddDebugLogLineN(logIPFilter, CFormat(wxT("error in line %d: %s")) % yyip_Line % wxString(char2unicode(yytext)));
	}

%%
