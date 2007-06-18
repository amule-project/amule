//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (C) 2005-2007 Dévai Tamás ( gonosztopi@amule.org )
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

#ifndef STATTREE_H
#define STATTREE_H

/**
 * @file StatTree.h
 *
 * Interface to stat tree nodes.
 *
 * This file defines various classes representing
 * statistics tree nodes.
 */


#ifndef EC_REMOTE	// CLIENT_GUI

// Try to determine sizeof(int)
#if defined(__INT_MAX__) && defined(__LONG_MAX__)
	#if __INT_MAX__ != __LONG_MAX__
		#define USE_64BIT_ARCH
	#endif
#else
	// #include <bits/wordsize.h> would be enough, but we shouldn't depend on
	// non-standard includes. However, we still do it :)
	#include <stdint.h>
	#ifdef __WORDSIZE
		#if __WORDSIZE == 64
			#define USE_64BIT_ARCH
		#endif
	#endif
#endif
// If we cannot determine wordsize, assume that we use 32bit arch.

#define VIRTUAL virtual

#else
#define VIRTUAL
#endif /* !EC_REMOTE / EC_REMOTE */


#include <list>			// Needed for std::list
#include <wx/string.h>		// Needed for wxString
#include <wx/thread.h>		// Needed for wxMutex

#ifndef EC_REMOTE

#include <wx/datetime.h>	// Needed for wxDateTime
#include "GetTickCount.h"	// Needed for GetTickCount64()


/**
 * Stat tree flags
 */
enum EStatTreeFlags
{
	stNone		= 0,		///< Nothing. Really.
	stSortChildren	= 1,		///< Childrens are sorted descending by their ID.
	stShowPercent	= 2,		/*!< Shows percentage compared to parent.
					 *   Counters only, whose parent is also counter! (khmm...)
					 */
	stHideIfZero	= 4,		///< Hides item (and children) if value is zero.
	stSortByValue	= 8,		/*!< Together with stSortChildren, sorts children by their value.
					 *   WARNING! This assumes that value-sorted children are
					 *   counters!
					 *   Sort-by-value works only on children with ID between 0x00000100-0x7fffffff.
					 *   @note CStatTreeItemBase::ReSortChildren() must be called to get sort order right.
					 */
	stCapChildren	= 16		///< Caps children list.
					/*!< Shows only top n children, where n is set by CStatisticsDlg::FillTree() to thePrefs::GetMaxClientVersions().
					 *   The list itself is not changed, only visibility is ignored on items
					 *   outside the top n visible.
					 *
					 *   On an EC request, visibility range can be set with the EC_TAG_STATTREE_CAPPING tag.
					 */
};

enum EValueType
{
	vtUnknown,
	vtString,
	vtInteger,
	vtFloat
};

/**
 * Display modes for Simple and Counter items
 */
enum EDisplayMode
{
	dmDefault,		///< Default display mode.
	dmTime,			///< Treat integer value as time in seconds.
	dmBytes			///< Treat integer value as bytes count.
};

#endif /* !EC_REMOTE */


class CStatTreeItemBase;
typedef std::list<CStatTreeItemBase*>::iterator	StatTreeItemIterator;


class CECTag;


/**
 * Base tree item class
 */
class CStatTreeItemBase
{
public:

#ifndef EC_REMOTE
	/**
	 * Creates an item with a constant label.
	 *
	 * @param label	Visible text for the item.
	 * @param flags Flags to use.
	 */
	CStatTreeItemBase(const wxString &label, unsigned flags = stNone)
		: m_label(label), m_parent(NULL), m_flags(flags), m_id(0)
		{}
#else
	/**
	 * Creates an item with a constant label.
	 *
	 * @param label	Visible text for the item.
	 */
	CStatTreeItemBase(const wxString &label)
		: m_label(label)
		{}

	/**
	 * Creates an item (and the whole subtree) from an EC tag.
	 *
	 * @param tag EC tag containing a stat subtree.
	 */
	CStatTreeItemBase(const CECTag *tag);
#endif

