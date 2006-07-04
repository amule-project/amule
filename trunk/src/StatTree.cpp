//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (C) 2005-2006Dévai Tamás ( gonosztopi@amule.org )
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

#include "StatTree.h"

#ifndef EC_REMOTE


#ifndef AMULE_DAEMON
#include <common/Format.h>		// Needed for CFormat

#define a_brackets_b(a,b)	(a + wxT(" (") + b + wxT(")"))

#endif /* !AMULE_DAEMON */

#endif /* !EC_REMOTE */

#include <ec/ECTag.h>		// Needed for CECTag

#include <wx/intl.h>

#ifdef EC_REMOTE
	#include <ec/ECSpecialTags.h>	// Needed for CEC_StatTree_Node_Tag
#endif

/* CStatTreeItemBase */

#ifdef EC_REMOTE
CStatTreeItemBase::CStatTreeItemBase(const CECTag* tag)
	: m_label(((CEC_StatTree_Node_Tag*)tag)->GetDisplayString())
{
	wxASSERT(tag->GetTagName() == EC_TAG_STATTREE_NODE);

	for (int i = 0; i < tag->GetTagCount(); ++i) {
		const CECTag* tmp = tag->GetTagByIndex(i);
		if (tmp->GetTagName() == EC_TAG_STATTREE_NODE) {
			m_children.push_back(new CStatTreeItemBase(tmp));
		}
	}
}
#endif /* EC_REMOTE */

CStatTreeItemBase::~CStatTreeItemBase()
{
	for (std::list<CStatTreeItemBase*>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
		delete *it;
	}
	m_children.clear();
}

#ifndef EC_REMOTE
CStatTreeItemBase* CStatTreeItemBase::AddChild(CStatTreeItemBase* child, uint32_t id, bool skipOneLevel)
{
	wxMutexLocker lock(m_lock);

	if (skipOneLevel) {
		child->m_parent = m_parent;
	} else {
		child->m_parent = this;
	}
	child->m_id = id;

	if (m_flags & stSortChildren) {
		std::list<CStatTreeItemBase*>::iterator it = m_children.begin();
		while (it != m_children.end() && id < (*it)->m_id) {
			++it;
		}
		m_children.insert(it, child);
	} else {
		m_children.push_back(child);
	}
	return child;
}
#endif /* !EC_REMOTE */

bool CStatTreeItemBase::HasVisibleChildren()
{
	wxMutexLocker lock(m_lock);

	for (std::list<CStatTreeItemBase*>::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
		if ((*it)->IsVisible()) {
			return true;
		}
	}
	return false;
}

#ifndef EC_REMOTE
bool CStatTreeItemBase::HasChildWithId(uint32_t id)
{
	wxMutexLocker lock(m_lock);

	for (std::list<CStatTreeItemBase*>::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
		if ((*it)->m_id == id) {
			return true;
		}
	}
	return false;
}

CStatTreeItemBase* CStatTreeItemBase::GetChildById(uint32_t id)
{
	wxMutexLocker lock(m_lock);

	for (std::list<CStatTreeItemBase*>::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
		if ((*it)->m_id == id) {
			return *it;
		}
	}
	return NULL;
}
#endif /* !EC_REMOTE */

// Note: these functions do not lock the list, because it is already locked at the time they're called
#ifndef EC_REMOTE
StatTreeItemIterator CStatTreeItemBase::GetFirstVisibleChild(uint32_t max_children)
{
	StatTreeItemIterator it = m_children.begin();
	m_visible_counter = --max_children;
	while (it != m_children.end() && !(*it)->IsVisible()) ++it;
	return it;
}

void CStatTreeItemBase::GetNextVisibleChild(StatTreeItemIterator& it)
{
	if (m_flags & stCapChildren) {
		if (m_visible_counter == 0) {
			it = m_children.end();
			return;
		} else {
			--m_visible_counter;
		}
	}
	if (it != m_children.end()) ++it;
	while (it != m_children.end() && !(*it)->IsVisible()) ++it;
}
#endif

//
// Anything below is only for core.
//
#ifndef EC_REMOTE

