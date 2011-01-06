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


#include <sqlite3.h>
#include "Database.h"
#include "Logger.h"
#include <protocol/kad/Constants.h>
//#include "kademlia\kademlia\IndexedDB.h"


CDatabase::CDatabase()
{
	m_db = 0;
	m_transactionActive = false;
}


bool CDatabase::Init(const wxString& dbname)
{
	int rc = 0;
	try {
		rc = sqlite3_initialize();
		if (rc != SQLITE_OK) {
			throw wxString(wxT("sqlite3_initialize"));
		}
		rc = sqlite3_open(dbname.ToUTF8(), &m_db);
		if (rc != SQLITE_OK) {
			throw wxString(wxT("sqlite3_open"));
		}

		// Create tables
		//
		CDatabaseTransaction transaction(this);
		KadInitTables(true);

	} catch(wxString s) {
		ShowError(s);
		return false;
	}
	return true;
}


CDatabase::~CDatabase()
{
	if (m_db) {
		sqlite3_close(m_db);
	}
	sqlite3_shutdown();
}


void CDatabase::ShowError(wxString error)
{
	const char * err = sqlite3_errmsg(m_db);
	error = wxT("SQLITE ERROR: ") + error;
	if (err) {
		error = error + wxT(": ") + wxString::FromUTF8(err);
	}
	AddLogLineC(error);
}


// execute command, throws on error
void CDatabase::Exec(const char * cmd, const wxString& debugLocation)
{
	char * err = 0;
	int rc = sqlite3_exec(m_db, cmd, 0, 0, &err);
	if (rc != SQLITE_OK) {
		wxString error = debugLocation + wxT("(") + wxString::FromUTF8(cmd) + wxT(")");
		if (err) {
			error = error + wxT(": ") + wxString::FromUTF8(err);
		}
//		wxFAIL;
		throw error;
	}
}


CDatabase::CStatement::CStatement(sqlite3 * db)
{
	m_stmt = 0;
	m_db = db;
}


CDatabase::CStatement::~CStatement()
{
	if (m_stmt) {
		int rc = sqlite3_finalize(m_stmt);
		if (rc != SQLITE_OK) {
			wxFAIL;
			throw wxString(wxT("sqlite3_finalize"));
		}
	}
}


void CDatabase::CStatement::Prepare(const char * cmd)
{
	int rc = sqlite3_prepare_v2(m_db, cmd, -1, &m_stmt, NULL);
	if (rc != SQLITE_OK) {
		wxFAIL;
		throw wxString(wxT("sqlite3_prepare_v2 ") + wxString::FromUTF8(cmd));
	}
}


void CDatabase::CStatement::Bind(int pos, uint64 val)
{
	int rc = sqlite3_bind_int64(m_stmt, pos, val);
	if (rc != SQLITE_OK) {
		wxFAIL;
		throw wxString(CFormat(wxT("sqlite3_bind_int64 %d %u")) % pos % val);
	}
}


void CDatabase::CStatement::Bind(int pos, double val)
{
	int rc = sqlite3_bind_double(m_stmt, pos, val);
	if (rc != SQLITE_OK) {
		wxFAIL;
		throw wxString(CFormat(wxT("sqlite3_bind_double %d %f")) % pos % val);
	}
}


void CDatabase::CStatement::Bind(int pos, const wxString& val)
{
	m_strings[pos] = val.ToUTF8();
	int rc = sqlite3_bind_text(m_stmt, pos, m_strings[pos], -1, NULL);
	if (rc != SQLITE_OK) {
		wxFAIL;
		throw wxString(CFormat(wxT("sqlite3_bind_text %d %s")) % pos % val);
	}
}


int CDatabase::CStatement::Step()
{
	int rc = sqlite3_step(m_stmt);
	if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE) {
		wxFAIL;
		throw wxString(CFormat(wxT("sqlite3_step %d")) % rc);
	}
	return rc;
}


void CDatabase::CStatement::Reset()
{
	int rc = sqlite3_reset(m_stmt);
	if (rc != SQLITE_OK) {
		wxFAIL;
		throw wxString(wxT("sqlite3_reset"));
	}
}


uint32 CDatabase::CStatement::ColumnUint32(int col)
{
	return sqlite3_column_int(m_stmt, col);
}


