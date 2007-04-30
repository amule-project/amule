//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2006 Madcat ( madcat@_@users.sf.net / sharedaemon.sf.net )
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

const int versionMajor		= 1;
const int versionMinor		= 3;
const int versionRevision	= 0;

#include <cstdlib>
#include <sstream>
#include <iostream>
#include <fstream>

#ifdef __APPLE__
	#include <CoreServices/CoreServices.h>
#elif defined(__WIN32__)
	#include <winerror.h>
	#include <shlobj.h>
	#include <shlwapi.h>
#endif

#include "FileLock.h"
#include "MuleCollection.h"

using std::string;


string GetLinksFilePath(const string& configDir)
{
	if (!configDir.empty()) {
#ifdef __WIN32__
		char buffer[MAX_PATH + 1];
		configDir.copy(buffer, MAX_PATH);
		if (PathAppend(buffer, "ED2KLinks")) {
			string strDir;
			strDir.assign(buffer);
			return strDir;
		}
#else
		string strDir = configDir;
		if (strDir.at(strDir.length() - 1) != '/') {
			strDir += '/';
		}
		return strDir + "ED2KLinks";
#endif
	}


#ifdef __APPLE__

	std::string strDir;
	
	FSRef fsRef;
	if (FSFindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &fsRef) == noErr) {
		CFURLRef urlRef = CFURLCreateFromFSRef(NULL, &fsRef);
		if (urlRef != NULL) {
			UInt8 buffer[PATH_MAX + 1];
			if (CFURLGetFileSystemRepresentation(urlRef, true, buffer, sizeof(buffer))) {
				strDir.assign((char*) buffer);
			}
			CFRelease(urlRef) ;
		}
	}

	return strDir + "/aMule/ED2KLinks";

#elif defined(__WIN32__)

	std::string strDir;
	LPITEMIDLIST pidl;
	char buffer[MAX_PATH + 1];

	HRESULT hr = SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);

	if (SUCCEEDED(hr)) {
		if (SHGetPathFromIDList(pidl, buffer)) {
			if (PathAppend(buffer, "aMule\\ED2KLinks")) {
				strDir.assign(buffer);
			}
		}
	}

	if (pidl) {
		LPMALLOC pMalloc;
		SHGetMalloc(&pMalloc);
		if (pMalloc) {
			pMalloc->Free(pidl);
			pMalloc->Release();
		}
	}

	return strDir;

#else

	return string( getenv("HOME") ) + "/.aMule/ED2KLinks";

#endif
}

/**
 * Converts a hexadecimal number to a char.
 *
 * @param hex The hex-number, must be at most 2 digits long.
 * @return The resulting char or \0 if conversion failed.
 */
char HexToDec( const string& hex )
{
	char result = 0;
	
	for ( size_t i = 0; i < hex.length(); ++i ) {
		char cur = toupper( hex.at(i) );
		result *= 16;
		
		if ( isdigit( cur ) ) {
			result += cur - '0';
		} else if ( cur >= 'A' && cur <= 'F' ) {
			result += cur - 'A' + 10;
		} else {
			return '\0';
		}
	}

	return result;
}


/**
 * This function converts all valid HTML escape-codes to their corresponding chars.
 *
 * @param str The string to unescape.
 * @return The unescaped version of the input string.
 */
string Unescape( const string& str )
{
	string result;
	result.reserve( str.length() );
	
	for ( size_t i = 0; i < str.length(); ++i ) {
		if ( str.at(i) == '%' && ( i + 2 < str.length() ) ) {
			char unesc = HexToDec( str.substr( i + 1, 2 ) );

			if ( unesc ) {
				i += 2;

				result += unesc;
			} else {
				// If conversion failed, then we just add the escape-code
				// and continue past it like nothing happened.
				result += str.at(i);
			}
		} else {
			result += str.at(i);
		}
	}

	return result;
}


/**
 * Returns the string with whitespace stripped from both ends.
 */
string strip( const string& str )
{
	int first = 0;
	int last  = str.length() - 1;

	// A simple but no very optimized way to narrow down the 
	// usable text within the string.
	while ( first <= last ) {
		if ( isspace( str.at(first) ) ) {
			first++;
		} else if ( isspace( str.at(last) ) ) {
			last--;
		} else {
			break;
		}
	};
	
	return str.substr( first, 1 + last - first );	
}