bool CStatTreeItemBase::ValueSort(CStatTreeItemBase* a, CStatTreeItemBase* b)
{
	if (a->m_id < 0x00000100 || a->m_id > 0x7fffffff || b->m_id < 0x00000100 || b->m_id > 0x7fffffff) {
		return a->m_id > b->m_id;
	} else {
		return ((CStatTreeItemCounter*)a)->GetValue() > ((CStatTreeItemCounter*)b)->GetValue();
	}
}

#ifndef AMULE_DAEMON
wxString CStatTreeItemBase::GetDisplayString() const
{
	return wxGetTranslation(m_label);
}
#endif

CECTag* CStatTreeItemBase::CreateECTag(uint32_t max_children)
{
	if (IsVisible()) {
		wxMutexLocker lock(m_lock);
		CECTag* tag = new CECTag(EC_TAG_STATTREE_NODE, m_label);
		AddECValues(tag);
		m_visible_counter = max_children - 1;
		for (std::list<CStatTreeItemBase*>::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
			CECTag* tmp = (*it)->CreateECTag(max_children);
			if (tmp) {
				tag->AddTag(*tmp);
				delete tmp;
				if (m_flags & stCapChildren) {
					if (m_visible_counter == 0) {
						break;
					} else {
						--m_visible_counter;
					}
				}
			}
		}
		return tag;
	} else {
		return NULL;
	}
}


/* CStatTreeItemSimple */

#ifndef AMULE_DAEMON
wxString CStatTreeItemSimple::GetDisplayString() const
{
	switch (m_valuetype) {
		case vtInteger:
			switch (m_displaymode) {
				case dmTime:	return CFormat(wxGetTranslation(m_label)) % CastSecondsToHM(m_intvalue);
				case dmBytes:	return CFormat(wxGetTranslation(m_label)) % CastItoXBytes(m_intvalue);
				default:	return CFormat(wxGetTranslation(m_label)) % m_intvalue;
			}
		case vtFloat:	return wxString::Format(wxGetTranslation(m_label), m_floatvalue);
		case vtString:	return CFormat(wxGetTranslation(m_label)) % m_stringvalue;
		default:	return wxGetTranslation(m_label);
	}
}
#endif

bool CStatTreeItemSimple::IsVisible() const
{
	if (m_flags & stHideIfZero) {
		switch (m_valuetype) {
			case vtInteger:	return m_intvalue != 0;
			case vtFloat:	return m_floatvalue != 0.0;
			case vtString:	return !m_stringvalue.IsEmpty();
			default: return false;
		}
	}
	return true;
}

void CStatTreeItemSimple::AddECValues(CECTag* tag) const
{
	switch (m_valuetype) {
		case vtInteger: {
			 if (m_displaymode == dmTime) {
				 CECTag value(EC_TAG_STAT_NODE_VALUE, (uint32)m_intvalue);
				 value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_TIME));
				 tag->AddTag(value);
			 } else {
				 CECTag value(EC_TAG_STAT_NODE_VALUE, (uint64)m_intvalue);
				 if (m_displaymode == dmBytes) {
					 value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_BYTES));
				 }
				 tag->AddTag(value);
			 }
			 break;
		}
		case vtFloat: {
			CECTag value(EC_TAG_STAT_NODE_VALUE, m_floatvalue);
			value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_DOUBLE));
			tag->AddTag(value);
			break;
		}
		case vtString: {
			CECTag value(EC_TAG_STAT_NODE_VALUE, m_stringvalue);
			value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_STRING));
			tag->AddTag(value);
			break;
		}
		default:
			break;
	}
}


/* CStatTreeItemCounter */

#ifndef AMULE_DAEMON
wxString CStatTreeItemCounter::GetDisplayString() const
{
	wxString my_label = wxGetTranslation(m_label);
	// This is needed for client names, for example
	if (my_label == m_label) {
		if (m_label.Right(4) == wxT(": %s")) {
			my_label = wxGetTranslation(m_label.Mid(0, m_label.Length() - 4)) + wxString(wxT(": %s"));
		}
	}
	CFormat label(my_label);
	if (m_displaymode == dmBytes) {
		return label % CastItoXBytes(m_value);
	} else {
		wxString result = CFormat(wxT("%u")) % m_value;
		if ((m_flags & stShowPercent) && m_parent) {
			result.append(wxString::Format(wxT(" (%.2f%%)"), ((double)m_value / ((CStatTreeItemCounter*)m_parent)->m_value) * 100.0));
		}
		return label % result;
	}
}
#endif