uint64 CDatabase::CStatement::ColumnUint64(int col)
{
	return sqlite3_column_int64(m_stmt, col);
}


double CDatabase::CStatement::ColumnDouble(int col)
{
	return sqlite3_column_double(m_stmt, col);
}


wxString CDatabase::CStatement::ColumnStr(int col)
{
	return wxString::FromUTF8((const char*) sqlite3_column_text(m_stmt, col));
}



CDatabaseTransaction::CDatabaseTransaction(CDatabase * db)
{
	m_db = db;
	if (db->m_transactionActive) {
		// There is already a transaction active, and they can't be nested. Do nothing.
		m_active = false;
	} else {
		m_active = true;
		db->m_transactionActive = true;
		m_db->Exec("BEGIN;");
	}
}


void CDatabaseTransaction::Commit()
{
	if (m_active) {
		m_db->Exec("COMMIT;");
		m_active = false;
		m_db->m_transactionActive = false;
	}
}


void CDatabaseTransaction::SyncBetween()
{
	// This is allowed no matter if active or not. So a nested "Transaction" can still sync.
	m_db->Exec("COMMIT;BEGIN;");
}




void CDatabase::KadInitTables(bool cleanup)
{
	if (cleanup) {
		Exec(	"DROP TABLE IF EXISTS kad_keyword;"
				"DROP TABLE IF EXISTS kad_keyword_source;"
				"DROP TABLE IF EXISTS kad_keyword_publishIP;"
				"DROP TABLE IF EXISTS kad_keyword_filename;"
				"DROP TABLE IF EXISTS kad_keyword_tag;");
	}
	// Kad key index
	// Hashes are stored as two integers (which are 64 bit)
	//
	// root table with keyword hashes, which are referenced in other tables by their rowid
	// (called "keyID" in src)
	Exec("CREATE TABLE IF NOT EXISTS kad_keyword ("
			"keyword_hash_low integer,"				// lower half of keyword hash  (low part has similarities for the part of the network we index)
			"keyword_hash_high integer);");			// upper half of keyword hash, indexed 
	Exec("CREATE INDEX IF NOT EXISTS idx_kad_keyword ON kad_keyword (keyword_hash_high);");
	// A keyword can be found in several sources (= shared files). These are stored here. They are referenced by keyword rowid.
	// (called "sourceID" in src)
	Exec("CREATE TABLE IF NOT EXISTS kad_keyword_source ("
			"kad_keyword_rowid integer,"		// keyword to which this source belongs, indexed
			"source_hash_low integer,"			// lower half of source hash
			"source_hash_high integer,"			// upper half of source hash, indexed
			"source_size integer,"				// file size (if there are several file sizes for one hash, which should not happen,
												// each gets an own entry here)
			"trustValue float,"					// 0-10000, how well we trust this to be a valid source
			"lastTrustValueCalc integer,"		// last time we calculated the trust value
			"publishIPs integer,"               // number of publish IPs stored for this source
			"lifetime integer);"				// time when this source will be regarded as invalid
			);
	Exec("CREATE INDEX IF NOT EXISTS idx_kad_keyword_source ON kad_keyword_source (kad_keyword_rowid, source_hash_high);");
	// Each source can published by several clients, referenced by source rowid.
	Exec("CREATE TABLE IF NOT EXISTS kad_keyword_publishIP ("
			"kad_keyword_publishIP_ID integer PRIMARY KEY AUTOINCREMENT,"  // make sure IPs are ordered in order of insertion / update
			"kad_keyword_source_rowid integer,"	// source to which this IP belongs
			"ip integer,"						// IP from which it was published
			"publish_time integer);");			// time of last reception
	Exec("CREATE INDEX IF NOT EXISTS idx_kad_keyword_publishIP ON kad_keyword_publishIP (kad_keyword_source_rowid, ip);");
	// Each source can have several file names, referenced by source rowid.
	Exec("CREATE TABLE IF NOT EXISTS kad_keyword_filename ("
			"kad_keyword_source_rowid integer,"	// source to which this filename belongs
			"filename string,"					// filename
			"popularityIndex integer);");		// how often was that filename used
	Exec("CREATE INDEX IF NOT EXISTS idx_kad_keyword_filename ON kad_keyword_filename (kad_keyword_source_rowid);");
	// Each source has a list of tags (for things like MP3 bitrate), referenced by source rowid.
	Exec("CREATE TABLE IF NOT EXISTS kad_keyword_tag ("
			"kad_keyword_source_rowid integer,"	// source to which this tag belongs
			"tagname string,"					// tag name
			"tagvalue,"							// value is typeless
			"tagnumber integer);");				// to keep tags ordered (should not matter, but keeps results comparable)
	Exec("CREATE INDEX IF NOT EXISTS idx_kad_keyword_tag ON kad_keyword_tag (kad_keyword_source_rowid);");
}


