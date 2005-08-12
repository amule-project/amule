//
// This file is part of the aMule Project.
//
// Copyright (c) 2005 Mikkel Schubert ( xaignar@users.sourceforge.net )
// Copyright (c) 2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "MuleDebug.h"
#endif

#include <exception>			// Needed for std::exception
#include <cxxabi.h>				// Needed for __cxxabiv1::
#include <csignal>				// Needed for raise()

#include <wx/string.h>

#include "MuleDebug.h"			// Interface declaration
#include "StringFunctions.h"	// Needed for unicode2char
#include "OtherFunctions.h"		// Needed for GetFullMuleVersion()

#ifdef __LINUX__
	#include <execinfo.h>
	#include <cxxabi.h>
	#include <wx/thread.h>
	#if wxUSE_GUI
		#include <wx/utils.h>
	#endif
	#include <unistd.h> // Seems to be needed at least on Creteil's box
#endif


#if wxCHECK_VERSION(2,6,0) && wxUSE_STACKWALKER && defined(__WXMSW__)
	#include <wx/stackwalk.h>
#elif HAVE_BFD
	#include <ansidecl.h>
	#include <bfd.h>
#endif

#if wxCHECK_VERSION(2, 5, 0)
	#include <wx/arrstr.h>
#endif

/**
 * This functions displays a verbose description of 
 * any unhandled exceptions that occour and then
 * terminate the program by raising SIGABRT.
 */
void OnUnhandledException()
{
	std::type_info *t = __cxxabiv1::__cxa_current_exception_type();
	if (t) {
		// Note that "name" is the mangled name.
		char const *name = t->name();
		int status = -1;
		char *dem = 0;

		dem = __cxxabiv1::__cxa_demangle(name, 0, 0, &status);
		fprintf(stderr, "\naMule terminated after throwing an instance of '%s'\n", (status ? name : dem));
		fprintf(stderr, "\tVersion: %s\n", (const char*)unicode2char(GetFullMuleVersion()));
		free(dem);

		try {
			throw;
		} catch (const std::exception& e) {
			fprintf(stderr, "\twhat(): %s\n", e.what());
		} catch (const CMuleException& e) {
			fprintf(stderr, "\twhat(): %s\n", (const char*)unicode2char(e.what()));
		} catch (const wxString& e) {
			fprintf(stderr, "\twhat(): %s\n", (const char*)unicode2char(e));
		} catch (...) {
			// Unable to retrieve cause of exception
		}

		fprintf(stderr, "\tbacktrace:\n%s\n", (const char*)unicode2char(get_backtrace(1)));
	}

	raise(SIGABRT);
};


void InstallMuleExceptionHandler()
{
	std::set_terminate(OnUnhandledException);
}


// Make it 1 for getting the file path also
#define TOO_VERBOSE_BACKTRACE 0 

#if wxCHECK_VERSION(2,6,0) && wxUSE_STACKWALKER && defined(__WXMSW__)

// Derived class to define the actions to be done on frame print.
// I was tempted to name it MuleSkyWalker
class MuleStackWalker : public wxStackWalker
{
public:
	MuleStackWalker() {};
	~MuleStackWalker() {};

	void OnStackFrame(const wxStackFrame& frame)
	{
		wxString btLine = wxString::Format(wxT("[%u] "), frame.GetLevel());
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
			btLine += wxString::Format(wxT("0x%lx"), frame.GetAddress());
		}

		if (frame.HasSourceLocation()) {
			btLine += wxT(" at ") +
#if TOO_VERBOSE_BACKTRACE
			          frame.GetFileName()
#else
					  frame.GetFileName().AfterLast(wxT('/'))
#endif
			          + wxString::Format(wxT(":%u"),frame.GetLine());
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

#if HAVE_BFD

static bfd *abfd;
static asymbol **symbol_list;
static bool have_backtrace_symbols = false;
static const char *file_name;
static const char *function_name;
static unsigned int line_number;
static int found;


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
		fprintf (stderr, "Error while getting vector size for backtrace symbols : %s" , bfd_errmsg(bfd_get_error()));
		return -1;
	}