/**
 * Returns true if the string is a valid number.
 */
bool isNumber( const string& str )
{
	for ( size_t i = 0; i < str.length(); i++ ) {
		if ( !isdigit( str.at(i) ) ) {
			return false;
		}
	}

	return str.length();
}


/**
 * Returns true if the string is a valid Base16 representation of a MD4 Hash.
 */
bool isMD4Hash( const string& str )
{
	for ( size_t i = 0; i < str.length(); i++ ) {
		const char c = toupper( str.at(i) );
		
		if ( !isdigit( c ) && ( c < 'A' || c > 'F' ) ) {
			return false;
		}
	}

	return str.length() == 32;
}


/**
 * Returns a description of the current version of "ed2k".
 */
string getVersion()
{
  	std::ostringstream v;
	
	v << "aMule ED2k link parser v"
		<< versionMajor << "."
		<< versionMinor << "."
		<< versionRevision;

	return v.str();
}


/**
 * Helper-function for printing link-errors.
 */
void badLink( const string& type, const string& err, const string& uri )
{
	std::cout << "Invalid " << type << "-link, " + err << ":\n"
		<< "\t" << uri << std::endl;
}


/**
 * Writes a string to the ED2KLinks file. 
 *
 * If errors are detected, it will terminate the program.
 */
void writeLink( const string& uri, const string& config_dir )
{
	// Attempt to lock the ED2KLinks file
	static CFileLock lock(GetLinksFilePath(config_dir));
	static std::ofstream file;
	
	if (not file.is_open()) {
		string path = GetLinksFilePath(config_dir);
		file.open( path.c_str(), std::ofstream::out | std::ofstream::app );

		if (not file.is_open()) {
			std::cout << "ERROR! Failed to open " << path << " for writing!" << std::endl;
			exit(1);
		}
	}

	file << uri << std::endl;

	std::cout << "Link succesfully queued." << std::endl;
}


/**
 * Writes the the specified URI to the ED2KLinks file if it is a valid file-link.
 *
 * @param uri The URI to check.
 * @return True if the URI was written, false otherwise.
 */
bool checkFileLink( const string& uri )
{
	if ( uri.substr( 0, 13 ) == "ed2k://|file|" ) {
		size_t base_off = 12;
		size_t name_off = uri.find( '|', base_off + 1 );
		size_t size_off = uri.find( '|', name_off + 1 );
		size_t hash_off = uri.find( '|', size_off + 1 );

		bool valid = true;
		valid &= ( base_off < name_off );
		valid &= ( name_off < size_off );
		valid &= ( size_off < hash_off );
		valid &= ( hash_off != string::npos );
	
		if ( !valid ) {
			badLink( "file", "invalid link format", uri );
			return false;
		}
		
		string name = uri.substr( base_off + 1, name_off - base_off - 1 );
		string size = uri.substr( name_off + 1, size_off - name_off - 1 );
		string hash = uri.substr( size_off + 1, hash_off - size_off - 1 );

		if ( name.empty() ) {
			badLink( "file", "no name specified", uri );
			return false;
		}

		if ( !isNumber( size ) ) {
			badLink( "file", "invalid size", uri );
			return false;
		}

		if ( !isMD4Hash( hash ) ) {
			badLink( "file", "invalid MD4 hash", uri );
			return false;
		}

		return true;
	}

	return false;
}


/**
 * Writes the the specified URI to the ED2KLinks file if it is a valid server-link.
 *
 * @param uri The URI to check.
 * @return True if the URI was written, false otherwise.
 */
bool checkServerLink( const string& uri )
{
	if ( uri.substr( 0, 15 ) == "ed2k://|server|" ) {
		size_t base_off = 14;
		size_t host_off = uri.find( '|', base_off + 1 );
		size_t port_off = uri.find( '|', host_off + 1 );

		bool valid = true;
		valid &= ( base_off < host_off );
		valid &= ( host_off < port_off );
		valid &= ( port_off != string::npos );
	
		if ( !valid || uri.at( port_off + 1 ) != '/' ) {
			badLink( "server", "invalid link format", uri );
			return false;
		}
		
		string host = uri.substr( base_off + 1, host_off - base_off - 1 );
		string port = uri.substr( host_off + 1, port_off - host_off - 1 );

		if ( host.empty() ) {
			badLink( "server", "no hostname specified", uri );
			return false;
		}

		if ( !isNumber( port ) ) {
			badLink( "server", "invalid port", uri );
			return false;
		}

		return true;
	}

	return false;
}