uint64 CDatabase::KadGetKeyword(uint64 hashlow, uint64 hashhigh)
{
	CStatement stmt(m_db);
	stmt.Prepare("SELECT rowid FROM kad_keyword WHERE keyword_hash_high = ?1 AND keyword_hash_low = ?2;");
	stmt.Bind(1, hashhigh);
	stmt.Bind(2, hashlow);
	int rc = stmt.Step();
	if (rc == SQLITE_ROW) {
		return stmt.ColumnUint64(0);
	} else {
		return 0;	// not found
	}
}


uint64 CDatabase::KadInsertKeyword(uint64 hashlow, uint64 hashhigh)
{
	CStatement stmt(m_db);
	stmt.Prepare("INSERT INTO kad_keyword (keyword_hash_low, keyword_hash_high) VALUES (?1, ?2);");
	stmt.Bind(1, hashlow);
	stmt.Bind(2, hashhigh);
	stmt.Step();
	return KadGetKeyword(hashlow, hashhigh);
}


uint64 CDatabase::KadGetSource(uint64 keyIndex, uint64 hashlow, uint64 hashhigh, uint64 size)
{
	CStatement stmt(m_db);
	stmt.Prepare("SELECT rowid FROM kad_keyword_source WHERE kad_keyword_rowid = ?1 AND source_hash_high = ?2 AND source_hash_low = ?3 AND source_size = ?4;");
	stmt.Bind(1, keyIndex);
	stmt.Bind(2, hashhigh);
	stmt.Bind(3, hashlow);
	stmt.Bind(4, size);
	int rc = stmt.Step();
	if (rc == SQLITE_ROW) {
		return stmt.ColumnUint64(0);
	} else {
		return 0;	// not found
	}
}


uint64 CDatabase::KadInsertSource(uint64 keyIndex, uint64 hashlow, uint64 hashhigh, uint64 size, uint32 lifetime)
{
	CStatement stmt(m_db);
	stmt.Prepare("INSERT INTO kad_keyword_source (kad_keyword_rowid, source_hash_low, source_hash_high, source_size, trustValue, lastTrustValueCalc, lifetime)"
				 "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7);");
	stmt.Bind(1, keyIndex);
	stmt.Bind(2, hashlow);
	stmt.Bind(3, hashhigh);
	stmt.Bind(4, size);
	stmt.Bind(5, 0.0);	// trustValue
	stmt.Bind(6, 0);	// lastTrustValueCalc
	stmt.Bind(7, lifetime);
	stmt.Step();
	return KadGetSource(keyIndex, hashlow, hashhigh, size);
}


void CDatabase::KadUpdateSource(uint64 sourceIndex, uint32 lifetime)
{
	CStatement stmt(m_db);
	stmt.Prepare("UPDATE kad_keyword_source SET lifetime = ?1 WHERE kad_keyword_rowid = ?2;");
	stmt.Bind(1, lifetime);
	stmt.Bind(2, sourceIndex);
	stmt.Step();
}


uint32 CDatabase::KadCountSource(uint64 keyIndex)
{
	CStatement stmt(m_db);
	stmt.Prepare("SELECT COUNT(1) FROM kad_keyword_source WHERE kad_keyword_rowid = ?1;");
	stmt.Bind(1, keyIndex);
	stmt.Step();
	return stmt.ColumnUint32(0);
}