	if (vectorsize == 0) {
		fprintf (stderr, "Error while getting backtrace symbols : No symbols (%s)" , bfd_errmsg(bfd_get_error()));
		return -1;
	}

	*symbol_list_ptr = (asymbol**)malloc(vectorsize);

	if (*symbol_list_ptr == NULL) {
		fprintf (stderr, "Error while getting backtrace symbols : Cannot allocate memory");
		return -1;
	}

	vectorsize = bfd_canonicalize_symtab(abfd, *symbol_list_ptr);

	if (vectorsize < 0) {
		fprintf(stderr, "Error while getting symbol table : %s", bfd_errmsg(bfd_get_error()));
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
	abfd = bfd_openr("/proc/self/exe", NULL);

	if (abfd == NULL) {
		fprintf(stderr, "Error while opening file for backtrace symbols : %s", bfd_errmsg(bfd_get_error()));
		return;
	}

	if (!(bfd_check_format_matches(abfd, bfd_object, NULL))) {
		fprintf (stderr, "Error while init. backtrace symbols : %s" , bfd_errmsg (bfd_get_error ()));
		bfd_close(abfd);
		return;
	}

	have_backtrace_symbols = (get_backtrace_symbols(abfd, &symbol_list) > 0); 
}


void get_file_line_info(bfd *abfd, asection *section, void* _address)
{
	wxASSERT(symbol_list);

	if (found) {
		return;
	}

	if ((section->flags & SEC_ALLOC) == 0) {
		return;
	}

	bfd_vma vma = bfd_get_section_vma(abfd, section);

	unsigned long address = (unsigned long)_address;
	if (address < vma) {
		return;
	}

	bfd_size_type size = bfd_section_size(abfd, section);
	if (address > (vma + size)) {
		return;
	}

	found =  bfd_find_nearest_line(abfd, section, symbol_list,
									address - vma,
									&file_name, &function_name, &line_number);
}

#endif // HAVE_BFD

wxString demangle(const wxString& function)
{
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
}


// Print a stack backtrace if available
wxString get_backtrace(unsigned n)
{
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

	wxString libname[num_entries];
	wxString funcname[num_entries];
	wxString address[num_entries];
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

#if HAVE_BFD
	if (!have_backtrace_symbols) {
		init_backtrace_info();
		wxASSERT(have_backtrace_symbols);
	}

	for (int i = 0; i < num_entries; ++i) {
		file_name = NULL;
		function_name = NULL;
		line_number = 0;
		found = false ;

		unsigned long addr;
		address[i].ToULong(&addr,0); // As it's "0x" prepended, wx will read it as base 16. Hopefully.

		bfd_map_over_sections(abfd, get_file_line_info, (void*)addr);

		if (found) {
			wxString function = wxConvCurrent->cMB2WX(function_name);
			wxString demangled = demangle(function);
			if (!demangled.IsEmpty()) {
				function = demangled;
				funcname[i] = demangled;
			}
			out.Insert(wxConvCurrent->cMB2WX(function_name),i*2);
			out.Insert(wxConvCurrent->cMB2WX(file_name) + wxString::Format(wxT(":%u"),line_number),i*2+1);
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

#if wxUSE_GUI
		::wxEnableTopLevelWindows(false);
		hasLineNumberInfo = wxExecute(command, out) != -1;
		::wxEnableTopLevelWindows(true);
#endif	/* wxUSE_GUI */
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
}

#else	/* !__LINUX__ */

wxString get_backtrace(unsigned WXUNUSED(n))
{
	fprintf(stderr, "--== no BACKTRACE for your platform ==--\n\n");
	return wxEmptyString;
}

#endif /* !__LINUX__ */

void print_backtrace(unsigned n)
{
	wxString trace = get_backtrace(n);

	// This is because the string is ansi anyway, and the conv classes are very slow
#if wxUSE_UNICODE
	fwprintf(stderr, L"%ls\n", (const wchar_t *)(trace.c_str()) );
#else
	fprintf(stderr, "%s\n", (const char *)(trace.c_str()));
#endif
}
