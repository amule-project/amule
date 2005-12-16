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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <wx/string.h>
#include <wx/arrstr.h>

enum ESearchOperators
{
	SEARCHOP_AND,
	SEARCHOP_OR,
	SEARCHOP_NOT
};

#define	SEARCHOPTOK_AND	wxT("\255AND")
#define	SEARCHOPTOK_OR	wxT("\255OR")
#define	SEARCHOPTOK_NOT	wxT("\255NOT")

class CSearchExpr
{
public:
	CSearchExpr(){}
	CSearchExpr(const wxString& pszString)
	{
		Add(pszString);
	}
	
	void Add(ESearchOperators eOperator) 
	{
		if (eOperator == SEARCHOP_AND) {
			m_aExpr.Add(SEARCHOPTOK_AND);
		}
		if (eOperator == SEARCHOP_OR) {
			m_aExpr.Add(SEARCHOPTOK_OR);
		}
		if (eOperator == SEARCHOP_NOT) {
			m_aExpr.Add(SEARCHOPTOK_NOT);
		}
	}
	
	void Add(const wxString& pszString)
	{
		m_aExpr.Add(pszString);
	}

	void Add(const CSearchExpr* pexpr)
	{
		//m_aExpr.Append(pexpr->m_aExpr);
		for (unsigned int i=0; i < pexpr->m_aExpr.Count(); ++i) {
			m_aExpr.Add(pexpr->m_aExpr[i]);
		}
	}
	
	void Concatenate(const wxString& pstrString)
	{
		wxASSERT( m_aExpr.Count() == 1 );
		m_aExpr[0] += ' ';
		m_aExpr[0] += pstrString;
	}
	
	wxArrayString m_aExpr;
};
