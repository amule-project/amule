//
// This file is part of the aMule Project.
//
// Copyright (c) 2005-2011 Mikkel Schubert ( xaignar@users.sourceforge.net )
// Copyright (c) 2005-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <cstdlib>			// Needed for std::abort()

#ifdef HAVE_CONFIG_H
#	include "config.h"		// Needed for HAVE_CXXABI and HAVE_EXECINFO
#endif

#include "MuleDebug.h"			// Interface declaration
#include "StringFunctions.h"		// Needed for unicode2char
#include "Format.h"			// Needed for CFormat

#ifdef HAVE_EXECINFO
#	include <execinfo.h>
#	include <wx/utils.h>			// Needed for wxArrayString
#	ifndef HAVE_BFD
#		include <wx/thread.h>		// Needed for wxThread
#	endif
#endif

#ifdef HAVE_CXXABI
#	ifdef HAVE_TYPEINFO
#		include <typeinfo>	// Needed for some MacOSX versions with broken system headers
#	endif
#	include <cxxabi.h>
#endif


#if wxUSE_STACKWALKER && defined(__WINDOWS__)
	#include <wx/stackwalk.h> // Do_not_auto_remove
#elif defined(HAVE_BFD)
	#include <ansidecl.h> // Do_not_auto_remove
	#include <bfd.h> // Do_not_auto_remove
#endif

#include <vector>


/**
 * This functions displays a verbose description of
 * any unhandled exceptions that occour and then
 * terminate the program by raising SIGABRT.
 */
void OnUnhandledException()
{
	// Revert to the original exception handler, to avoid
	// infinate recursion, in case something goes wrong in
	// this function.
	std::set_terminate(std::abort);

#ifdef HAVE_CXXABI
	std::type_info *t = __cxxabiv1::__cxa_current_exception_type();
	FILE* output = stderr;
#else
	FILE* output = stdout;
	bool t = true;
#endif
	if (t) {
		int status = -1;
		char *dem = 0;
#ifdef HAVE_CXXABI
		// Note that "name" is the mangled name.
		char const *name = t->name();

		dem = __cxxabiv1::__cxa_demangle(name, 0, 0, &status);
#else
		const char* name = "Unknown";
#endif
		fprintf(output, "\nTerminated after throwing an instance of '%s'\n", (status ? name : dem));
		free(dem);

		try {
			throw;
		} catch (const std::exception& e) {
			fprintf(output, "\twhat(): %s\n", e.what());
		} catch (const CMuleException& e) {
			fprintf(output, "\twhat(): %s\n", (const char*)unicode2char(e.what()));
		} catch (const wxString& e) {
			fprintf(output, "\twhat(): %s\n", (const char*)unicode2char(e));
		} catch (...) {
			// Unable to retrieve cause of exception
		}

		fprintf(output, "\tbacktrace:\n%s\n", (const char*)unicode2char(get_backtrace(1)));
	}
	std::abort();
}


void InstallMuleExceptionHandler()
{
	std::set_terminate(OnUnhandledException);
}


// Make it 1 for getting the file path also
#define TOO_VERBOSE_BACKTRACE 0

#if wxUSE_STACKWALKER && defined(__WINDOWS__)

// Derived class to define the actions to be done on frame print.
// I was tempted to name it MuleSkyWalker
class MuleStackWalker : public wxStackWalker
{
public:
	MuleStackWalker() {};
	~MuleStackWalker() {};

	void OnStackFrame(const wxStackFrame& frame)
	{
		wxString btLine = CFormat(wxT("[%u] ")) % frame.GetLevel();
		wxString filename = frame.GetName();

		if (!filename.IsEmpty()) {
			btLine += filename + wxT(" (") +
#if TOO_VERBOSE_BACKTRACE
			        frame.GetModule()
#else
				frame.GetModule().AfterLast(wxT('/'))
#endif
			        + wxT(")");
		} else {
			btLine += CFormat(wxT("%p")) % frame.GetAddress();
		}

		if (frame.HasSourceLocation()) {
			btLine += wxT(" at ") +
#if TOO_VERBOSE_BACKTRACE
			        frame.GetFileName()
#else
				frame.GetFileName().AfterLast(wxT('/'))
#endif
			        + CFormat(wxT(":%u")) % frame.GetLine();
		} else {
			btLine += wxT(" (Unknown file/line)");
		}

		//! Contains the entire backtrace
		m_trace += btLine + wxT("\n");
	}