	/**
	 * Deletes all children.
	 */
	VIRTUAL	~CStatTreeItemBase();

#ifndef EC_REMOTE
	/**
	 * Adds a new child node.
	 *
	 * @param child child to add.
	 * @param id	an optional ID for the new item.
	 * @param skipOneLevel activates a trick to let the code work for aMule OSInfo and Version trees.
	 *
	 * @return the newly added item.
	 */
	CStatTreeItemBase *AddChild(
		CStatTreeItemBase* child,
		uint32_t id = 0,
		bool skipOneLevel = false);
#endif

	/**
	 * Check for children.
	 *
	 * @return true if this node has children, false otherwise.
	 */
	bool HasChildren() { wxMutexLocker lock(m_lock); return !m_children.empty(); }

	/**
	 * Check for visible children.
	 *
	 * @return true if this node has children and at least one of them is visible.
	 */
	bool HasVisibleChildren();

#ifndef EC_REMOTE

	/**
	 * Check for a given child.
	 *
	 * @return true if this node has a child with the given ID.
	 */
	bool HasChildWithId(uint32_t id);

	/**
	 * Access a specific child.
	 *
	 * @return the child with the given ID, or NULL if not found.
	 */
	CStatTreeItemBase *GetChildById(uint32_t id);

	/**
	 * Get the first visible child.
	 *
	 * @param max_children The maximum number of children to show, when the stCapChildren flag is set. Otherwise it has no effect. (0 = unlimited)
	 *
	 * @return An iterator, that should be passed to GetNextVisibleChild() and IsAtEndOfList().
	 */
	StatTreeItemIterator GetFirstVisibleChild(uint32_t max_children);

	/**
	 * Get the next visible child.
	 */
	void GetNextVisibleChild(StatTreeItemIterator& it);

#else /* EC_REMOTE */

	/**
	 * Get the first visible child.
	 *
	 * @return An iterator, that should be passed to GetNextVisibleChild() and IsAtEndOfList().
	 *
	 * @note On a remote list every item is visible.
	 */
	StatTreeItemIterator GetFirstVisibleChild() { return m_children.begin(); }

	/**
	 * Get the next visible child.
	 *
	 * @note On a remote list every item is visible.
	 */
	void GetNextVisibleChild(StatTreeItemIterator& it) { ++it; }

#endif /* !EC_REMOTE / EC_REMOTE */

	/**
	 * Check if we are past the end of child list.
	 */
	bool IsAtEndOfList(StatTreeItemIterator& it) { return it == m_children.end(); }

#ifndef EC_REMOTE
	/**
	 * Resorts children for the stSortByValue flag.
	 */
	void ReSortChildren() { wxMutexLocker lock(m_lock); m_children.sort(ValueSort); }
#endif

#ifndef AMULE_DAEMON
#ifndef EC_REMOTE
	/**
	 * Returns a string that will be displayed on the GUI tree.
	 */
	virtual	wxString GetDisplayString() const;
#else
	/**
	 * Returns the associated text (GUI item label).
	 */
	const wxString& GetDisplayString() const { return m_label; }
#endif /* !EC_REMOTE / EC_REMOTE */

	/**
	 * Returns the mutex used to lock the child list of this node.
	 *
	 * This function is used by CStatisticsDlg to be able to lock the
	 * core tree while updating the GUI tree.
	 */
	wxMutex& GetLock() { return m_lock; }
#endif /* !AMULE_DAEMON */

	/**
	 * Check whether this node is visible.
	 */
	VIRTUAL	bool IsVisible() const { return true; }

#ifndef EC_REMOTE
	/**
	 * Create an EC tag from this node (and children).
	 *
	 * @param max_children The maximum number of children to show, when the stCapChildren flag is set. Otherwise it has no effect. (0 = unlimited)
	 *
	 * @return A EC tag containing this node and all its children.
	 */
	virtual	CECTag *CreateECTag(uint32_t max_children);
#endif

protected:

#ifndef EC_REMOTE
	/**
	 * Add values to the EC tag being generated.
	 *
	 * Should have a real implementation in children which have some value.
	 * The given parameter is the tag to which values should be added.
	 */
	virtual	void AddECValues(CECTag*) const {}
#endif

