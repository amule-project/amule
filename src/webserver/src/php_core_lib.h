//
// This file is part of the aMule Project.

// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
#ifndef _PHP_CORE_LIB_H_
#define _PHP_CORE_LIB_H_

/*
 * This is interface to CPP parts of amuleweb
 */
#ifdef __cplusplus

#include <list>

class CWriteStrBuffer {
		std::list<char *> m_buf_list;
		int m_total_length;
		int m_alloc_size;
		char *m_curr_buf;
		char *m_buf_ptr;
		int m_curr_buf_left;
		
		void AllocBuf();
	public:
		CWriteStrBuffer();
		~CWriteStrBuffer();
		
		void Write(const char *s);
		void CopyAll(char *dst_buffer);
		const int Length() { return m_total_length; }
};

class CPhPLibContext {
		PHP_SYN_NODE *m_syn_tree_top;
		PHP_SCOPE_TABLE m_global_scope;
	public:
		// take a "snapshot" of global vars
		CPhPLibContext();
		~CPhPLibContext();
		
		// init global vars, so parser/execution cat start
		void SetContext();
}

#ifdef __cplusplus

void php_init_core_lib();

void php_native_var_dump(PHP_SCOPE_TABLE scope, PHP_VALUE_NODE *result);
void php_var_dump(PHP_VALUE_NODE *node, int ident);

#endif //_PHP_CORE_LIB_H_