void CStatTreeItemCounter::AddECValues(CECTag* tag) const
{
	CECTag value(EC_TAG_STAT_NODE_VALUE, (uint64)m_value);
	if (m_displaymode == dmBytes) {
		value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_BYTES));
	} else {
		value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_ISTRING));
		if ((m_flags & stShowPercent) && m_parent) {
			CECTag tmp(EC_TAG_STAT_NODE_VALUE, ((double)m_value / ((CStatTreeItemCounter*)m_parent)->m_value) * 100.0);
			tmp.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_DOUBLE));
			value.AddTag(tmp);
		}
	}
	tag->AddTag(value);
}


#ifndef USE_64BIT_ARCH
/* CStatTreeItemNativeCounter */

#ifndef AMULE_DAEMON
wxString CStatTreeItemNativeCounter::GetDisplayString() const
{
	wxString my_label = wxGetTranslation(m_label);
	if (my_label == m_label) {
		if (m_label.Right(4) == wxT(": %s")) {
			my_label = wxGetTranslation(m_label.Mid(0, m_label.Length() - 4)) + wxString(wxT(": %s"));
		}
	}
	CFormat label(my_label);
	if (m_displaymode == dmBytes) {
		return label % CastItoXBytes(m_value);
	} else {
		wxString result = CFormat(wxT("%u")) % m_value;
		if ((m_flags & stShowPercent) && m_parent) {
			result.append(wxString::Format(wxT(" (%.2f%%)"), ((double)m_value / ((CStatTreeItemNativeCounter*)m_parent)->m_value) * 100.0));
		}
		return label % result;
	}
}
#endif

void CStatTreeItemNativeCounter::AddECValues(CECTag* tag) const
{
	CECTag value(EC_TAG_STAT_NODE_VALUE, (uint64)m_value);
	if (m_displaymode == dmBytes) {
		value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_BYTES));
	} else {
		value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_ISTRING));
		if ((m_flags & stShowPercent) && m_parent) {
			CECTag tmp(EC_TAG_STAT_NODE_VALUE, ((double)m_value / ((CStatTreeItemNativeCounter*)m_parent)->m_value) * 100.0);
			tmp.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_DOUBLE));
			value.AddTag(tmp);
		}
	}
	tag->AddTag(value);
}
#endif /* USE_64BIT_ARCH */


/* CStatTreeItemUlDlCounter */

#ifndef AMULE_DAEMON
wxString CStatTreeItemUlDlCounter::GetDisplayString() const
{
	return CFormat(wxGetTranslation(m_label)) % a_brackets_b(CastItoXBytes(m_value), CastItoXBytes(m_value + m_totalfunc()));
};
#endif

void CStatTreeItemUlDlCounter::AddECValues(CECTag* tag) const
{
	CECTag value(EC_TAG_STAT_NODE_VALUE, m_value);
	value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_BYTES));
	CECTag tmp(EC_TAG_STAT_NODE_VALUE, m_value + m_totalfunc());
	tmp.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_BYTES));
	value.AddTag(tmp);
	tag->AddTag(value);
}


/* CStatTreeItemCounterMax */

#ifndef AMULE_DAEMON
wxString CStatTreeItemCounterMax::GetDisplayString() const
{
	return wxString::Format(wxGetTranslation(m_label), m_value);
}
#endif

void CStatTreeItemCounterMax::AddECValues(CECTag* tag) const
{
	tag->AddTag(CECTag(EC_TAG_STAT_NODE_VALUE, (uint64)m_value));
}


/* CStatTreeItemPackets */

#ifndef AMULE_DAEMON
wxString CStatTreeItemPackets::GetDisplayString() const
{
	return CFormat(wxGetTranslation(m_label)) % a_brackets_b(CastItoXBytes(m_bytes), CastItoIShort(m_packets));
}
#endif

void CStatTreeItemPackets::AddECValues(CECTag* tag) const
{
	CECTag value(EC_TAG_STAT_NODE_VALUE, m_bytes);
	value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_BYTES));
	CECTag tmp(EC_TAG_STAT_NODE_VALUE, (uint64)m_packets);
	tmp.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_ISHORT));
	value.AddTag(tmp);
	tag->AddTag(value);
}