	wxString m_trace;
};


wxString get_backtrace(unsigned n)
{
	MuleStackWalker walker; // Texas ranger?
	walker.Walk(n); // Skip this one and Walk() also!

	return walker.m_trace;
}

#elif defined(__LINUX__)

#ifdef HAVE_BFD

static bfd* s_abfd;
static asymbol** s_symbol_list;
static bool s_have_backtrace_symbols = false;
static const char* s_file_name;
static const char* s_function_name;
static unsigned int s_line_number;
static int s_found;


/*
* read all symbols in the executable into an array
* and return the pointer to the array in symbol_list.
* Also return the number of actual symbols read
* If there's any error, return -1
*/
static int get_backtrace_symbols(bfd *abfd, asymbol ***symbol_list_ptr)
{
	int vectorsize = bfd_get_symtab_upper_bound(abfd);

	if (vectorsize < 0) {
		fprintf (stderr, "Error while getting vector size for backtrace symbols : %s",
			bfd_errmsg(bfd_get_error()));
		return -1;
	}

	if (vectorsize == 0) {
		fprintf (stderr, "Error while getting backtrace symbols : No symbols (%s)",
			bfd_errmsg(bfd_get_error()));
		return -1;
	}

	*symbol_list_ptr = (asymbol**)malloc(vectorsize);

	if (*symbol_list_ptr == NULL) {
		fprintf (stderr, "Error while getting backtrace symbols : Cannot allocate memory");
		return -1;
	}

	vectorsize = bfd_canonicalize_symtab(abfd, *symbol_list_ptr);

	if (vectorsize < 0) {
		fprintf(stderr, "Error while getting symbol table : %s",
			bfd_errmsg(bfd_get_error()));
		return -1;
	}

	return vectorsize;
}


/*
* print file, line and function information for address
* The info is actually set into global variables. This
* function is called from the iterator bfd_map_over_sections
*
*/
void init_backtrace_info()
{
	bfd_init();
	s_abfd = bfd_openr("/proc/self/exe", NULL);

	if (s_abfd == NULL) {
		fprintf(stderr, "Error while opening file for backtrace symbols : %s",
			bfd_errmsg(bfd_get_error()));
		return;
	}

	if (!(bfd_check_format_matches(s_abfd, bfd_object, NULL))) {
		fprintf (stderr, "Error while init. backtrace symbols : %s",
			bfd_errmsg (bfd_get_error ()));
		bfd_close(s_abfd);
		return;
	}

	s_have_backtrace_symbols = (get_backtrace_symbols(s_abfd, &s_symbol_list) > 0);
}


void get_file_line_info(bfd *abfd, asection *section, void* _address)
{
	wxASSERT(s_symbol_list);

	if (s_found) {
		return;
	}

	if ((section->flags & SEC_ALLOC) == 0) {
		return;
	}

	bfd_vma vma = section->vma;

	unsigned long address = (unsigned long)_address;
	if (address < vma) {
		return;
	}

	bfd_size_type size = section->size;
	if (address > (vma + size)) {
		return;
	}

	s_found =  bfd_find_nearest_line(abfd, section, s_symbol_list,
		address - vma, &s_file_name, &s_function_name, &s_line_number);
}

#endif // HAVE_BFD

static wxString demangle(const wxString& function)
{
#ifdef HAVE_CXXABI
	wxString result;

	if (function.Mid(0,2) == wxT("_Z")) {
		int status;
		char *demangled = abi::__cxa_demangle(function.mb_str(), NULL, NULL, &status);

		if (!status) {
			result = wxConvCurrent->cMB2WX(demangled);
		}

		if (demangled) {
			free(demangled);
		}
	}

	return result;
#else
	return wxEmptyString;
#endif
}