	//! Unformatted and untranslated label of the node. Note: On remote gui it is already formatted and translated.
	const wxString m_label;
#ifndef EC_REMOTE

	//! Parent of this node.
	CStatTreeItemBase *m_parent;

	//! Flags for the node.
	unsigned m_flags;
#endif

private:

#ifndef EC_REMOTE
	//! Function used when sorting children by value.
	static bool ValueSort(CStatTreeItemBase* a, CStatTreeItemBase* b);

	//! ID of this node.
	uint32_t m_id;

	//! Counter to keep track of displayed visible items
	// (needed for the stCapChildren flag)
	uint32_t m_visible_counter;
#endif

	//! Children of this node.
	std::list<CStatTreeItemBase*>	m_children;

	//! Lock to protect list from simultanous access.
	wxMutex m_lock;
};


//
// Anything below is only for core.
//
#ifndef EC_REMOTE

/**
 * Simple tree item.
 *
 * This tree item has one value and nothing speciality. :)
 * The value might be an arbitrary integer or floating point type,
 * or a wxString string.
 *
 * The item is able to display value in different formats, see SetDisplayMode().
 *
 * @note that you have to specify the right format code on 'label', i.e.:
 * %s for string and integers with displayMode dmTime or dmBytes,
 * %u or similar for integers, and
 * %f or similar for floating point types.
 *
 * @note You have to call SetValue() after creation for non-integer values, otherwise
 * you'll get undesired results.
 */
class CStatTreeItemSimple : public CStatTreeItemBase
{
public:

	/**
	 * Constructor.
	 *
	 * @see CStatTreeItemBase::CStatTreeItemBase
	 */
	CStatTreeItemSimple(
		const wxString &label,
		unsigned flags = stNone,
		enum EDisplayMode displaymode = dmDefault)
	:
	CStatTreeItemBase(label, flags),
	m_valuetype(vtUnknown),
	m_displaymode(displaymode)
	{
		SetValue((uint64_t)0);
	}

	/**
	 * Sets the desired display mode of value.
	 */
	void SetDisplayMode(enum EDisplayMode mode)
	{
		m_displaymode = mode;
	}

	/**
	 * Sets an integer type value.
	 *
	 * @param value the value to be set.
	 */
	void SetValue(uint64_t value)
	{
		m_valuetype = vtInteger;
		m_intvalue = value;
	}

	/**
	 * Sets a floating point type value.
	 *
	 * @param value the value to be set.
	 */
	void SetValue(double value)
	{
		m_valuetype = vtFloat;
		m_floatvalue = value;
	}

	/**
	 * Sets a string type value.
	 *
	 * @param value the value to be set.
	 */
	void SetValue(const wxString& value)
	{
		m_valuetype = vtString;
		m_stringvalue = value;
	}

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeItemBase::GetDisplayString()
	 */
	virtual	wxString GetDisplayString() const;
#endif

	/**
	 * @see CStatTreeItemBase::IsVisible()
	 */
	virtual	bool IsVisible() const;

protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void AddECValues(CECTag *tag) const;

	//! Type of the value.
	enum EValueType m_valuetype;
	//! Display mode of the value.
	enum EDisplayMode m_displaymode;
	//! Union to save space.
	union
	{
		uint64_t m_intvalue;	///< Integer value.
		double   m_floatvalue;	///< Floating point value.
	};
	wxString m_stringvalue;	///< String value.
};


/**
 * Counter-type tree item.
 *
 * Able to show percentage compared to parent, hide itself
 * when value is zero, and nice functions for changing the value.
 * stShowPercent and stHideIfZero flags take effect only on
 * this node.
 */
class CStatTreeItemCounter : public CStatTreeItemBase
{
public:
	/**
	 * Constructor.
	 *
	 * @see CStatTreeItemBase::CStatTreeItemBase
	 */
	CStatTreeItemCounter(
		const wxString &label,
		unsigned flags = stNone)
	:
	CStatTreeItemBase(label, flags),
	m_value(0),
	m_displaymode(dmDefault) {}