/**
 * Writes the the specified URI to the ED2KLinks file if it is a valid serverlist-link.
 *
 * @param uri The URI to check.
 * @return True if the URI was written, false otherwise.
 */
bool checkServerListLink( const string& uri )
{
	if ( uri.substr( 0, 19 ) == "ed2k://|serverlist|" ) {
		size_t base_off = 19;
		size_t path_off = uri.find( '|', base_off + 1 );

		bool valid = true;
		valid &= ( base_off < path_off );
		valid &= ( path_off != string::npos );
	
		if ( !valid ) {
			badLink( "serverlist", "invalid link format", uri );
			return false;
		}
		
		string path = uri.substr( base_off + 1, path_off - base_off - 1 );

		if ( path.empty() ) {
			badLink( "serverlist", "no hostname specified", uri );
			return false;
		}

		return true;
	}

	return false;
}


int main(int argc, char *argv[])
{
	bool errors = false;
	string config_path;
	for ( int i = 1; i < argc; i++ ) {
		string arg = strip( Unescape( string( argv[i] ) ) );

		if ( arg.substr( 0, 8 ) == "ed2k://|" ) {
			// Ensure the URI is valid
			if ( arg.at( arg.length() - 1 ) != '/' ) {
				arg += '/';
			}
			
			string type = arg.substr( 8, arg.find( '|', 9 ) - 8 );
		
			if ( (type == "file") and checkFileLink( arg ) ) {
				writeLink( arg, config_path );
			} else if ( (type == "server") and checkServerLink( arg ) ) {
				writeLink( arg, config_path );
			} else if ( (type == "serverlist") and checkServerListLink( arg ) ) {
				writeLink( arg, config_path );
			} else {
				std::cout << "Unknown or invalid link-type:\n\t" << arg << std::endl;
				errors = true;
			}
		} else if (arg == "-c" || arg == "--config-dir") {
			if (i < argc - 1) {
				config_path = argv[++i];
			} else {
				std::cerr << "Missing mandatory argument for " << arg << std::endl;
				errors = true;
			}
		} else if (arg.substr(0, 2) == "-c") {
			config_path = arg.substr(2);
		} else if (arg.substr(0, 13) == "--config-dir=") {
			config_path = arg.substr(13);
		} else if (arg == "-h" || arg == "--help") {
			std::cout << getVersion()
				<< "\n\n"
				<< "Usage:\n"
				<< "    --help, -h              Prints this help.\n"
				<< "    --config-dir, -c        Specifies the aMule configuration directory.\n"
				<< "    --version, -v           Displays version info.\n\n"
				<< "    ed2k://|file|           Causes the file to be queued for download.\n"
				<< "    ed2k://|server|         Causes the server to be listed or updated.\n"
				<< "    ed2k://|serverlist|     Causes aMule to update the current serverlist.\n\n"
				<< "    --emulecollection, -e   Loads all links of an emulecollection\n\n"
				<< "*** NOTE: Option order is important! ***\n"
				<< std::endl;
			
		} else if (arg == "-v" || arg == "--version") {
			std::cout << getVersion() << std::endl;
		} else if (arg == "-e" || arg == "--emulecollection") {
			if (i < argc - 1) {
				CMuleCollection my_collection;
				if (my_collection.Open( /* emulecollection file */ argv[++i] ))
				{
					for(int e = 0;e < my_collection.GetFileCount();e++)
						writeLink( my_collection.GetEd2kLink(e), config_path );
				} else {
					std::cerr << "Invalid emulecollection file: " << argv[i] << std::endl;
					errors = true;
				}
			} else {
				std::cerr << "Missing mandatory argument for " << arg << std::endl;
				errors = true;
			}
			std::cout << getVersion() << std::endl;
		} else {
			std::cerr << "Bad parameter value:\n\t" << arg << "\n" << std::endl;
			errors = true;
		}
	}

	return ( errors ? 1 : 0 );
}

// File_checked_for_headers