/* CStatTreeItemPacketTotals */

#ifndef AMULE_DAEMON
wxString CStatTreeItemPacketTotals::GetDisplayString() const
{
	uint32_t tmp_packets = m_packets;
	uint64_t tmp_bytes = m_bytes;

	for (std::vector<CStatTreeItemPackets*>::const_iterator it = m_counters.begin(); it != m_counters.end(); ++it) {
		tmp_packets += (*it)->m_packets;
		tmp_bytes += (*it)->m_bytes;
	}
	return CFormat(wxGetTranslation(m_label)) % a_brackets_b(CastItoXBytes(tmp_bytes), CastItoIShort(tmp_packets));
}
#endif

void CStatTreeItemPacketTotals::AddECValues(CECTag* tag) const
{
	uint32_t tmp_packets = m_packets;
	uint64_t tmp_bytes = m_bytes;

	for (std::vector<CStatTreeItemPackets*>::const_iterator it = m_counters.begin(); it != m_counters.end(); ++it) {
		tmp_packets += (*it)->m_packets;
		tmp_bytes += (*it)->m_bytes;
	}

	CECTag value(EC_TAG_STAT_NODE_VALUE, tmp_bytes);
	value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_BYTES));
	CECTag tmp(EC_TAG_STAT_NODE_VALUE, (uint64)tmp_packets);
	tmp.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_ISHORT));
	value.AddTag(tmp);
	tag->AddTag(value);
}


/* CStatTreeItemTimer */

#ifndef AMULE_DAEMON
wxString CStatTreeItemTimer::GetDisplayString() const
{
	return CFormat(wxGetTranslation(m_label)) % CastSecondsToHM(m_value ? GetTimerSeconds() : 0);
}
#endif

void CStatTreeItemTimer::AddECValues(CECTag* tag) const
{
	CECTag value(EC_TAG_STAT_NODE_VALUE, m_value ? (uint32)GetTimerSeconds() : (uint32)0);
	value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_TIME));
	tag->AddTag(value);
}


/* CStatTreeItemAverage */

#ifndef AMULE_DAEMON
wxString CStatTreeItemAverage::GetDisplayString() const
{
	if ((*m_divisor) != 0) {
		switch (m_displaymode) {
			case dmBytes:	return CFormat(wxGetTranslation(m_label)) % CastItoXBytes((*m_dividend)/(*m_divisor));
			case dmTime:	return CFormat(wxGetTranslation(m_label)) % CastSecondsToHM((*m_dividend)/(*m_divisor));
			default:	return CFormat(wxGetTranslation(m_label)) % (CFormat(wxT("%u")) % ((uint64)(*m_dividend)/(*m_divisor))).GetString();
		}
	} else {
		return CFormat(wxGetTranslation(m_label)) % wxT("-");
	}
}
#endif

void CStatTreeItemAverage::AddECValues(CECTag* tag) const
{
	if ((*m_divisor) != 0) {
		uint64 data = (*m_dividend)/(*m_divisor);
		if (m_displaymode == dmTime) {
			CECTag value(EC_TAG_STAT_NODE_VALUE, (uint32)data);
			value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_TIME));
			tag->AddTag(value);
		} else {
			CECTag value(EC_TAG_STAT_NODE_VALUE, data);
			if (m_displaymode == dmBytes) {
				value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_BYTES));
			}
			tag->AddTag(value);
		}
	} else {
		CECTag value(EC_TAG_STAT_NODE_VALUE, wxString(wxT("-")));
		value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_STRING));
		tag->AddTag(value);
	}
}


/* CStatTreeItemAverageSpeed */

#ifndef AMULE_DAEMON
wxString CStatTreeItemAverageSpeed::GetDisplayString() const
{
	uint64 time = m_timer->GetTimerSeconds();
	if (time) {
		return CFormat(wxGetTranslation(m_label)) % CastItoSpeed((*m_counter)/time);
	} else {
		return CFormat(wxGetTranslation(m_label)) % CastItoSpeed(0);
	}
}
#endif

