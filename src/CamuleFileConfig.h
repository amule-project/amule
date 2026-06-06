//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
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

#ifndef CAMULEFILECONFIG_H
#define CAMULEFILECONFIG_H

#include <wx/fileconf.h>

#include "CCtypeAsciiScope.h"


// wxFileConfig subclass that wraps every read / write / flush /
// path-navigating call with CCtypeAsciiScope. amule.conf is parsed
// and written through this subclass exclusively (see
// amuleAppCommon::Initialize), so wxFileConfig's internal lookups
// always run under a deterministic ASCII case-fold regardless of the
// user's UI language.
//
// The Read / Has / Delete / Rename / SetPath overrides matter
// because they all walk the same sorted m_aEntries / m_aSubgroups
// arrays via CmpNoCase and would silently miss entries under a
// non-C locale. The Write overrides are the ones that turn a missed
// lookup into a *persistent* duplicate by appending a new entry.
class CamuleFileConfig : public wxFileConfig
{
public:
	using wxFileConfig::wxFileConfig;

	bool Flush(bool bCurrentOnly = false) override
	{
		CCtypeAsciiScope scope;
		return wxFileConfig::Flush(bCurrentOnly);
	}

	bool HasGroup(const wxString& strName) const override
	{
		CCtypeAsciiScope scope;
		return wxFileConfig::HasGroup(strName);
	}

	bool HasEntry(const wxString& strName) const override
	{
		CCtypeAsciiScope scope;
		return wxFileConfig::HasEntry(strName);
	}

	bool DeleteEntry(const wxString& key, bool bGroupIfEmptyAlso = true) override
	{
		CCtypeAsciiScope scope;
		return wxFileConfig::DeleteEntry(key, bGroupIfEmptyAlso);
	}

	bool DeleteGroup(const wxString& szKey) override
	{
		CCtypeAsciiScope scope;
		return wxFileConfig::DeleteGroup(szKey);
	}

	bool RenameEntry(const wxString& oldName, const wxString& newName) override
	{
		CCtypeAsciiScope scope;
		return wxFileConfig::RenameEntry(oldName, newName);
	}

	bool RenameGroup(const wxString& oldName, const wxString& newName) override
	{
		CCtypeAsciiScope scope;
		return wxFileConfig::RenameGroup(oldName, newName);
	}

	void SetPath(const wxString& strPath) override
	{
		CCtypeAsciiScope scope;
		wxFileConfig::SetPath(strPath);
	}

protected:
	bool DoReadString(const wxString& key, wxString* pStr) const override
	{
		CCtypeAsciiScope scope;
		return wxFileConfig::DoReadString(key, pStr);
	}

	bool DoReadLong(const wxString& key, long* pl) const override
	{
		CCtypeAsciiScope scope;
		return wxFileConfig::DoReadLong(key, pl);
	}

#if wxUSE_BASE64
	bool DoReadBinary(const wxString& key, wxMemoryBuffer* buf) const override
	{
		CCtypeAsciiScope scope;
		return wxFileConfig::DoReadBinary(key, buf);
	}
#endif

	bool DoWriteString(const wxString& key, const wxString& szValue) override
	{
		CCtypeAsciiScope scope;
		return wxFileConfig::DoWriteString(key, szValue);
	}

	bool DoWriteLong(const wxString& key, long lValue) override
	{
		CCtypeAsciiScope scope;
		return wxFileConfig::DoWriteLong(key, lValue);
	}

#if wxUSE_BASE64
	bool DoWriteBinary(const wxString& key, const wxMemoryBuffer& buf) override
	{
		CCtypeAsciiScope scope;
		return wxFileConfig::DoWriteBinary(key, buf);
	}
#endif
};

#endif // CAMULEFILECONFIG_H
