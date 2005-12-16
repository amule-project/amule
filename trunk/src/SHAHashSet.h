//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 Angel Vidal (Kry) ( kry@amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

/* 
 SHA haset basically exists of 1 Tree for all Parts (9.28MB) + n  Trees
 for all blocks (180KB) while n is the number of Parts.
 This means it is NOT a complete hashtree, since the 9.28MB is a given level, in order
 to be able to create a hashset format similar to the MD4 one.

 If the number of elements for the next level are odd (for example 21 blocks to spread into 2 hashs)
 the majority of elements will go into the left branch if the parent node was a left branch
 and into the right branch if the parent node was a right branch. The first node is always
 taken as a left branch.

Example tree:
	FileSize: 19506000 Bytes = 18,6 MB

								X (18,6)                                   MasterHash
							 /     \
						 X (18,55)   \
					/		\	       \
                   X(9,28)  x(9,28)   X (0,05MB)						   PartHashs
			   /      \    /       \        \
		X(4,75)   X(4,57) X(4,57)  X(4,75)   \

						[...............]
X(180KB)   X(180KB)  [...] X(140KB) | X(180KB) X(180KB [...]			   BlockHashs
									v
						 Border between first and second Part (9.28MB)

HashsIdentifier:
When sending hashs, they are send with a 16bit identifier which specifies its postion in the
tree (so StartPosition + HashDataSize would lead to the same hash)
The identifier basically describes the way from the top of the tree to the hash. a set bit (1)
means follow the left branch, a 0 means follow the right. The highest bit which is set is seen as the start-
postion (since the first node is always seend as left).

Example

								x                   0000000000000001
							 /     \		
						 x		    \				0000000000000011
					  /		\	       \
                    x       _X_          x 	        0000000000000110


*/

#ifndef __SHAHAHSET_H__
#define __SHAHAHSET_H__

#include <deque>
#include <set>
#include <list>

#include "Types.h"

#define HASHSIZE		20
#define KNOWN2_MET_FILENAME		wxT("known2.met")

enum EAICHStatus {
	AICH_ERROR = 0,
	AICH_EMPTY,
	AICH_UNTRUSTED,
	AICH_TRUSTED,
	AICH_VERIFIED,
	AICH_HASHSETCOMPLETE
};

class CFileDataIO;
class CKnownFile;
class CMemFile;
class CPartFile;
class CUpDownClient;

using namespace std;

/////////////////////////////////////////////////////////////////////////////////////////
///CAICHHash
class CAICHHash 
{
public:
	~CAICHHash()									{;}
	CAICHHash()										{ memset(m_abyBuffer, 0, HASHSIZE); }
	CAICHHash(CFileDataIO* file)					{ Read(file); }
	CAICHHash(byte* data)							{ Read(data); }
	CAICHHash(const CAICHHash& k1)					{ *this = k1; }
	CAICHHash&	operator=(const CAICHHash& k1)		{ memcpy(m_abyBuffer, k1.m_abyBuffer, HASHSIZE); return *this; }
	friend bool operator==(const CAICHHash& k1,const CAICHHash& k2)	{ return memcmp(k1.m_abyBuffer, k2.m_abyBuffer, HASHSIZE) == 0;}
	friend bool operator!=(const CAICHHash& k1,const CAICHHash& k2)	{ return !(k1 == k2); }
	void		Read(CFileDataIO* file);
	void		Write(CFileDataIO* file) const;
	void		Read(byte* data)					{ memcpy(m_abyBuffer, data, HASHSIZE); }
	wxString		GetString() const;
	byte*		GetRawHash()						{ return m_abyBuffer; }

	static uint32	GetHashSize()						{ return HASHSIZE;}
	
	unsigned int DecodeBase32(const wxString &base32);
	
private:
	byte m_abyBuffer[HASHSIZE];
};