void CStatTreeItemAverageSpeed::AddECValues(CECTag* tag) const
{
	uint64 time = m_timer->GetTimerSeconds();
	if (time) {
		CECTag value(EC_TAG_STAT_NODE_VALUE, (uint32)((*m_counter)/time));
		value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_SPEED));
		tag->AddTag(value);
	} else {
		CECTag value(EC_TAG_STAT_NODE_VALUE, (uint32)0);
		value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_SPEED));
		tag->AddTag(value);
	}
}


/* CStatTreeItemRatio */

#ifndef AMULE_DAEMON
wxString CStatTreeItemRatio::GetDisplayString() const
{
	if (m_counter1->GetValue() && m_counter2->GetValue()) {
		if ((*m_counter2) < (*m_counter1)) {
			return CFormat(wxGetTranslation(m_label)) % wxString::Format(wxT("%.2f : 1"), (float)(*m_counter1)/(*m_counter2));
		} else {
			return CFormat(wxGetTranslation(m_label)) % wxString::Format(wxT("1 : %.2f"), (float)(*m_counter2)/(*m_counter1));
		}
	} else {
		return CFormat(wxGetTranslation(m_label)) % _("Not available");
	}
}
#endif

void CStatTreeItemRatio::AddECValues(CECTag* tag) const
{
	wxString result;
	if (m_counter1->GetValue() && m_counter2->GetValue()) {
		if ((*m_counter2) < (*m_counter1)) {
			result = wxString::Format(wxT("%.2f : 1"), (float)(*m_counter1)/(*m_counter2));
		} else {
			result = wxString::Format(wxT("1 : %.2f"), (float)(*m_counter2)/(*m_counter1));
		}
	} else {
		result = wxTRANSLATE("Not available");
	}

	CECTag value(EC_TAG_STAT_NODE_VALUE, result);
	value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_STRING));
	tag->AddTag(value);
}


/* CStatTreeItemReconnects */

#ifndef AMULE_DAEMON
wxString CStatTreeItemReconnects::GetDisplayString() const
{
	return CFormat(wxGetTranslation(m_label)) % (m_value ? m_value - 1 : 0);
}
#endif

void CStatTreeItemReconnects::AddECValues(CECTag* tag) const
{
	tag->AddTag(CECTag(EC_TAG_STAT_NODE_VALUE, m_value ? (uint64)(m_value - 1) : (uint64)0));
}

/* CStatTreeItemMaxConnLimitReached */

#ifndef AMULE_DAEMON
wxString CStatTreeItemMaxConnLimitReached::GetDisplayString() const
{
	if (m_count) {
		return CFormat(wxGetTranslation(m_label)) % (wxString::Format(wxT("%i : "), m_count) + m_time.FormatISODate() + wxT(" ") + m_time.FormatISOTime());
	} else {
		return CFormat(wxGetTranslation(m_label)) % _("Never");
	}
}
#endif

void CStatTreeItemMaxConnLimitReached::AddECValues(CECTag *tag) const
{
	wxString result;
	if (m_count) {
		result = wxString::Format(wxT("%i : "), m_count) + m_time.FormatISODate() + wxT(" ") + m_time.FormatISOTime();
	} else {
		result = wxTRANSLATE("Never");
	}

	CECTag value(EC_TAG_STAT_NODE_VALUE, result);
	value.AddTag(CECTag(EC_TAG_STAT_VALUE_TYPE, (uint8)EC_VALUE_STRING));
	tag->AddTag(value);
}

/* CStatTreeItemTotalClients */

#ifndef AMULE_DAEMON
wxString CStatTreeItemTotalClients::GetDisplayString() const
{
	return CFormat(wxGetTranslation(m_label)) % (m_known->GetValue() + m_unknown->GetValue()) % m_known->GetValue();
}
#endif

void CStatTreeItemTotalClients::AddECValues(CECTag* tag) const
{
	CECTag value1(EC_TAG_STAT_NODE_VALUE, (uint64)(m_known->GetValue() + m_unknown->GetValue()));
	tag->AddTag(value1);
	CECTag value2(EC_TAG_STAT_NODE_VALUE, (uint64)m_known->GetValue());
	tag->AddTag(value2);
}

#endif /* !EC_REMOTE */
// File_checked_for_headers
