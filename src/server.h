//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifndef SERVER_H
#define SERVER_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/list.h>		// Needed for WX_DECLARE_LIST
#include <wx/treebase.h>	// Needed for wxTreeCtrl and wxTreeItemId

#include "types.h"		// Needed for uint8, uint16 and uint32
#include "CString.h"		// Needed for CString
#include "CFile.h"

class CTag;
class TagList;
class ServerMet_Struct;
class CSafeFile;

// Server TCP flags
#define SRV_TCPFLG_COMPRESSION		0x00000001

// Server UDP flags
#define SRV_UDPFLG_EXT_GETSOURCES	0x00000001
#define SRV_UDPFLG_EXT_GETFILES		0x00000002

WX_DECLARE_LIST(CTag,TagList);

class CServer {
public:
	CServer(ServerMet_Struct* in_data);
	CServer(uint16 in_port, char* i_addr);
	CServer(CServer* pOld);
	~CServer();
	void	AddTag(CTag* in_tag)					{taglist->Append(in_tag);}
	char*	GetListName()						{return listname;}
	char*	GetFullIP()						{return ipfull;}
	char*	GetAddress();
	uint16	GetPort()						{return port;}
	void	FillWindowTags(wxTreeCtrl* wnd,wxTreeItemId rootitem);
	bool	AddTagFromFile(CFile* servermet);
	void	SetListName(char* newname);
	void	SetDescription(char* newdescription);
	uint32	GetIP()							{return ip;}
	uint32	GetFiles()						{return files;} 
	uint32	GetUsers()						{return users;} 
	char*	GetDescription()					{return description;} 
	uint32	GetPing()						{return ping;} 
	uint32	GetPreferences()					{return preferences;} 
	uint32	GetMaxUsers()						{return maxusers;}
	void	SetMaxUsers(uint32 in_maxusers) 			{maxusers = in_maxusers;}
	void	SetUserCount(uint32 in_users)				{users = in_users;}
	void	SetFileCount(uint32 in_files)				{files = in_files;}
	void	ResetFailedCount()					{failedcount = 0;} 
	void	AddFailedCount()					{failedcount++;} 
	uint32	GetFailedCount()					{return failedcount;} 
	void	SetID(uint32 newip);
	char*	GetDynIP()						{return dynip;}
	bool	HasDynIP()						{return dynip;}
	void	SetDynIP(char* newdynip);
	uint32	GetLastPinged()						{return lastpinged;}
	void	SetLastPinged(uint32 in_lastpinged)			{lastpinged = in_lastpinged;}
	void	SetPing(uint32 in_ping)					{ping = in_ping;}
	void	SetPreference(uint32 in_preferences)			{preferences = in_preferences;}
	void	SetIsStaticMember(bool in)				{staticservermember=in;}
	bool	IsStaticMember()					{return staticservermember;}
	uint32	GetChallenge()						{return challenge;}
	void	SetChallenge(uint32 in_challenge)			{challenge = in_challenge;}
	uint32	GetSoftFiles()						{return softfiles;}
	void	SetSoftFiles(uint32 in_softfiles)			{softfiles = in_softfiles;}
	uint32	GetHardFiles()						{return hardfiles;}
	void	SetHardFiles(uint32 in_hardfiles)			{hardfiles = in_hardfiles;}
	const	CString& GetVersion() const				{return m_strVersion;}
	void	SetVersion(CString pszVersion)				{m_strVersion = pszVersion;}
	void	SetTCPFlags(uint32 uFlags)				{m_uTCPFlags = uFlags;}
	uint32	GetTCPFlags() const					{return m_uTCPFlags;}
	void	SetUDPFlags(uint32 uFlags)				{m_uUDPFlags = uFlags;}
	uint32	GetUDPFlags() const					{return m_uUDPFlags;}

private:
	uint32		challenge;
	uint32		lastpinged;
	uint32		files;
	uint32		users;
	uint32		maxusers;
	uint32		softfiles;
	uint32		hardfiles;
	uint32		preferences;
	uint32		ping;
	char*		description;
	char*		listname;
	char*		dynip;
	uint32		tagcount;
	char		ipfull[17];
	uint32		ip;
	uint16		port;
	uint32		failedcount; 
	TagList* taglist;
	//CTypedPtrList<CPtrList, CTag*>*	taglist;
	uint8		staticservermember;
	CString		m_strVersion;
	uint32		m_uTCPFlags;
	uint32		m_uUDPFlags;
	
	void Init();

};

#endif // SERVER_H