	/**
	 * Retrieve counter value.
	 */
	uint64_t GetValue() const { return m_value; }

	/**
	 * Retrieve counter value.
	 */
	operator uint64_t() const { return m_value; }

	/**
	 * Set counter to given value.
	 */
	void SetValue(uint64_t value) { m_value = value; }

	/**
	 * Set counter to given value.
	 */
	void operator=(uint64_t value) { m_value = value; }

	/**
	 * Increase value by 1.
	 */
	void operator++() { ++m_value; }

	/**
	 * Decrease value by 1.
	 */
	void operator--() { --m_value; }

	/**
	 * Increase value by given amount.
	 */
	void operator+=(uint64_t value) { m_value += value; }

	/**
	 * Decrease value by given amount.
	 */
	void operator-=(uint64_t value) { m_value -= value; }

	/**
	 * Sets the desired display mode of value.
	 */
	void SetDisplayMode(enum EDisplayMode mode) { m_displaymode = mode; }

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeItemBase::GetDisplayString()
	 */
	virtual	wxString GetDisplayString() const;
#endif

	/**
	 * @see CStatTreeItemBase::IsVisible()
	 */
	virtual	bool IsVisible() const
	{
		return (m_flags & stHideIfZero) ? (m_value != 0) : true;
	}

protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void AddECValues(CECTag *tag) const;

	//! Actual value of the counter.
	uint64_t m_value;

	//! Display mode of the value.
	enum EDisplayMode m_displaymode;
};

// For speed reasons, we define a 32bit counter for 32bit archs.
#ifndef USE_64BIT_ARCH
/**
 * Counter-type tree item, with native-sized value.
 *
 * On 32bit arcs it's defined as a separate class, on
 * 64bit archs it is typedef'd to CStatTreeItemCounter.
 *
 * @see CStatTreeItemCounter
 */
class CStatTreeItemNativeCounter : public CStatTreeItemBase
{
public:

	/**
	 * Constructor.
	 *
	 * @see CStatTreeItemCounter::CStatTreeItemCounter
	 */
	CStatTreeItemNativeCounter(
		const wxString &label,
		unsigned flags = stNone)
	:
	CStatTreeItemBase(label, flags),
	m_value(0),
	m_displaymode(dmDefault) {}

	/**
	 * Retrieve counter value.
	 */
	uint32_t GetValue() const { return m_value; }

	/**
	 * Retrieve counter value.
	 */
	operator uint32_t() const { return m_value; }

	/**
	 * Set counter to given value.
	 */
	void SetValue(uint32_t value) { m_value = value; }

	/**
	 * Set counter to given value.
	 */
	void operator=(uint32_t value) { m_value = value; }

	/**
	 * Increase value by 1.
	 */
	void operator++() { ++m_value; }

	/**
	 * Decrease value by 1.
	 */
	void operator--() { --m_value; }

	/**
	 * Increase value by given amount.
	 */
	void operator+=(uint32_t value) { m_value += value; }

	/**
	 * Decrease value by given amount.
	 */
	void operator-=(uint32_t value) { m_value -= value; }

	/**
	 * Sets the desired display mode of value.
	 */
	void SetDisplayMode(enum EDisplayMode mode) { m_displaymode = mode; }

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeItemCounter::GetDisplayString()
	 */
	virtual	wxString GetDisplayString() const;
#endif

	/**
	 * @see CStatTreeItemCounter::IsVisible()
	 */
	virtual	bool IsVisible() const
	{
		return (m_flags & stHideIfZero) ? (m_value != 0) : true;
	}

protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void AddECValues(CECTag *tag) const;

	//! Actual value of the counter.
	uint32_t m_value;

	//! Display mode of the value.
	enum EDisplayMode m_displaymode;
};
#else // using 64bit arch
	typedef CStatTreeItemCounter CStatTreeItemNativeCounter;
#endif /* USE_64BIT_ARCH */

/**
 * A counter, which does not display its value :P
 */
