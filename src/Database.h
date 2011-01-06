//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Stu Redman ( sturedman@amule.org / http://www.amule.org )
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

#include "Tag.h"		// needed for TagPtrList;
#include <common/Macros.h>

class CDatabase {
friend class CDatabaseTransaction;

public:
	CDatabase();
	bool Init(const wxString& dbname);
	~CDatabase();
	void Exec(const char * cmd, const wxString& debugLocation = wxEmptyString);
	void ShowError(wxString error);

	// Initial cleanup
	void KadStartupCleanup();
	// Get rowid of keyword, 0 if not existing
	uint64 KadGetKeyword(uint64 hashlow, uint64 hashhigh);
	// Insert new keyword, returns new rowid
	uint64 KadInsertKeyword(uint64 hashlow, uint64 hashhigh);
	// Get rowid of source (= shared file), 0 if not existing
	uint64 KadGetSource(uint64 keyIndex, uint64 hashlow, uint64 hashhigh, uint64 size);
	// Insert new source, returns new rowid
	uint64 KadInsertSource(uint64 keyIndex, uint64 hashlow, uint64 hashhigh, uint64 size, uint32 lifetime);
	// Update existing source
	void KadUpdateSource(uint64 sourceIndex, uint32 lifetime);
	// Count sources for a keyword
	uint32 KadCountSource(uint64 keyIndex);
	// Replace Taglist for a source, returns number of tags inserted
	int KadReplaceTaglist(uint64 sourceIndex, const TagPtrList * tags, bool clearOldTags = true);
	// Set a name for a source
	void KadSetSourcename(uint64 sourceIndex, const wxString& name, uint32 popularity, bool fastRefresh);
	// Set an ip for a source, returns 0: new ip, 1: refresh, 2: fast refresh
	int	KadSetKeywordPublishIP(uint64 sourceIndex, uint32 ip, uint32 lastPublish);
	// Get trust value for a source
	double KadGetTrustValue(uint64 sourceIndex);

private:
	// Init KAD tables
	void KadInitTables(bool cleanup);

	class CStatement {
	public:
		CStatement(struct sqlite3 *);
		~CStatement();
		void Prepare(const char * cmd);
		void Bind(int pos, uint64 val);
		void Bind(int pos, uint32 val) { Bind(pos, (uint64) val); }
		void Bind(int pos, int val) { Bind(pos, (uint64) (sint64) val); }
		void Bind(int pos, double val);
		void Bind(int pos, const wxString& val);
		int	 Step();
		void Reset();
		uint32		ColumnUint32(int col);
		uint64		ColumnUint64(int col);
		double		ColumnDouble(int col);
		wxString	ColumnStr(int col);
	private:
		struct sqlite3_stmt * m_stmt;
		struct sqlite3 * m_db;
		// Bind doesn't copy string objects internal, so store them here
		std::map<int, wxCharBuffer> m_strings;
	};

	struct sqlite3 * m_db;
	bool				m_transactionActive;

	// Kad stuff
	typedef std::map<uint32_t, uint32_t>	GlobalPublishIPTrackingMap;
	GlobalPublishIPTrackingMap	m_globalPublishIPs;		// tracks count of publishings for each 255.255.255.0/24 subnet

	// Recalculate trust value for a source
	double KadRecalculateTrustValue(uint64 sourceIndex);
	// Mark trust value as invalid (will be recalculated next time it is requested)
	void KadClearTrustValue(uint64 sourceIndex);
	// Track ip adresses for trust calculation
	void AdjustGlobalPublishTracking(uint32 ip, bool increase, const wxString& DEBUG_ONLY(dbgReason));
};

// Make a transaction.
class CDatabaseTransaction {
public:
	CDatabaseTransaction(CDatabase * db);
	~CDatabaseTransaction() { Commit(); }
	void SyncBetween();
	void Commit();

private:
	CDatabase * m_db;
	bool m_active;
};
