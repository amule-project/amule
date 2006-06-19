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

#ifndef OBSERVABLE_H
#define OBSERVABLE_H


#include <set>

#include "OtherFunctions.h"		// Needed for CMutexUnlocker


template <typename TEST> class CObservable;


/**
 * This class implements the observable part of an Observer/Observable pattern.
 *
 * The EventType paramter is used to specify a protocol for the event-type used
 * by a particular Observer/Observable set and allows for any level of
 * information passing to be used, depending on the context.
 *
 * In order to simplify matters for subclasses, both the Observer and the
 * Observable class keep track of which objects are observing what, so that
 * instances can safely be created and destroyed without having to manually
 * keep the observers and observables in sync. 
 */
template <typename EventType>
class CObserver 
{
	friend class CObservable<EventType>;
public:
	typedef CObservable<EventType> ObservableType;

	/**
	 * Destructor.
	 *
	 * All observables which has this object registered are notified
	 * as to avoid dangling pointers. This will not result in actual
	 * events.
	 */
	virtual ~CObserver();	


protected:
	/**
	 * This function is called when an observed subject publishes an event.
	 *
	 * @param o The publisher of the event.
	 * @param e The actual event.
	 */
	virtual void ReceiveNotification( const ObservableType* o, const EventType& e ) = 0;
	
	
private:
	//! Mutex used to make access to the list of observed objects thread safe.
	wxMutex m_mutex;

	typedef std::set<ObservableType*> ObservableSetType;
	//! List of objects being observed.
	ObservableSetType m_list;
};



/**
 * This class implements the Observable part of the Observer/Observable pattern.
 */
template <typename EventType>
class CObservable
{
	friend class CObserver<EventType>;

public:
	//! The observer-type accepted by this class
	typedef CObserver<EventType> ObserverType;


	/**
	 * Destructor.
	 */
	virtual ~CObservable();


	/**
	 * This function subscribes an observer to events from this observable.
	 *
	 * @param o The observer that wishes to observe this object.
	 * @return True if it succesfully subscribed, false otherwise.
	 *
	 * If the subscription was succesful, ObserverAdded() will be called
	 * on "o", allowing the subclass to initialize the the observer's state.
	 */
	bool AddObserver( ObserverType* o );

	/**
	 * This function removes an observer from the list of subscribers.
	 *
	 * @param o The observer to unsubscribe from this observable.
	 * @return True if the observer was removed, false otherwise.
	 * 
	 * ObserverRemoved() will be called for the observer "o", allowing
	 * the subclass to take steps to avoid outdated data being kept. 
	 */
	bool RemoveObserver( ObserverType* o );

protected:
	/**
	 * This function notifies all or an specific observer of an event.
	 * 
	 * @param e The event to be published.
	 * @param o A specific subscribing observer or NULL for all subscribers.
	 * 
	 * The purpose of the second parameter is to allow notifications of
	 * specific observers when the ObserverAdded() or ObserverRemoved()
	 * functions are called and it should not be used outside of these
	 * functions.
	 */
	void NotifyObservers( const EventType& e, ObserverType* o = NULL );


	/**
	 * This function removes all observers from this object.
	 *
	 * ObserverRemoved is called on each observer.
	 */
	void RemoveAllObservers();
	
	
	/**
	 * This function is called when an observer has been added to the observable.
	 */
	virtual void ObserverAdded( ObserverType* ) {};
	
	
	/**
	 * This function is called when observers are removed from the observable.
	 *
	 * Exceptions to this are:
	 *  - When the Observable is being destroyed.
	 *  - When the Observer is being destroyed.
	 */
	virtual void ObserverRemoved( ObserverType* ) {};

	
private:
	//! Mutex used to ensure thread-safety of the basic operations.
	wxMutex m_mutex;

	typedef std::set<ObserverType*> ObserverSetType;	
	typedef typename ObserverSetType::iterator myIteratorType;

	//! Set of all observers subscribing to this observable.
	ObserverSetType m_list;
};




///////////////////////////////////////////////////////////////////////////////


	

template <typename EventType>
CObserver<EventType>::~CObserver()
{
	wxMutexLocker lock( m_mutex );
	
	while ( !m_list.empty() ) {
		ObservableType* o = *m_list.begin();
		
		{
			wxMutexLocker oLock(o->m_mutex);
			o->m_list.erase( this );
		}
		
		m_list.erase( m_list.begin() );
	}
}


template <typename EventType>
CObservable<EventType>::~CObservable()
{
	wxMutexLocker lock( m_mutex );

	while ( !m_list.empty() ) {
		ObserverType* o = *m_list.begin();
		
		{
			wxMutexLocker oLock(o->m_mutex);
			o->m_list.erase( this );
		}

		m_list.erase( m_list.begin() );
	}
}


template <typename EventType>
bool CObservable<EventType>::AddObserver( CObserver<EventType>* o )
{
	wxASSERT( o );

	{
		wxMutexLocker lock(m_mutex);
		if ( !m_list.insert( o ).second ) {
			return false;
		}
	}

	{
		wxMutexLocker oLock(o->m_mutex);
		o->m_list.insert( this );
	}

	ObserverAdded( o );
	
	return true;
}


template <typename EventType>
bool CObservable<EventType>::RemoveObserver( CObserver<EventType>* o )
{
	wxASSERT( o );

	{
		wxMutexLocker lock(m_mutex);
		if ( !m_list.erase( o ) ) {
			return false;
		}
	}

	{
		wxMutexLocker oLock(o->m_mutex);
		o->m_list.erase( this );
	}

	ObserverRemoved( o );

	return true;
}


template <typename EventType>
void CObservable<EventType>::NotifyObservers( const EventType& e, ObserverType* o )
{
	wxMutexLocker lock(m_mutex);

	if ( o ) {
		o->ReceiveNotification( this, e );
	} else {
		myIteratorType it = m_list.begin();
		for ( ; it != m_list.end(); ) {
			CMutexUnlocker unlocker(m_mutex);
			(*it++)->ReceiveNotification( this, e );
		}
	}
}


template <typename EventType>
void CObservable<EventType>::RemoveAllObservers()
{
	wxMutexLocker lock(m_mutex);
	
	while ( !m_list.empty() ) {
		ObserverType* o = *m_list.begin();
		m_list.erase( m_list.begin() );
		CMutexUnlocker unlocker(m_mutex);
		
		{
			wxMutexLocker oLock(o->m_mutex);
			o->m_list.erase( this );
		}

		ObserverRemoved( o );
	}
}


#endif
// File_checked_for_headers