class CStatTreeItemHiddenCounter : public CStatTreeItemCounter
{
public:
	/**
	 * Constructor.
	 *
	 * @see CStatTreeItemCounter::CStatTreeItemCounter
	 */
	CStatTreeItemHiddenCounter(
		const wxString &label,
		unsigned flags = stNone)
	:
	CStatTreeItemCounter(label, flags) {}

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeItemBase::GetDisplayString()
	 */
	virtual	wxString GetDisplayString() const
	{
		return CStatTreeItemBase::GetDisplayString();
	}
#endif

	/**
	 * @see CStatTreeItemBase::IsVisible()
	 */
	virtual bool IsVisible() const { return true; }

protected:
	//! Do nothing here.
	virtual	void AddECValues(CECTag*) const {}
};


/**
 * Item for the session/total upload/download counter
 */
class CStatTreeItemUlDlCounter : public CStatTreeItemCounter
{
public:
	/**
	 * @param label     format text for item.
	 * @param totalfunc function that will return the totals.
	 */
	CStatTreeItemUlDlCounter(
		const wxString &label,
		uint64_t (*totalfunc)(),
		unsigned flags = stNone)
	:
	CStatTreeItemCounter(label, flags),
	m_totalfunc(totalfunc) {}

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeBase::GetDisplayString()
	 */
	virtual	wxString GetDisplayString() const;
#endif

protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void AddECValues(CECTag *tag) const;

	//! A function whose return value is the total (without current) value.
	uint64_t (*m_totalfunc)();
};


/**
 * Counter-like tree item which remembers its max value.
 *
 * Used for active connections counter, to be able to get peak connections.
 */
class CStatTreeItemCounterMax : public CStatTreeItemBase
{
public:
	/**
	 * @see CStatTreeItemBase::CStatTreeItemBase
	 */
	CStatTreeItemCounterMax(const wxString &label)
	:
	CStatTreeItemBase(label),
	m_value(0),
	m_max_value(0)
	{}

	/**
	 * Increase value
	 */
	void operator++()
	{
		if (++m_value > m_max_value) {
			m_max_value = m_value;
		}
	}

	/**
	 * Decrease value
	 */
	void operator--() { --m_value; }

	/**
	 * Retrieve actual value
	 */
	uint32_t GetValue() { return m_value; }

	/**
	 * Retrieve max value
	 */
	uint32_t GetMaxValue() { return m_max_value; }

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeItemBase::GetDisplayString()
	 */
	virtual	wxString GetDisplayString() const;
#endif

protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void AddECValues(CECTag *tag) const;

	//! Actual value of the counter.
	uint32_t m_value;

	//! Maximal value the counter has ever reached.
	uint32_t m_max_value;
};


/**
 * Tree item for counting packets
 */
class CStatTreeItemPackets : public CStatTreeItemBase
{
	friend class CStatTreeItemPacketTotals;

public:
	/**
	 * @see CStatTreeItemBase::CStatTreeItemBase
	 */
	CStatTreeItemPackets(const wxString &label)
	:
	CStatTreeItemBase(label, stNone),
	m_packets(0),
	m_bytes(0) {}

	/**
	 * Add a packet of size 'size'.
	 */
	void operator+=(long size)
	{
		++m_packets;
		m_bytes += size;
	}

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeItemBase::GetDisplayString()
	 */
	virtual	wxString GetDisplayString() const;
#endif

protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void AddECValues(CECTag *tag) const;

	//! Total number of packets.
	uint32_t m_packets;

	//! Total bytes in the packets.
	uint64_t m_bytes;
};


/**
 * Tree item for counting totals on packet counters.
 *
 * This item sums up a number of packet counters, plus adds its own values.
 */
class CStatTreeItemPacketTotals : public CStatTreeItemPackets
{
public:
	/**
	 * @see CStatTreeItemPackets::CStatTreeItemPackets
	 */
	CStatTreeItemPacketTotals(const wxString &label)
	:
	CStatTreeItemPackets(label) {}

	/**
	 * Adds a packet counter, whose values should be counted in the totals.
	 */
	void AddPacketCounter(CStatTreeItemPackets* counter)
	{
		m_counters.push_back(counter);
	}

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeItemPackets::GetDisplayString()
	 */
	virtual	wxString GetDisplayString() const;
#endif

protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void AddECValues(CECTag *tag) const;

