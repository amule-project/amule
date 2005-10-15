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

#ifndef NODELISTCTRL_H
#define SERVERLISTCTRL_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "NodeListCtrl.h"
#endif

#include "MuleListCtrl.h"	// Needed for CMuleListCtrl

#define	COLUMN_NODE_ID	0
#define	COLUMN_NODE_TYPE	1
#define	COLUMN_NODE_DISTANCE	2

namespace Kademlia {
	class CContact;
}

class wxListEvent;
class wxCommandEvent;

/** 
 * The CNodeListCtrl is used to display the list of nodes which the user
 * can connect to. It is a permanently sorted list in that it always ensure
 *  that the items are sorted in the correct order.
 */
class CNodeListCtrl : public CMuleListCtrl 
{
public:
	/**
	 * Constructor.
	 *
	 * @see CNodeListCtrl::CMuleListCtrl
	 */
	CNodeListCtrl(
	            wxWindow *parent,
                wxWindowID winid = -1,
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize,
                long style = wxLC_ICON,
                const wxValidator& validator = wxDefaultValidator,
                const wxString &name = wxT("nodelistctrl") );
	
	/**
	 * Destructor.
	 */
	virtual ~CNodeListCtrl();


	/**
	 * Adds a node to the list.
	 *
	 * @param A pointer to the new node.
	 *
	 * Internally this function calls RefreshNode and ShowNodeCount, with 
	 * the result that it is legal to add nodes already in the list, though
	 * not recommended.
	 */
	bool	AddNode( const Kademlia::CContact* toadd );
	
	/**
	 * Removes a node from the list.
	 *
	 * @param server The node to be removed.
	 *
	 */
	void	RemoveNode( const Kademlia::CContact* node);
	
	/**
	 * Removes all nodes with the specified state. 
	 *
	 * @param state All items with this state will be removed, default being all.
	 */
	void	RemoveAllNodes( int state = wxLIST_STATE_DONTCARE);
	
	
	/**
	 * Updates the displayed information on a node.
	 *
	 * @param node The node to be updated.
	 *
	 * This function will not only update the displayed information, it will also 
	 * reposition the item should it be nescecarry to enforce the current sorting.
	 * Also note that this function does not require that the node actually is
	 * on the list already, since AddNode makes use of it, but this should 
	 * generally be avoided, since it will result in the node-count getting
	 * skewed until the next AddNode call.
	 */	
	void	RefreshNode( const Kademlia::CContact* node );
	
	/**
	 * This function updates the node-count in the server-wnd.
	 */
	void	ShowNodeCount();
	
private:

	/**
	 * Event-handler for handling item activation (connect).
	 */
	void	OnItemActivated( wxListEvent& event );
	
	/**
	 * Event-handler for displaying the popup-menu.
	 */
	void	OnItemRightClicked( wxListEvent& event );
	
	/**
	 * Event-handler for node bootstrap
	 */
	void	OnBootstrapNode( wxCommandEvent& event );
	
	/**
	 * Event-handler for node removal.
	 */
	void	OnRemoveNodes( wxCommandEvent& event );

	/**
	 * Event-handler for deleting nodes when the delete-key is pressed.
	 */
	void	OnKeyPressed( wxKeyEvent& event );
	
	/**
	 * Sorter function.
	 *
	 * @see wxListCtrl::SortItems
	 */
	static int wxCALLBACK SortProc(long item1, long item2, long sortData);

	DECLARE_EVENT_TABLE()
};

#endif