// Print a stack backtrace if available
wxString get_backtrace(unsigned n)
{
#ifdef HAVE_EXECINFO
	// (stkn) create backtrace
	void *bt_array[100];	// 100 should be enough ?!?
	char **bt_strings;
	int num_entries;

	if ((num_entries = backtrace(bt_array, 100)) < 0) {
		fprintf(stderr, "* Could not generate backtrace\n");
		return wxEmptyString;
	}

	if ((bt_strings = backtrace_symbols(bt_array, num_entries)) == NULL) {
		fprintf(stderr, "* Could not get symbol names for backtrace\n");
		return wxEmptyString;
	}

	std::vector<wxString> libname(num_entries);
	std::vector<wxString> funcname(num_entries);
	std::vector<wxString> address(num_entries);
	wxString AllAddresses;

	for (int i = 0; i < num_entries; ++i) {
		wxString wxBtString = wxConvCurrent->cMB2WX(bt_strings[i]);
		int posLPar = wxBtString.Find(wxT('('));
		int posRPar = wxBtString.Find(wxT(')'));
		int posLBra = wxBtString.Find(wxT('['));
		int posRBra = wxBtString.Find(wxT(']'));
		bool hasFunction = true;
		if (posLPar == -1 || posRPar == -1) {
			if (posLBra == -1 || posRBra == -1) {
				/* It is important to have exactly num_entries
				* addresses in AllAddresses */
				AllAddresses += wxT("0x0000000 ");
				continue;
			}
			posLPar = posLBra;
			hasFunction = false;
		}
		/* Library name */
		int len = posLPar;
		libname[i] = wxBtString.Mid(0, len);
		/* Function name */
		if (hasFunction) {
			int posPlus = wxBtString.Find(wxT('+'), true);
			if (posPlus == -1)
				posPlus = posRPar;
			len = posPlus - posLPar - 1;
			funcname[i] = wxBtString.Mid(posLPar + 1, len);
			wxString demangled = demangle(funcname[i]);
			if (!demangled.IsEmpty()) {
				funcname[i] = demangled;
			}
		}
		/* Address */
		if ( posLBra == -1 || posRBra == -1) {
			address[i] = wxT("0x0000000");
		} else {
			len = posRBra - posLBra - 1;
			address[i] = wxBtString.Mid(posLBra + 1, len);
			AllAddresses += address[i] + wxT(" ");
		}
	}
	free(bt_strings);

	/* Get line numbers from addresses */
	wxArrayString out;
	bool hasLineNumberInfo = false;

#ifdef HAVE_BFD
	if (!s_have_backtrace_symbols) {
		init_backtrace_info();
		wxASSERT(s_have_backtrace_symbols);
	}

	for (int i = 0; i < num_entries; ++i) {
		s_file_name = NULL;
		s_function_name = NULL;
		s_line_number = 0;
		s_found = false ;

		unsigned long addr;
		address[i].ToULong(&addr,0); // As it's "0x" prepended, wx will read it as base 16. Hopefully.

		bfd_map_over_sections(s_abfd, get_file_line_info, (void*)addr);

		if (s_found) {
			wxString function = wxConvCurrent->cMB2WX(s_function_name);
			wxString demangled = demangle(function);
			if (!demangled.IsEmpty()) {
				function = demangled;
				funcname[i] = demangled;
			}
			out.Insert(wxConvCurrent->cMB2WX(s_function_name),i*2);
			out.Insert(CFormat(wxT("%s:%u")) % wxString(wxConvCurrent->cMB2WX(s_file_name)) % s_line_number, i*2+1);
		} else {
			out.Insert(wxT("??"),i*2);
			out.Insert(wxT("??"),i*2+1);
		}
	}

	hasLineNumberInfo = true;

#else	/* !HAVE_BFD */
	if (wxThread::IsMain()) {
		wxString command;
		command << wxT("addr2line -C -f -s -e /proc/") <<
		getpid() << wxT("/exe ") << AllAddresses;
		// The output of the command is this wxArrayString, in which
		// the even elements are the function names, and the odd elements
		// are the line numbers.

		hasLineNumberInfo = wxExecute(command, out) != -1;
	}

#endif	/* HAVE_BFD / !HAVE_BFD */

	wxString trace;
	// Remove 'n+1' first entries (+1 because of this function)
	for (int i = n+1; i < num_entries; ++i) {
		/* If we have no function name, use the result from addr2line */
		if (funcname[i].IsEmpty()) {
			if (hasLineNumberInfo) {
				funcname[i] = out[2*i];
			} else {
				funcname[i] = wxT("??");
			}
		}
		wxString btLine;
		btLine << wxT("[") << i << wxT("] ") << funcname[i] << wxT(" in ");
		/* If addr2line did not find a line number, use bt_string */
		if (!hasLineNumberInfo || out[2*i+1].Mid(0,2) == wxT("??")) {
			btLine += libname[i] + wxT("[") + address[i] + wxT("]");
		} else if (hasLineNumberInfo) {
#if TOO_VERBOSE_BACKTRACE
			btLine += out[2*i+1];
#else
			btLine += out[2*i+1].AfterLast(wxT('/'));
#endif
		} else {
			btLine += libname[i];
		}

		trace += btLine + wxT("\n");
	}

	return trace;
#else /* !HAVE_EXECINFO */
	fprintf(stderr, "--== cannot generate backtrace ==--\n\n");
	return wxEmptyString;
#endif /* HAVE_EXECINFO */
}