int CDatabase::KadReplaceTaglist(uint64 sourceIndex, const TagPtrList * tags, bool clearOldTags)
{
	if (clearOldTags) {
		CStatement stmt1(m_db);
		stmt1.Prepare("DELETE FROM kad_keyword_tag WHERE kad_keyword_source_rowid = ?1;");
		stmt1.Bind(1, sourceIndex);
		stmt1.Step();
	}
	int tagsInserted = 0;
	if (tags) {
		CStatement stmt2(m_db);
		stmt2.Prepare("INSERT INTO kad_keyword_tag (kad_keyword_source_rowid, tagname, tagvalue, tagnumber) VALUES (?1, ?2, ?3, ?4);");
		stmt2.Bind(1, sourceIndex);
		for (TagPtrList::const_iterator it = tags->begin(); it != tags->end(); it++) {
			const CTag * tag = *it;
			if (tag->IsInt()) {
				stmt2.Bind(3, tag->GetInt());
			} else if (tag->IsFloat()) {
				stmt2.Bind(3, (double) tag->GetFloat());
			} else if (tag->IsStr()) {
				stmt2.Bind(3, tag->GetStr());
			} else {
				// reject other tags
				continue;
			}
			stmt2.Bind(2, tag->GetName());
			stmt2.Bind(4, ++tagsInserted);
			stmt2.Step();
			stmt2.Reset();
		}
	}
	/*
	for (int i = 1; i <= tagsInserted; i++) {
		CStatement stmt3(m_db);
		stmt3.Prepare("SELECT tagname, tagvalue FROM kad_keyword_tag WHERE kad_keyword_source_rowid = ?1 AND tagnumber = ?2;");
		stmt3.Bind(1, sourceIndex);
		stmt3.Bind(2, i);
		wxString name(wxT("???")), value(wxT("???"));
		int rc = stmt3.Step();
		if (rc == SQLITE_ROW) {
			name = stmt3.ColumnStr(0);
			value = stmt3.ColumnStr(1);
		}
		AddDebugLogLineN(logSQL, CFormat(wxT("Src %d Tag %d Name %s Value %s")) % sourceIndex % i % name % value);
	}
	*/

	return tagsInserted;
}


void CDatabase::KadSetSourcename(uint64 sourceIndex, const wxString& name, uint32 popularity, bool fastRefresh)
{
	wxString nameLower = name.Lower();
	CStatement stmt1(m_db);
	stmt1.Prepare("SELECT rowid, popularityIndex FROM kad_keyword_filename WHERE kad_keyword_source_rowid = ?1 AND lower(filename) = ?2;");
	stmt1.Bind(1, sourceIndex);
	stmt1.Bind(2, name.Lower());
	int rc = stmt1.Step();
	if (rc == SQLITE_ROW) {
		// already there, increase popularity
		if (!fastRefresh) {
			CStatement stmt2(m_db);
			stmt2.Prepare("UPDATE kad_keyword_filename SET popularityIndex = ?1 WHERE rowid = ?2;");
			stmt2.Bind(1, stmt1.ColumnUint32(1) + 1);
			stmt2.Bind(2, stmt1.ColumnUint64(0));
			stmt2.Step();
		}
	} else {
		// insert new name
		CStatement stmt2(m_db);
		stmt2.Prepare("INSERT INTO kad_keyword_filename (kad_keyword_source_rowid, filename, popularityIndex) VALUES (?1, ?2, ?3);");
		stmt2.Bind(1, sourceIndex);
		stmt2.Bind(2, name);
		stmt2.Bind(3, popularity);
		stmt2.Step();
	}
}


