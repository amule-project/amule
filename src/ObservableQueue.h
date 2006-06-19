//
// This file is part of the aMule Project.
//
// Copyright (C) 2005-2006Mikkel Schubert ( xaignar@users.sourceforge.net )
// Copyright (C) 2005-2006aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef OBSERVABLEQUEUE_H
#define OBSERVABLEQUEUE_H

#include "Observable.h"



template <typename ValueType>
class CQueueObserver;


/**
 * This class defines the protocol used by the CObservableQueue class.
 *
 * This specific protocol can be used by a class to make changes to its
 * contents visible, assuming that said contents does not rely on being
 * in a specific order, as insertions commands do not specify positions.
 * 
 * The class CQueueObserver implements a queue that will always be kept
 * in sync with the class observed, so that it is possible to handle all
 * elements in a container object over a longer period of time, which 
 * would normally result in problems with changes accomulating in the list.
 */
template <typename ValueType>
class CQueueEvent
{
public:
	//! The type of list used by this protocol to store values.
	typedef std::vector<ValueType> ValueList;

	//! The events used by this observable type.
	//! Note: If more than one instance of a value has been added, removed or
	//! updated, then the resulting events should reflect this fact by
	//! including the same of instances as has been changed.
	enum Type {
		//! Signals a QueueObserver that it has been added to a queue.
		STARTING,
		//! Signals a QueueObserver that it has been removed from a queue.
		STOPPING,
		//! Signals that one or more values have been added.
		INSERTED,
		//! Signals that one or more values have been removed.
		REMOVED,
		//! Signals that the following values have been updated.
		CHANGED,
		//! Signals that all values have been removed.
		CLEARED,
		//! Contains the contents of the queue at subscription time.
		INITIAL
	};

	
	/**
	 * Constructor for misc events.
	 */
	CQueueEvent( Type event );
	
	
	/**
	 * Constructor for events regarding multiple values.
	 *
	 * Note: CQueueEvent does not take ownership of the specified list.
	 */
	CQueueEvent( Type event, const ValueList* list );


	/**
	 * Constructor for events regarding a single value
	 */
	CQueueEvent( Type event, const ValueType& value );


	/**
	 * Returns the event-type.
	 */
	Type GetEvent() const {
		return m_type;
	}

	/**
	 * Returns the number of available values passed with the event.
	 */
	size_t GetCount() const;

	/**
	 * Returns a copy of the ith value.
	 */
	ValueType GetValue( size_t i ) const;
	
	
private:
	//! Pointer to a list of values. May be NULL.
	const ValueList* m_list;
	//! Pointer to a single value. May be NULL.
	const ValueType* m_value;
	//! The actual event-type.	
	Type m_type;
};



/**
 * This class forms the superclass for observable queues or lists.
 *
 * The protocol defined above is used (CQueueEvent).
 * 
 * By subclassing this class, a container object can ensure that classes 
 * observing it are able to keep in sync with the changes made to the 
 * contents, with a few limits.
 *
 * For one thing, the value is assumed to be both key and value, so
 * multimaps cannot be used with this class, since it will only pass
 * the key. Nor can lists that rely on a specific order be used, since
 * neither insertion nor deletion operations specify a specific
 * object or position, instead just a "add/remove one of these" rule.
 *
 * The main purpose of this class is to allow another class to follow
 * a list or queue, regardles of changes in actual order of the items
 * and regardles of changes to the contents.
 */
template <typename ValueType>
class CObservableQueue : public CObservable< CQueueEvent<ValueType> >
{
public:
	/**
	 * Destructor.
	 *
	 * Sends STOPPING to all observers.
	 */
	virtual ~CObservableQueue();
	
protected:
	typedef CQueueEvent< ValueType > EventType;
	typedef CObserver< EventType > ObserverType;
	typedef typename EventType::ValueList ValueList;

	
	/**
	 * Sends a STARTING event to new observers.
	 */
	virtual void ObserverAdded( ObserverType* );

	/**
	 * Sends a STOPPING event to removed observers.
	 */
	virtual void ObserverRemoved( ObserverType* );
};



/**
 * This class is an automatically synchronized queue connected with an ObservableQueue.
 *
 * Note that this observer can only be assigned to ONE observable at a time!
 * 
 * This class implements a queue (out-order not specified) that allows an 
 * ObservableQueue object to be object to be used as a queue without making
 * by another object in a safe manner. Changes to the contents of the original
 * queue are propagated to this queue, such that it will never contain values
 * not found in the original queue and such that it will add new values added
 * to the original queue.
 *
 * This allows the contents to be accessed safely, when for instance it is
 * needed to iterate over the contents over a period of time, where one 
 * cannot be certain of the current state of the actual contents of the 
 * original lists.
 *
 * This class supersedes such broken solutions such as remembering the last
 * used position in the original list, since no changes made to the original
 * list will result in items being skipped or treated twice*.
 *
 * * With the exception of the same item being removed and then re-added, in
 *   which case the CQueueObserver class will consider it a new item.
 */
template <typename ValueType>
class CQueueObserver : public CObserver< CQueueEvent<ValueType> >
{
public:
	typedef CQueueEvent<ValueType> EventType;
	typedef typename CObserver< EventType >::ObservableType ObservableType;
	typedef typename EventType::ValueList ValueList;


