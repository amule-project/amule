//
// This file is part of the aMule Project.

// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2005-2011 Froenchenko Leonid ( lfroen@gmail.com / http://www.amule.org )
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
#ifndef _PHP_CORE_LIB_H_
#define _PHP_CORE_LIB_H_

/*
 * This is interface to CPP parts of amuleweb
 */
#ifdef __cplusplus


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
		
		void Write(const char *s, int len = -1);
		void CopyAll(char *dst_buffer);
		int Length() { return m_total_length; }
};

class CWebServerBase;
class CSession;

class CPhPLibContext {
		PHP_SYN_NODE *m_syn_tree_top;
		PHP_SCOPE_TABLE m_global_scope;
		
		CWriteStrBuffer *m_curr_str_buffer;

		CWebServerBase *m_server;
	public:
		// parse file and take a "snapshot" of global vars
		CPhPLibContext(CWebServerBase *server, const char *file);
		CPhPLibContext(CWebServerBase *server, char *php_buf, int len);
		~CPhPLibContext();
		
		// init global vars, so parser/execution can start
		void SetContext();
		void Execute(CWriteStrBuffer *);
		
#ifdef __GNUC__
		static void Printf(const char *str, ...)  __attribute__ ((__format__ (__printf__, 1, 2)));
#else
		static void Printf(const char *str, ...);
#endif
		static void Print(const char *str);

		static CPhPLibContext *g_curr_context;

#ifndef PHP_STANDALONE_EN
		CWebServerBase *WebServer() { return m_server; }
#endif
};

class CPhpFilter {
	public:
		CPhpFilter(CWebServerBase *server, CSession *sess,
			const char *file, CWriteStrBuffer *buff);
};

#endif // __cplusplus

void php_init_core_lib();
void php_init_amule_lib();

void load_session_vars(const char *target, std::map<std::string, std::string> &varmap);
void save_session_vars(std::map<std::string, std::string> &varmap);

#endif //_PHP_CORE_LIB_H_
// File_checked_for_headers