// Set an ip for a source, returns 0: new ip, 1: refresh, 2: fast refresh
int	CDatabase::KadSetKeywordPublishIP(uint64 sourceIndex, uint32 ip, uint32 lastPublish)
{
	int refresh = 0;
	bool normalMode = false;	// true: normal operation mode   false: import data from saved file, special handling
	if (lastPublish == 0) {
		normalMode = true;
		lastPublish = time(NULL);
		CStatement stmt1(m_db);
		stmt1.Prepare("SELECT kad_keyword_publishIP_ID, publish_time FROM kad_keyword_publishIP WHERE kad_keyword_source_rowid = ?1 AND ip = ?2;");
		stmt1.Bind(1, sourceIndex);
		stmt1.Bind(2, ip);
		int rc = stmt1.Step();
		if (rc == SQLITE_ROW) {
			// Already there, so it's a refresh. Remove row and reinsert it to keep IPs ordered by publish time
			CStatement stmt2(m_db);
			stmt2.Prepare("DELETE FROM kad_keyword_publishIP WHERE kad_keyword_publishIP_ID = ?1;");
			stmt2.Bind(1, stmt1.ColumnUint64(0));
			stmt2.Step();
			// check if it was a fast refresh
			refresh = (lastPublish - stmt1.ColumnUint32(1)) < (KADEMLIAREPUBLISHTIMES - HR2S(1)) ? 2 : 1;
		}
	}
	// insert new ip
	CStatement stmt3(m_db);
	stmt3.Prepare("INSERT INTO kad_keyword_publishIP (kad_keyword_source_rowid, ip, publish_time) VALUES (?1, ?2, ?3);");
	stmt3.Bind(1, sourceIndex);
	stmt3.Bind(2, ip);
	stmt3.Bind(3, lastPublish);
	stmt3.Step();
	if (!refresh) {
		// Adjust tracking
		if (normalMode) {	// another thing used only to make import of saved files work.
			AdjustGlobalPublishTracking(ip, true, wxT("new publisher"));
		}
		// Check/adjust limit
		CStatement stmt4(m_db);
		stmt4.Prepare("SELECT publishIPs FROM kad_keyword_source WHERE rowid = ?1;");
		stmt4.Bind(1, sourceIndex);
		stmt4.Step();
		uint32 publishIPs = stmt4.ColumnUint32(0);
		// we keep track of max 100 IPs, in order to avoid too much time for calculation/storing/loading.
		if (publishIPs >= 100) {
			// drop oldest ip from list: 
			// find it,
			CStatement stmt5(m_db);
			stmt5.Prepare("SELECT kad_keyword_publishIP_ID, ip FROM kad_keyword_publishIP WHERE kad_keyword_source_rowid = ?1 ORDER BY kad_keyword_publishIP_ID;");
			stmt5.Bind(1, sourceIndex);
			stmt5.Step();
			// adjust tracking,
			if (normalMode) {
				AdjustGlobalPublishTracking(stmt5.ColumnUint32(1), false, wxT("more than 100 publishers purge"));
			}
			// and remove it
			CStatement stmt6(m_db);
			stmt6.Prepare("DELETE FROM kad_keyword_publishIP WHERE kad_keyword_publishIP_ID = ?1;");
			stmt6.Bind(1, stmt5.ColumnUint64(0));
			stmt6.Step();
		} else {
			// increase counter
			CStatement stmt6(m_db);
			stmt6.Prepare("UPDATE kad_keyword_source SET publishIPs = ?1 WHERE rowid = ?2;");
			stmt6.Bind(1, publishIPs + 1);
			stmt6.Bind(2, sourceIndex);
			stmt6.Step();
		}
		// Since we added a new publisher, we want to (re)calculate the trust value for this entry before we use it again,
		// so mark it as invalid for now.
		if (normalMode) {
			KadClearTrustValue(sourceIndex);
		}
	}
	return refresh;
}


void CDatabase::AdjustGlobalPublishTracking(uint32_t ip, bool increase, const wxString& DEBUG_ONLY(dbgReason))
{
	uint32_t count = 0;
	uint32_t maskedIP = ip & 0xFFFFFF00; // /24 netmask, take care of endian if needed
	bool found = false;
	GlobalPublishIPTrackingMap::const_iterator it = m_globalPublishIPs.find(maskedIP);
	if (it != m_globalPublishIPs.end()) {
		count = it->second;
		found = true;
	}

	if (increase) {
		count++;
	} else {
		count--;
	}

	if (found && count == 0) {
		m_globalPublishIPs.erase(maskedIP);
	} else if (found || increase) {
		m_globalPublishIPs[maskedIP] = count;
	} else {
		wxFAIL;
	}
#ifdef __DEBUG__
	if (!dbgReason.IsEmpty()) {
		AddDebugLogLineN(logKadEntryTracking, CFormat(wxT("%s %s (%s) - (%s), new count %u"))
			% (increase ? wxT("Adding") : wxT("Removing")) % KadIPToString(maskedIP) % KadIPToString(ip) % dbgReason % count);
	}
#endif
}


