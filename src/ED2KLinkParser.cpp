/*
  This file is part of aMule
  Copyright (C) 2003 Madcat ( madcat@_@users.sf.net / sharedaemon.sf.net )

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

bool HexToDec(char x, char &res)
{
	if ( isdigit(x) ) {
		res = x - '0';
		return true;
	} else if ( toupper(x)>='A' && toupper(x)<='F' ) {
		res = toupper(x) - 'A' + 10;
		return true;
	} else {
		return false;
	}
}

void UnescapeURI(char *buf)
{
	char *outbuf = buf;
	char c;
	for ( ; (c = *buf++); outbuf++) {
		char digit1, digit2;
		if ( (c == '%') && HexToDec(buf[0],digit1) && HexToDec(buf[1],digit2) ) {
			*outbuf = digit1 * 16 + digit2;
			buf+=2;
		} else {
			*outbuf = c;
		}
	}
	*outbuf++ = '\0';
}

void AddLink(char *ed2klink)
{
FILE *ed2kfile;
char filename[100];
	
	/* First do some checking wether the link is correct. */
	if ((ed2klink[strlen(ed2klink)-35] != '|') && (ed2klink[strlen(ed2klink)-34] != '|')) {
		printf("Invalid ED2K link.\nReason: Character before hash (34/35 chars before end) must be |\n");
		return;
	}

	/* Link seemed ok, add it to file. */
	sprintf(filename,"%s/.aMule/ED2KLinks",getenv("HOME"));
	ed2kfile = fopen(filename,"a");
	if (ed2kfile != NULL) {
		fprintf(ed2kfile,"%s\n",ed2klink);
		printf("Successfully wrote ED2K link to file.\n");
		fclose(ed2kfile);
	} else {
		printf("Error opening file %s.\n", filename);
	}
}

// emanuelw(20030924) added: AddServer()
//ed2k://|server|111.111.111.111|1111|/
void AddServer(char *ed2klink)
{
	FILE *ed2kfile;
	char filename[100];
	char bufferIP[16];
	char bufferPORT[6];
	
	char* server;
	char* ip;
	char* port;
	char* portEnd;
	
	memset(bufferIP,0,16);
	memset(bufferPORT,0,6);
	
	server = strchr(ed2klink,'|') + 1;
	
	if(server != NULL)
		ip = strchr(server,'|') + 1;
		
	if(ip != NULL)
		port = strchr(ip,'|') + 1;
		
	if (port != NULL)
	{
		portEnd = strchr(port,'|') + 1;
		sprintf(filename,"%s/.aMule/ED2KServers",getenv("HOME"));
		ed2kfile = fopen(filename,"a");
		if (ed2kfile != NULL)
		{
			strncpy(bufferIP,ip,strlen(ip)-strlen(port)-1);
			strncpy(bufferPORT,port,strlen(port)-strlen(portEnd)-1);
			fprintf(ed2kfile, "%s:%s,1,\n", bufferIP, bufferPORT);
			printf("Successfully wrote ED2K-Server link to file.\n");
			fclose(ed2kfile);
		} 
		else
			printf("Error opening file %s.\n", filename);		
	}
	else
		printf("Invalid ED2K-Server link.\n");	
}

int main(int argc, char *argv[])
{
	int result = 0;
	for (int i=1;i<argc;i++) {
		if ( !argv[i] ) {
			continue;
		}
		char *param = strdup(argv[i]);
		UnescapeURI(param);
		if (!strncmp(param, "ed2k://|file|", 13) && (strlen(param)>55)) {
			AddLink(param);
		} else if (!strncmp(param, "ed2k://|server|", 15) && (strlen(param)>32)) {
			AddServer(param);
		} else if (!strncmp(param, "--version", 9)) {
			printf("aMule ED2K links parser v1.01\n");
		} else if (!strncmp(param, "ed2k://|server|", 15) && (strlen(param)>32)) {
			AddServer(param);
		} else if (!strncmp(param, "--help", 6)) {
			printf("aMule ED2K links parser v1.01\n\n");
			printf("Enter ed2k links as commandline arguments, and they will be saved into $HOME/.aMule/ED2KLinks\n");
			printf("file, from where aMule will pick them up and add to download queue once per second.\n");
			printf("Currently, only file links are supported.\n\n");
			printf("Usage:\n");
			printf("       --help          Prints this help.\n");
			printf("       --version       Displays version info.\n");
			printf("       ed2k://|file|   Sends ed2k link to $HOME/.aMule/ED2KLinks file.\n");
			printf("       ed2k://|server| Sends ed2k link to $HOME/.aMule/ED2KServer file.\n");
		} else {
			if (strncmp(param, "ed2k://|file|", 13)) {
				printf("Invalid ED2K link.\nReason: First 13 characters don't match 'ed2k://|file|'\n");
			}
			if (strlen(param)<=55) {
				printf("Invalid ED2K Link.\nReason: Link length is below 55 characters.\n");
			}
			result = 1;
		}
		free(param);
	}
	return result;
}