	/**
	 * Constructor.
	 */
	CQueueObserver();
	
	
	/**
	 * Overloaded notification function.
	 */
	void ReceiveNotification( const ObservableType*, const EventType& e );

	
	/**
	 * Returns the next element from the queue.
	 *
	 * Note: Objects will not be returned in the same order as 
	 * they were found in the original observable. Also, note 
	 * that calling GetNext() on an empty queue should only be
	 * done if the default contructed value does not match a 
	 * valid object and can be used to check for End of Queue.
	 */
	ValueType GetNext();

	/**
	 * Returns the number of remaining elements in the queue.
	 */
	size_t GetRemaining() const;

	/**
	 * Returns true if the observer is currently assigned to an observable-queue.
	 */
	bool IsActive() const;

	/**
	 * Clears the queue and readds itself to the current object being observed.
	 */
	void Reset();
	
private:
	//! Lock used to ensure the threadsafety of the class
	mutable wxMutex m_mutex;
	
	typedef std::multiset<ValueType> Queue;
	typedef typename Queue::iterator QueueIterator;
	//! The remaining items.
	Queue m_queue;

	//! Used to check that we are only subscribed to one queue at a time
	const ObservableType* m_owner;
};




///////////////////////////////////////////////////




template <typename ValueType>
CQueueEvent<ValueType>::CQueueEvent( Type event )
	: m_list( NULL  ),
	  m_value( NULL ),
	  m_type( event )
{

}


template <typename ValueType>
CQueueEvent<ValueType>::CQueueEvent( Type event, const ValueList* list )
	: m_list( list  ),
	  m_value( NULL ),
	  m_type( event )
{
	wxASSERT( list );
}


template <typename ValueType>
CQueueEvent<ValueType>::CQueueEvent( Type event, const ValueType& value )
	: m_list( NULL  ),
	  m_value( &value ),
	  m_type( event )
{
}


template <typename ValueType>
size_t CQueueEvent<ValueType>::GetCount() const {
	if ( m_list ) {
		return m_list->size();
	} else if ( m_value ) {
		return 1;
	} else {
		return 0;
	}
}


template <typename ValueType>
ValueType CQueueEvent<ValueType>::GetValue( size_t i ) const {
	if ( m_list ) {
		return (*m_list).at( i );
	} else if ( m_value && i == 0 ) {
		return *m_value;
	} else {
		wxASSERT( false );
		return ValueType();
	}
}




template <typename ValueType>
CObservableQueue<ValueType>::~CObservableQueue()
{
	this->RemoveAllObservers();
}


template <typename ValueType>
void CObservableQueue<ValueType>::ObserverAdded( ObserverType* o )
{
	NotifyObservers( EventType( EventType::STARTING ), o );
}


template <typename ValueType>
void CObservableQueue<ValueType>::ObserverRemoved( ObserverType* o )
{
	NotifyObservers( EventType( EventType::STOPPING ), o );
}




template <typename ValueType>
CQueueObserver<ValueType>::CQueueObserver()
{
	m_owner = NULL;
}


template <typename ValueType>
void CQueueObserver<ValueType>::ReceiveNotification( const ObservableType* o, const EventType& e )
{
	wxMutexLocker lock( m_mutex );

	if ( e.GetEvent() == EventType::INSERTED || e.GetEvent() == EventType::INITIAL ) {
		for ( size_t i = 0; i < e.GetCount(); i++ ) {
			m_queue.insert( e.GetValue( i ) );
		}
	} else if ( e.GetEvent() == EventType::REMOVED ) {
		for ( size_t i = 0; i < e.GetCount(); i++ ) {
			QueueIterator it = m_queue.find( e.GetValue( i ) );

			if ( it != m_queue.end() ) {
				m_queue.erase( it );
			}
		}
	} else if ( e.GetEvent() == EventType::CLEARED ) {
		m_queue.clear();
	} else if ( e.GetEvent() == EventType::STOPPING ) {
		m_queue.clear();
		m_owner = NULL;
	} else if ( e.GetEvent() == EventType::STARTING ) {
		wxASSERT(m_owner == NULL);
		m_owner = o;
	} else {
		wxASSERT( false );
	}
}


template <typename ValueType>
ValueType CQueueObserver<ValueType>::GetNext()
{
	wxMutexLocker lock( m_mutex );
	
	if ( m_queue.size() ) {
		ValueType v = *m_queue.begin();
		m_queue.erase( m_queue.begin() );

		return v;
	}

	return ValueType();
}


template <typename ValueType>
size_t CQueueObserver<ValueType>::GetRemaining() const
{
	wxMutexLocker lock( m_mutex );
	
	return m_queue.size();
}


template <typename ValueType>
bool CQueueObserver<ValueType>::IsActive() const
{
	return m_owner;
}


template <typename ValueType>
void CQueueObserver<ValueType>::Reset()
{
	ObservableType* owner;
	
	{
		wxMutexLocker lock(m_mutex);
		m_queue.clear();	
		owner = const_cast<ObservableType*>( m_owner );
	}

	owner->RemoveObserver( this );
	owner->AddObserver( this );	
}



#endif
// File_checked_for_headers