	//! List of packet counters to sum.
	std::vector<CStatTreeItemPackets*> m_counters;
};


/**
 * Tree item for timer type nodes.
 */
class CStatTreeItemTimer : public CStatTreeItemBase
{
public:

	/**
	 * @see CStatTreeItemBase::CStatTreeItemBase
	 */
	CStatTreeItemTimer(
		const wxString &label,
		unsigned flags = stNone)
	:
	CStatTreeItemBase(label, flags),
	m_value(0) {}

	/**
	 * Sets timer start time (and thus starts timer).
	 */
	void SetStartTime(uint64_t value) { m_value = value; }

	/**
	 * Starts the timer if it's not running.
	 */
	void StartTimer() { if (!m_value) m_value = GetTickCount64(); }

	/**
	 * Stops the timer.
	 */
	void StopTimer() { m_value = 0; }

	/**
	 * Check whether the timer is running.
	 */
	bool IsRunning() const { return m_value != 0; }

	/**
	 * Reset timer unconditionally.
	 */
	void ResetTimer() { m_value = GetTickCount64(); }

	/**
	 * Get timer value.
	 */
	uint64_t GetTimerValue() const
	{
		return m_value ? GetTickCount64() - m_value : 0;
	}

	/**
	 * Get timer value (in ticks).
	 */
	operator uint64_t() const
	{
		return m_value ? GetTickCount64() - m_value : 0;
	}

	/**
	 * Get elapsed time in seconds.
	 */
	uint64_t GetTimerSeconds() const
	{
		return m_value ? (GetTickCount64() - m_value) / 1000 : 0;
	}

	/**
	 * Get start time of the timer.
	 */
	uint64_t GetTimerStart() const { return m_value; }

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeItemBase::GetDisplayString()
	 */
	virtual	wxString GetDisplayString() const;
#endif

	/**
	 * @see CStatTreeItemBase::IsVisible()
	 */
	virtual	bool IsVisible() const
	{
		return (m_flags & stHideIfZero) ? m_value != 0 : true;
	}

protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void AddECValues(CECTag *tag) const;

	//! Tick count value when timer was started.
	uint64_t m_value;
};


/**
 * Tree item for shared files average size.
 *
 * Average is counted as dividend / divisor, if divisor is non-zero.
 */
class CStatTreeItemAverage : public CStatTreeItemBase
{
public:
	/**
	 * @see CStatTreeItemBase::CStatTreeItemBase
	 *
	 * @param dividend What to divide.
	 * @param divisor Divide by what.
	 */
	CStatTreeItemAverage(
		const wxString &label,
		const CStatTreeItemCounter *dividend,
		const CStatTreeItemCounter *divisor,
		enum EDisplayMode displaymode)
	:
	CStatTreeItemBase(label, stNone),
	m_dividend(dividend),
	m_divisor(divisor),
	m_displaymode(displaymode) {}

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeItemBase::GetDisplayString()
	 */
	virtual	wxString GetDisplayString() const;
#endif

	/**
	 * @see CStatTreeItemBase::IsVisible()
	 */
	virtual	bool IsVisible() const	{ return (*m_divisor) != 0; }

protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void AddECValues(CECTag *tag) const;

	//! What to divide.
	const CStatTreeItemCounter *m_dividend;

	//! Divide by what.
	const CStatTreeItemCounter *m_divisor;

	//! Display mode.
	enum EDisplayMode m_displaymode;
};


/**
 * Tree item for average up/down speed.
 */
class CStatTreeItemAverageSpeed : public CStatTreeItemBase
{
public:

	/**
	 * @see CStatTreeItemBase::CStatTreeItemBase
	 *
	 * @param counter Session up/down counter.
	 * @param timer Session uptime timer.
	 */
	CStatTreeItemAverageSpeed(
		const wxString &label,
		const CStatTreeItemUlDlCounter *counter,
		const CStatTreeItemTimer *timer)
	:
	CStatTreeItemBase(label, stNone),
	m_counter(counter),
	m_timer(timer) {}

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeItemBase::GetDisplayString()
	 */
	virtual	wxString GetDisplayString() const;
#endif

protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void AddECValues(CECTag *tag) const;