/////////////////////////////////////////////////////////////////////////////////////////
///CAICHHashAlgo
class CAICHHashAlgo 
{
public:
	virtual ~CAICHHashAlgo() {};
	virtual void	Reset() = 0;
	virtual void	Add(const void* pData, uint32 nLength) = 0;
	virtual void	Finish(CAICHHash& Hash) = 0;
	virtual void	GetHash(CAICHHash& Hash) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////////
///CAICHHashTree
class CAICHHashTree
{
	// Madness from eMule, and gcc don't like being friend with itself...
	// That probably means gcc is not friend of gcc. So gcc hates itself.
	//	friend class CAICHHashTree; 
	friend class CAICHHashSet;
public:
	CAICHHashTree(uint32 nDataSize, bool bLeftBranch, uint32 nBaseSize);
	~CAICHHashTree();
	void			SetBlockHash(uint32 nSize, uint32 nStartPos, CAICHHashAlgo* pHashAlg);
	bool			ReCalculateHash(CAICHHashAlgo* hashalg, bool bDontReplace );
	bool			VerifyHashTree(CAICHHashAlgo* hashalg, bool bDeleteBadTrees);
	CAICHHashTree*	FindHash(uint32 nStartPos, uint32 nSize)					{uint8 buffer = 0; return FindHash(nStartPos, nSize, &buffer);}

protected:
	CAICHHashTree*	FindHash(uint32 nStartPos, uint32 nSize, uint8* nLevel);
	bool			CreatePartRecoveryData(uint32 nStartPos, uint32 nSize, CFileDataIO* fileDataOut, uint16 wHashIdent);
	void			WriteHash(CFileDataIO* fileDataOut, uint16 wHashIdent) const;
	bool			WriteLowestLevelHashs(CFileDataIO* fileDataOut, uint16 wHashIdent, bool bNoIdent = false) const;
	bool			LoadLowestLevelHashs(CFileDataIO* fileInput);
	bool			SetHash(CFileDataIO* fileInput, uint16 wHashIdent, sint8 nLevel = (-1), bool bAllowOverwrite = true);
	CAICHHashTree*	m_pLeftTree;
	CAICHHashTree*	m_pRightTree;

public:
	CAICHHash		m_Hash;
	uint32			m_nDataSize;		// size of data which is covered by this hash
	uint32			m_nBaseSize;		// blocksize on which the lowest hash is based on
	bool			m_bIsLeftBranch;		// left or right branch of the tree
	bool			m_bHashValid;			// the hash is valid and not empty
};

/////////////////////////////////////////////////////////////////////////////////////////
///CAICHUntrustedHashs
class CAICHUntrustedHash {
public:
	CAICHUntrustedHash&	operator=(const CAICHUntrustedHash& k1)		{ m_adwIpsSigning = k1.m_adwIpsSigning; m_Hash = k1.m_Hash ; return *this; }
	bool	AddSigningIP(uint32 dwIP);	

	CAICHHash				m_Hash;
	set<uint32>	m_adwIpsSigning;
};

/////////////////////////////////////////////////////////////////////////////////////////
///CAICHUntrustedHashs
class CAICHRequestedData {
public:
	CAICHRequestedData()	{m_nPart = 0; m_pPartFile = NULL; m_pClient= NULL;}
	CAICHRequestedData&	operator=(const CAICHRequestedData& k1)		{ m_nPart = k1.m_nPart; m_pPartFile = k1.m_pPartFile; m_pClient = k1.m_pClient; return *this; }
	uint16			m_nPart;
	CPartFile*		m_pPartFile;
	CUpDownClient*	m_pClient;
};


using namespace std;
	
typedef std::list<CAICHRequestedData> CAICHRequestedDataList;

/////////////////////////////////////////////////////////////////////////////////////////
///CAICHHashSet
class CAICHHashSet
{
public:
	CAICHHashSet(CKnownFile*	pOwner);
	~CAICHHashSet(void);
	bool			CreatePartRecoveryData(uint32 nPartStartPos, CFileDataIO* fileDataOut, bool bDbgDontLoad = false);
	bool			ReadRecoveryData(uint32 nPartStartPos, CMemFile* fileDataIn);
	bool			ReCalculateHash(bool bDontReplace = false);
	bool			VerifyHashTree(bool bDeleteBadTrees);
	void			UntrustedHashReceived(const CAICHHash& Hash, uint32 dwFromIP);
	bool			IsPartDataAvailable(uint32 nPartStartPos);
	void			SetStatus(EAICHStatus bNewValue)			{m_eStatus = bNewValue;}
	EAICHStatus		GetStatus()	const							{return m_eStatus;}
	
	void			FreeHashSet();
	void			SetFileSize(uint32 nSize);
	
	CAICHHash&		GetMasterHash()						{return m_pHashTree.m_Hash;} 
	void			SetMasterHash(const CAICHHash& Hash, EAICHStatus eNewStatus);
	bool			HasValidMasterHash()				{return m_pHashTree.m_bHashValid;}

	bool			SaveHashSet();
	bool			LoadHashSet(); // only call directly when debugging

	CAICHHashAlgo*	GetNewHashAlgo();
	static void		ClientAICHRequestFailed(CUpDownClient* pClient);
	static void		RemoveClientAICHRequest(const CUpDownClient* pClient);
	static bool		IsClientRequestPending(const CPartFile* pForFile, uint16 nPart);
	static CAICHRequestedData GetAICHReqDetails(const  CUpDownClient* pClient);
	void			DbgTest();
	
	void SetOwner(CKnownFile* owner) { m_pOwner = owner;}

	CAICHHashTree	m_pHashTree;
	
	static CAICHRequestedDataList m_liRequestedData;
	
private:
	CKnownFile*		m_pOwner;
	EAICHStatus		m_eStatus;
	deque<CAICHUntrustedHash> m_aUntrustedHashs;
};

#endif  //__SHAHAHSET_H__