#elif defined( __APPLE__ )

// According to sources, parts of this code originate at http://www.tlug.org.za/wiki/index.php/Obtaining_a_stack_trace_in_C_upon_SIGSEGV
// which doesn't exist anymore.

// Other code (stack frame list and demangle related) has been modified from code with license:

// Copyright 2007 Edd Dawson.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <dlfcn.h>
#include <cxxabi.h>
#include <sstream>

class stack_frame
{
public:
	stack_frame(const void * f_instruction, const std::string& f_function) : frame_instruction(f_instruction), frame_function(f_function) {};

	const void *instruction() const { return frame_instruction; }
	const std::string& function() const { return frame_function; }

private:
	const void * frame_instruction;
	const std::string frame_function;
};

std::string demangle(const char *name)
{
	int status = 0;
	char *d = 0;
	std::string ret = name;
	try {
		if ((d = abi::__cxa_demangle(name, 0, 0, &status))) {
			ret = d;
		}
	} catch(...) {  }

	std::free(d);
	return ret;
}


void fill_frames(std::list<stack_frame> &frames)
{
	try {
		void **fp = (void **) __builtin_frame_address (1);
		void *saved_pc = NULL;

		// First frame is skipped
		while (fp != NULL) {
			fp = (void**)(*fp);
			if (*fp == NULL) {
				break;
			}

#if defined(__i386__)
			saved_pc = fp[1];
#elif defined(__ppc__)
			saved_pc = *(fp + 2);
#else
			// ?
			saved_pc = *(fp + 2);
#endif
			if (saved_pc) {
				Dl_info info;

				if (dladdr(saved_pc, &info)) {
					frames.push_back(stack_frame(saved_pc, demangle(info.dli_sname) + " in " + info.dli_fname));
				}
			}
		}
	} catch (...) {
		// Nothing to be done here, just leave.
	}
}

wxString get_backtrace(unsigned n)
{
	std::list<stack_frame> frames;
	fill_frames(frames);
	std::ostringstream backtrace;
	std::list<stack_frame>::iterator it = frames.begin();

	int count = 0;
	while (it != frames.end()) {
		if (count >= n) {
			backtrace << (*it).instruction() << " : " << (*it).function() << std::endl;
			++it;
		}

		++count;
	}

	return wxString(backtrace.str().c_str(), wxConvUTF8);
}

#else /* ! __APPLE__ */

wxString get_backtrace(unsigned WXUNUSED(n))
{
	return wxT("--== no BACKTRACE for your platform ==--\n\n");
}

#endif /* !__LINUX__ */

void print_backtrace(unsigned n)
{
	wxString trace = get_backtrace(n);

	// This is because the string is ansi anyway, and the conv classes are very slow
	fprintf(stderr, "%s\n", (const char*)unicode2char(trace));
}

// File_checked_for_headers