double CDatabase::KadRecalculateTrustValue(uint64 sourceIndex)
{
#define PUBLISHPOINTSSPERSUBNET	10.0
	// The trustvalue is supposed to be an indicator how trustworthy/important (or spammy) this entry is and lies between 0 and ~10000,
	// but mostly we say everything below 1 is bad, everything above 1 is good. It is calculated by looking at how many different
	// IPs/24 have published this entry and how many entries each of those IPs have.
	// Each IP/24 has x (say 3) points. This means if one IP publishes 3 different entries without any other IP publishing those entries,
	// each of those entries will have 3 / 3 = 1 Trustvalue. Thats fine. If it publishes 6 alone, each entry has 3 / 6 = 0.5 trustvalue - not so good
	// However if there is another publisher for entry 5, which only publishes this entry then we have 3/6 + 3/1 = 3.5 trustvalue for this entry
	//
	// What's the point? With this rating we try to avoid getting spammed with entries for a given keyword by a small IP range, which blends out
	// all other entries for this keyword do to its amount as well as giving an indicator for the searcher. So if we are the node to index "Knoppix", and someone
	// from 1 IP publishes 500 times "knoppix casino 500% bonus.txt", all those entries will have a trustvalue of 0.006 and we make sure that
	// on search requests for knoppix, those entries are only returned after all entries with a trustvalue > 1 were sent (if there is still space).
	//
	// Its important to note that entry with < 1 do NOT get ignored or singled out, this only comes into play if we have 300 more results for
	// a search request rating > 1

	double trustValue = 0.0;

	CStatement stmt1(m_db);
	stmt1.Prepare("SELECT ip FROM kad_keyword_publishIP WHERE kad_keyword_source_rowid = ?1;");
	stmt1.Bind(1, sourceIndex);
	while (stmt1.Step() == SQLITE_ROW) {
		uint32 count = 0;
		uint32 ip = stmt1.ColumnUint32(0);
		GlobalPublishIPTrackingMap::const_iterator itMap = m_globalPublishIPs.find(ip & 0xFFFFFF00);  // /24 netmask, take care of endian if needed
		if (itMap != m_globalPublishIPs.end()) {
			count = itMap->second;
		}
		if (count > 0) {
			trustValue += PUBLISHPOINTSSPERSUBNET / count;
		} else {
			AddDebugLogLineC(logKadEntryTracking, wxT("Inconsistency in KadRecalculateTrustValue()"));
			wxFAIL;
		}
	}
	wxASSERT(trustValue > 0.0);

	CStatement stmt2(m_db);
	stmt2.Prepare("UPDATE kad_keyword_source SET trustValue = ?1, lastTrustValueCalc = ?2 WHERE rowid = ?3;");
	stmt2.Bind(1, trustValue);
	stmt2.Bind(2, (uint32) time(NULL));
	stmt2.Bind(3, sourceIndex);
	stmt2.Step();

	return trustValue;
}


void CDatabase::KadClearTrustValue(uint64 sourceIndex)
{
	CStatement stmt1(m_db);
	stmt1.Prepare("UPDATE kad_keyword_source SET lastTrustValueCalc = ?1 WHERE rowid = ?2;");
	stmt1.Bind(1, 0);
	stmt1.Bind(2, sourceIndex);
	stmt1.Step();
}


double CDatabase::KadGetTrustValue(uint64 sourceIndex)
{
	CStatement stmt1(m_db);
	stmt1.Prepare("SELECT trustValue, lastTrustValueCalc FROM kad_keyword_source WHERE rowid = ?1;");
	stmt1.Bind(1, sourceIndex);
	stmt1.Step();
	double trustValue = stmt1.ColumnDouble(0);

	// update if last calcualtion is too old
	if (time(NULL) - stmt1.ColumnUint32(1) > MIN2S(10)) {
		trustValue = KadRecalculateTrustValue(sourceIndex);
	}
	return trustValue;
}


void CDatabase::KadStartupCleanup()
{
	// force recalculation of all trust values
	Exec("UPDATE kad_keyword_source SET lastTrustValueCalc = 0;");
	// rebuild GlobalPublishIPTrackingMap
	m_globalPublishIPs.clear();
	// iterate over all ips
	CStatement stmt1(m_db);
	stmt1.Prepare("SELECT ip FROM kad_keyword_publishIP;");
	while (stmt1.Step() == SQLITE_ROW) {
		AdjustGlobalPublishTracking(stmt1.ColumnUint32(0), true, wxT("KadStartupCleanup"));// wxEmptyString);
	}
}




