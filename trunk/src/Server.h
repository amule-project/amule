//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef SERVER_H
#define SERVER_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "Server.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/list.h>		// Needed for WX_DECLARE_LIST

#include <list>

#include "Types.h"		// Needed for uint8, uint16 and uint32

#ifdef CLIENT_GUI
#include "ECSpecialTags.h"
#endif

class CTag;
class ServerMet_Struct;
class CFileDataIO;

// Server TCP flags
#define	SRV_TCPFLG_COMPRESSION		0x00000001
#define	SRV_TCPFLG_NEWTAGS			0x00000008
#define	SRV_TCPFLG_UNICODE			0x00000010

// Server UDP flags
#define	SRV_UDPFLG_EXT_GETSOURCES		0x00000001
#define	SRV_UDPFLG_EXT_GETFILES			0x00000002
#define	SRV_UDPFLG_EXT_NEWTAGS			0x00000008
#define	SRV_UDPFLG_EXT_UNICODE			0x00000010
#define	SRV_UDPFLG_EXT_GETSOURCES2	0x00000020

// Server priority
#define SRV_PR_LOW                      2
#define SRV_PR_NORMAL                   0
#define SRV_PR_HIGH                     1
// Server priority max and min values
#define SRV_PR_MAX			2
#define SRV_PR_MID			1
#define SRV_PR_MIN			0

typedef std::list<CTag*> TagList;

class CServer
{
public:
	CServer(ServerMet_Struct* in_data);
	CServer(uint16 in_port, const wxString i_addr);
	CServer(CServer* pOld);

#ifdef CLIENT_GUI
	CServer(CEC_Server_Tag *);
#endif

	~CServer();
	void		AddTag(CTag* in_tag)	{m_taglist.push_back(in_tag);}
	const wxString &GetListName() const	{return listname;}
	const wxString &GetFullIP() const	{return ipfull;}
	
	const wxString &GetAddress() const {
		if (!dynip.IsEmpty()) {
			return dynip;
		} else {
			return ipfull;
		}
	}	
	
	// the official port
	uint16  GetPort() const			{return realport ? realport : port;}
	// the connection port
	uint16  GetConnPort() const		{return port;}
	void    SetPort(uint32 val)		{realport = val;}
	bool	AddTagFromFile(CFileDataIO* servermet);
	void	SetListName(const wxString& newname);
	void	SetDescription(const wxString& newdescription);
	uint32	GetIP() const			{return ip;}
	uint32	GetFiles() const		{return files;} 
	uint32	GetUsers() const		{return users;} 
	const wxString	&GetDescription() const	{return description;} 
	uint32	GetPing() const			{return ping;} 
	uint32	GetPreferences() const		{return preferences;} 
	uint32	GetMaxUsers() const		{return maxusers;}
	void	SetMaxUsers(uint32 in_maxusers) {maxusers = in_maxusers;}
	void	SetUserCount(uint32 in_users)	{users = in_users;}
	void	SetFileCount(uint32 in_files)	{files = in_files;}
	void	ResetFailedCount()		{failedcount = 0;} 
	void	AddFailedCount()		{failedcount++;} 
	uint32	GetFailedCount() const		{return failedcount;} 
	void	SetID(uint32 newip);
	const wxString &GetDynIP() const	{return dynip;}
	bool	HasDynIP() const		{return dynip.Length();}
	void	SetDynIP(const wxString& newdynip);
	uint32	GetLastPinged() const		{return lastpinged;}
	void	SetLastPinged(uint32 in_lastpinged) {lastpinged = in_lastpinged;}
	void	SetPing(uint32 in_ping)		{ping = in_ping;}
	void	SetPreference(uint32 in_preferences) {preferences = in_preferences;}
	void	SetIsStaticMember(bool in)	{staticservermember=in;}
	bool	IsStaticMember() const		{return staticservermember;}
	uint32	GetChallenge() const		{return challenge;}
	void	SetChallenge(uint32 in_challenge) {challenge = in_challenge;}
	uint32	GetSoftFiles() const		{return softfiles;}
	void	SetSoftFiles(uint32 in_softfiles) {softfiles = in_softfiles;}
	uint32	GetHardFiles() const		{return hardfiles;}
	void	SetHardFiles(uint32 in_hardfiles) {hardfiles = in_hardfiles;}
	const	wxString &GetVersion() const	{return m_strVersion;}
	void	SetVersion(const wxString &pszVersion)	{m_strVersion = pszVersion;}
	void	SetTCPFlags(uint32 uFlags)	{m_uTCPFlags = uFlags;}
	uint32	GetTCPFlags() const		{return m_uTCPFlags;}
	void	SetUDPFlags(uint32 uFlags)	{m_uUDPFlags = uFlags;}
	uint32	GetUDPFlags() const		{return m_uUDPFlags;}
	uint32	GetLowIDUsers() const		{return m_uLowIDUsers;}
	void	SetLowIDUsers(uint32 uLowIDUsers) {m_uLowIDUsers = uLowIDUsers;}
	uint32	GetDescReqChallenge() const	{return m_uDescReqChallenge;}
	void	SetDescReqChallenge(uint32 uDescReqChallenge) {m_uDescReqChallenge = uDescReqChallenge;}
	uint8	GetLastDescPingedCount() const	{return lastdescpingedcout;}
	void	SetLastDescPingedCount(bool reset);
	bool	GetUnicodeSupport() const				{return GetTCPFlags() & SRV_TCPFLG_UNICODE;}
	const wxString& GetAuxPortsList() const	{return m_auxPorts;}
	void	SetAuxPortsList(const wxString& val)	{m_auxPorts = val;}
	
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
	wxString	description;
	wxString	listname;
	wxString	dynip;
	uint32		tagcount;
	wxString	ipfull;
	uint32		ip;
	uint16		port;
	uint16		realport;
	uint32		failedcount; 
	uint32		m_uDescReqChallenge;
	uint8		lastdescpingedcout;
	TagList		m_taglist;
	//CTypedPtrList<CPtrList, CTag*>*	taglist;
	uint8		staticservermember;
	wxString	m_strVersion;
	uint32		m_uTCPFlags;
	uint32		m_uUDPFlags;
	uint32		m_uLowIDUsers;
	wxString	m_auxPorts;
	
	void Init();

};

#endif // SERVER_H