	//! Session sent/received bytes counter.
	const CStatTreeItemUlDlCounter *m_counter;

	//! Session uptime.
	const CStatTreeItemTimer *m_timer;
};


/**
 * Tree item for displaying ratio between two counters.
 *
 * Ratio is counted as counter1:counter2.
 */
class CStatTreeItemRatio : public CStatTreeItemBase
{
public:
	/**
	 * @see CStatTreeItemBase::CStatTreeItemBase
	 *
	 * @param cnt1 First counter to use.
	 * @param cnt2 Second counter to use.
	 */
	CStatTreeItemRatio(
		const wxString &label,
		const CStatTreeItemCounter *cnt1,
		const CStatTreeItemCounter* cnt2)
	:
	CStatTreeItemBase(label, stNone),
	m_counter1(cnt1),
	m_counter2(cnt2) {}

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeItemBase::GetDisplayString()
	 */
	virtual	wxString GetDisplayString() const;
#endif	

protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void AddECValues(CECTag *tag) const;

	//! First counter.
	const CStatTreeItemCounter *m_counter1;

	//! Second counter.
	const CStatTreeItemCounter *m_counter2;
};


/**
 * Special counter for reconnects.
 */
class CStatTreeItemReconnects : public CStatTreeItemNativeCounter {
public:
	/**
	 * @see CStatTreeItemBase::CStatTreeItemBase
	 */
	CStatTreeItemReconnects(const wxString &label)
	:
	CStatTreeItemNativeCounter(label, stNone) {}

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeItemBase::GetDisplayString()
	 */
	virtual	wxString GetDisplayString() const;
#endif

protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void AddECValues(CECTag *tag) const;
};

/**
 * Special item for Max Connection Limit Reached
 */
class CStatTreeItemMaxConnLimitReached : public CStatTreeItemBase
{
public:
	/**
	 * @see CStatTreeItemBase::CStatTreeItemBase
	 */
	CStatTreeItemMaxConnLimitReached(const wxString &label)
	:
	CStatTreeItemBase(label),
	m_count(0) {}

	/**
	 * Increase counter and save time.
	 */
	void operator++()
	{
		++m_count;
		m_time.SetToCurrent();
	}

#ifndef AMULE_DAEMON
	/**
	 * Returns a string to be displayed on GUI.
	 *
	 * For m_count == 0 it will display "Never",
	 * for other values it will display the counter value and the
	 * date & time of the event.
	 */
	virtual	wxString GetDisplayString() const;
#endif

protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void AddECValues(CECTag *tag) const;

	//! Number of times max conn limit reached.
	uint32_t m_count;

	//! Last time when max conn limit reached.
	wxDateTime m_time;
};

/**
 * Special item for total client count
 */
class CStatTreeItemTotalClients : public CStatTreeItemBase
{
public:
	/**
	 * @see CStatTreeItemBase::CStatTreeItemBase
	 *
	 * @param known Counter that counts known clients.
	 * @param unknown Counter that counts unknown clients.
	 */
	CStatTreeItemTotalClients(
		const wxString &label,
		const CStatTreeItemCounter *known,
		const CStatTreeItemCounter *unknown)
	:
	CStatTreeItemBase(label),
	m_known(known),
	m_unknown(unknown) {}

#ifndef AMULE_DAEMON
	/**
	 * @see CStatTreeItemBase::GetDisplayString()
	 */
	virtual	wxString GetDisplayString() const;
#endif

protected:
	/**
	 * Add values to EC tag being generated.
	 *
	 * @param tag The tag to which values should be added.
	 *
	 * @see CStatTreeItemBase::AddECValues
	 */
	virtual	void AddECValues(CECTag *tag) const;

	//! Counter counting known clients.
	const CStatTreeItemCounter *m_known;

	//! Counter counting unknown clients.
	const CStatTreeItemCounter *m_unknown;
};

#endif /* !EC_REMOTE */

#endif /* STATTREE_H */
// File_checked_for_headers
