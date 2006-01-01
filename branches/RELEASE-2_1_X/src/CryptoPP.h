////////////////////////////////////////////////////////////////////////////////
// 
// This file contains a subset of the Crypto++ library (version 5.2.1), with 
// kind permission from Wei Dai. Please note that this file should not reflect 
// on the real Crypto++ library in any way, as this file been greatly mangled to 
// reduce the code-size, since aMule only makes use of RSA classes (for Secure 
// Identification) and the MD4 hashing class (for FileIDs). 
//
// For the full Crypto++ library, please refer to the official Crypto++ website,
// which can be found at, http://www.cryptopp.com
// 
////////////////////////////////////////////////////////////////////////////////
// 
// Compilation Copyright (c) 1995-2004 by Wei Dai.  All rights reserved.
// This copyright applies only to this software distribution package 
// as a compilation, and does not imply a copyright on any particular 
// file in the package.
// 
// The following files are copyrighted by their respective original authors,
// and their use is subject to additional licenses included in these files.
// 
// mars.cpp - Copyright 1998 Brian Gladman.

// All other files in this compilation are placed in the public domain by
// Wei Dai and other contributors.
// 
// I would like to thank the following authors for placing their works into
// the public domain:
// 
// Joan Daemen - 3way.cpp
// Leonard Janke - cast.cpp, seal.cpp
// Steve Reid - cast.cpp
// Phil Karn - des.cpp
// Michael Paul Johnson - diamond.cpp
// Andrew M. Kuchling - md2.cpp, md4.cpp
// Colin Plumb - md5.cpp, md5mac.cpp
// Seal Woods - rc6.cpp
// Chris Morgan - rijndael.cpp
// Paulo Baretto - rijndael.cpp, skipjack.cpp, square.cpp
// Richard De Moliner - safer.cpp
// Matthew Skala - twofish.cpp
// Kevin Springle - camellia.cpp, shacal2.cpp, ttmac.cpp, whrlpool.cpp, ripemd.cpp
// 
// Permission to use, copy, modify, and distribute this compilation for
// any purpose, including commercial applications, is hereby granted
// without fee, subject to the following restrictions:
// 
// 1. Any copy or modification of this compilation in any form, except
// in object code form as part of an application software, must include
// the above copyright notice and this license.
// 
// 2. Users of this software agree that any modification or extension
// they provide to Wei Dai will be considered public domain and not
// copyrighted unless it includes an explicit copyright notice.
// 
// 3. Wei Dai makes no warranty or representation that the operation of the
// software in this compilation will be error-free, and Wei Dai is under no
// obligation to provide any services, by way of maintenance, update, or
// otherwise.  THE SOFTWARE AND ANY DOCUMENTATION ARE PROVIDED "AS IS"
// WITHOUT EXPRESS OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE. IN NO EVENT WILL WEI DAI OR ANY OTHER CONTRIBUTOR BE LIABLE FOR
// DIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
// 
// 4. Users will not use Wei Dai or any other contributor's name in any 
// publicity or advertising, without prior written consent in each case.
// 
// 5. Export of this software from the United States may require a
// specific license from the United States Government.  It is the
// responsibility of any person or organization contemplating export
// to obtain such a license before exporting.
// 
// 6. Certain parts of this software may be protected by patents.  It
// is the users' responsibility to obtain the appropriate
// licenses before using those parts.
// 
// If this compilation is used in object code form in an application
// software, acknowledgement of the author is not required but would be
// appreciated. The contribution of any useful modifications or extensions
// to Wei Dai is not required but would also be appreciated.
// 
////////////////////////////////////////////////////////////////////////////////

#ifndef CRYPTOPP_H
#define CRYPTOPP_H

#include <inttypes.h>

////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_CONFIG_H5392
#define CRYPTOPP_CONFIG_H

// ***************** Important Settings ********************
#ifdef wxBYTE_ORDER
#if wxBYTE_ORDER == wxLITTLE_ENDIAN
#	define IS_LITTLE_ENDIAN
#else
#	define IS_BIG_ENDIAN
#endif
#endif


// define this if running on a big-endian CPU
#if !defined(IS_LITTLE_ENDIAN) && (defined(__BIG_ENDIAN__) || defined(__sparc) || defined(__sparc__) || defined(__hppa__) || defined(__mips__) || (defined(__MWERKS__) && !defined(__INTEL__)))
#	define IS_BIG_ENDIAN
#endif

// define this if running on a little-endian CPU
// big endian will be assumed if IS_LITTLE_ENDIAN is not defined
#ifndef IS_BIG_ENDIAN
#	define IS_LITTLE_ENDIAN
#endif

// define this if you want to disable all OS-dependent features,
// such as sockets and OS-provided random number generators
// #define NO_OS_DEPENDENCE

// Define this to use features provided by Microsoft's CryptoAPI.
// Currently the only feature used is random number generation.
// This macro will be ignored if NO_OS_DEPENDENCE is defined.
#define USE_MS_CRYPTOAPI

// Define this to 1 to enforce the requirement in FIPS 186-2 Change Notice 1 that only 1024 bit moduli be used
#ifndef DSA_1024_BIT_MODULUS_ONLY
#	define DSA_1024_BIT_MODULUS_ONLY 1
#endif

// ***************** Less Important Settings ***************

#define GZIP_OS_CODE 0

// Try this if your CPU has 256K internal cache or a slow multiply instruction
// and you want a (possibly) faster IDEA implementation using log tables
// #define IDEA_LARGECACHE

// Define this if, for the linear congruential RNG, you want to use
// the original constants as specified in S.K. Park and K.W. Miller's
// CACM paper.
// #define LCRNG_ORIGINAL_NUMBERS

// choose which style of sockets to wrap (mostly useful for cygwin which has both)
#define PREFER_BERKELEY_STYLE_SOCKETS
// #define PREFER_WINDOWS_STYLE_SOCKETS

// ***************** Important Settings Again ********************
// But the defaults should be ok.

// namespace support is now required
#ifdef NO_NAMESPACE
#	error namespace support is now required
#endif

// Define this to workaround a Microsoft CryptoAPI bug where
// each call to CryptAcquireContext causes a 100 KB memory leak.
// Defining this will cause Crypto++ to make only one call to CryptAcquireContext.
#define WORKAROUND_MS_BUG_Q258000

#ifdef CRYPTOPP_DOXYGEN_PROCESSING
// Avoid putting "CryptoPP::" in front of everything in Doxygen output
#	define CryptoPP
#	define NAMESPACE_BEGIN(x)
#	define NAMESPACE_END
// Get Doxygen to generate better documentation for these typedefs
#	define DOCUMENTED_TYPEDEF(x, y) class y : public x {};
#else
#	define NAMESPACE_BEGIN(x) namespace x {
#	define NAMESPACE_END }
#	define DOCUMENTED_TYPEDEF(x, y) typedef x y;
#endif
#define ANONYMOUS_NAMESPACE_BEGIN namespace {
#define USING_NAMESPACE(x) using namespace x;
#define DOCUMENTED_NAMESPACE_BEGIN(x) namespace x {
#define DOCUMENTED_NAMESPACE_END }

// What is the type of the third parameter to bind?
// For Unix, the new standard is ::socklen_t (typically unsigned int), and the old standard is int.
// Unfortunately there is no way to tell whether or not socklen_t is defined.
// To work around this, TYPE_OF_SOCKLEN_T is a macro so that you can change it from the makefile.
#ifndef TYPE_OF_SOCKLEN_T
#	if defined(_WIN32) || defined(__CYGWIN__) || defined(__MACH__)
#		define TYPE_OF_SOCKLEN_T int
#	else
#		define TYPE_OF_SOCKLEN_T ::socklen_t
#	endif
#endif

#if defined(__CYGWIN__) && defined(PREFER_WINDOWS_STYLE_SOCKETS)
#	define __USE_W32_SOCKETS
#endif

typedef unsigned char byte;		// put in global namespace to avoid ambiguity with other byte typedefs

NAMESPACE_BEGIN(CryptoPP)

typedef unsigned short word16;
typedef unsigned int word32;

#if defined(__GNUC__) || defined(__MWERKS__)
	#define WORD64_AVAILABLE
	typedef uint64_t word64;
	#define W64LIT(x) x##LL
#elif defined(_MSC_VER) || defined(__BCPLUSPLUS__)
	#define WORD64_AVAILABLE
	typedef unsigned __int64 word64;
	#define W64LIT(x) x##ui64
#endif

// define largest word type
#ifdef WORD64_AVAILABLE
	typedef word64 lword;
#else
	typedef word32 lword;
#endif

#if defined(__alpha__) || defined(__ia64__) || defined(_ARCH_PPC64) || defined(__x86_64__) || defined(__mips64)
	// These platforms have 64-bit CPU registers. Unfortunately most C++ compilers doesn't
	// allow any way to access the 64-bit by 64-bit multiply instruction without using
	// assembly, so in order to use word64 as word, the assembly instruction must be defined
	// in Dword::Multiply().
	typedef word32 hword;
	typedef word64 word;
#else
	#define CRYPTOPP_NATIVE_DWORD_AVAILABLE
	#ifdef WORD64_AVAILABLE
		#define CRYPTOPP_SLOW_WORD64 // defined this if your CPU is not 64-bit to use alternative code that avoids word64
		typedef word16 hword;
		typedef word32 word;
		typedef word64 dword;
	#else
		typedef word8 hword;
		typedef word16 word;
		typedef word32 dword;
	#endif
#endif

const unsigned int WORD_SIZE = sizeof(word);
const unsigned int WORD_BITS = WORD_SIZE * 8;

#if defined(_MSC_VER) || defined(__BCPLUSPLUS__)
	#define INTEL_INTRINSICS
	#define FAST_ROTATE
#elif defined(__MWERKS__) && TARGET_CPU_PPC
	#define PPC_INTRINSICS
	#define FAST_ROTATE
#elif defined(__GNUC__) && defined(__i386__)
	// GCC does peephole optimizations which should result in using rotate instructions
	#define FAST_ROTATE
#endif

NAMESPACE_END

// VC60 workaround: it doesn't allow typename in some places
#if defined(_MSC_VER) && (_MSC_VER < 1300)
#define CPP_TYPENAME
#else
#define CPP_TYPENAME typename
#endif

#define CRYPTOPP_NO_VTABLE

#ifdef _MSC_VER
	// 4231: nonstandard extension used : 'extern' before template explicit instantiation
	// 4250: dominance
	// 4251: member needs to have dll-interface
	// 4275: base needs to have dll-interface
	// 4660: explicitly instantiating a class that's already implicitly instantiated
	// 4661: no suitable definition provided for explicit template instantiation request
	// 4786: identifer was truncated in debug information
	// 4355: 'this' : used in base member initializer list
#	pragma warning(disable: 4231 4250 4251 4275 4660 4661 4786 4355)
#endif

#if (defined(_MSC_VER) && _MSC_VER <= 1300) || defined(__MWERKS__) || defined(_STLPORT_VERSION)
#define CRYPTOPP_DISABLE_UNCAUGHT_EXCEPTION
#endif

#ifndef CRYPTOPP_DISABLE_UNCAUGHT_EXCEPTION
#define CRYPTOPP_UNCAUGHT_EXCEPTION_AVAILABLE
#endif

// CodeWarrior defines _MSC_VER
#if !defined(CRYPTOPP_DISABLE_X86ASM) && ((defined(_MSC_VER) && !defined(__MWERKS__) && defined(_M_IX86)) || (defined(__GNUC__) && defined(__i386__)))
	// The x86 version of MacOSX fails when asm is enabled.
	#if !defined(__i386__) || !defined(__APPLE__)
		#define CRYPTOPP_X86ASM_AVAILABLE
	#endif
#endif

// ***************** determine availability of OS features ********************

#ifndef NO_OS_DEPENDENCE

#if defined(_WIN32) || defined(__CYGWIN__)
#define CRYPTOPP_WIN32_AVAILABLE
#endif

#if defined(__unix__) || defined(__MACH__) || defined(__NetBSD__)
#define CRYPTOPP_UNIX_AVAILABLE
#endif

#if defined(WORD64_AVAILABLE) && (defined(CRYPTOPP_WIN32_AVAILABLE) || defined(CRYPTOPP_UNIX_AVAILABLE))
#	define HIGHRES_TIMER_AVAILABLE
#endif

#ifdef CRYPTOPP_UNIX_AVAILABLE
#	define HAS_BERKELEY_STYLE_SOCKETS
#endif

#ifdef CRYPTOPP_WIN32_AVAILABLE
#	define HAS_WINDOWS_STYLE_SOCKETS
#endif

#if defined(HIGHRES_TIMER_AVAILABLE) && (defined(HAS_BERKELEY_STYLE_SOCKETS) || defined(HAS_WINDOWS_STYLE_SOCKETS))
#	define SOCKETS_AVAILABLE
#endif

#if defined(HAS_WINDOWS_STYLE_SOCKETS) && (!defined(HAS_BERKELEY_STYLE_SOCKETS) || defined(PREFER_WINDOWS_STYLE_SOCKETS))
#	define USE_WINDOWS_STYLE_SOCKETS
#else
#	define USE_BERKELEY_STYLE_SOCKETS
#endif

#if defined(CRYPTOPP_WIN32_AVAILABLE) && !defined(USE_BERKELEY_STYLE_SOCKETS)
#	define WINDOWS_PIPES_AVAILABLE
#endif

#if defined(CRYPTOPP_WIN32_AVAILABLE) && defined(USE_MS_CRYPTOAPI)
#	define NONBLOCKING_RNG_AVAILABLE
#	define OS_RNG_AVAILABLE
#endif

#if defined(CRYPTOPP_UNIX_AVAILABLE) || defined(CRYPTOPP_DOXYGEN_PROCESSING)
#	define NONBLOCKING_RNG_AVAILABLE
#	define BLOCKING_RNG_AVAILABLE
#	define OS_RNG_AVAILABLE
#	define HAS_PTHREADS
#	define THREADS_AVAILABLE
#endif

#ifdef CRYPTOPP_WIN32_AVAILABLE
#	define HAS_WINTHREADS
#	define THREADS_AVAILABLE
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#	define CRYPTOPP_MALLOC_ALIGNMENT_IS_16
#endif

#if defined(__linux__) || defined(__sun__) || defined(__CYGWIN__)
#	define CRYPTOPP_MEMALIGN_AVAILABLE
#endif

#endif	// NO_OS_DEPENDENCE

// ***************** DLL related ********************

#define CRYPTOPP_DLL
#define CRYPTOPP_API
#define CRYPTOPP_CDECL

#if defined(CRYPTOPP_MANUALLY_INSTANTIATE_TEMPLATES) && !defined(CRYPTOPP_EXPORTS)
#define CRYPTOPP_STATIC_TEMPLATE_CLASS template class
#elif defined(__MWERKS__)
#define CRYPTOPP_STATIC_TEMPLATE_CLASS extern class
#else
#define CRYPTOPP_STATIC_TEMPLATE_CLASS extern template class
#endif

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_STDCPP_H
#define CRYPTOPP_STDCPP_H

#include <stddef.h>
#include <assert.h>
#include <limits.h>
#include <memory>
#include <string>
#include <exception>
#include <typeinfo>


#ifdef _MSC_VER
#include <string.h>	// CodeWarrior doesn't have memory.h
#include <algorithm>
#include <map>
#include <vector>
#include <locale>

// re-disable this
#pragma warning(disable: 4231)
#endif

#if defined(_MSC_VER) && defined(_CRTAPI1)
#define CRYPTOPP_MSVCRT6
#endif

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// cryptlib.h - written and placed in the public domain by Wei Dai
/*! \file
 	This file contains the declarations for the abstract base
	classes that provide a uniform interface to this library.
*/

/*!	\mainpage <a href="http://www.cryptopp.com">Crypto++</a><sup><small>TM</small></sup> Library 5.2.1 Reference Manual
<dl>
<dt>Abstract Base Classes<dd>
	cryptlib.h
<dt>Symmetric Ciphers<dd>
	SymmetricCipherDocumentation
<dt>Hash Functions<dd>
	HAVAL, MD2, MD4, MD5, PanamaHash, RIPEMD160, RIPEMD320, RIPEMD128, RIPEMD256, SHA, SHA256, SHA384, SHA512, Tiger, Whirlpool
<dt>Non-Cryptographic Checksums<dd>
	CRC32, Adler32
<dt>Message Authentication Codes<dd>
	#MD5MAC, XMACC, HMAC, CBC_MAC, DMAC, PanamaMAC, TTMAC
<dt>Random Number Generators<dd>
	NullRNG(), LC_RNG, RandomPool, BlockingRng, NonblockingRng, AutoSeededRandomPool, AutoSeededX917RNG
<dt>Password-based Cryptography<dd>
	PasswordBasedKeyDerivationFunction
<dt>Public Key Cryptosystems<dd>
	DLIES, ECIES, LUCES, RSAES, RabinES, LUC_IES
<dt>Public Key Signature Schemes<dd>
	DSA, GDSA, ECDSA, NR, ECNR, LUCSS, RSASS, RabinSS, RWSS, ESIGN
<dt>Key Agreement<dd>
	#DH, DH2, #MQV, ECDH, ECMQV, XTR_DH
<dt>Algebraic Structures<dd>
	Integer, PolynomialMod2, PolynomialOver, RingOfPolynomialsOver,
	ModularArithmetic, MontgomeryRepresentation, GFP2_ONB,
	GF2NP, GF256, GF2_32, EC2N, ECP
<dt>Secret Sharing and Information Dispersal<dd>
	SecretSharing, SecretRecovery, InformationDispersal, InformationRecovery
<dt>Compression<dd>
	Deflator, Inflator, Gzip, Gunzip, ZlibCompressor, ZlibDecompressor
<dt>Input Source Classes<dd>
	StringSource, FileSource, SocketSource, WindowsPipeSource, RandomNumberSource
<dt>Output Sink Classes<dd>
	StringSinkTemplate, ArraySink, FileSink, SocketSink, WindowsPipeSink
<dt>Filter Wrappers<dd>
	StreamTransformationFilter, HashFilter, HashVerificationFilter, SignerFilter, SignatureVerificationFilter
<dt>Binary to Text Encoders and Decoders<dd>
	HexEncoder, HexDecoder, Base64Encoder, Base64Decoder, Base32Encoder, Base32Decoder
<dt>Wrappers for OS features<dd>
	Timer, Socket, WindowsHandle, ThreadLocalStorage, ThreadUserTimer
<dt>FIPS 140 related<dd>
	fips140.h
</dl>

In the FIPS 140-2 validated DLL version of Crypto++, only the following implementation class are available.
<dl>
<dt>Block Ciphers<dd>
	AES, DES_EDE2, DES_EDE3, SKIPJACK
<dt>Cipher Modes (replace template parameter BC with one of the block ciphers above)<dd>
	ECB_Mode\<BC\>, CTR_Mode\<BC\>, CBC_Mode\<BC\>, CFB_Mode\<BC\>, OFB_Mode\<BC\>
<dt>Hash Functions<dd>
	SHA
<dt>Public Key Signature Schemes<dd>
	RSASS\<PKCS1v15, SHA\>, DSA, ECDSA\<ECP, SHA\>, ECDSA\<EC2N, SHA\>
<dt>Message Authentication Codes<dd>
	HMAC\<SHA\>, CBC_MAC\<DES_EDE2\>, CBC_MAC\<DES_EDE3\>
<dt>Random Number Generators<dd>
	AutoSeededX917RNG\<DES_EDE3\>
<dt>Key Agreement<dd>
	#DH
<dt>Public Key Cryptosystems<dd>
	RSAES\<OAEP\<SHA\> \>
</dl>

<p>This reference manual is a work in progress. Some classes are still lacking detailed descriptions.
<p>Click <a href="CryptoPPRef.zip">here</a> to download a zip archive containing this manual.
<p>Thanks to Ryan Phillips for providing the Doxygen configuration file
and getting me started with this manual.
*/

#ifndef CRYPTOPP_CRYPTLIB_H
#define CRYPTOPP_CRYPTLIB_H

//- #include "config.h"
//- #include "stdcpp.h"

NAMESPACE_BEGIN(CryptoPP)

// forward declarations
class Integer;

//! used to specify a direction for a cipher to operate in (encrypt or decrypt)
enum CipherDir {ENCRYPTION,	DECRYPTION};

// VC60 workaround: using enums as template parameters causes problems
template <typename ENUM_TYPE, int VALUE>
struct EnumToType
{
	static ENUM_TYPE ToEnum() {return (ENUM_TYPE)VALUE;}
};

enum ByteOrder {LITTLE_ENDIAN_ORDER = 0, BIG_ENDIAN_ORDER = 1};
typedef EnumToType<ByteOrder, LITTLE_ENDIAN_ORDER> LittleEndian;
typedef EnumToType<ByteOrder, BIG_ENDIAN_ORDER> BigEndian;

//! base class for all exceptions thrown by Crypto++
class CRYPTOPP_DLL Exception : public std::exception
{
public:
	//! error types
	enum ErrorType {
		//! a method is not implemented
		NOT_IMPLEMENTED,
		//! invalid function argument
		INVALID_ARGUMENT,
		//! BufferedTransformation received a Flush(true) signal but can't flush buffers
		CANNOT_FLUSH,
		//! data integerity check (such as CRC or MAC) failed
		DATA_INTEGRITY_CHECK_FAILED,
		//! received input data that doesn't conform to expected format
		INVALID_DATA_FORMAT,
		//! error reading from input device or writing to output device
		IO_ERROR,
		//! some error not belong to any of the above categories
		OTHER_ERROR
	};

	explicit Exception(ErrorType errorType, const std::string &s) : m_errorType(errorType), m_what(s) {}
	virtual ~Exception() throw() {}
	const char *what() const throw() {return (m_what.c_str());}
	const std::string &GetWhat() const {return m_what;}
	void SetWhat(const std::string &s) {m_what = s;}
	ErrorType GetErrorType() const {return m_errorType;}
	void SetErrorType(ErrorType errorType) {m_errorType = errorType;}

private:
	ErrorType m_errorType;
	std::string m_what;
};

//! exception thrown when an invalid argument is detected
class CRYPTOPP_DLL InvalidArgument : public Exception
{
public:
	explicit InvalidArgument(const std::string &s) : Exception(INVALID_ARGUMENT, s) {}
};

//! exception thrown by decryption filters when trying to decrypt an invalid ciphertext
class CRYPTOPP_DLL InvalidDataFormat : public Exception
{
public:
	explicit InvalidDataFormat(const std::string &s) : Exception(INVALID_DATA_FORMAT, s) {}
};

//! exception thrown by a class if a non-implemented method is called
class CRYPTOPP_DLL NotImplemented : public Exception
{
public:
	explicit NotImplemented(const std::string &s) : Exception(NOT_IMPLEMENTED, s) {}
};

//! exception thrown by a class when Flush(true) is called but it can't completely flush its buffers
class CRYPTOPP_DLL CannotFlush : public Exception
{
public:
	explicit CannotFlush(const std::string &s) : Exception(CANNOT_FLUSH, s) {}
};

//! error reported by the operating system
class CRYPTOPP_DLL OS_Error : public Exception
{
public:
	OS_Error(ErrorType errorType, const std::string &s, const std::string& operation, int errorCode)
		: Exception(errorType, s), m_operation(operation), m_errorCode(errorCode) {}
	~OS_Error() throw() {}

	// the operating system API that reported the error
	const std::string & GetOperation() const {return m_operation;}
	// the error code return by the operating system
	int GetErrorCode() const {return m_errorCode;}

protected:
	std::string m_operation;
	int m_errorCode;
};

//! used to return decoding results
struct CRYPTOPP_DLL DecodingResult
{
	explicit DecodingResult() : isValidCoding(false), messageLength(0) {}
	explicit DecodingResult(unsigned int len) : isValidCoding(true), messageLength(len) {}

	bool operator==(const DecodingResult &rhs) const {return isValidCoding == rhs.isValidCoding && messageLength == rhs.messageLength;}
	bool operator!=(const DecodingResult &rhs) const {return !operator==(rhs);}

	bool isValidCoding;
	unsigned int messageLength;

};

//! interface for retrieving values given their names
/*! \note This class is used to safely pass a variable number of arbitrarily typed arguments to functions
	and to read values from keys and crypto parameters.
	\note To obtain an object that implements NameValuePairs for the purpose of parameter
	passing, use the MakeParameters() function.
	\note To get a value from NameValuePairs, you need to know the name and the type of the value. 
	Call GetValueNames() on a NameValuePairs object to obtain a list of value names that it supports.
	Then look at the Name namespace documentation to see what the type of each value is, or
	alternatively, call GetIntValue() with the value name, and if the type is not int, a
	ValueTypeMismatch exception will be thrown and you can get the actual type from the exception object.
*/
class CRYPTOPP_NO_VTABLE NameValuePairs
{
public:
	virtual ~NameValuePairs() {}

	//! exception thrown when trying to retrieve a value using a different type than expected
	class CRYPTOPP_DLL ValueTypeMismatch : public InvalidArgument
	{
	public:
		ValueTypeMismatch(const std::string &name, const std::type_info &stored, const std::type_info &retrieving)
			: InvalidArgument("NameValuePairs: type mismatch for '" + name + "', stored '" + stored.name() + "', trying to retrieve '" + retrieving.name() + "'")
			, m_stored(stored), m_retrieving(retrieving) {}

		const std::type_info & GetStoredTypeInfo() const {return m_stored;}
		const std::type_info & GetRetrievingTypeInfo() const {return m_retrieving;}

	private:
		const std::type_info &m_stored;
		const std::type_info &m_retrieving;
	};

	//! get a copy of this object or a subobject of it
	template <class T>
	bool GetThisObject(T &object) const
	{
		return GetValue((std::string("ThisObject:")+typeid(T).name()).c_str(), object);
	}

	//! get a pointer to this object, as a pointer to T
	template <class T>
	bool GetThisPointer(T *&p) const
	{
		return GetValue((std::string("ThisPointer:")+typeid(T).name()).c_str(), p);
	}

	//! get a named value, returns true if the name exists
	template <class T>
	bool GetValue(const char *name, T &value) const
	{
		return GetVoidValue(name, typeid(T), &value);
	}

	//! get a named value, returns the default if the name doesn't exist
	template <class T>
	T GetValueWithDefault(const char *name, T defaultValue) const
	{
		GetValue(name, defaultValue);
		return defaultValue;
	}

	//! get a list of value names that can be retrieved
	CRYPTOPP_DLL std::string GetValueNames() const
		{std::string result; GetValue("ValueNames", result); return result;}

	//! get a named value with type int
	/*! used to ensure we don't accidentally try to get an unsigned int
		or some other type when we mean int (which is the most common case) */
	CRYPTOPP_DLL bool GetIntValue(const char *name, int &value) const
		{return GetValue(name, value);}

	//! get a named value with type int, with default
	CRYPTOPP_DLL int GetIntValueWithDefault(const char *name, int defaultValue) const
		{return GetValueWithDefault(name, defaultValue);}

	//! used by derived classes to check for type mismatch
	CRYPTOPP_DLL static void ThrowIfTypeMismatch(const char *name, const std::type_info &stored, const std::type_info &retrieving)
		{if (stored != retrieving) throw ValueTypeMismatch(name, stored, retrieving);}

	template <class T>
	void GetRequiredParameter(const char *className, const char *name, T &value) const
	{
		if (!GetValue(name, value))
			throw InvalidArgument(std::string(className) + ": missing required parameter '" + name + "'");
	}

	CRYPTOPP_DLL void GetRequiredIntParameter(const char *className, const char *name, int &value) const
	{
		if (!GetIntValue(name, value))
			throw InvalidArgument(std::string(className) + ": missing required parameter '" + name + "'");
	}

	//! to be implemented by derived classes, users should use one of the above functions instead
	CRYPTOPP_DLL virtual bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const =0;
};

//! empty set of name-value pairs
class CRYPTOPP_DLL NullNameValuePairs : public NameValuePairs
{
public:
	bool GetVoidValue(const char* /* name */, const std::type_info& /* valueType */, void* /* pValue */) const {return false;}
};

//! _
extern CRYPTOPP_DLL const NullNameValuePairs g_nullNameValuePairs;

// ********************************************************

//! interface for cloning objects, this is not implemented by most classes yet
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Clonable
{
public:
	virtual ~Clonable() {}
	//! this is not implemented by most classes yet
	virtual Clonable* Clone() const {throw NotImplemented("Clone() is not implemented yet.");}	// TODO: make this =0
};

//! interface for all crypto algorithms

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Algorithm : public Clonable
{
public:
	/*! When FIPS 140-2 compliance is enabled and checkSelfTestStatus == true,
		this constructor throws SelfTestFailure if the self test hasn't been run or fails. */
	Algorithm(bool checkSelfTestStatus = true);
	//! returns name of this algorithm, not universally implemented yet
	virtual std::string AlgorithmName() const {return "unknown";}
};

//! keying interface for crypto algorithms that take byte strings as keys

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE SimpleKeyingInterface
{
public:
	virtual ~SimpleKeyingInterface() {};
	
	//! returns smallest valid key length in bytes */
	virtual unsigned int MinKeyLength() const =0;
	//! returns largest valid key length in bytes */
	virtual unsigned int MaxKeyLength() const =0;
	//! returns default (recommended) key length in bytes */
	virtual unsigned int DefaultKeyLength() const =0;

	//! returns the smallest valid key length in bytes that is >= min(n, GetMaxKeyLength())
	virtual unsigned int GetValidKeyLength(unsigned int n) const =0;

	//! returns whether n is a valid key length
	virtual bool IsValidKeyLength(unsigned int n) const
		{return n == GetValidKeyLength(n);}

	//! set or reset the key of this object
	/*! \param params is used to specify Rounds, BlockSize, etc */
	virtual void SetKey(const byte *key, unsigned int length, const NameValuePairs &params = g_nullNameValuePairs) =0;

	//! calls SetKey() with an NameValuePairs object that just specifies "Rounds"
	void SetKeyWithRounds(const byte *key, unsigned int length, int rounds);

	//! calls SetKey() with an NameValuePairs object that just specifies "IV"
	void SetKeyWithIV(const byte *key, unsigned int length, const byte *iv);

	enum IV_Requirement {STRUCTURED_IV = 0, RANDOM_IV, UNPREDICTABLE_RANDOM_IV, INTERNALLY_GENERATED_IV, NOT_RESYNCHRONIZABLE};
	//! returns the minimal requirement for secure IVs
	virtual IV_Requirement IVRequirement() const =0;

	//! returns whether this object can be resynchronized (i.e. supports initialization vectors)
	/*! If this function returns true, and no IV is passed to SetKey() and CanUseStructuredIVs()==true, an IV of all 0's will be assumed. */
	bool IsResynchronizable() const {return IVRequirement() < NOT_RESYNCHRONIZABLE;}
	//! returns whether this object can use random IVs (in addition to ones returned by GetNextIV)
	bool CanUseRandomIVs() const {return IVRequirement() <= UNPREDICTABLE_RANDOM_IV;}
	//! returns whether this object can use random but possibly predictable IVs (in addition to ones returned by GetNextIV)
	bool CanUsePredictableIVs() const {return IVRequirement() <= RANDOM_IV;}
	//! returns whether this object can use structured IVs, for example a counter (in addition to ones returned by GetNextIV)
	bool CanUseStructuredIVs() const {return IVRequirement() <= STRUCTURED_IV;}

	//! returns size of IVs used by this object
	virtual unsigned int IVSize() const {throw NotImplemented("SimpleKeyingInterface: this object doesn't support resynchronization");}
	//! resynchronize with an IV
	virtual void Resynchronize(const byte* /* IV */) {throw NotImplemented("SimpleKeyingInterface: this object doesn't support resynchronization");}
	//! get a secure IV for the next message
	/*! This method should be called after you finish encrypting one message and are ready to start the next one.
		After calling it, you must call SetKey() or Resynchronize() before using this object again. 
		This method is not implemented on decryption objects. */
	virtual void GetNextIV(byte* /* IV */) {throw NotImplemented("SimpleKeyingInterface: this object doesn't support GetNextIV()");}

protected:
	void ThrowIfInvalidKeyLength(const Algorithm &algorithm, unsigned int length);
	void ThrowIfResynchronizable();			// to be called when no IV is passed
	void ThrowIfInvalidIV(const byte *iv);	// check for NULL IV if it can't be used
	const byte * GetIVAndThrowIfInvalid(const NameValuePairs &params);

	inline void AssertValidKeyLength(unsigned int length) const
	{
		assert(IsValidKeyLength(length));
	}
};

//! interface for the data processing part of block ciphers

/*! Classes derived from BlockTransformation are block ciphers
	in ECB mode (for example the DES::Encryption class), which are stateless,
	and they can make assumptions about the memory alignment of their inputs and outputs.
	These classes should not be used directly, but only in combination with
	a mode class (see CipherModeDocumentation in modes.h).
*/
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE BlockTransformation : public Algorithm
{
public:
	//! encrypt or decrypt inBlock, xor with xorBlock, and write to outBlock
	virtual void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const =0;

	//! encrypt or decrypt one block
	/*! \pre size of inBlock and outBlock == BlockSize() */
	void ProcessBlock(const byte *inBlock, byte *outBlock) const
		{ProcessAndXorBlock(inBlock, NULL, outBlock);}

	//! encrypt or decrypt one block in place
	void ProcessBlock(byte *inoutBlock) const
		{ProcessAndXorBlock(inoutBlock, NULL, inoutBlock);}

	//! block size of the cipher in bytes
	virtual unsigned int BlockSize() const =0;

	//! block pointers must be divisible by this
	virtual unsigned int BlockAlignment() const {return 4;}

	//! returns true if this is a permutation (i.e. there is an inverse transformation)
	virtual bool IsPermutation() const {return true;}

	//! returns true if this is an encryption object
	virtual bool IsForwardTransformation() const =0;

	//! return number of blocks that can be processed in parallel, for bit-slicing implementations
	virtual unsigned int OptimalNumberOfParallelBlocks() const {return 1;}

	//! encrypt or decrypt multiple blocks, for bit-slicing implementations
	virtual void ProcessAndXorMultipleBlocks(const byte *inBlocks, const byte *xorBlocks, byte *outBlocks, unsigned int numberOfBlocks) const;
};

//! interface for the data processing part of stream ciphers

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE StreamTransformation : public Algorithm
{
public:
	//! return a reference to this object, 
	/*! This function is useful for passing a temporary StreamTransformation object to a 
		function that takes a non-const reference. */
	StreamTransformation& Ref() {return *this;}

	//! returns block size, if input must be processed in blocks, otherwise 1
	virtual unsigned int MandatoryBlockSize() const {return 1;}

	//! returns the input block size that is most efficient for this cipher
	/*! \note optimal input length is n * OptimalBlockSize() - GetOptimalBlockSizeUsed() for any n > 0 */
	virtual unsigned int OptimalBlockSize() const {return MandatoryBlockSize();}
	//! returns how much of the current block is used up
	virtual unsigned int GetOptimalBlockSizeUsed() const {return 0;}

	//! returns how input should be aligned for optimal performance
	virtual unsigned int OptimalDataAlignment() const {return 1;}

	//! encrypt or decrypt an array of bytes of specified length
	/*! \note either inString == outString, or they don't overlap */
	virtual void ProcessData(byte *outString, const byte *inString, unsigned int length) =0;

	//! for ciphers where the last block of data is special, encrypt or decrypt the last block of data
	/*! For now the only use of this function is for CBC-CTS mode. */
	virtual void ProcessLastBlock(byte *outString, const byte *inString, unsigned int length);
	//! returns the minimum size of the last block, 0 indicating the last block is not special
	virtual unsigned int MinLastBlockSize() const {return 0;}

	//! same as ProcessData(inoutString, inoutString, length)
	inline void ProcessString(byte *inoutString, unsigned int length)
		{ProcessData(inoutString, inoutString, length);}
	//! same as ProcessData(outString, inString, length)
	inline void ProcessString(byte *outString, const byte *inString, unsigned int length)
		{ProcessData(outString, inString, length);}
	//! implemented as {ProcessData(&input, &input, 1); return input;}
	inline byte ProcessByte(byte input)
		{ProcessData(&input, &input, 1); return input;}

	//! returns whether this cipher supports random access
	virtual bool IsRandomAccess() const =0;
	//! for random access ciphers, seek to an absolute position
	virtual void Seek(lword /* n */)
	{
		assert(!IsRandomAccess());
		throw NotImplemented("StreamTransformation: this object doesn't support random access");
	}

	//! returns whether this transformation is self-inverting (e.g. xor with a keystream)
	virtual bool IsSelfInverting() const =0;
	//! returns whether this is an encryption object
	virtual bool IsForwardTransformation() const =0;
};

//! interface for hash functions and data processing part of MACs

/*! HashTransformation objects are stateful.  They are created in an initial state,
	change state as Update() is called, and return to the initial
	state when Final() is called.  This interface allows a large message to
	be hashed in pieces by calling Update() on each piece followed by
	calling Final().
*/
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE HashTransformation : public Algorithm
{
public:
	//! process more input
	virtual void Update(const byte *input, unsigned int length) =0;

	//! request space to write input into
	virtual byte * CreateUpdateSpace(unsigned int &size) {size=0; return NULL;}

	//! compute hash for current message, then restart for a new message
	/*!	\pre size of digest == DigestSize(). */
	virtual void Final(byte *digest)
		{TruncatedFinal(digest, DigestSize());}

	//! discard the current state, and restart with a new message
	virtual void Restart()
		{TruncatedFinal(NULL, 0);}

	//! size of the hash returned by Final()
	virtual unsigned int DigestSize() const =0;

	//! block size of underlying compression function, or 0 if not block based
	virtual unsigned int BlockSize() const {return 0;}

	//! input to Update() should have length a multiple of this for optimal speed
	virtual unsigned int OptimalBlockSize() const {return 1;}

	//! returns how input should be aligned for optimal performance
	virtual unsigned int OptimalDataAlignment() const {return 1;}

	//! use this if your input is in one piece and you don't want to call Update() and Final() separately
	virtual void CalculateDigest(byte *digest, const byte *input, unsigned int length)
		{Update(input, length); Final(digest);}

	//! verify that digest is a valid digest for the current message, then reinitialize the object
	/*! Default implementation is to call Final() and do a bitwise comparison
		between its output and digest. */
	virtual bool Verify(const byte *digest)
		{return TruncatedVerify(digest, DigestSize());}

	//! use this if your input is in one piece and you don't want to call Update() and Verify() separately
	virtual bool VerifyDigest(const byte *digest, const byte *input, unsigned int length)
		{Update(input, length); return Verify(digest);}

	//! truncated version of Final()
	virtual void TruncatedFinal(byte *digest, unsigned int digestSize) =0;

	//! truncated version of CalculateDigest()
	virtual void CalculateTruncatedDigest(byte *digest, unsigned int digestSize, const byte *input, unsigned int length)
		{Update(input, length); TruncatedFinal(digest, digestSize);}

	//! truncated version of Verify()
	virtual bool TruncatedVerify(const byte *digest, unsigned int digestLength);

	//! truncated version of VerifyDigest()
	virtual bool VerifyTruncatedDigest(const byte *digest, unsigned int digestLength, const byte *input, unsigned int length)
		{Update(input, length); return TruncatedVerify(digest, digestLength);}

protected:
	void ThrowIfInvalidTruncatedSize(unsigned int size) const;
};

typedef HashTransformation HashFunction;

template <class T>
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE SimpleKeyedTransformation : public T, public SimpleKeyingInterface
{
public:
	void ThrowIfInvalidKeyLength(unsigned int length)
		{SimpleKeyingInterface::ThrowIfInvalidKeyLength(*this, length);}
};

typedef SimpleKeyedTransformation<BlockTransformation> BlockCipher;
typedef SimpleKeyedTransformation<StreamTransformation> SymmetricCipher;
typedef SimpleKeyedTransformation<HashTransformation> MessageAuthenticationCode;

//! interface for random number generators
/*! All return values are uniformly distributed over the range specified.
*/
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE RandomNumberGenerator : public Algorithm
{
public:
	//! generate new random byte and return it
	virtual byte GenerateByte() =0;

	//! generate new random bit and return it
	/*! Default implementation is to call GenerateByte() and return its parity. */
	virtual unsigned int GenerateBit();

	//! generate a random 32 bit word in the range min to max, inclusive
	virtual word32 GenerateWord32(word32 a=0, word32 b=0xffffffffL);

	//! generate random array of bytes
	/*! Default implementation is to call GenerateByte() size times. */
	virtual void GenerateBlock(byte *output, unsigned int size);

	//! generate and discard n bytes
	/*! Default implementation is to call GenerateByte() n times. */
	virtual void DiscardBytes(unsigned int n);

	//! randomly shuffle the specified array, resulting permutation is uniformly distributed
	template <class IT> void Shuffle(IT begin, IT end)
	{
		for (; begin != end; ++begin)
			std::iter_swap(begin, begin + GenerateWord32(0, end-begin-1));
	}

};

//! returns a reference that can be passed to functions that ask for a RNG but doesn't actually use it
CRYPTOPP_DLL RandomNumberGenerator & NullRNG();

class WaitObjectContainer;

//! interface for objects that you can wait for

class CRYPTOPP_NO_VTABLE Waitable
{
public:
	virtual ~Waitable() {};
	
	//! maximum number of wait objects that this object can return
	virtual unsigned int GetMaxWaitObjectCount() const =0;
	//! put wait objects into container
	virtual void GetWaitObjects(WaitObjectContainer &container) =0;
	//! wait on this object
	/*! same as creating an empty container, calling GetWaitObjects(), and calling Wait() on the container */
	bool Wait(unsigned long milliseconds);
};

//! interface for buffered transformations

/*! BufferedTransformation is a generalization of BlockTransformation,
	StreamTransformation, and HashTransformation.

	A buffered transformation is an object that takes a stream of bytes
	as input (this may be done in stages), does some computation on them, and
	then places the result into an internal buffer for later retrieval.  Any
	partial result already in the output buffer is not modified by further
	input.

	If a method takes a "blocking" parameter, and you
	pass "false" for it, the method will return before all input has been processed if
	the input cannot be processed without waiting (for network buffers to become available, for example).
	In this case the method will return true
	or a non-zero integer value. When this happens you must continue to call the method with the same
	parameters until it returns false or zero, before calling any other method on it or
	attached BufferedTransformation. The integer return value in this case is approximately
	the number of bytes left to be processed, and can be used to implement a progress bar.

	For functions that take a "propagation" parameter, propagation != 0 means pass on the signal to attached
	BufferedTransformation objects, with propagation decremented at each step until it reaches 0.
	-1 means unlimited propagation.

	\nosubgrouping
*/
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE BufferedTransformation : public Algorithm, public Waitable
{
public:
	// placed up here for CW8
	static const std::string NULL_CHANNEL;	// the empty string ""

	BufferedTransformation() : Algorithm(false) {}

	//! return a reference to this object
	/*! This function is useful for passing a temporary BufferedTransformation object to a 
		function that takes a non-const reference. */
	BufferedTransformation& Ref() {return *this;}

	//!	\name INPUT
	//@{
		//! input a byte for processing
		unsigned int Put(byte inByte, bool blocking=true)
			{return Put(&inByte, 1, blocking);}
		//! input multiple bytes
		unsigned int Put(const byte *inString, unsigned int length, bool blocking=true)
			{return Put2(inString, length, 0, blocking);}

		//! input a 16-bit word
		unsigned int PutWord16(word16 value, ByteOrder order=BIG_ENDIAN_ORDER, bool blocking=true);
		//! input a 32-bit word
		unsigned int PutWord32(word32 value, ByteOrder order=BIG_ENDIAN_ORDER, bool blocking=true);

		//! request space which can be written into by the caller, and then used as input to Put()
		/*! \param size is requested size (as a hint) for input, and size of the returned space for output */
		/*! \note The purpose of this method is to help avoid doing extra memory allocations. */
		virtual byte * CreatePutSpace(unsigned int &size) {size=0; return NULL;}

		virtual bool CanModifyInput() const {return false;}

		//! input multiple bytes that may be modified by callee
		unsigned int PutModifiable(byte *inString, unsigned int length, bool blocking=true)
			{return PutModifiable2(inString, length, 0, blocking);}

		bool MessageEnd(int propagation=-1, bool blocking=true)
			{return !!Put2(NULL, 0, propagation < 0 ? -1 : propagation+1, blocking);}
		unsigned int PutMessageEnd(const byte *inString, unsigned int length, int propagation=-1, bool blocking=true)
			{return Put2(inString, length, propagation < 0 ? -1 : propagation+1, blocking);}

		//! input multiple bytes for blocking or non-blocking processing
		/*! \param messageEnd means how many filters to signal MessageEnd to, including this one */
		virtual unsigned int Put2(const byte *inString, unsigned int length, int messageEnd, bool blocking) =0;
		//! input multiple bytes that may be modified by callee for blocking or non-blocking processing
		/*! \param messageEnd means how many filters to signal MessageEnd to, including this one */
		virtual unsigned int PutModifiable2(byte *inString, unsigned int length, int messageEnd, bool blocking)
			{return Put2(inString, length, messageEnd, blocking);}

		//! thrown by objects that have not implemented nonblocking input processing
		struct BlockingInputOnly : public NotImplemented
			{BlockingInputOnly(const std::string &s) : NotImplemented(s + ": Nonblocking input is not implemented by this object.") {}};
	//@}

	//!	\name WAITING
	//@{
		unsigned int GetMaxWaitObjectCount() const;
		void GetWaitObjects(WaitObjectContainer &container);
	//@}

	//!	\name SIGNALS
	//@{
		virtual void IsolatedInitialize(const NameValuePairs& /* parameters */) {throw NotImplemented("BufferedTransformation: this object can't be reinitialized");}
		virtual bool IsolatedFlush(bool hardFlush, bool blocking) =0;
		virtual bool IsolatedMessageSeriesEnd(bool /* blocking */) {return false;}

		//! initialize or reinitialize this object
		virtual void Initialize(const NameValuePairs &parameters=g_nullNameValuePairs, int propagation=-1);
		//! flush buffered input and/or output
		/*! \param hardFlush is used to indicate whether all data should be flushed
			\note Hard flushes must be used with care. It means try to process and output everything, even if
			there may not be enough data to complete the action. For example, hard flushing a HexDecoder would
			cause an error if you do it after inputing an odd number of hex encoded characters.
			For some types of filters, for example ZlibDecompressor, hard flushes can only
			be done at "synchronization points". These synchronization points are positions in the data
			stream that are created by hard flushes on the corresponding reverse filters, in this
			example ZlibCompressor. This is useful when zlib compressed data is moved across a
			network in packets and compression state is preserved across packets, as in the ssh2 protocol.
		*/
		virtual bool Flush(bool hardFlush, int propagation=-1, bool blocking=true);
		//! mark end of a series of messages
		/*! There should be a MessageEnd immediately before MessageSeriesEnd. */
		virtual bool MessageSeriesEnd(int propagation=-1, bool blocking=true);

		//! set propagation of automatically generated and transferred signals
		/*! propagation == 0 means do not automaticly generate signals */
		virtual void SetAutoSignalPropagation(int /* propagation */) {}

		//!
		virtual int GetAutoSignalPropagation() const {return 0;}
public:

	//@}

	//!	\name RETRIEVAL OF ONE MESSAGE
	//@{
		//! returns number of bytes that is currently ready for retrieval
		/*! All retrieval functions return the actual number of bytes
			retrieved, which is the lesser of the request number and
			MaxRetrievable(). */
		virtual unsigned long MaxRetrievable() const;

		//! returns whether any bytes are currently ready for retrieval
		virtual bool AnyRetrievable() const;

		//! try to retrieve a single byte
		virtual unsigned int Get(byte &outByte);
		//! try to retrieve multiple bytes
		virtual unsigned int Get(byte *outString, unsigned int getMax);

		//! peek at the next byte without removing it from the output buffer
		virtual unsigned int Peek(byte &outByte) const;
		//! peek at multiple bytes without removing them from the output buffer
		virtual unsigned int Peek(byte *outString, unsigned int peekMax) const;

		//! try to retrieve a 16-bit word
		unsigned int GetWord16(word16 &value, ByteOrder order=BIG_ENDIAN_ORDER);
		//! try to retrieve a 32-bit word
		unsigned int GetWord32(word32 &value, ByteOrder order=BIG_ENDIAN_ORDER);

		//! try to peek at a 16-bit word
		unsigned int PeekWord16(word16 &value, ByteOrder order=BIG_ENDIAN_ORDER);
		//! try to peek at a 32-bit word
		unsigned int PeekWord32(word32 &value, ByteOrder order=BIG_ENDIAN_ORDER);

		//! move transferMax bytes of the buffered output to target as input
		unsigned long TransferTo(BufferedTransformation &target, unsigned long transferMax=ULONG_MAX, const std::string &channel=NULL_CHANNEL)
			{TransferTo2(target, transferMax, channel); return transferMax;}

		//! discard skipMax bytes from the output buffer
		virtual unsigned long Skip(unsigned long skipMax=ULONG_MAX);

		//! copy copyMax bytes of the buffered output to target as input
		unsigned long CopyTo(BufferedTransformation &target, unsigned long copyMax=ULONG_MAX, const std::string &channel=NULL_CHANNEL) const
			{return CopyRangeTo(target, 0, copyMax, channel);}

		//! copy copyMax bytes of the buffered output, starting at position (relative to current position), to target as input
		unsigned long CopyRangeTo(BufferedTransformation &target, unsigned long position, unsigned long copyMax=ULONG_MAX, const std::string &channel=NULL_CHANNEL) const
			{unsigned long i = position; CopyRangeTo2(target, i, i+copyMax, channel); return i-position;}

	//@}

	//!	\name RETRIEVAL OF MULTIPLE MESSAGES
	//@{
		//!
		virtual unsigned long TotalBytesRetrievable() const;
		//! number of times MessageEnd() has been received minus messages retrieved or skipped
		virtual unsigned int NumberOfMessages() const;
		//! returns true if NumberOfMessages() > 0
		virtual bool AnyMessages() const;
		//! start retrieving the next message
		/*!
			Returns false if no more messages exist or this message 
			is not completely retrieved.
		*/
		virtual bool GetNextMessage();
		//! skip count number of messages
		virtual unsigned int SkipMessages(unsigned int count=UINT_MAX);
		//!
		unsigned int TransferMessagesTo(BufferedTransformation &target, unsigned int count=UINT_MAX, const std::string &channel=NULL_CHANNEL)
			{TransferMessagesTo2(target, count, channel); return count;}
		//!
		unsigned int CopyMessagesTo(BufferedTransformation &target, unsigned int count=UINT_MAX, const std::string &channel=NULL_CHANNEL) const;

		//!
		virtual void SkipAll();
		//!
		void TransferAllTo(BufferedTransformation &target, const std::string &channel=NULL_CHANNEL)
			{TransferAllTo2(target, channel);}
		//!
		void CopyAllTo(BufferedTransformation &target, const std::string &channel=NULL_CHANNEL) const;

		virtual bool GetNextMessageSeries() {return false;}
		virtual unsigned int NumberOfMessagesInThisSeries() const {return NumberOfMessages();}
		virtual unsigned int NumberOfMessageSeries() const {return 0;}
	//@}

	//!	\name NON-BLOCKING TRANSFER OF OUTPUT
	//@{
		virtual unsigned int TransferTo2(BufferedTransformation &target, unsigned long &byteCount, const std::string &channel=NULL_CHANNEL, bool blocking=true) =0;
		virtual unsigned int CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end=ULONG_MAX, const std::string &channel=NULL_CHANNEL, bool blocking=true) const =0;
		unsigned int TransferMessagesTo2(BufferedTransformation &target, unsigned int &messageCount, const std::string &channel=NULL_CHANNEL, bool blocking=true);
		unsigned int TransferAllTo2(BufferedTransformation &target, const std::string &channel=NULL_CHANNEL, bool blocking=true);
	//@}

	//!	\name CHANNELS
	//@{
		struct NoChannelSupport : public NotImplemented
			{NoChannelSupport() : NotImplemented("BufferedTransformation: this object doesn't support multiple channels") {}};

		unsigned int ChannelPut(const std::string &channel, byte inByte, bool blocking=true)
			{return ChannelPut(channel, &inByte, 1, blocking);}
		unsigned int ChannelPut(const std::string &channel, const byte *inString, unsigned int length, bool blocking=true)
			{return ChannelPut2(channel, inString, length, 0, blocking);}

		unsigned int ChannelPutModifiable(const std::string &channel, byte *inString, unsigned int length, bool blocking=true)
			{return ChannelPutModifiable2(channel, inString, length, 0, blocking);}

		unsigned int ChannelPutWord16(const std::string &channel, word16 value, ByteOrder order=BIG_ENDIAN_ORDER, bool blocking=true);
		unsigned int ChannelPutWord32(const std::string &channel, word32 value, ByteOrder order=BIG_ENDIAN_ORDER, bool blocking=true);

		bool ChannelMessageEnd(const std::string &channel, int propagation=-1, bool blocking=true)
			{return !!ChannelPut2(channel, NULL, 0, propagation < 0 ? -1 : propagation+1, blocking);}
		unsigned int ChannelPutMessageEnd(const std::string &channel, const byte *inString, unsigned int length, int propagation=-1, bool blocking=true)
			{return ChannelPut2(channel, inString, length, propagation < 0 ? -1 : propagation+1, blocking);}

		virtual byte * ChannelCreatePutSpace(const std::string &channel, unsigned int &size);

		virtual unsigned int ChannelPut2(const std::string &channel, const byte *begin, unsigned int length, int messageEnd, bool blocking);
		virtual unsigned int ChannelPutModifiable2(const std::string &channel, byte *begin, unsigned int length, int messageEnd, bool blocking);

		virtual bool ChannelFlush(const std::string &channel, bool hardFlush, int propagation=-1, bool blocking=true);
		virtual bool ChannelMessageSeriesEnd(const std::string &channel, int propagation=-1, bool blocking=true);

		virtual void SetRetrievalChannel(const std::string &channel);
	//@}

	//!	\name ATTACHMENT
	/*! Some BufferedTransformation objects (e.g. Filter objects)
		allow other BufferedTransformation objects to be attached. When
		this is done, the first object instead of buffering its output,
		sents that output to the attached object as input. The entire
		attachment chain is deleted when the anchor object is destructed.
	*/
	//@{
		//! returns whether this object allows attachment
		virtual bool Attachable() {return false;}
		//! returns the object immediately attached to this object or NULL for no attachment
		virtual BufferedTransformation *AttachedTransformation() {assert(!Attachable()); return 0;}
		//!
		virtual const BufferedTransformation *AttachedTransformation() const
			{return const_cast<BufferedTransformation *>(this)->AttachedTransformation();}
		//! delete the current attachment chain and replace it with newAttachment
		virtual void Detach(BufferedTransformation* /* newAttachment */ = 0)
			{assert(!Attachable()); throw NotImplemented("BufferedTransformation: this object is not attachable");}
		//! add newAttachment to the end of attachment chain
		virtual void Attach(BufferedTransformation *newAttachment);
	//@}

protected:
	static int DecrementPropagation(int propagation)
		{return propagation != 0 ? propagation - 1 : 0;}
};

//! returns a reference to a BufferedTransformation object that discards all input
BufferedTransformation & TheBitBucket();

//! interface for crypto material, such as public and private keys, and crypto parameters

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CryptoMaterial : public NameValuePairs
{
public:
	//! exception thrown when invalid crypto material is detected
	class CRYPTOPP_DLL InvalidMaterial : public InvalidDataFormat
	{
	public:
		explicit InvalidMaterial(const std::string &s) : InvalidDataFormat(s) {}
	};

	//! assign values from source to this object
	/*! \note This function can be used to create a public key from a private key. */
	virtual void AssignFrom(const NameValuePairs &source) =0;

	//! check this object for errors
	/*! \param level denotes the level of thoroughness:
		0 - using this object won't cause a crash or exception (rng is ignored)
		1 - this object will probably function (encrypt, sign, etc.) correctly (but may not check for weak keys and such)
		2 - make sure this object will function correctly, and do reasonable security checks
		3 - do checks that may take a long time
		\return true if the tests pass */
	virtual bool Validate(RandomNumberGenerator &rng, unsigned int level) const =0;

	//! throws InvalidMaterial if this object fails Validate() test
	virtual void ThrowIfInvalid(RandomNumberGenerator &rng, unsigned int level) const
		{if (!Validate(rng, level)) throw InvalidMaterial("CryptoMaterial: this object contains invalid values");}

//	virtual std::vector<std::string> GetSupportedFormats(bool includeSaveOnly=false, bool includeLoadOnly=false);

	//! save key into a BufferedTransformation
	virtual void Save(BufferedTransformation& /* bt */) const
		{throw NotImplemented("CryptoMaterial: this object does not support saving");}

	//! load key from a BufferedTransformation
	/*! \throws KeyingErr if decode fails
		\note Generally does not check that the key is valid.
			Call ValidateKey() or ThrowIfInvalidKey() to check that. */
	virtual void Load(BufferedTransformation& /* bt */)
		{throw NotImplemented("CryptoMaterial: this object does not support loading");}

	//! \return whether this object supports precomputation
	virtual bool SupportsPrecomputation() const {return false;}
	//! do precomputation
	/*! The exact semantics of Precompute() is varies, but
		typically it means calculate a table of n objects
		that can be used later to speed up computation. */
	virtual void Precompute(unsigned int /* n */)
		{assert(!SupportsPrecomputation()); throw NotImplemented("CryptoMaterial: this object does not support precomputation");}
	//! retrieve previously saved precomputation
	virtual void LoadPrecomputation(BufferedTransformation& /* storedPrecomputation */)
		{assert(!SupportsPrecomputation()); throw NotImplemented("CryptoMaterial: this object does not support precomputation");}
	//! save precomputation for later use
	virtual void SavePrecomputation(BufferedTransformation& /* storedPrecomputation */) const
		{assert(!SupportsPrecomputation()); throw NotImplemented("CryptoMaterial: this object does not support precomputation");}

	// for internal library use
	void DoQuickSanityCheck() const	{ThrowIfInvalid(NullRNG(), 0);}
};

//! interface for generatable crypto material, such as private keys and crypto parameters

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE GeneratableCryptoMaterial : virtual public CryptoMaterial
{
public:
	//! generate a random key or crypto parameters
	/*! \throws KeyingErr if algorithm parameters are invalid, or if a key can't be generated
		(e.g., if this is a public key object) */
	virtual void GenerateRandom(RandomNumberGenerator& /* rng */, const NameValuePairs& /* params */ = g_nullNameValuePairs)
		{throw NotImplemented("GeneratableCryptoMaterial: this object does not support key/parameter generation");}

	//! calls the above function with a NameValuePairs object that just specifies "KeySize"
	void GenerateRandomWithKeySize(RandomNumberGenerator &rng, unsigned int keySize);
};

//! interface for public keys

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PublicKey : virtual public CryptoMaterial
{
};

//! interface for private keys

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PrivateKey : public GeneratableCryptoMaterial
{
};

//! interface for asymmetric algorithms

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE AsymmetricAlgorithm : public Algorithm
{
public:
	//! returns a reference to the crypto material used by this object
	virtual CryptoMaterial & AccessMaterial() =0;
	//! returns a const reference to the crypto material used by this object
	virtual const CryptoMaterial & GetMaterial() const =0;

	//! for backwards compatibility, calls AccessMaterial().Load(bt)
	void BERDecode(BufferedTransformation &bt)
		{AccessMaterial().Load(bt);}
	//! for backwards compatibility, calls GetMaterial().Save(bt)
	void DEREncode(BufferedTransformation &bt) const
		{GetMaterial().Save(bt);}
};

//! interface for asymmetric algorithms using public keys

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PublicKeyAlgorithm : public AsymmetricAlgorithm
{
public:
	// VC60 workaround: no co-variant return type
	CryptoMaterial & AccessMaterial() {return AccessPublicKey();}
	const CryptoMaterial & GetMaterial() const {return GetPublicKey();}

	virtual PublicKey & AccessPublicKey() =0;
	virtual const PublicKey & GetPublicKey() const {return const_cast<PublicKeyAlgorithm *>(this)->AccessPublicKey();}
};

//! interface for asymmetric algorithms using private keys

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PrivateKeyAlgorithm : public AsymmetricAlgorithm
{
public:
	CryptoMaterial & AccessMaterial() {return AccessPrivateKey();}
	const CryptoMaterial & GetMaterial() const {return GetPrivateKey();}

	virtual PrivateKey & AccessPrivateKey() =0;
	virtual const PrivateKey & GetPrivateKey() const {return const_cast<PrivateKeyAlgorithm *>(this)->AccessPrivateKey();}
};

/*! This class provides an interface common to encryptors and decryptors
	for querying their plaintext and ciphertext lengths.
*/
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PK_CryptoSystem
{
public:
	virtual ~PK_CryptoSystem() {}

	//! maximum length of plaintext for a given ciphertext length
	/*! \note This function returns 0 if ciphertextLength is not valid (too long or too short). */
	virtual unsigned int MaxPlaintextLength(unsigned int ciphertextLength) const =0;

	//! calculate length of ciphertext given length of plaintext
	/*! \note This function returns 0 if plaintextLength is not valid (too long). */
	virtual unsigned int CiphertextLength(unsigned int plaintextLength) const =0;

	//! this object supports the use of the parameter with the given name
	/*! some possible parameter names: EncodingParameters, KeyDerivationParameters */
	virtual bool ParameterSupported(const char *name) const =0;

	//! return fixed ciphertext length, if one exists, otherwise return 0
	/*! \note "Fixed" here means length of ciphertext does not depend on length of plaintext.
		It usually does depend on the key length. */
	virtual unsigned int FixedCiphertextLength() const {return 0;}

	//! return maximum plaintext length given the fixed ciphertext length, if one exists, otherwise return 0
	virtual unsigned int FixedMaxPlaintextLength() const {return 0;}

};

//! interface for public-key encryptors
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PK_Encryptor : virtual public PK_CryptoSystem, public PublicKeyAlgorithm
{
public:
	//! exception thrown when trying to encrypt plaintext of invalid length
	class CRYPTOPP_DLL InvalidPlaintextLength : public Exception
	{
	public:
		InvalidPlaintextLength() : Exception(OTHER_ERROR, "PK_Encryptor: invalid plaintext length") {}
	};

	//! encrypt a byte string
	/*! \pre CiphertextLength(plaintextLength) != 0 (i.e., plaintext isn't too long)
		\pre size of ciphertext == CiphertextLength(plaintextLength)
	*/
	virtual void Encrypt(RandomNumberGenerator &rng, 
		const byte *plaintext, unsigned int plaintextLength, 
		byte *ciphertext, const NameValuePairs &parameters = g_nullNameValuePairs) const =0;

	//! create a new encryption filter
	/*! \note The caller is responsible for deleting the returned pointer.
		\note Encoding parameters should be passed in the "EP" channel.
	*/
	virtual BufferedTransformation * CreateEncryptionFilter(RandomNumberGenerator &rng, 
		BufferedTransformation *attachment=NULL, const NameValuePairs &parameters = g_nullNameValuePairs) const;
};

//! interface for public-key signers and verifiers

/*! This class provides an interface common to signers and verifiers
	for querying scheme properties.
*/
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PK_SignatureScheme
{
public:
	//! invalid key exception, may be thrown by any function in this class if the private or public key has a length that can't be used
	class CRYPTOPP_DLL InvalidKeyLength : public Exception
	{
	public:
		InvalidKeyLength(const std::string &message) : Exception(OTHER_ERROR, message) {}
	};

	//! key too short exception, may be thrown by any function in this class if the private or public key is too short to sign or verify anything
	class CRYPTOPP_DLL KeyTooShort : public InvalidKeyLength
	{
	public:
		KeyTooShort() : InvalidKeyLength("PK_Signer: key too short for this signature scheme") {}
	};

	virtual ~PK_SignatureScheme() {}

	//! signature length if it only depends on the key, otherwise 0
	virtual unsigned int SignatureLength() const =0;

	//! maximum signature length produced for a given length of recoverable message part
	virtual unsigned int MaxSignatureLength(unsigned int /* recoverablePartLength */ = 0) const {return SignatureLength();}

	//! length of longest message that can be recovered, or 0 if this signature scheme does not support message recovery
	virtual unsigned int MaxRecoverableLength() const =0;

	//! length of longest message that can be recovered from a signature of given length, or 0 if this signature scheme does not support message recovery
	virtual unsigned int MaxRecoverableLengthFromSignatureLength(unsigned int signatureLength) const =0;

	//! requires a random number generator to sign
	/*! if this returns false, NullRNG() can be passed to functions that take RandomNumberGenerator & */
	virtual bool IsProbabilistic() const =0;

	//! whether or not a non-recoverable message part can be signed
	virtual bool AllowNonrecoverablePart() const =0;

	//! if this function returns true, during verification you must input the signature before the message, otherwise you can input it at anytime */
	virtual bool SignatureUpfront() const {return false;}

	//! whether you must input the recoverable part before the non-recoverable part during signing
	virtual bool RecoverablePartFirst() const =0;
};

//! interface for accumulating messages to be signed or verified
/*! Only Update() should be called
	on this class. No other functions inherited from HashTransformation should be called.
*/
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PK_MessageAccumulator : public HashTransformation
{
public:
	//! should not be called on PK_MessageAccumulator
	unsigned int DigestSize() const
		{throw NotImplemented("PK_MessageAccumulator: DigestSize() should not be called");}
	//! should not be called on PK_MessageAccumulator
	void TruncatedFinal(byte* /* digest */, unsigned int /* digestSize */) 
		{throw NotImplemented("PK_MessageAccumulator: TruncatedFinal() should not be called");}
};

//! interface for public-key signers

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PK_Signer : public PK_SignatureScheme, public PrivateKeyAlgorithm
{
public:
	//! create a new HashTransformation to accumulate the message to be signed
	virtual PK_MessageAccumulator * NewSignatureAccumulator(RandomNumberGenerator &rng) const =0;

	virtual void InputRecoverableMessage(PK_MessageAccumulator &messageAccumulator, const byte *recoverableMessage, unsigned int recoverableMessageLength) const =0;

	//! sign and delete messageAccumulator (even in case of exception thrown)
	/*! \pre size of signature == MaxSignatureLength()
		\return actual signature length
	*/
	virtual unsigned int Sign(RandomNumberGenerator &rng, PK_MessageAccumulator *messageAccumulator, byte *signature) const;

	//! sign and restart messageAccumulator
	/*! \pre size of signature == MaxSignatureLength()
		\return actual signature length
	*/
	virtual unsigned int SignAndRestart(RandomNumberGenerator &rng, PK_MessageAccumulator &messageAccumulator, byte *signature, bool restart=true) const =0;

	//! sign a message
	/*! \pre size of signature == MaxSignatureLength()
		\return actual signature length
	*/
	virtual unsigned int SignMessage(RandomNumberGenerator &rng, const byte *message, unsigned int messageLen, byte *signature) const;

	//! sign a recoverable message
	/*! \pre size of signature == MaxSignatureLength(recoverableMessageLength)
		\return actual signature length
	*/
	virtual unsigned int SignMessageWithRecovery(RandomNumberGenerator &rng, const byte *recoverableMessage, unsigned int recoverableMessageLength, 
		const byte *nonrecoverableMessage, unsigned int nonrecoverableMessageLength, byte *signature) const;
};

//! interface for public-key signature verifiers
/*! The Recover* functions throw NotImplemented if the signature scheme does not support
	message recovery.
	The Verify* functions throw InvalidDataFormat if the scheme does support message
	recovery and the signature contains a non-empty recoverable message part. The
	Recovery* functions should be used in that case.
*/
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PK_Verifier : public PK_SignatureScheme, public PublicKeyAlgorithm
{
public:
	//! create a new HashTransformation to accumulate the message to be verified
	virtual PK_MessageAccumulator * NewVerificationAccumulator() const =0;

	//! input signature into a message accumulator
	virtual void InputSignature(PK_MessageAccumulator &messageAccumulator, const byte *signature, unsigned int signatureLength) const =0;

	//! check whether messageAccumulator contains a valid signature and message, and delete messageAccumulator (even in case of exception thrown)
	virtual bool Verify(PK_MessageAccumulator *messageAccumulator) const;

	//! check whether messageAccumulator contains a valid signature and message, and restart messageAccumulator
	virtual bool VerifyAndRestart(PK_MessageAccumulator &messageAccumulator) const =0;

	//! check whether input signature is a valid signature for input message
	virtual bool VerifyMessage(const byte *message, unsigned int messageLen, 
		const byte *signature, unsigned int signatureLength) const;

	//! recover a message from its signature
	/*! \pre size of recoveredMessage == MaxRecoverableLengthFromSignatureLength(signatureLength)
	*/
	virtual DecodingResult Recover(byte *recoveredMessage, PK_MessageAccumulator *messageAccumulator) const;

	//! recover a message from its signature
	/*! \pre size of recoveredMessage == MaxRecoverableLengthFromSignatureLength(signatureLength)
	*/
	virtual DecodingResult RecoverAndRestart(byte *recoveredMessage, PK_MessageAccumulator &messageAccumulator) const =0;

	//! recover a message from its signature
	/*! \pre size of recoveredMessage == MaxRecoverableLengthFromSignatureLength(signatureLength)
	*/
	virtual DecodingResult RecoverMessage(byte *recoveredMessage, 
		const byte *nonrecoverableMessage, unsigned int nonrecoverableMessageLength, 
		const byte *signature, unsigned int signatureLength) const;
};

//! interface for domains of authenticated key agreement protocols

//! BER Decode Exception Class, may be thrown during an ASN1 BER decode operation
class CRYPTOPP_DLL BERDecodeErr : public InvalidArgument
{
public: 
	BERDecodeErr() : InvalidArgument("BER decode error") {}
	BERDecodeErr(const std::string &s) : InvalidArgument(s) {}
};

//! interface for encoding and decoding ASN1 objects
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE ASN1Object
{
public:
	virtual ~ASN1Object() {}
	//! decode this object from a BufferedTransformation, using BER (Basic Encoding Rules)
	virtual void BERDecode(BufferedTransformation &bt) =0;
	//! encode this object into a BufferedTransformation, using DER (Distinguished Encoding Rules)
	virtual void DEREncode(BufferedTransformation &bt) const =0;
	//! encode this object into a BufferedTransformation, using BER
	/*! this may be useful if DEREncode() would be too inefficient */
	virtual void BEREncode(BufferedTransformation &bt) const {DEREncode(bt);}
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_SMARTPTR_H
#define CRYPTOPP_SMARTPTR_H

//- #include "config.h"
#include <algorithm>

NAMESPACE_BEGIN(CryptoPP)

template <class T> class simple_ptr
{
public:
	simple_ptr() : m_p(NULL) {}
	~simple_ptr() {delete m_p;}
	T *m_p;
};

template <class T> class member_ptr
{
public:
	explicit member_ptr(T *p = NULL) : m_p(p) {}

	~member_ptr();

	const T& operator*() const { return *m_p; }
	T& operator*() { return *m_p; }

	const T* operator->() const { return m_p; }
	T* operator->() { return m_p; }

	const T* get() const { return m_p; }
	T* get() { return m_p; }

	T* release()
	{
		T *old_p = m_p;
		m_p = 0;
		return old_p;
	} 

	void reset(T *p = 0);

protected:
	member_ptr(const member_ptr<T>& rhs);		// copy not allowed
	void operator=(const member_ptr<T>& rhs);	// assignment not allowed

	T *m_p;
};

template <class T> member_ptr<T>::~member_ptr() {delete m_p;}
template <class T> void member_ptr<T>::reset(T *p) {delete m_p; m_p = p;}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_MISC_H
#define CRYPTOPP_MISC_H

//- #include "cryptlib.h"
//- #include "smartptr.h"

#ifdef INTEL_INTRINSICS
#include <stdlib.h>
#endif

NAMESPACE_BEGIN(CryptoPP)

// ************** compile-time assertion ***************

template<int>
struct CompileTimeAssert;

template<>
struct CompileTimeAssert<true> {};

#if defined(_WIN32) || defined(__CYGWIN__)
#define CRYPTOPP_COMPILE_ASSERT( x )
#define CRYPTOPP_COMPILE_ASSERT_GLOBAL(assertion)
#define CRYPTOPP_COMPILE_ASSERT_INSTANCE(assertion, instance)
#define CRYPTOPP_ASSERT_JOIN(X, Y)
#define CRYPTOPP_DO_ASSERT_JOIN(X, Y)

#else
#define CRYPTOPP_COMPILE_ASSERT( x ) \
    { CompileTimeAssert<((x) != 0)> ERROR; (void)ERROR; } 

#define CRYPTOPP_COMPILE_ASSERT_GLOBAL(assertion) CRYPTOPP_COMPILE_ASSERT_INSTANCE(assertion, __LINE__)
#if defined(CRYPTOPP_EXPORTS) || defined(CRYPTOPP_IMPORTS)
#define CRYPTOPP_COMPILE_ASSERT_INSTANCE(assertion, instance)
#else
#define CRYPTOPP_COMPILE_ASSERT_INSTANCE(assertion, instance) \
	CompileTimeAssert<(assertion)> CRYPTOPP_ASSERT_JOIN(cryptopp_assert_, instance)
#endif
#define CRYPTOPP_ASSERT_JOIN(X, Y) CRYPTOPP_DO_ASSERT_JOIN(X, Y)
#define CRYPTOPP_DO_ASSERT_JOIN(X, Y) X##Y
#endif

// ************** misc classes ***************

class CRYPTOPP_DLL Empty
{
};

//! _
template <class BASE1, class BASE2>
class CRYPTOPP_NO_VTABLE TwoBases : public BASE1, public BASE2
{
};

template <class T>
class ObjectHolder
{
protected:
	T m_object;
};

class NotCopyable
{
public:
	NotCopyable() {}
private:
    NotCopyable(const NotCopyable &);
    void operator=(const NotCopyable &);
};

template <class T>
struct NewObject
{
	T* operator()() const {return new T;}
};

/*! This function safely initializes a static object in a multithreaded environment without using locks.
	It may leak memory when two threads try to initialize the static object at the same time
	but this should be acceptable since each static object is only initialized once per session.
*/
template <class T, class F = NewObject<T>, int instance=0>
class Singleton
{
public:
	Singleton(F objectFactory = F()) : m_objectFactory(objectFactory) {}

	// VC60 workaround: use "..." to prevent this function from being inlined
	const T & Ref(...) const;

private:
	F m_objectFactory;
};

template <class T, class F, int instance>
const T & Singleton<T, F, instance>::Ref(...) const
{
	static simple_ptr<T> s_pObject;
	static char s_objectState = 0;

retry:
	switch (s_objectState)
	{
	case 0:
		s_objectState = 1;
		try
		{
			s_pObject.m_p = m_objectFactory();
		}
		catch(...)
		{
			s_objectState = 0;
			throw;
		}
		s_objectState = 2;
		break;
	case 1:
		goto retry;
	default:
		break;
	}
	return *s_pObject.m_p;
}

// ************** misc functions ***************

// can't use std::min or std::max in MSVC60 or Cygwin 1.1.0
template <class T> inline const T& STDMIN(const T& a, const T& b)
{
	return b < a ? b : a;
}

template <class T> inline const T& STDMAX(const T& a, const T& b)
{
	return a < b ? b : a;
}

#define RETURN_IF_NONZERO(x) unsigned int returnedValue = x; if (returnedValue) return returnedValue

// this version of the macro is fastest on Pentium 3 and Pentium 4 with MSVC 6 SP5 w/ Processor Pack
#define GETBYTE(x, y) (unsigned int)byte((x)>>(8*(y)))
// these may be faster on other CPUs/compilers
// #define GETBYTE(x, y) (unsigned int)(((x)>>(8*(y)))&255)
// #define GETBYTE(x, y) (((byte *)&(x))[y])

CRYPTOPP_DLL unsigned int Parity(unsigned long);
CRYPTOPP_DLL unsigned int BytePrecision(unsigned long);
CRYPTOPP_DLL unsigned int BitPrecision(unsigned long);
CRYPTOPP_DLL unsigned long Crop(unsigned long, unsigned int size);

inline unsigned int BitsToBytes(unsigned int bitCount)
{
	return ((bitCount+7)/(8));
}

inline unsigned int BytesToWords(unsigned int byteCount)
{
	return ((byteCount+WORD_SIZE-1)/WORD_SIZE);
}

inline unsigned int BitsToWords(unsigned int bitCount)
{
	return ((bitCount+WORD_BITS-1)/(WORD_BITS));
}

inline unsigned int BitsToDwords(unsigned int bitCount)
{
	return ((bitCount+2*WORD_BITS-1)/(2*WORD_BITS));
}

CRYPTOPP_DLL void xorbuf(byte *buf, const byte *mask, unsigned int count);
CRYPTOPP_DLL void xorbuf(byte *output, const byte *input, const byte *mask, unsigned int count);

template <class T>
inline bool IsPowerOf2(T n)
{
	return n > 0 && (n & (n-1)) == 0;
}

template <class T1, class T2>
inline T2 ModPowerOf2(T1 a, T2 b)
{
	assert(IsPowerOf2(b));
	return T2(a) & (b-1);
}

template <class T>
inline T RoundDownToMultipleOf(T n, T m)
{
	return n - (IsPowerOf2(m) ? ModPowerOf2(n, m) : (n%m));
}

template <class T>
inline T RoundUpToMultipleOf(T n, T m)
{
	return RoundDownToMultipleOf(n+m-1, m);
}

template <class T>
inline unsigned int GetAlignment(T* /* dummy */ = NULL)	// VC60 workaround
{
#if defined(_MSC_VER) and (_MSC_VER >= 1300)
	return __alignof(T);
#elif defined(__GNUC__)
	return __alignof__(T);
#else
	return sizeof(T);
#endif
}

inline bool IsAlignedOn(const void *p, unsigned int alignment)
{
	return IsPowerOf2(alignment) ? ModPowerOf2((size_t)p, alignment) == 0 : (size_t)p % alignment == 0;
}

template <class T>
inline bool IsAligned(const void *p, T* /* dummy */ = NULL)	// VC60 workaround
{
	return IsAlignedOn(p, GetAlignment<T>());
}

#ifdef IS_LITTLE_ENDIAN
	typedef LittleEndian NativeByteOrder;
#else
	typedef BigEndian NativeByteOrder;
#endif

inline ByteOrder GetNativeByteOrder()
{
	return NativeByteOrder::ToEnum();
}

inline bool NativeByteOrderIs(ByteOrder order)
{
	return order == GetNativeByteOrder();
}

template <class T>		// can't use <sstream> because GCC 2.95.2 doesn't have it
inline std::string IntToString(T a, unsigned int base = 10)
{
	if (a == 0)
		return "0";
	bool negate = false;
	if (a < 0)
	{
		negate = true;
		a = 0-a;	// VC .NET does not like -a
	}
	std::string result;
	while (a > 0)
	{
		T digit = a % base;
		result = char((digit < 10 ? '0' : ('a' - 10)) + digit) + result;
		a /= base;
	}
	if (negate)
		result = "-" + result;
	return result;
}

// Avoid warnings about compare against zero for unsigned
template <>        // can't use <sstream> because GCC 2.95.2 doesn't have it 
inline std::string IntToString<unsigned int>(unsigned int a, unsigned int base) 
{
	if (a == 0)
		return "0";
	std::string result;
	while (a > 0)
	{
		unsigned digit = a % base;
		result = char((digit < 10 ? '0' : ('a' - 10)) + digit) + result;
		a /= base;
	}
	return result;
}

template <class T1, class T2>
inline T1 SaturatingSubtract(T1 a, T2 b)
{
	CRYPTOPP_COMPILE_ASSERT(T1(-1)>0);	// T1 is unsigned type
	CRYPTOPP_COMPILE_ASSERT(T2(-1)>0);	// T2 is unsigned type
	return T1((a > b) ? (a - b) : 0);
}

template <class T>
inline CipherDir GetCipherDir(const T &obj)
{
	return obj.IsForwardTransformation() ? ENCRYPTION : DECRYPTION;
}

void CallNewHandler();

// ************** rotate functions ***************

template <class T> inline T rotlFixed(T x, unsigned int y)
{
	assert(y < sizeof(T)*8);
	return (x<<y) | (x>>(sizeof(T)*8-y));
}

template <class T> inline T rotrFixed(T x, unsigned int y)
{
	assert(y < sizeof(T)*8);
	return (x>>y) | (x<<(sizeof(T)*8-y));
}

template <class T> inline T rotlVariable(T x, unsigned int y)
{
	assert(y < sizeof(T)*8);
	return (x<<y) | (x>>(sizeof(T)*8-y));
}

template <class T> inline T rotrVariable(T x, unsigned int y)
{
	assert(y < sizeof(T)*8);
	return (x>>y) | (x<<(sizeof(T)*8-y));
}

template <class T> inline T rotlMod(T x, unsigned int y)
{
	y %= sizeof(T)*8;
	return (x<<y) | (x>>(sizeof(T)*8-y));
}

template <class T> inline T rotrMod(T x, unsigned int y)
{
	y %= sizeof(T)*8;
	return (x>>y) | (x<<(sizeof(T)*8-y));
}

#ifdef INTEL_INTRINSICS

#pragma intrinsic(_lrotl, _lrotr)

template<> inline word32 rotlFixed<word32>(word32 x, unsigned int y)
{
	assert(y < 32);
	return y ? _lrotl(x, y) : x;
}

template<> inline word32 rotrFixed<word32>(word32 x, unsigned int y)
{
	assert(y < 32);
	return y ? _lrotr(x, y) : x;
}

template<> inline word32 rotlVariable<word32>(word32 x, unsigned int y)
{
	assert(y < 32);
	return _lrotl(x, y);
}

template<> inline word32 rotrVariable<word32>(word32 x, unsigned int y)
{
	assert(y < 32);
	return _lrotr(x, y);
}

template<> inline word32 rotlMod<word32>(word32 x, unsigned int y)
{
	return _lrotl(x, y);
}

template<> inline word32 rotrMod<word32>(word32 x, unsigned int y)
{
	return _lrotr(x, y);
}

#endif // #ifdef INTEL_INTRINSICS

#ifdef PPC_INTRINSICS

template<> inline word32 rotlFixed<word32>(word32 x, unsigned int y)
{
	assert(y < 32);
	return y ? __rlwinm(x,y,0,31) : x;
}

template<> inline word32 rotrFixed<word32>(word32 x, unsigned int y)
{
	assert(y < 32);
	return y ? __rlwinm(x,32-y,0,31) : x;
}

template<> inline word32 rotlVariable<word32>(word32 x, unsigned int y)
{
	assert(y < 32);
	return (__rlwnm(x,y,0,31));
}

template<> inline word32 rotrVariable<word32>(word32 x, unsigned int y)
{
	assert(y < 32);
	return (__rlwnm(x,32-y,0,31));
}

template<> inline word32 rotlMod<word32>(word32 x, unsigned int y)
{
	return (__rlwnm(x,y,0,31));
}

template<> inline word32 rotrMod<word32>(word32 x, unsigned int y)
{
	return (__rlwnm(x,32-y,0,31));
}

#endif // #ifdef PPC_INTRINSICS

// ************** endian reversal ***************

template <class T>
inline unsigned int GetByte(ByteOrder order, T value, unsigned int index)
{
	if (order == LITTLE_ENDIAN_ORDER)
		return GETBYTE(value, index);
	else
		return GETBYTE(value, sizeof(T)-index-1);
}

inline byte ByteReverse(byte value)
{
	return value;
}

inline word16 ByteReverse(word16 value)
{
	return rotlFixed(value, 8U);
}

inline word32 ByteReverse(word32 value)
{
#ifdef PPC_INTRINSICS
	// PPC: load reverse indexed instruction
	return (word32)__lwbrx(&value,0);
#elif defined(FAST_ROTATE)
	// 5 instructions with rotate instruction, 9 without
	return (rotrFixed(value, 8U) & 0xff00ff00) | (rotlFixed(value, 8U) & 0x00ff00ff);
#else
	// 6 instructions with rotate instruction, 8 without
	value = ((value & 0xFF00FF00) >> 8) | ((value & 0x00FF00FF) << 8);
	return rotlFixed(value, 16U);
#endif
}

#ifdef WORD64_AVAILABLE
inline word64 ByteReverse(word64 value)
{
#ifdef CRYPTOPP_SLOW_WORD64
	return (word64(ByteReverse(word32(value))) << 32) | ByteReverse(word32(value>>32));
#else
	value = ((value & W64LIT(0xFF00FF00FF00FF00)) >> 8) | ((value & W64LIT(0x00FF00FF00FF00FF)) << 8);
	value = ((value & W64LIT(0xFFFF0000FFFF0000)) >> 16) | ((value & W64LIT(0x0000FFFF0000FFFF)) << 16);
	return rotlFixed(value, 32U);
#endif
}
#endif

inline byte BitReverse(byte value)
{
	value = ((value & 0xAA) >> 1) | ((value & 0x55) << 1);
	value = ((value & 0xCC) >> 2) | ((value & 0x33) << 2);
	return rotlFixed(value, 4);
}

inline word16 BitReverse(word16 value)
{
	value = ((value & 0xAAAA) >> 1) | ((value & 0x5555) << 1);
	value = ((value & 0xCCCC) >> 2) | ((value & 0x3333) << 2);
	value = ((value & 0xF0F0) >> 4) | ((value & 0x0F0F) << 4);
	return ByteReverse(value);
}

inline word32 BitReverse(word32 value)
{
	value = ((value & 0xAAAAAAAA) >> 1) | ((value & 0x55555555) << 1);
	value = ((value & 0xCCCCCCCC) >> 2) | ((value & 0x33333333) << 2);
	value = ((value & 0xF0F0F0F0) >> 4) | ((value & 0x0F0F0F0F) << 4);
	return ByteReverse(value);
}

#ifdef WORD64_AVAILABLE
inline word64 BitReverse(word64 value)
{
#ifdef CRYPTOPP_SLOW_WORD64
	return (word64(BitReverse(word32(value))) << 32) | BitReverse(word32(value>>32));
#else
	value = ((value & W64LIT(0xAAAAAAAAAAAAAAAA)) >> 1) | ((value & W64LIT(0x5555555555555555)) << 1);
	value = ((value & W64LIT(0xCCCCCCCCCCCCCCCC)) >> 2) | ((value & W64LIT(0x3333333333333333)) << 2);
	value = ((value & W64LIT(0xF0F0F0F0F0F0F0F0)) >> 4) | ((value & W64LIT(0x0F0F0F0F0F0F0F0F)) << 4);
	return ByteReverse(value);
#endif
}
#endif

template <class T>
inline T BitReverse(T value)
{
	if (sizeof(T) == 1)
		return (T)BitReverse((byte)value);
	else if (sizeof(T) == 2)
		return (T)BitReverse((word16)value);
	else if (sizeof(T) == 4)
		return (T)BitReverse((word32)value);
	else
	{
#ifdef WORD64_AVAILABLE
		assert(sizeof(T) == 8);
		return (T)BitReverse((word64)value);
#else
		assert(false);
		return 0;
#endif
	}
}

template <class T>
inline T ConditionalByteReverse(ByteOrder order, T value)
{
	return NativeByteOrderIs(order) ? value : ByteReverse(value);
}

template <class T>
void ByteReverse(T *out, const T *in, unsigned int byteCount)
{
	assert(byteCount % sizeof(T) == 0);
	unsigned int count = byteCount/sizeof(T);
	for (unsigned int i=0; i<count; i++)
		out[i] = ByteReverse(in[i]);
}

template <class T>
inline void ConditionalByteReverse(ByteOrder order, T *out, const T *in, unsigned int byteCount)
{
	if (!NativeByteOrderIs(order))
		ByteReverse(out, in, byteCount);
	else if (in != out)
		memcpy(out, in, byteCount);
}

template <class T>
inline void GetUserKey(ByteOrder order, T *out, unsigned int outlen, const byte *in, unsigned int inlen)
{
	const unsigned int U = sizeof(T);
	assert(inlen <= outlen*U);
	memcpy(out, in, inlen);
	memset((byte *)out+inlen, 0, outlen*U-inlen);
	ConditionalByteReverse(order, out, out, RoundUpToMultipleOf(inlen, U));
}

inline byte UnalignedGetWordNonTemplate(ByteOrder /*order*/, const byte *block, byte*)
{
	return block[0];
}

inline word16 UnalignedGetWordNonTemplate(ByteOrder order, const byte *block, word16*)
{
	return (order == BIG_ENDIAN_ORDER)
		? block[1] | (block[0] << 8)
		: block[0] | (block[1] << 8);
}

inline word32 UnalignedGetWordNonTemplate(ByteOrder order, const byte *block, word32*)
{
	return (order == BIG_ENDIAN_ORDER)
		? word32(block[3]) | (word32(block[2]) << 8) | (word32(block[1]) << 16) | (word32(block[0]) << 24)
		: word32(block[0]) | (word32(block[1]) << 8) | (word32(block[2]) << 16) | (word32(block[3]) << 24);
}

#ifdef WORD64_AVAILABLE
inline word64 UnalignedGetWordNonTemplate(ByteOrder order, const byte *block, word64*)
{
	return (order == BIG_ENDIAN_ORDER)
		?
		(word64(block[7]) |
		(word64(block[6]) <<  8) |
		(word64(block[5]) << 16) |
		(word64(block[4]) << 24) |
		(word64(block[3]) << 32) |
		(word64(block[2]) << 40) |
		(word64(block[1]) << 48) |
		(word64(block[0]) << 56))
		:
		(word64(block[0]) |
		(word64(block[1]) <<  8) |
		(word64(block[2]) << 16) |
		(word64(block[3]) << 24) |
		(word64(block[4]) << 32) |
		(word64(block[5]) << 40) |
		(word64(block[6]) << 48) |
		(word64(block[7]) << 56));
}
#endif

template <class T>
inline T UnalignedGetWord(ByteOrder order, const byte *block, T*dummy=NULL)
{
	return UnalignedGetWordNonTemplate(order, block, dummy);
}

inline void UnalignedPutWord(ByteOrder /*order*/, byte *block, byte value, const byte *xorBlock = NULL)
{
	block[0] = xorBlock ? (value ^ xorBlock[0]) : value;
}

inline void UnalignedPutWord(ByteOrder order, byte *block, word16 value, const byte *xorBlock = NULL)
{
	if (order == BIG_ENDIAN_ORDER)
	{
		block[0] = GETBYTE(value, 1);
		block[1] = GETBYTE(value, 0);
	}
	else
	{
		block[0] = GETBYTE(value, 0);
		block[1] = GETBYTE(value, 1);
	}

	if (xorBlock)
	{
		block[0] ^= xorBlock[0];
		block[1] ^= xorBlock[1];
	}
}

inline void UnalignedPutWord(ByteOrder order, byte *block, word32 value, const byte *xorBlock = NULL)
{
	if (order == BIG_ENDIAN_ORDER)
	{
		block[0] = GETBYTE(value, 3);
		block[1] = GETBYTE(value, 2);
		block[2] = GETBYTE(value, 1);
		block[3] = GETBYTE(value, 0);
	}
	else
	{
		block[0] = GETBYTE(value, 0);
		block[1] = GETBYTE(value, 1);
		block[2] = GETBYTE(value, 2);
		block[3] = GETBYTE(value, 3);
	}

	if (xorBlock)
	{
		block[0] ^= xorBlock[0];
		block[1] ^= xorBlock[1];
		block[2] ^= xorBlock[2];
		block[3] ^= xorBlock[3];
	}
}

#ifdef WORD64_AVAILABLE
inline void UnalignedPutWord(ByteOrder order, byte *block, word64 value, const byte *xorBlock = NULL)
{
	if (order == BIG_ENDIAN_ORDER)
	{
		block[0] = GETBYTE(value, 7);
		block[1] = GETBYTE(value, 6);
		block[2] = GETBYTE(value, 5);
		block[3] = GETBYTE(value, 4);
		block[4] = GETBYTE(value, 3);
		block[5] = GETBYTE(value, 2);
		block[6] = GETBYTE(value, 1);
		block[7] = GETBYTE(value, 0);
	}
	else
	{
		block[0] = GETBYTE(value, 0);
		block[1] = GETBYTE(value, 1);
		block[2] = GETBYTE(value, 2);
		block[3] = GETBYTE(value, 3);
		block[4] = GETBYTE(value, 4);
		block[5] = GETBYTE(value, 5);
		block[6] = GETBYTE(value, 6);
		block[7] = GETBYTE(value, 7);
	}

	if (xorBlock)
	{
		block[0] ^= xorBlock[0];
		block[1] ^= xorBlock[1];
		block[2] ^= xorBlock[2];
		block[3] ^= xorBlock[3];
		block[4] ^= xorBlock[4];
		block[5] ^= xorBlock[5];
		block[6] ^= xorBlock[6];
		block[7] ^= xorBlock[7];
	}
}
#endif

template <class T>
inline T GetWord(bool assumeAligned, ByteOrder order, const byte *block)
{
	if (assumeAligned)
	{
		assert(IsAligned<T>(block));
		return ConditionalByteReverse(order, *reinterpret_cast<const T *>(block));
	}
	else
		return UnalignedGetWord<T>(order, block);
}

template <class T>
inline void GetWord(bool assumeAligned, ByteOrder order, T &result, const byte *block)
{
	result = GetWord<T>(assumeAligned, order, block);
}

template <class T>
inline void PutWord(bool assumeAligned, ByteOrder order, byte *block, T value, const byte *xorBlock = NULL)
{
	if (assumeAligned)
	{
		assert(IsAligned<T>(block));
		if (xorBlock)
			*reinterpret_cast<T *>(block) = ConditionalByteReverse(order, value) ^ *reinterpret_cast<const T *>(xorBlock);
		else
			*reinterpret_cast<T *>(block) = ConditionalByteReverse(order, value);
	}
	else
		UnalignedPutWord(order, block, value, xorBlock);
}

template <class T, class B, bool A=true>
class GetBlock
{
public:
	GetBlock(const void *block)
		: m_block((const byte *)block) {}

	template <class U>
	inline GetBlock<T, B, A> & operator()(U &x)
	{
		CRYPTOPP_COMPILE_ASSERT(sizeof(U) >= sizeof(T));
		x = GetWord<T>(A, B::ToEnum(), m_block);
		m_block += sizeof(T);
		return *this;
	}

private:
	const byte *m_block;
};

// ************** help remove warning on g++ ***************

template <bool overflow> struct SafeShifter;

template<> struct SafeShifter<true>
{
	template <class T>
	static inline T RightShift(T /* value */, unsigned int /* bits */)
	{
		return 0;
	}

	template <class T>
	static inline T LeftShift(T /* value */, unsigned int /* bits */)
	{
		return 0;
	}
};

template<> struct SafeShifter<false>
{
	template <class T>
	static inline T RightShift(T value, unsigned int bits)
	{
		return value >> bits;
	}

	template <class T>
	static inline T LeftShift(T value, unsigned int bits)
	{
		return value << bits;
	}
};

template <unsigned int bits, class T>
inline T SafeRightShift(T value)
{
	return SafeShifter<(bits>=(8*sizeof(T)))>::RightShift(value, bits);
}

template <unsigned int bits, class T>
inline T SafeLeftShift(T value)
{
	return SafeShifter<(bits>=(8*sizeof(T)))>::LeftShift(value, bits);
}

NAMESPACE_END

#endif // MISC_H
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// secblock.h - written and placed in the public domain by Wei Dai

#ifndef CRYPTOPP_SECBLOCK_H
#define CRYPTOPP_SECBLOCK_H

//- #include "config.h"
//- #include "misc.h"
#include <string.h>		// CodeWarrior doesn't have memory.h
#include <assert.h>

NAMESPACE_BEGIN(CryptoPP)

// ************** secure memory allocation ***************

template<class T>
class AllocatorBase
{
public:
	typedef T value_type;
	typedef size_t size_type;
#ifdef CRYPTOPP_MSVCRT6
	typedef ptrdiff_t difference_type;
#else
	typedef std::ptrdiff_t difference_type;
#endif
	typedef T * pointer;
	typedef const T * const_pointer;
	typedef T & reference;
	typedef const T & const_reference;

	pointer address(reference r) const {return (&r);}
	const_pointer address(const_reference r) const {return (&r); }
	void construct(pointer p, const T& val) {new (p) T(val);}
	void destroy(pointer p) {p->~T();}
	size_type max_size() const {return ~size_type(0)/sizeof(T);}	// switch to std::numeric_limits<T>::max later

protected:
	static void CheckSize(size_t n)
	{
		if (n > ~size_t(0) / sizeof(T))
			throw InvalidArgument("AllocatorBase: requested size would cause integer overflow");
	}
};

#define CRYPTOPP_INHERIT_ALLOCATOR_TYPES	\
typedef typename AllocatorBase<T>::value_type value_type;\
typedef typename AllocatorBase<T>::size_type size_type;\
typedef typename AllocatorBase<T>::difference_type difference_type;\
typedef typename AllocatorBase<T>::pointer pointer;\
typedef typename AllocatorBase<T>::const_pointer const_pointer;\
typedef typename AllocatorBase<T>::reference reference;\
typedef typename AllocatorBase<T>::const_reference const_reference;

template <class T, class A>
typename A::pointer StandardReallocate(A& a, T *p, typename A::size_type oldSize, typename A::size_type newSize, bool preserve)
{
	if (oldSize == newSize)
		return p;

	if (preserve)
	{
		A b;
		typename A::pointer newPointer = b.allocate(newSize, NULL);
		memcpy(newPointer, p, sizeof(T)*STDMIN(oldSize, newSize));
		a.deallocate(p, oldSize);
		std::swap(a, b);
		return newPointer;
	}
	else
	{
		a.deallocate(p, oldSize);
		return a.allocate(newSize, NULL);
	}
}

template <class T>
class AllocatorWithCleanup : public AllocatorBase<T>
{
public:
	CRYPTOPP_INHERIT_ALLOCATOR_TYPES

	pointer allocate(size_type n, const void * = NULL)
	{
		CheckSize(n);
		if (n == 0)
			return NULL;
		return new T[n];
	}

	void deallocate(void *p, size_type n)
	{
		memset(p, 0, n*sizeof(T));
		delete [] (T *)p;
	}

	pointer reallocate(T *p, size_type oldSize, size_type newSize, bool preserve)
	{
		return StandardReallocate(*this, p, oldSize, newSize, preserve);
	}

	// VS.NET STL enforces the policy of "All STL-compliant allocators have to provide a
	// template class member called rebind".
    template <class U> struct rebind { typedef AllocatorWithCleanup<U> other; };
};

template <class T>
class NullAllocator : public AllocatorBase<T>
{
public:
	CRYPTOPP_INHERIT_ALLOCATOR_TYPES

	pointer allocate(size_type /* n */, const void * = NULL)
	{
		assert(false);
		return NULL;
	}

	void deallocate(void* /* p */, size_type /* n */)
	{
		assert(false);
	}

	size_type max_size() const {return 0;}
};

// This allocator can't be used with standard collections because
// they require that all objects of the same allocator type are equivalent.
// So this is for use with SecBlock only.
template <class T, size_t S, class A = NullAllocator<T> >
class FixedSizeAllocatorWithCleanup : public AllocatorBase<T>
{
public:
	CRYPTOPP_INHERIT_ALLOCATOR_TYPES

	FixedSizeAllocatorWithCleanup() : m_allocated(false) {}

	pointer allocate(size_type n)
	{
		if (n <= S && !m_allocated)
		{
			m_allocated = true;
			return m_array;
		}
		else
			return m_fallbackAllocator.allocate(n);
	}

	pointer allocate(size_type n, const void *hint)
	{
		if (n <= S && !m_allocated)
		{
			m_allocated = true;
			return m_array;
		}
		else
			return m_fallbackAllocator.allocate(n, hint);
	}

	void deallocate(void *p, size_type n)
	{
		if (p == m_array)
		{
			assert(n <= S);
			assert(m_allocated);
			m_allocated = false;
			memset(p, 0, n*sizeof(T));
		}
		else
			m_fallbackAllocator.deallocate(p, n);
	}

	pointer reallocate(pointer p, size_type oldSize, size_type newSize, bool preserve)
	{
		if (p == m_array && newSize <= S)
		{
			assert(oldSize <= S);
			if (oldSize > newSize)
				memset(p + newSize, 0, (oldSize-newSize)*sizeof(T));
			return p;
		}

		pointer newPointer = allocate(newSize, NULL);
		if (preserve)
			memcpy(newPointer, p, sizeof(T)*STDMIN(oldSize, newSize));
		deallocate(p, oldSize);
		return newPointer;
	}

	size_type max_size() const {return STDMAX(m_fallbackAllocator.max_size(), S);}

private:
	T m_array[S];
	A m_fallbackAllocator;
	bool m_allocated;
};

//! a block of memory allocated using A
template <class T, class A = AllocatorWithCleanup<T> >
class SecBlock
{
public:
    explicit SecBlock(unsigned int blocksize=0)
		: m_size(blocksize) {m_ptr = m_alloc.allocate(blocksize, NULL);}
	SecBlock(const SecBlock<T, A> &t)
		: m_size(t.m_size) {m_ptr = m_alloc.allocate(m_size, NULL); memcpy(m_ptr, t.m_ptr, m_size*sizeof(T));}
	SecBlock(const T *t, unsigned int len)
		: m_size(len)
	{
		m_ptr = m_alloc.allocate(len, NULL);
		if (t == NULL)
			memset(m_ptr, 0, len*sizeof(T));
		else
			memcpy(m_ptr, t, len*sizeof(T));
	}

	~SecBlock()
		{m_alloc.deallocate(m_ptr, m_size);}

	operator const void *() const
		{return m_ptr;}
	operator void *()
		{return m_ptr;}
#if defined(__GNUC__) && __GNUC__ < 3	// reduce warnings
	operator const void *()
		{return m_ptr;}
#endif

	operator const T *() const
		{return m_ptr;}
	operator T *()
		{return m_ptr;}
#if defined(__GNUC__) && __GNUC__ < 3	// reduce warnings
	operator const T *()
		{return m_ptr;}
#endif

	template <typename I>
	T *operator +(I offset)
		{return m_ptr+offset;}

	template <typename I>
	const T *operator +(I offset) const
		{return m_ptr+offset;}

	template <typename I>
	T& operator[](I index)
		{assert((unsigned int)index < m_size); return m_ptr[index];}

	template <typename I>
	const T& operator[](I index) const
		{assert((unsigned int)index < m_size); return m_ptr[index];}

	typedef typename A::value_type value_type;
	typedef typename A::pointer iterator;
	typedef typename A::const_pointer const_iterator;
	typedef typename A::size_type size_type;

	iterator begin()
		{return m_ptr;}
	const_iterator begin() const
		{return m_ptr;}
	iterator end()
		{return m_ptr+m_size;}
	const_iterator end() const
		{return m_ptr+m_size;}

	typename A::pointer data() {return m_ptr;}
	typename A::const_pointer data() const {return m_ptr;}

	size_type size() const {return m_size;}
	bool empty() const {return m_size == 0;}

	void Assign(const T *t, unsigned int len)
	{
		New(len);
		memcpy(m_ptr, t, len*sizeof(T));
	}

	void Assign(const SecBlock<T, A> &t)
	{
		New(t.m_size);
		memcpy(m_ptr, t.m_ptr, m_size*sizeof(T));
	}

	SecBlock& operator=(const SecBlock<T, A> &t)
	{
		Assign(t);
		return *this;
	}

	bool operator==(const SecBlock<T, A> &t) const
	{
		return m_size == t.m_size && memcmp(m_ptr, t.m_ptr, m_size*sizeof(T)) == 0;
	}

	bool operator!=(const SecBlock<T, A> &t) const
	{
		return !operator==(t);
	}

	void New(unsigned int newSize)
	{
		m_ptr = m_alloc.reallocate(m_ptr, m_size, newSize, false);
		m_size = newSize;
	}

	void CleanNew(unsigned int newSize)
	{
		New(newSize);
		memset(m_ptr, 0, m_size*sizeof(T));
	}

	void Grow(unsigned int newSize)
	{
		if (newSize > m_size)
		{
			m_ptr = m_alloc.reallocate(m_ptr, m_size, newSize, true);
			m_size = newSize;
		}
	}

	void CleanGrow(unsigned int newSize)
	{
		if (newSize > m_size)
		{
			m_ptr = m_alloc.reallocate(m_ptr, m_size, newSize, true);
			memset(m_ptr+m_size, 0, (newSize-m_size)*sizeof(T));
			m_size = newSize;
		}
	}

	void resize(unsigned int newSize)
	{
		m_ptr = m_alloc.reallocate(m_ptr, m_size, newSize, true);
		m_size = newSize;
	}

	void swap(SecBlock<T, A> &b)
	{
		std::swap(m_alloc, b.m_alloc);
		std::swap(m_size, b.m_size);
		std::swap(m_ptr, b.m_ptr);
	}

//private:
	A m_alloc;
	unsigned int m_size;
	T *m_ptr;
};

typedef SecBlock<byte> SecByteBlock;
typedef SecBlock<word> SecWordBlock;

template <class T, unsigned int S, class A = FixedSizeAllocatorWithCleanup<T, S> >
class FixedSizeSecBlock : public SecBlock<T, A>
{
public:
	explicit FixedSizeSecBlock() : SecBlock<T, A>(S) {}
};

template<class T, class U>
inline bool operator==(const CryptoPP::AllocatorWithCleanup<T>&, const CryptoPP::AllocatorWithCleanup<U>&) {return (true);}
template<class T, class U>
inline bool operator!=(const CryptoPP::AllocatorWithCleanup<T>&, const CryptoPP::AllocatorWithCleanup<U>&) {return (false);}

NAMESPACE_END

NAMESPACE_BEGIN(std)
template <class T, class A>
inline void swap(CryptoPP::SecBlock<T, A> &a, CryptoPP::SecBlock<T, A> &b)
{
	a.swap(b);
}

#if defined(_STLPORT_VERSION) && !defined(_STLP_MEMBER_TEMPLATE_CLASSES)
template <class _Tp1, class _Tp2>
inline CryptoPP::AllocatorWithCleanup<_Tp2>&
__stl_alloc_rebind(CryptoPP::AllocatorWithCleanup<_Tp1>& __a, const _Tp2*)
{
	return (CryptoPP::AllocatorWithCleanup<_Tp2>&)(__a);
}
#endif

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_INTEGER_H
#define CRYPTOPP_INTEGER_H

/** \file */

//- #include "cryptlib.h"
//- #include "secblock.h"

#include <iosfwd>
#include <algorithm>

#ifdef CRYPTOPP_X86ASM_AVAILABLE

#ifdef _M_IX86
	#if (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 500)) || (defined(__ICL) && (__ICL >= 500))
		#define SSE2_INTRINSICS_AVAILABLE
		#define CRYPTOPP_MM_MALLOC_AVAILABLE
	#elif defined(_MSC_VER)
		// _mm_free seems to be the only way to tell if the Processor Pack is installed or not
		#include <malloc.h>
		#if defined(_mm_free)
			#define SSE2_INTRINSICS_AVAILABLE
			#define CRYPTOPP_MM_MALLOC_AVAILABLE
		#endif
	#endif
#endif

// SSE2 intrinsics work in GCC 3.3 or later
#if defined(__SSE2__) && (__GNUC__ > 3 || __GNUC_MINOR__ > 2)
	#define SSE2_INTRINSICS_AVAILABLE
#endif

#endif

NAMESPACE_BEGIN(CryptoPP)

#if defined(SSE2_INTRINSICS_AVAILABLE)
	template <class T>
	class AlignedAllocator : public AllocatorBase<T>
	{
	public:
		CRYPTOPP_INHERIT_ALLOCATOR_TYPES

		pointer allocate(size_type n, const void *);
		void deallocate(void *p, size_type n);
		pointer reallocate(T *p, size_type oldSize, size_type newSize, bool preserve)
		{
			return StandardReallocate(*this, p, oldSize, newSize, preserve);
		}

	#if !(defined(CRYPTOPP_MALLOC_ALIGNMENT_IS_16) || defined(CRYPTOPP_MEMALIGN_AVAILABLE) || defined(CRYPTOPP_MM_MALLOC_AVAILABLE))
	#define CRYPTOPP_NO_ALIGNED_ALLOC
		AlignedAllocator() : m_pBlock(NULL) {}
	protected:
		void *m_pBlock;
	#endif
	};

	template class CRYPTOPP_DLL AlignedAllocator<word>;
	typedef SecBlock<word, AlignedAllocator<word> > SecAlignedWordBlock;
#else
	typedef SecWordBlock SecAlignedWordBlock;
#endif

void CRYPTOPP_DLL DisableSSE2();

//! multiple precision integer and basic arithmetics
/*! This class can represent positive and negative integers
	with absolute value less than (256**sizeof(word)) ** (256**sizeof(int)).
	\nosubgrouping
*/
class CRYPTOPP_DLL Integer : public ASN1Object
{
public:
	//! \name ENUMS, EXCEPTIONS, and TYPEDEFS
	//@{
		//! division by zero exception
		class DivideByZero : public Exception
		{
		public:
			DivideByZero() : Exception(OTHER_ERROR, "Integer: division by zero") {}
		};

		//!
		class RandomNumberNotFound : public Exception
		{
		public:
			RandomNumberNotFound() : Exception(OTHER_ERROR, "Integer: no integer satisfies the given parameters") {}
		};

		//!
		enum Sign {POSITIVE=0, NEGATIVE=1};

		//!
		enum Signedness {
		//!
			UNSIGNED,
		//!
			SIGNED};

		//!
		enum RandomNumberType {
		//!
			ANY,
		//!
			PRIME};
	//@}

	//! \name CREATORS
	//@{
		//! creates the zero integer
		Integer();

		//! copy constructor
		Integer(const Integer& t);

		//! convert from signed long
		Integer(signed long value);

		//! convert from lword
		Integer(Sign s, lword value);

		//! convert from two words
		Integer(Sign s, word highWord, word lowWord);

		//! convert from string
		/*! str can be in base 2, 8, 10, or 16.  Base is determined by a
			case insensitive suffix of 'h', 'o', or 'b'.  No suffix means base 10.
		*/
		explicit Integer(const char *str);
		explicit Integer(const wchar_t *str);

		//! convert from big-endian byte array
		Integer(const byte *encodedInteger, unsigned int byteCount, Signedness s=UNSIGNED);

		//! convert from big-endian form stored in a BufferedTransformation
		Integer(BufferedTransformation &bt, unsigned int byteCount, Signedness s=UNSIGNED);

		//! convert from BER encoded byte array stored in a BufferedTransformation object
		explicit Integer(BufferedTransformation &bt);

		//! create a random integer
		/*! The random integer created is uniformly distributed over [0, 2**bitcount). */
		Integer(RandomNumberGenerator &rng, unsigned int bitcount);

		//! avoid calling constructors for these frequently used integers
		static const Integer &Zero();
		//! avoid calling constructors for these frequently used integers
		static const Integer &One();
		//! avoid calling constructors for these frequently used integers
		static const Integer &Two();

		//! create a random integer of special type
		/*! Ideally, the random integer created should be uniformly distributed
			over {x | min <= x <= max and x is of rnType and x % mod == equiv}.
			However the actual distribution may not be uniform because sequential
			search is used to find an appropriate number from a random starting
			point.
			May return (with very small probability) a pseudoprime when a prime
			is requested and max > lastSmallPrime*lastSmallPrime (lastSmallPrime
			is declared in nbtheory.h).
			\throw RandomNumberNotFound if the set is empty.
		*/
		Integer(RandomNumberGenerator &rng, const Integer &min, const Integer &max, RandomNumberType rnType=ANY, const Integer &equiv=Zero(), const Integer &mod=One());

		//! return the integer 2**e
		static Integer Power2(unsigned int e);
	//@}

	//! \name ENCODE/DECODE
	//@{
		//! minimum number of bytes to encode this integer
		/*! MinEncodedSize of 0 is 1 */
		unsigned int MinEncodedSize(Signedness=UNSIGNED) const;
		//! encode in big-endian format
		/*! unsigned means encode absolute value, signed means encode two's complement if negative.
			if outputLen < MinEncodedSize, the most significant bytes will be dropped
			if outputLen > MinEncodedSize, the most significant bytes will be padded
		*/
		unsigned int Encode(byte *output, unsigned int outputLen, Signedness=UNSIGNED) const;
		//!
		unsigned int Encode(BufferedTransformation &bt, unsigned int outputLen, Signedness=UNSIGNED) const;

		//! encode using Distinguished Encoding Rules, put result into a BufferedTransformation object
		void DEREncode(BufferedTransformation &bt) const;

		//! encode absolute value as big-endian octet string
		void DEREncodeAsOctetString(BufferedTransformation &bt, unsigned int length) const;

		//! encode absolute value in OpenPGP format, return length of output
		unsigned int OpenPGPEncode(byte *output, unsigned int bufferSize) const;
		//! encode absolute value in OpenPGP format, put result into a BufferedTransformation object
		unsigned int OpenPGPEncode(BufferedTransformation &bt) const;

		//!
		void Decode(const byte *input, unsigned int inputLen, Signedness=UNSIGNED);
		//! 
		//* Precondition: bt.MaxRetrievable() >= inputLen
		void Decode(BufferedTransformation &bt, unsigned int inputLen, Signedness=UNSIGNED);

		//!
		void BERDecode(const byte *input, unsigned int inputLen);
		//!
		void BERDecode(BufferedTransformation &bt);

		//! decode nonnegative value as big-endian octet string
		void BERDecodeAsOctetString(BufferedTransformation &bt, unsigned int length);

		class OpenPGPDecodeErr : public Exception
		{
		public: 
			OpenPGPDecodeErr() : Exception(INVALID_DATA_FORMAT, "OpenPGP decode error") {}
		};

		//!
		void OpenPGPDecode(const byte *input, unsigned int inputLen);
		//!
		void OpenPGPDecode(BufferedTransformation &bt);
	//@}

	//! \name ACCESSORS
	//@{
		//! return true if *this can be represented as a signed long
		bool IsConvertableToLong() const;
		//! return equivalent signed long if possible, otherwise undefined
		signed long ConvertToLong() const;

		//! number of significant bits = floor(log2(abs(*this))) + 1
		unsigned int BitCount() const;
		//! number of significant bytes = ceiling(BitCount()/8)
		unsigned int ByteCount() const;
		//! number of significant words = ceiling(ByteCount()/sizeof(word))
		unsigned int WordCount() const;

		//! return the i-th bit, i=0 being the least significant bit
		bool GetBit(unsigned int i) const;
		//! return the i-th byte
		byte GetByte(unsigned int i) const;
		//! return n lowest bits of *this >> i
		unsigned long GetBits(unsigned int i, unsigned int n) const;

		//!
		bool IsZero() const {return !*this;}
		//!
		bool NotZero() const {return !IsZero();}
		//!
		bool IsNegative() const {return sign == NEGATIVE;}
		//!
		bool NotNegative() const {return !IsNegative();}
		//!
		bool IsPositive() const {return NotNegative() && NotZero();}
		//!
		bool NotPositive() const {return !IsPositive();}
		//!
		bool IsEven() const {return GetBit(0) == 0;}
		//!
		bool IsOdd() const	{return GetBit(0) == 1;}
	//@}

	//! \name MANIPULATORS
	//@{
		//!
		Integer&  operator=(const Integer& t);

		//!
		Integer&  operator+=(const Integer& t);
		//!
		Integer&  operator-=(const Integer& t);
		//!
		Integer&  operator*=(const Integer& t)	{return *this = Times(t);}
		//!
		Integer&  operator/=(const Integer& t)	{return *this = DividedBy(t);}
		//!
		Integer&  operator%=(const Integer& t)	{return *this = Modulo(t);}
		//!
		Integer&  operator/=(word t)  {return *this = DividedBy(t);}
		//!
		Integer&  operator%=(word t)  {return *this = Modulo(t);}

		//!
		Integer&  operator<<=(unsigned int);
		//!
		Integer&  operator>>=(unsigned int);

		//!
		void Randomize(RandomNumberGenerator &rng, unsigned int bitcount);
		//!
		void Randomize(RandomNumberGenerator &rng, const Integer &min, const Integer &max);
		//! set this Integer to a random element of {x | min <= x <= max and x is of rnType and x % mod == equiv}
		/*! returns false if the set is empty */
		bool Randomize(RandomNumberGenerator &rng, const Integer &min, const Integer &max, RandomNumberType rnType, const Integer &equiv=Zero(), const Integer &mod=One());

		bool GenerateRandomNoThrow(RandomNumberGenerator &rng, const NameValuePairs &params = g_nullNameValuePairs);
		void GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &params = g_nullNameValuePairs)
		{
			if (!GenerateRandomNoThrow(rng, params))
				throw RandomNumberNotFound();
		}

		//! set the n-th bit to value
		void SetBit(unsigned int n, bool value=1);
		//! set the n-th byte to value
		void SetByte(unsigned int n, byte value);

		//!
		void Negate();
		//!
		void SetPositive() {sign = POSITIVE;}
		//!
		void SetNegative() {if (!!(*this)) sign = NEGATIVE;}

		//!
		void swap(Integer &a);
	//@}

	//! \name UNARY OPERATORS
	//@{
		//!
		bool		operator!() const;
		//!
		Integer 	operator+() const {return *this;}
		//!
		Integer 	operator-() const;
		//!
		Integer&	operator++();
		//!
		Integer&	operator--();
		//!
		Integer 	operator++(int) {Integer temp = *this; ++*this; return temp;}
		//!
		Integer 	operator--(int) {Integer temp = *this; --*this; return temp;}
	//@}

	//! \name BINARY OPERATORS
	//@{
		//! signed comparison
		/*! \retval -1 if *this < a
			\retval  0 if *this = a
			\retval  1 if *this > a
		*/
		int Compare(const Integer& a) const;

		//!
		Integer Plus(const Integer &b) const;
		//!
		Integer Minus(const Integer &b) const;
		//!
		Integer Times(const Integer &b) const;
		//!
		Integer DividedBy(const Integer &b) const;
		//!
		Integer Modulo(const Integer &b) const;
		//!
		Integer DividedBy(word b) const;
		//!
		word Modulo(word b) const;

		//!
		Integer operator>>(unsigned int n) const	{return Integer(*this)>>=n;}
		//!
		Integer operator<<(unsigned int n) const	{return Integer(*this)<<=n;}
	//@}

	//! \name OTHER ARITHMETIC FUNCTIONS
	//@{
		//!
		Integer AbsoluteValue() const;
		//!
		Integer Doubled() const {return Plus(*this);}
		//!
		Integer Squared() const {return Times(*this);}
		//! extract square root, if negative return 0, else return floor of square root
		Integer SquareRoot() const;
		//! return whether this integer is a perfect square
		bool IsSquare() const;

		//! is 1 or -1
		bool IsUnit() const;
		//! return inverse if 1 or -1, otherwise return 0
		Integer MultiplicativeInverse() const;

		//! modular multiplication
		CRYPTOPP_DLL friend Integer a_times_b_mod_c(const Integer &x, const Integer& y, const Integer& m);
		//! modular exponentiation
		CRYPTOPP_DLL friend Integer a_exp_b_mod_c(const Integer &x, const Integer& e, const Integer& m);

		//! calculate r and q such that (a == d*q + r) && (0 <= r < abs(d))
		static void Divide(Integer &r, Integer &q, const Integer &a, const Integer &d);
		//! use a faster division algorithm when divisor is short
		static void Divide(word &r, Integer &q, const Integer &a, word d);

		//! returns same result as Divide(r, q, a, Power2(n)), but faster
		static void DivideByPowerOf2(Integer &r, Integer &q, const Integer &a, unsigned int n);

		//! greatest common divisor
		static Integer Gcd(const Integer &a, const Integer &n);
		//! calculate multiplicative inverse of *this mod n
		Integer InverseMod(const Integer &n) const;
		//!
		word InverseMod(word n) const;
	//@}

	//! \name INPUT/OUTPUT
	//@{
		//!
		friend CRYPTOPP_DLL std::istream& operator>>(std::istream& in, Integer &a);
		//!
		friend CRYPTOPP_DLL std::ostream& operator<<(std::ostream& out, const Integer &a);
	//@}

private:
	friend class ModularArithmetic;
	friend class MontgomeryRepresentation;
	friend class HalfMontgomeryRepresentation;

	Integer(word value, unsigned int length);

	int PositiveCompare(const Integer &t) const;
	friend void PositiveAdd(Integer &sum, const Integer &a, const Integer &b);
	friend void PositiveSubtract(Integer &diff, const Integer &a, const Integer &b);
	friend void PositiveMultiply(Integer &product, const Integer &a, const Integer &b);
	friend void PositiveDivide(Integer &remainder, Integer &quotient, const Integer &dividend, const Integer &divisor);

	SecAlignedWordBlock reg;
	Sign sign;
};

//!
inline bool operator==(const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)==0;}
//!
inline bool operator!=(const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)!=0;}
//!
inline bool operator> (const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)> 0;}
//!
inline bool operator>=(const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)>=0;}
//!
inline bool operator< (const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)< 0;}
//!
inline bool operator<=(const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)<=0;}
//!
inline CryptoPP::Integer operator+(const CryptoPP::Integer &a, const CryptoPP::Integer &b) {return a.Plus(b);}
//!
inline CryptoPP::Integer operator-(const CryptoPP::Integer &a, const CryptoPP::Integer &b) {return a.Minus(b);}
//!
inline CryptoPP::Integer operator*(const CryptoPP::Integer &a, const CryptoPP::Integer &b) {return a.Times(b);}
//!
inline CryptoPP::Integer operator/(const CryptoPP::Integer &a, const CryptoPP::Integer &b) {return a.DividedBy(b);}
//!
inline CryptoPP::Integer operator%(const CryptoPP::Integer &a, const CryptoPP::Integer &b) {return a.Modulo(b);}
//!
inline CryptoPP::Integer operator/(const CryptoPP::Integer &a, CryptoPP::word b) {return a.DividedBy(b);}
//!
inline CryptoPP::word    operator%(const CryptoPP::Integer &a, CryptoPP::word b) {return a.Modulo(b);}

NAMESPACE_END

NAMESPACE_BEGIN(std)
template<> inline void swap(CryptoPP::Integer &a, CryptoPP::Integer &b)
{
	a.swap(b);
}
NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_ALGEBRA_H
#define CRYPTOPP_ALGEBRA_H

//- #include "config.h"

NAMESPACE_BEGIN(CryptoPP)

class Integer;

// "const Element&" returned by member functions are references
// to internal data members. Since each object may have only
// one such data member for holding results, the following code
// will produce incorrect results:
// abcd = group.Add(group.Add(a,b), group.Add(c,d));
// But this should be fine:
// abcd = group.Add(a, group.Add(b, group.Add(c,d));

//! Abstract Group
template <class T> class CRYPTOPP_NO_VTABLE AbstractGroup
{
public:
	typedef T Element;

	virtual ~AbstractGroup() {}

	virtual bool Equal(const Element &a, const Element &b) const =0;
	virtual const Element& Identity() const =0;
	virtual const Element& Add(const Element &a, const Element &b) const =0;
	virtual const Element& Inverse(const Element &a) const =0;
	virtual bool InversionIsFast() const {return false;}

	virtual const Element& Double(const Element &a) const;
	virtual const Element& Subtract(const Element &a, const Element &b) const;
	virtual Element& Accumulate(Element &a, const Element &b) const;
	virtual Element& Reduce(Element &a, const Element &b) const;

	virtual Element ScalarMultiply(const Element &a, const Integer &e) const;
	virtual Element CascadeScalarMultiply(const Element &x, const Integer &e1, const Element &y, const Integer &e2) const;

	virtual void SimultaneousMultiply(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const;
};

//! Abstract Ring
template <class T> class CRYPTOPP_NO_VTABLE AbstractRing : public AbstractGroup<T>
{
public:
	typedef T Element;

	AbstractRing() {m_mg.m_pRing = this;}
	AbstractRing(const AbstractRing &source) {m_mg.m_pRing = this;}
	AbstractRing& operator=(const AbstractRing &source) {return *this;}

	virtual bool IsUnit(const Element &a) const =0;
	virtual const Element& MultiplicativeIdentity() const =0;
	virtual const Element& Multiply(const Element &a, const Element &b) const =0;
	virtual const Element& MultiplicativeInverse(const Element &a) const =0;

	virtual const Element& Square(const Element &a) const;
	virtual const Element& Divide(const Element &a, const Element &b) const;

	virtual Element Exponentiate(const Element &a, const Integer &e) const;
	virtual Element CascadeExponentiate(const Element &x, const Integer &e1, const Element &y, const Integer &e2) const;

	virtual void SimultaneousExponentiate(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const;

	virtual const AbstractGroup<T>& MultiplicativeGroup() const
		{return m_mg;}

private:
	class MultiplicativeGroupT : public AbstractGroup<T>
	{
	public:
		const AbstractRing<T>& GetRing() const
			{return *m_pRing;}

		bool Equal(const Element &a, const Element &b) const
			{return GetRing().Equal(a, b);}

		const Element& Identity() const
			{return GetRing().MultiplicativeIdentity();}

		const Element& Add(const Element &a, const Element &b) const
			{return GetRing().Multiply(a, b);}

		Element& Accumulate(Element &a, const Element &b) const
			{return a = GetRing().Multiply(a, b);}

		const Element& Inverse(const Element &a) const
			{return GetRing().MultiplicativeInverse(a);}

		const Element& Subtract(const Element &a, const Element &b) const
			{return GetRing().Divide(a, b);}

		Element& Reduce(Element &a, const Element &b) const
			{return a = GetRing().Divide(a, b);}

		const Element& Double(const Element &a) const
			{return GetRing().Square(a);}

		Element ScalarMultiply(const Element &a, const Integer &e) const
			{return GetRing().Exponentiate(a, e);}

		Element CascadeScalarMultiply(const Element &x, const Integer &e1, const Element &y, const Integer &e2) const
			{return GetRing().CascadeExponentiate(x, e1, y, e2);}

		void SimultaneousMultiply(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const
			{GetRing().SimultaneousExponentiate(results, base, exponents, exponentsCount);}

		const AbstractRing<T> *m_pRing;
	};

	MultiplicativeGroupT m_mg;
};

// ********************************************************

//! Abstract Euclidean Domain
template <class T> class CRYPTOPP_NO_VTABLE AbstractEuclideanDomain : public AbstractRing<T>
{
public:
	typedef T Element;

	virtual void DivisionAlgorithm(Element &r, Element &q, const Element &a, const Element &d) const =0;

	virtual const Element& Mod(const Element &a, const Element &b) const =0;
	virtual const Element& Gcd(const Element &a, const Element &b) const;

protected:
	mutable Element result;
};

// ********************************************************

//! EuclideanDomainOf
template <class T> class EuclideanDomainOf : public AbstractEuclideanDomain<T>
{
public:
	typedef T Element;

	EuclideanDomainOf() {}

	bool Equal(const Element &a, const Element &b) const
		{return a==b;}

	const Element& Identity() const
		{return Element::Zero();}

	const Element& Add(const Element &a, const Element &b) const
		{return result = a+b;}

	Element& Accumulate(Element &a, const Element &b) const
		{return a+=b;}

	const Element& Inverse(const Element &a) const
		{return result = -a;}

	const Element& Subtract(const Element &a, const Element &b) const
		{return result = a-b;}

	Element& Reduce(Element &a, const Element &b) const
		{return a-=b;}

	const Element& Double(const Element &a) const
		{return result = a.Doubled();}

	const Element& MultiplicativeIdentity() const
		{return Element::One();}

	const Element& Multiply(const Element &a, const Element &b) const
		{return result = a*b;}

	const Element& Square(const Element &a) const
		{return result = a.Squared();}

	bool IsUnit(const Element &a) const
		{return a.IsUnit();}

	const Element& MultiplicativeInverse(const Element &a) const
		{return result = a.MultiplicativeInverse();}

	const Element& Divide(const Element &a, const Element &b) const
		{return result = a/b;}

	const Element& Mod(const Element &a, const Element &b) const
		{return result = a%b;}

	void DivisionAlgorithm(Element &r, Element &q, const Element &a, const Element &d) const
		{Element::Divide(r, q, a, d);}

	bool operator==(const EuclideanDomainOf<T> &rhs) const
		{return true;}

private:
	mutable Element result;
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_MODARITH_H
#define CRYPTOPP_MODARITH_H

// implementations are in integer.cpp

//- #include "cryptlib.h"
//- #include "misc.h"
//- #include "integer.h"
//- #include "algebra.h"

NAMESPACE_BEGIN(CryptoPP)

//! ring of congruence classes modulo n
/*! \note this implementation represents each congruence class as the smallest non-negative integer in that class */
class CRYPTOPP_DLL ModularArithmetic : public AbstractRing<Integer>
{
public:

	typedef int RandomizationParameter;
	typedef Integer Element;

	ModularArithmetic(const Integer &mod = Integer::One())
		: AbstractRing<Integer>(),
		  modulus(mod), result((word)0, modulus.reg.size()) {}

	ModularArithmetic(const ModularArithmetic &ma)
		: AbstractRing<Integer>(),
		  modulus(ma.modulus), result((word)0, modulus.reg.size()) {}

	ModularArithmetic(BufferedTransformation &bt);	// construct from BER encoded parameters

	virtual ModularArithmetic * Clone() const {return new ModularArithmetic(*this);}

	void DEREncode(BufferedTransformation &bt) const;

	void DEREncodeElement(BufferedTransformation &out, const Element &a) const;
	void BERDecodeElement(BufferedTransformation &in, Element &a) const;

	const Integer& GetModulus() const {return modulus;}
	void SetModulus(const Integer &newModulus) {modulus = newModulus; result.reg.resize(modulus.reg.size());}

	virtual bool IsMontgomeryRepresentation() const {return false;}

	virtual Integer ConvertIn(const Integer &a) const
		{return a%modulus;}

	virtual Integer ConvertOut(const Integer &a) const
		{return a;}

	const Integer& Half(const Integer &a) const;

	bool Equal(const Integer &a, const Integer &b) const
		{return a==b;}

	const Integer& Identity() const
		{return Integer::Zero();}

	const Integer& Add(const Integer &a, const Integer &b) const;

	Integer& Accumulate(Integer &a, const Integer &b) const;

	const Integer& Inverse(const Integer &a) const;

	const Integer& Subtract(const Integer &a, const Integer &b) const;

	Integer& Reduce(Integer &a, const Integer &b) const;

	const Integer& Double(const Integer &a) const
		{return Add(a, a);}

	const Integer& MultiplicativeIdentity() const
		{return Integer::One();}

	const Integer& Multiply(const Integer &a, const Integer &b) const
		{return result1 = a*b%modulus;}

	const Integer& Square(const Integer &a) const
		{return result1 = a.Squared()%modulus;}

	bool IsUnit(const Integer &a) const
		{return Integer::Gcd(a, modulus).IsUnit();}

	const Integer& MultiplicativeInverse(const Integer &a) const
		{return result1 = a.InverseMod(modulus);}

	const Integer& Divide(const Integer &a, const Integer &b) const
		{return Multiply(a, MultiplicativeInverse(b));}

	Integer CascadeExponentiate(const Integer &x, const Integer &e1, const Integer &y, const Integer &e2) const;

	void SimultaneousExponentiate(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const;

	unsigned int MaxElementBitLength() const
		{return (modulus-1).BitCount();}

	unsigned int MaxElementByteLength() const
		{return (modulus-1).ByteCount();}

	Element RandomElement( RandomNumberGenerator &rng , const RandomizationParameter& /*ignore_for_now*/ = 0 ) const
		// left RandomizationParameter arg as ref in case RandomizationParameter becomes a more complicated struct
	{ 
		return Element( rng , Integer( (long) 0) , modulus - Integer( (long) 1 )   ) ; 
	}   

	bool operator==(const ModularArithmetic &rhs) const
		{return modulus == rhs.modulus;}

	static const RandomizationParameter DefaultRandomizationParameter ;

protected:
	Integer modulus;
	mutable Integer result, result1;

};

// const ModularArithmetic::RandomizationParameter ModularArithmetic::DefaultRandomizationParameter = 0 ;

//! do modular arithmetics in Montgomery representation for increased speed
/*! \note the Montgomery representation represents each congruence class [a] as a*r%n, where r is a convenient power of 2 */
class CRYPTOPP_DLL MontgomeryRepresentation : public ModularArithmetic
{
public:
	MontgomeryRepresentation(const Integer &modulus);	// modulus must be odd

	virtual ModularArithmetic * Clone() const {return new MontgomeryRepresentation(*this);}

	bool IsMontgomeryRepresentation() const {return true;}

	Integer ConvertIn(const Integer &a) const
		{return (a<<(WORD_BITS*modulus.reg.size()))%modulus;}

	Integer ConvertOut(const Integer &a) const;

	const Integer& MultiplicativeIdentity() const
		{return result1 = Integer::Power2(WORD_BITS*modulus.reg.size())%modulus;}

	const Integer& Multiply(const Integer &a, const Integer &b) const;

	const Integer& Square(const Integer &a) const;

	const Integer& MultiplicativeInverse(const Integer &a) const;

	Integer CascadeExponentiate(const Integer &x, const Integer &e1, const Integer &y, const Integer &e2) const
		{return AbstractRing<Integer>::CascadeExponentiate(x, e1, y, e2);}

	void SimultaneousExponentiate(Element *results, const Element &base, const Integer *exponents, unsigned int exponentsCount) const
		{AbstractRing<Integer>::SimultaneousExponentiate(results, base, exponents, exponentsCount);}

private:
	Integer u;
	mutable SecAlignedWordBlock workspace;
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// simple.h - written and placed in the public domain by Wei Dai
/*! \file
 	Simple non-interface classes derived from classes in cryptlib.h.
*/

#ifndef CRYPTOPP_SIMPLE_H
#define CRYPTOPP_SIMPLE_H

//- #include "cryptlib.h"
//- #include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

//! _
template <class DERIVED, class BASE>
class CRYPTOPP_NO_VTABLE ClonableImpl : public BASE
{
public:
	Clonable * Clone() const {return new DERIVED(*static_cast<const DERIVED *>(this));}
};

//! _
template <class BASE, class ALGORITHM_INFO=BASE>
class CRYPTOPP_NO_VTABLE AlgorithmImpl : public BASE
{
public:
	static std::string StaticAlgorithmName() {return ALGORITHM_INFO::StaticAlgorithmName();}
	std::string AlgorithmName() const {return ALGORITHM_INFO::StaticAlgorithmName();}
};

//! _
class CRYPTOPP_DLL InvalidKeyLength : public InvalidArgument
{
public:
	explicit InvalidKeyLength(const std::string &algorithm, unsigned int length) : InvalidArgument(algorithm + ": " + IntToString(length) + " is not a valid key length") {}
};

//! _
// TODO: look into this virtual inheritance
class CRYPTOPP_DLL ASN1CryptoMaterial : virtual public ASN1Object, virtual public CryptoMaterial
{
public:
	void Save(BufferedTransformation &bt) const
		{BEREncode(bt);}
	void Load(BufferedTransformation &bt)
		{BERDecode(bt);}
};

// *****************************

//! _
template <class T>
class CRYPTOPP_NO_VTABLE Bufferless : public T
{
public:
	bool IsolatedFlush(bool /* hardFlush */, bool /* blocking */) {return false;}
};

//! _
template <class T>
class CRYPTOPP_NO_VTABLE Unflushable : public T
{
public:
	bool Flush(bool completeFlush, int propagation=-1, bool blocking=true)
		{return ChannelFlush(this->NULL_CHANNEL, completeFlush, propagation, blocking);}
	bool IsolatedFlush(bool /* hardFlush */, bool /* blocking */)
		{assert(false); return false;}
	bool ChannelFlush(const std::string &channel, bool hardFlush, int propagation=-1, bool blocking=true)
	{
		if (hardFlush && !InputBufferIsEmpty())
			throw CannotFlush("Unflushable<T>: this object has buffered input that cannot be flushed");
		else 
		{
			BufferedTransformation *attached = this->AttachedTransformation();
			return attached && propagation ? attached->ChannelFlush(channel, hardFlush, propagation-1, blocking) : false;
		}
	}

protected:
	virtual bool InputBufferIsEmpty() const {return false;}
};

//! _
template <class T>
class CRYPTOPP_NO_VTABLE InputRejecting : public T
{
public:
	struct InputRejected : public NotImplemented
		{InputRejected() : NotImplemented("BufferedTransformation: this object doesn't allow input") {}};

	// shouldn't be calling these functions on this class
	unsigned int Put2(const byte* /* begin */, unsigned int /* length */, int /* messageEnd */, bool /* blocking */)
		{throw InputRejected();}
	bool IsolatedFlush(bool, bool) {return false;}
	bool IsolatedMessageSeriesEnd(bool) {throw InputRejected();}

	unsigned int ChannelPut2(const std::string& /* channel */, const byte* /* begin */, unsigned int /* length */, int /* messageEnd */, bool /* blocking */)
		{throw InputRejected();}
	bool ChannelMessageSeriesEnd(const std::string &, int, bool) {throw InputRejected();}
};

//! _
template <class T>
class CRYPTOPP_NO_VTABLE CustomFlushPropagation : public T
{
public:
	virtual bool Flush(bool hardFlush, int propagation=-1, bool blocking=true) =0;

private:
	bool IsolatedFlush(bool /* hardFlush */, bool /* blocking */) {assert(false); return false;}
};

//! _
template <class T>
class CRYPTOPP_NO_VTABLE CustomSignalPropagation : public CustomFlushPropagation<T>
{
public:
	virtual void Initialize(const NameValuePairs &parameters=g_nullNameValuePairs, int propagation=-1) =0;

private:
	void IsolatedInitialize(const NameValuePairs& /* parameters */) {assert(false);}
};

//! _
template <class T>
class CRYPTOPP_NO_VTABLE AutoSignaling : public T
{
public:
	AutoSignaling(int propagation=-1) : m_autoSignalPropagation(propagation) {}

	void SetAutoSignalPropagation(int propagation)
		{m_autoSignalPropagation = propagation;}
	int GetAutoSignalPropagation() const
		{return m_autoSignalPropagation;}

private:
	int m_autoSignalPropagation;
};

//! A BufferedTransformation that only contains pre-existing data as "output"
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Store : public AutoSignaling<InputRejecting<BufferedTransformation> >
{
public:
	Store() : m_messageEnd(false) {}

	void IsolatedInitialize(const NameValuePairs &parameters)
	{
		m_messageEnd = false;
		StoreInitialize(parameters);
	}

	unsigned int NumberOfMessages() const {return m_messageEnd ? 0 : 1;}
	bool GetNextMessage();
	unsigned int CopyMessagesTo(BufferedTransformation &target, unsigned int count=UINT_MAX, const std::string &channel=NULL_CHANNEL) const;

protected:
	virtual void StoreInitialize(const NameValuePairs &parameters) =0;

	bool m_messageEnd;
};

//! A BufferedTransformation that doesn't produce any retrievable output
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Sink : public BufferedTransformation
{
protected:
	// make these functions protected to help prevent unintentional calls to them
	BufferedTransformation::Get;
	BufferedTransformation::Peek;
	BufferedTransformation::TransferTo;
	BufferedTransformation::CopyTo;
	BufferedTransformation::CopyRangeTo;
	BufferedTransformation::TransferMessagesTo;
	BufferedTransformation::CopyMessagesTo;
	BufferedTransformation::TransferAllTo;
	BufferedTransformation::CopyAllTo;
	unsigned int TransferTo2(BufferedTransformation& /* target */, unsigned long &transferBytes, const std::string& /* channel */ = NULL_CHANNEL, bool /* blocking */ = true)
		{transferBytes = 0; return 0;}
	unsigned int CopyRangeTo2(BufferedTransformation& /* target */, unsigned long& /* begin */, unsigned long /* end */ = ULONG_MAX, const std::string& /* channel */ = NULL_CHANNEL, bool /* blocking */ = true) const
		{return 0;}
};

class CRYPTOPP_DLL BitBucket : public Bufferless<Sink>
{
public:
	std::string AlgorithmName() const {return "BitBucket";}
	void IsolatedInitialize(const NameValuePairs& /* parameters */) {}
	unsigned int Put2(const byte* /* begin */, unsigned int /* length */, int /* messageEnd */, bool /* blocking */)
		{return 0;}
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// specification file for an unlimited queue for storing bytes

#ifndef CRYPTOPP_QUEUE_H
#define CRYPTOPP_QUEUE_H

//- #include "simple.h"
//#include <algorithm>

NAMESPACE_BEGIN(CryptoPP)

/** The queue is implemented as a linked list of byte arrays, but you don't need to
    know about that.  So just ignore this next line. :) */
class ByteQueueNode;

//! Byte Queue
class CRYPTOPP_DLL ByteQueue : public Bufferless<BufferedTransformation>
{
public:
	ByteQueue(unsigned int nodeSize=0);
	ByteQueue(const ByteQueue &copy);
	~ByteQueue();

	unsigned long MaxRetrievable() const
		{return CurrentSize();}
	bool AnyRetrievable() const
		{return !IsEmpty();}

	void IsolatedInitialize(const NameValuePairs &parameters);
	byte * CreatePutSpace(unsigned int &size);
	unsigned int Put2(const byte *inString, unsigned int length, int messageEnd, bool blocking);

	unsigned int Get(byte &outByte);
	unsigned int Get(byte *outString, unsigned int getMax);

	unsigned int Peek(byte &outByte) const;
	unsigned int Peek(byte *outString, unsigned int peekMax) const;

	unsigned int TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel=NULL_CHANNEL, bool blocking=true);
	unsigned int CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end=ULONG_MAX, const std::string &channel=NULL_CHANNEL, bool blocking=true) const;

	// these member functions are not inherited
	void SetNodeSize(unsigned int nodeSize);

	unsigned long CurrentSize() const;
	bool IsEmpty() const;

	void Clear();

	void Unget(byte inByte);
	void Unget(const byte *inString, unsigned int length);

	const byte * Spy(unsigned int &contiguousSize) const;

	void LazyPut(const byte *inString, unsigned int size);
	void LazyPutModifiable(byte *inString, unsigned int size);
	void UndoLazyPut(unsigned int size);
	void FinalizeLazyPut();

	ByteQueue & operator=(const ByteQueue &rhs);
	bool operator==(const ByteQueue &rhs) const;
	byte operator[](unsigned long i) const;
	void swap(ByteQueue &rhs);

	class Walker : public InputRejecting<BufferedTransformation>
	{
	public:
		Walker(const ByteQueue &queue)
			: m_queue(queue) {Initialize();}

		unsigned long GetCurrentPosition() {return m_position;}

		unsigned long MaxRetrievable() const
			{return m_queue.CurrentSize() - m_position;}

		void IsolatedInitialize(const NameValuePairs &parameters);

		unsigned int Get(byte &outByte);
		unsigned int Get(byte *outString, unsigned int getMax);

		unsigned int Peek(byte &outByte) const;
		unsigned int Peek(byte *outString, unsigned int peekMax) const;

		unsigned int TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel=NULL_CHANNEL, bool blocking=true);
		unsigned int CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end=ULONG_MAX, const std::string &channel=NULL_CHANNEL, bool blocking=true) const;

	private:
		const ByteQueue &m_queue;
		const ByteQueueNode *m_node;
		unsigned long m_position;
		unsigned int m_offset;
		const byte *m_lazyString;
		unsigned int m_lazyLength;
	};

	friend class Walker;

private:
	void CleanupUsedNodes();
	void CopyFrom(const ByteQueue &copy);
	void Destroy();

	bool m_autoNodeSize;
	unsigned int m_nodeSize;
	ByteQueueNode *m_head, *m_tail;
	byte *m_lazyString;
	unsigned int m_lazyLength;
	bool m_lazyStringModifiable;
};

NAMESPACE_END

NAMESPACE_BEGIN(std)
template<> inline void swap(CryptoPP::ByteQueue &a, CryptoPP::ByteQueue &b)
{
	a.swap(b);
}
NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_ALGPARAM_H
#define CRYPTOPP_ALGPARAM_H

//- #include "cryptlib.h"
//- #include "smartptr.h"
//- #include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! used to pass byte array input as part of a NameValuePairs object
/*! the deepCopy option is used when the NameValuePairs object can't
	keep a copy of the data available */
class ConstByteArrayParameter
{
public:
	ConstByteArrayParameter(const char *data = NULL, bool deepCopy = false)
	{
		Assign((const byte *)data, data ? strlen(data) : 0, deepCopy);
	}
	ConstByteArrayParameter(const byte *data, unsigned int datasize, bool deepCopy = false)
	{
		Assign(data, datasize, deepCopy);
	}
	template <class T> ConstByteArrayParameter(const T &string, bool deepCopy = false)
	{
        CRYPTOPP_COMPILE_ASSERT(sizeof(CPP_TYPENAME T::value_type) == 1);
		Assign((const byte *)string.data(), string.size(), deepCopy);
	}

	void Assign(const byte *data, unsigned int datasize, bool deepCopy)
	{
		if (deepCopy)
			m_block.Assign(data, datasize);
		else
		{
			m_data = data;
			m_size = datasize;
		}
		m_deepCopy = deepCopy;
	}

	const byte *begin() const {return m_deepCopy ? m_block.begin() : m_data;}
	const byte *end() const {return m_deepCopy ? m_block.end() : m_data + m_size;}
	unsigned int size() const {return m_deepCopy ? m_block.size() : m_size;}

private:
	bool m_deepCopy;
	const byte *m_data;
	unsigned int m_size;
	SecByteBlock m_block;
};

class ByteArrayParameter
{
public:
	ByteArrayParameter(byte *data = NULL, unsigned int datasize = 0)
		: m_data(data), m_size(datasize) {}
	ByteArrayParameter(SecByteBlock &block)
		: m_data(block.begin()), m_size(block.size()) {}

	byte *begin() const {return m_data;}
	byte *end() const {return m_data + m_size;}
	unsigned int size() const {return m_size;}

private:
	byte *m_data;
	unsigned int m_size;
};

class CRYPTOPP_DLL CombinedNameValuePairs : public NameValuePairs
{
public:
	CombinedNameValuePairs(const NameValuePairs &pairs1, const NameValuePairs &pairs2)
		: m_pairs1(pairs1), m_pairs2(pairs2) {}

	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;

private:
	const NameValuePairs &m_pairs1, &m_pairs2;
};

template <class T, class BASE>
class GetValueHelperClass
{
public:
	GetValueHelperClass(const T *pObject, const char *name, const std::type_info &valueType, void *pValue, const NameValuePairs *searchFirst)
		: m_pObject(pObject), m_name(name), m_valueType(&valueType), m_pValue(pValue), m_found(false), m_getValueNames(false)
	{
		if (strcmp(m_name, "ValueNames") == 0)
		{
			m_found = m_getValueNames = true;
			NameValuePairs::ThrowIfTypeMismatch(m_name, typeid(std::string), *m_valueType);
			if (searchFirst)
				searchFirst->GetVoidValue(m_name, valueType, pValue);
			if (typeid(T) != typeid(BASE))
				pObject->BASE::GetVoidValue(m_name, valueType, pValue);
			((*reinterpret_cast<std::string *>(m_pValue) += "ThisPointer:") += typeid(T).name()) += ';';
		}

		if (!m_found && strncmp(m_name, "ThisPointer:", 12) == 0 && strcmp(m_name+12, typeid(T).name()) == 0)
		{
			NameValuePairs::ThrowIfTypeMismatch(m_name, typeid(T *), *m_valueType);
			*reinterpret_cast<const T **>(pValue) = pObject;
			m_found = true;
			return;
		}

		if (!m_found && searchFirst)
			m_found = searchFirst->GetVoidValue(m_name, valueType, pValue);
		
		if (!m_found && typeid(T) != typeid(BASE))
			m_found = pObject->BASE::GetVoidValue(m_name, valueType, pValue);
	}

	operator bool() const {return m_found;}

	template <class R>
	GetValueHelperClass<T,BASE> & operator()(const char *name, const R & (T::*pm)() const)
	{
		if (m_getValueNames)
			(*reinterpret_cast<std::string *>(m_pValue) += name) += ";";
		if (!m_found && strcmp(name, m_name) == 0)
		{
			NameValuePairs::ThrowIfTypeMismatch(name, typeid(R), *m_valueType);
			*reinterpret_cast<R *>(m_pValue) = (m_pObject->*pm)();
			m_found = true;
		}
		return *this;
	}

	GetValueHelperClass<T,BASE> &Assignable()
	{
		if (m_getValueNames)
			((*reinterpret_cast<std::string *>(m_pValue) += "ThisObject:") += typeid(T).name()) += ';';
		if (!m_found && strncmp(m_name, "ThisObject:", 11) == 0 && strcmp(m_name+11, typeid(T).name()) == 0)
		{
			NameValuePairs::ThrowIfTypeMismatch(m_name, typeid(T), *m_valueType);
			*reinterpret_cast<T *>(m_pValue) = *m_pObject;
			m_found = true;
		}
		return *this;
	}

private:
	const T *m_pObject;
	const char *m_name;
	const std::type_info *m_valueType;
	void *m_pValue;
	bool m_found, m_getValueNames;
};

template <class BASE, class T>
GetValueHelperClass<T, BASE> GetValueHelper(const T *pObject, const char *name, const std::type_info &valueType, void *pValue, const NameValuePairs *searchFirst=NULL, BASE* /* dummy */ = NULL)
{
	return GetValueHelperClass<T, BASE>(pObject, name, valueType, pValue, searchFirst);
}

template <class T>
GetValueHelperClass<T, T> GetValueHelper(const T *pObject, const char *name, const std::type_info &valueType, void *pValue, const NameValuePairs *searchFirst=NULL)
{
	return GetValueHelperClass<T, T>(pObject, name, valueType, pValue, searchFirst);
}

// ********************************************************

template <class R>
R Hack_DefaultValueFromConstReferenceType(const R &)
{
	return R();
}

template <class R>
bool Hack_GetValueIntoConstReference(const NameValuePairs &source, const char *name, const R &value)
{
	return source.GetValue(name, const_cast<R &>(value));
}

template <class T, class BASE>
class AssignFromHelperClass
{
public:
	AssignFromHelperClass(T *pObject, const NameValuePairs &source)
		: m_pObject(pObject), m_source(source), m_done(false)
	{
		if (source.GetThisObject(*pObject))
			m_done = true;
		else if (typeid(BASE) != typeid(T))
			pObject->BASE::AssignFrom(source);
	}

	template <class R>
	AssignFromHelperClass & operator()(const char *name, void (T::*pm)(R))	// VC60 workaround: "const R &" here causes compiler error
	{
		if (!m_done)
		{
			R value = Hack_DefaultValueFromConstReferenceType(reinterpret_cast<R>(*(int *)NULL));
			if (!Hack_GetValueIntoConstReference(m_source, name, value))
				throw InvalidArgument(std::string(typeid(T).name()) + ": Missing required parameter '" + name + "'");
			(m_pObject->*pm)(value);
		}
		return *this;
	}

	template <class R, class S>
	AssignFromHelperClass & operator()(const char *name1, const char *name2, void (T::*pm)(R, S))	// VC60 workaround: "const R &" here causes compiler error
	{
		if (!m_done)
		{
			R value1 = Hack_DefaultValueFromConstReferenceType(reinterpret_cast<R>(*(int *)NULL));
			if (!Hack_GetValueIntoConstReference(m_source, name1, value1))
				throw InvalidArgument(std::string(typeid(T).name()) + ": Missing required parameter '" + name1 + "'");
			S value2 = Hack_DefaultValueFromConstReferenceType(reinterpret_cast<S>(*(int *)NULL));
			if (!Hack_GetValueIntoConstReference(m_source, name2, value2))
				throw InvalidArgument(std::string(typeid(T).name()) + ": Missing required parameter '" + name2 + "'");
			(m_pObject->*pm)(value1, value2);
		}
		return *this;
	}

private:
	T *m_pObject;
	const NameValuePairs &m_source;
	bool m_done;
};

template <class BASE, class T>
AssignFromHelperClass<T, BASE> AssignFromHelper(T *pObject, const NameValuePairs &source, BASE* /* dummy */ = NULL)
{
	return AssignFromHelperClass<T, BASE>(pObject, source);
}

template <class T>
AssignFromHelperClass<T, T> AssignFromHelper(T *pObject, const NameValuePairs &source)
{
	return AssignFromHelperClass<T, T>(pObject, source);
}

// ********************************************************

// This should allow the linker to discard Integer code if not needed.
CRYPTOPP_DLL extern bool (*AssignIntToInteger)(const std::type_info &valueType, void *pInteger, const void *pInt);

CRYPTOPP_DLL const std::type_info & IntegerTypeId();

class CRYPTOPP_DLL AlgorithmParametersBase : public NameValuePairs
{
public:
	class ParameterNotUsed : public Exception
	{
	public: 
		ParameterNotUsed(const char *name) : Exception(OTHER_ERROR, std::string("AlgorithmParametersBase: parameter \"") + name + "\" not used") {}
	};

	AlgorithmParametersBase(const char *name, bool throwIfNotUsed)
		: m_name(name), m_throwIfNotUsed(throwIfNotUsed), m_used(false) {}

	~AlgorithmParametersBase()
	{
#ifdef CRYPTOPP_UNCAUGHT_EXCEPTION_AVAILABLE
		if (!std::uncaught_exception())
#else
		try
#endif
		{
			if (m_throwIfNotUsed && !m_used)
				throw ParameterNotUsed(m_name);
		}
#ifndef CRYPTOPP_UNCAUGHT_EXCEPTION_AVAILABLE
		catch(...)
		{
		}
#endif
	}

	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;

protected:
	virtual void AssignValue(const char *name, const std::type_info &valueType, void *pValue) const =0;
	virtual const NameValuePairs & GetParent() const =0;

	const char *m_name;
	bool m_throwIfNotUsed;
	mutable bool m_used;
};

template <class T>
class AlgorithmParametersBase2 : public AlgorithmParametersBase
{
public:
	AlgorithmParametersBase2(const char *name, const T &value, bool throwIfNotUsed) : AlgorithmParametersBase(name, throwIfNotUsed), m_value(value) {}

	void AssignValue(const char *name, const std::type_info &valueType, void *pValue) const
	{
		// special case for retrieving an Integer parameter when an int was passed in
		if (!(AssignIntToInteger != NULL && typeid(T) == typeid(int) && AssignIntToInteger(valueType, pValue, &m_value)))
		{
			ThrowIfTypeMismatch(name, typeid(T), valueType);
			*reinterpret_cast<T *>(pValue) = m_value;
		}
	}

protected:
	T m_value;
};

template <class PARENT, class T>
class AlgorithmParameters : public AlgorithmParametersBase2<T>
{
public:
	AlgorithmParameters(const PARENT &parent, const char *name, const T &value, bool throwIfNotUsed)
		: AlgorithmParametersBase2<T>(name, value, throwIfNotUsed), m_parent(parent)
	{}

	AlgorithmParameters(const AlgorithmParameters &copy)
		: AlgorithmParametersBase2<T>(copy), m_parent(copy.m_parent)
	{
		copy.m_used = true;
	}

	template <class R>
	AlgorithmParameters<AlgorithmParameters<PARENT,T>, R> operator()(const char *name, const R &value) const
	{
		return AlgorithmParameters<AlgorithmParameters<PARENT,T>, R>(*this, name, value, this->m_throwIfNotUsed);
	}

	template <class R>
	AlgorithmParameters<AlgorithmParameters<PARENT,T>, R> operator()(const char *name, const R &value, bool throwIfNotUsed) const
	{
		return AlgorithmParameters<AlgorithmParameters<PARENT,T>, R>(*this, name, value, throwIfNotUsed);
	}

private:
	const NameValuePairs & GetParent() const {return m_parent;}
	PARENT m_parent;
};

//! Create an object that implements NameValuePairs for passing parameters
/*! \param throwIfNotUsed if true, the object will throw an exception if the value is not accessed
	\note throwIfNotUsed is ignored if using a compiler that does not support std::uncaught_exception(),
	such as MSVC 7.0 and earlier.
	\note A NameValuePairs object containing an arbitrary number of name value pairs may be constructed by
	repeatedly using operator() on the object returned by MakeParameters, for example:
	const NameValuePairs &parameters = MakeParameters(name1, value1)(name2, value2)(name3, value3);
*/
template <class T>
AlgorithmParameters<NullNameValuePairs,T> MakeParameters(const char *name, const T &value, bool throwIfNotUsed = true)
{
	return AlgorithmParameters<NullNameValuePairs,T>(g_nullNameValuePairs, name, value, throwIfNotUsed);
}

#define CRYPTOPP_GET_FUNCTION_ENTRY(name)		(Name::name(), &ThisClass::Get##name)
#define CRYPTOPP_SET_FUNCTION_ENTRY(name)		(Name::name(), &ThisClass::Set##name)
#define CRYPTOPP_SET_FUNCTION_ENTRY2(name1, name2)	(Name::name1(), Name::name2(), &ThisClass::Set##name1##And##name2)

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_FILTERS_H
#define CRYPTOPP_FILTERS_H

//- #include "simple.h"
//- #include "secblock.h"
//- #include "misc.h"
//- #include "smartptr.h"
//- #include "queue.h"
//- #include "algparam.h"

NAMESPACE_BEGIN(CryptoPP)

/// provides an implementation of BufferedTransformation's attachment interface
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Filter : public BufferedTransformation, public NotCopyable
{
public:
	Filter(BufferedTransformation *attachment = NULL);

	bool Attachable() {return true;}
	BufferedTransformation *AttachedTransformation();
	const BufferedTransformation *AttachedTransformation() const;
	void Detach(BufferedTransformation *newAttachment = NULL);

	unsigned int TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel=NULL_CHANNEL, bool blocking=true);
	unsigned int CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end=ULONG_MAX, const std::string &channel=NULL_CHANNEL, bool blocking=true) const;

	void Initialize(const NameValuePairs &parameters=g_nullNameValuePairs, int propagation=-1);
	bool Flush(bool hardFlush, int propagation=-1, bool blocking=true);
	bool MessageSeriesEnd(int propagation=-1, bool blocking=true);

protected:
	virtual BufferedTransformation * NewDefaultAttachment() const;
	void Insert(Filter *nextFilter);	// insert filter after this one

	virtual bool ShouldPropagateMessageEnd() const {return true;}
	virtual bool ShouldPropagateMessageSeriesEnd() const {return true;}

	void PropagateInitialize(const NameValuePairs &parameters, int propagation);

	unsigned int Output(int outputSite, const byte *inString, unsigned int length, int messageEnd, bool blocking, const std::string &channel=NULL_CHANNEL);
	unsigned int OutputModifiable(int outputSite, byte *inString, unsigned int length, int messageEnd, bool blocking, const std::string &channel=NULL_CHANNEL);
	bool OutputMessageEnd(int outputSite, int propagation, bool blocking, const std::string &channel=NULL_CHANNEL);
	bool OutputFlush(int outputSite, bool hardFlush, int propagation, bool blocking, const std::string &channel=NULL_CHANNEL);
	bool OutputMessageSeriesEnd(int outputSite, int propagation, bool blocking, const std::string &channel=NULL_CHANNEL);

private:
	member_ptr<BufferedTransformation> m_attachment;
	
protected:
	unsigned int m_inputPosition;
	int m_continueAt;
};

struct CRYPTOPP_DLL FilterPutSpaceHelper
{
	// desiredSize is how much to ask target, bufferSize is how much to allocate in m_tempSpace
	byte *HelpCreatePutSpace(BufferedTransformation &target, const std::string &channel, unsigned int minSize, unsigned int desiredSize, unsigned int &bufferSize)
	{
		assert(desiredSize >= minSize && bufferSize >= minSize);
		if (m_tempSpace.size() < minSize)
		{
			byte *result = target.ChannelCreatePutSpace(channel, desiredSize);
			if (desiredSize >= minSize)
			{
				bufferSize = desiredSize;
				return result;
			}
			m_tempSpace.New(bufferSize);
		}

		bufferSize = m_tempSpace.size();
		return m_tempSpace.begin();
	}
	byte *HelpCreatePutSpace(BufferedTransformation &target, const std::string &channel, unsigned int minSize)
		{return HelpCreatePutSpace(target, channel, minSize, minSize, minSize);}
	byte *HelpCreatePutSpace(BufferedTransformation &target, const std::string &channel, unsigned int minSize, unsigned int bufferSize)
		{return HelpCreatePutSpace(target, channel, minSize, minSize, bufferSize);}
	SecByteBlock m_tempSpace;
};

/*! FilterWithBufferedInput divides up the input stream into
	a first block, a number of middle blocks, and a last block.
	First and last blocks are optional, and middle blocks may
	be a stream instead (i.e. blockSize == 1).
*/
class CRYPTOPP_DLL FilterWithBufferedInput : public Filter
{
public:
	FilterWithBufferedInput(BufferedTransformation *attachment);
	//! firstSize and lastSize may be 0, blockSize must be at least 1
	FilterWithBufferedInput(unsigned int firstSize, unsigned int blockSize, unsigned int lastSize, BufferedTransformation *attachment);

	void IsolatedInitialize(const NameValuePairs &parameters);
	unsigned int Put2(const byte *inString, unsigned int length, int messageEnd, bool blocking)
	{
		return PutMaybeModifiable(const_cast<byte *>(inString), length, messageEnd, blocking, false);
	}
	unsigned int PutModifiable2(byte *inString, unsigned int length, int messageEnd, bool blocking)
	{
		return PutMaybeModifiable(inString, length, messageEnd, blocking, true);
	}
	/*! calls ForceNextPut() if hardFlush is true */
	bool IsolatedFlush(bool hardFlush, bool blocking);

	/*! The input buffer may contain more than blockSize bytes if lastSize != 0.
		ForceNextPut() forces a call to NextPut() if this is the case.
	*/
	void ForceNextPut();

protected:
	bool DidFirstPut() {return m_firstInputDone;}

	virtual void InitializeDerivedAndReturnNewSizes(const NameValuePairs& parameters, unsigned int& /* firstSize */, unsigned int& /* blockSize */, unsigned int& /* lastSize */)
		{InitializeDerived(parameters);}
	virtual void InitializeDerived(const NameValuePairs& /* parameters */) {}
	// FirstPut() is called if (firstSize != 0 and totalLength >= firstSize)
	// or (firstSize == 0 and (totalLength > 0 or a MessageEnd() is received))
	virtual void FirstPut(const byte *inString) =0;
	// NextPut() is called if totalLength >= firstSize+blockSize+lastSize
	virtual void NextPutSingle(const byte* /* inString */) {assert(false);}
	// Same as NextPut() except length can be a multiple of blockSize
	// Either NextPut() or NextPutMultiple() must be overriden
	virtual void NextPutMultiple(const byte *inString, unsigned int length);
	// Same as NextPutMultiple(), but inString can be modified
	virtual void NextPutModifiable(byte *inString, unsigned int length)
		{NextPutMultiple(inString, length);}
	// LastPut() is always called
	// if totalLength < firstSize then length == totalLength
	// else if totalLength <= firstSize+lastSize then length == totalLength-firstSize
	// else lastSize <= length < lastSize+blockSize
	virtual void LastPut(const byte *inString, unsigned int length) =0;
	virtual void FlushDerived() {}

private:
	unsigned int PutMaybeModifiable(byte *begin, unsigned int length, int messageEnd, bool blocking, bool modifiable);
	void NextPutMaybeModifiable(byte *inString, unsigned int length, bool modifiable)
	{
		if (modifiable) NextPutModifiable(inString, length);
		else NextPutMultiple(inString, length);
	}

	// This function should no longer be used, put this here to cause a compiler error
	// if someone tries to override NextPut().
	virtual int NextPut(const byte* /* inString */, unsigned int /* length */) {assert(false); return 0;}

	class BlockQueue
	{
	public:
		void ResetQueue(unsigned int blockSize, unsigned int maxBlocks);
		byte *GetBlock();
		byte *GetContigousBlocks(unsigned int &numberOfBytes);
		unsigned int GetAll(byte *outString);
		void Put(const byte *inString, unsigned int length);
		unsigned int CurrentSize() const {return m_size;}
		unsigned int MaxSize() const {return m_buffer.size();}

	private:
		SecByteBlock m_buffer;
		unsigned int m_blockSize, m_maxBlocks, m_size;
		byte *m_begin;
	};

	unsigned int m_firstSize, m_blockSize, m_lastSize;
	bool m_firstInputDone;
	BlockQueue m_queue;
};

//! Filter Wrapper for HashTransformation
class CRYPTOPP_DLL HashFilter : public Bufferless<Filter>, private FilterPutSpaceHelper
{
public:
	HashFilter(HashTransformation &hm, BufferedTransformation *attachment = NULL, bool putMessage=false)
		: m_hashModule(hm), m_putMessage(putMessage) {Detach(attachment);}

	void IsolatedInitialize(const NameValuePairs &parameters);
	unsigned int Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking);

	byte * CreatePutSpace(unsigned int &size) {return m_hashModule.CreateUpdateSpace(size);}

private:
	HashTransformation &m_hashModule;
	bool m_putMessage;
	byte *m_space;
};

// Used By ProxyFilter
class CRYPTOPP_DLL OutputProxy : public CustomSignalPropagation<Sink>
{
public:
	OutputProxy(BufferedTransformation &owner, bool passSignal) : m_owner(owner), m_passSignal(passSignal) {}

	bool GetPassSignal() const {return m_passSignal;}
	void SetPassSignal(bool passSignal) {m_passSignal = passSignal;}

	byte * CreatePutSpace(unsigned int &size)
		{return m_owner.AttachedTransformation()->CreatePutSpace(size);}
	unsigned int Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking)
		{return m_owner.AttachedTransformation()->Put2(begin, length, m_passSignal ? messageEnd : 0, blocking);}
	unsigned int PutModifiable2(byte *begin, unsigned int length, int messageEnd, bool blocking)
		{return m_owner.AttachedTransformation()->PutModifiable2(begin, length, m_passSignal ? messageEnd : 0, blocking);}
	void Initialize(const NameValuePairs &parameters=g_nullNameValuePairs, int propagation=-1)
		{if (m_passSignal) m_owner.AttachedTransformation()->Initialize(parameters, propagation);}
	bool Flush(bool hardFlush, int propagation=-1, bool blocking=true)
		{return m_passSignal ? m_owner.AttachedTransformation()->Flush(hardFlush, propagation, blocking) : false;}
	bool MessageSeriesEnd(int propagation=-1, bool blocking=true)
		{return m_passSignal ? m_owner.AttachedTransformation()->MessageSeriesEnd(propagation, blocking) : false;}

	unsigned int ChannelPut2(const std::string &channel, const byte *begin, unsigned int length, int messageEnd, bool blocking)
		{return m_owner.AttachedTransformation()->ChannelPut2(channel, begin, length, m_passSignal ? messageEnd : 0, blocking);}
	unsigned int ChannelPutModifiable2(const std::string &channel, byte *begin, unsigned int length, int messageEnd, bool blocking)
		{return m_owner.AttachedTransformation()->ChannelPutModifiable2(channel, begin, length, m_passSignal ? messageEnd : 0, blocking);}
	bool ChannelFlush(const std::string &channel, bool completeFlush, int propagation=-1, bool blocking=true)
		{return m_passSignal ? m_owner.AttachedTransformation()->ChannelFlush(channel, completeFlush, propagation, blocking) : false;}
	bool ChannelMessageSeriesEnd(const std::string &channel, int propagation=-1, bool blocking=true)
		{return m_passSignal ? m_owner.AttachedTransformation()->ChannelMessageSeriesEnd(channel, propagation, blocking) : false;}

private:
	BufferedTransformation &m_owner;
	bool m_passSignal;
};

//! Base class for Filter classes that are proxies for a chain of other filters.
class CRYPTOPP_DLL ProxyFilter : public FilterWithBufferedInput
{
public:
	ProxyFilter(BufferedTransformation *filter, unsigned int firstSize, unsigned int lastSize, BufferedTransformation *attachment);

	bool IsolatedFlush(bool hardFlush, bool blocking);

	void SetFilter(Filter *filter);
	void NextPutMultiple(const byte *s, unsigned int len);
	void NextPutModifiable(byte *inString, unsigned int length);

protected:
	member_ptr<BufferedTransformation> m_filter;
};

//! simple proxy filter that doesn't modify the underlying filter's input or output
class CRYPTOPP_DLL SimpleProxyFilter : public ProxyFilter
{
public:
	SimpleProxyFilter(BufferedTransformation *filter, BufferedTransformation *attachment)
		: ProxyFilter(filter, 0, 0, attachment) {}

	void FirstPut(const byte *) {}
	void LastPut(const byte *, unsigned int) {m_filter->MessageEnd();}
};

//! Append input to a string object
template <class T>
class StringSinkTemplate : public Bufferless<Sink>
{
public:
	// VC60 workaround: no T::char_type
	typedef typename T::traits_type::char_type char_type;

	StringSinkTemplate(T &output)
		: m_output(&output) {assert(sizeof(output[0])==1);}

	void IsolatedInitialize(const NameValuePairs &parameters)
		{if (!parameters.GetValue("OutputStringPointer", m_output)) throw InvalidArgument("StringSink: OutputStringPointer not specified");}

	unsigned int Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking)
	{
		if (length > 0)
		{
			typename T::size_type size = m_output->size();
			if (length < size && size + length > m_output->capacity())
				m_output->reserve(2*size);
		m_output->append((const char_type *)begin, (const char_type *)begin+length);
		}
		return 0;
	}

private:	
	T *m_output;
};

//! Append input to an std::string
typedef StringSinkTemplate<std::string> StringSink;

//! Copy input to a memory buffer
class CRYPTOPP_DLL ArraySink : public Bufferless<Sink>
{
public:
	ArraySink(const NameValuePairs &parameters = g_nullNameValuePairs) {IsolatedInitialize(parameters);}
	ArraySink(byte *buf, unsigned int size) : m_buf(buf), m_size(size), m_total(0) {}

	unsigned int AvailableSize() {return m_size - STDMIN(m_total, (unsigned long)m_size);}
	unsigned long TotalPutLength() {return m_total;}

	void IsolatedInitialize(const NameValuePairs &parameters);
	byte * CreatePutSpace(unsigned int &size);
	unsigned int Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking);

protected:
	byte *m_buf;
	unsigned int m_size;
	unsigned long m_total;
};

//! Xor input to a memory buffer
class CRYPTOPP_DLL ArrayXorSink : public ArraySink
{
public:
	ArrayXorSink(byte *buf, unsigned int size)
		: ArraySink(buf, size) {}

	unsigned int Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking);
	byte * CreatePutSpace(unsigned int &size) {return BufferedTransformation::CreatePutSpace(size);}
};

//! string-based implementation of Store interface
class StringStore : public Store
{
public:
	StringStore(const char *string = NULL)
		{StoreInitialize(MakeParameters("InputBuffer", ConstByteArrayParameter(string)));}
	StringStore(const byte *string, unsigned int length)
		{StoreInitialize(MakeParameters("InputBuffer", ConstByteArrayParameter(string, length)));}
	template <class T> StringStore(const T &string)
		{StoreInitialize(MakeParameters("InputBuffer", ConstByteArrayParameter(string)));}

	CRYPTOPP_DLL unsigned int TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel=NULL_CHANNEL, bool blocking=true);
	CRYPTOPP_DLL unsigned int CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end=ULONG_MAX, const std::string &channel=NULL_CHANNEL, bool blocking=true) const;

private:
	CRYPTOPP_DLL void StoreInitialize(const NameValuePairs &parameters);

	const byte *m_store;
	unsigned int m_length, m_count;
};

//! A Filter that pumps data into its attachment as input
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Source : public InputRejecting<Filter>
{
public:
	Source(BufferedTransformation *attachment = NULL)
		{Source::Detach(attachment);}

	unsigned long Pump(unsigned long pumpMax=ULONG_MAX)
		{Pump2(pumpMax); return pumpMax;}
	unsigned int PumpMessages(unsigned int count=UINT_MAX)
		{PumpMessages2(count); return count;}
	void PumpAll()
		{PumpAll2();}
	virtual unsigned int Pump2(unsigned long &byteCount, bool blocking=true) =0;
	virtual unsigned int PumpMessages2(unsigned int &messageCount, bool blocking=true) =0;
	virtual unsigned int PumpAll2(bool blocking=true);
	virtual bool SourceExhausted() const =0;

protected:
	void SourceInitialize(bool pumpAll, const NameValuePairs &parameters)
	{
		IsolatedInitialize(parameters);
		if (pumpAll)
			PumpAll();
	}
};

//! Turn a Store into a Source
template <class T>
class SourceTemplate : public Source
{
public:
	SourceTemplate<T>(BufferedTransformation *attachment)
		: Source(attachment) {}
	void IsolatedInitialize(const NameValuePairs &parameters)
		{m_store.IsolatedInitialize(parameters);}
	unsigned int Pump2(unsigned long &byteCount, bool blocking=true)
		{return m_store.TransferTo2(*AttachedTransformation(), byteCount, NULL_CHANNEL, blocking);}
	unsigned int PumpMessages2(unsigned int &messageCount, bool blocking=true)
		{return m_store.TransferMessagesTo2(*AttachedTransformation(), messageCount, NULL_CHANNEL, blocking);}
	unsigned int PumpAll2(bool blocking=true)
		{return m_store.TransferAllTo2(*AttachedTransformation(), NULL_CHANNEL, blocking);}
	bool SourceExhausted() const
		{return !m_store.AnyRetrievable() && !m_store.AnyMessages();}
	void SetAutoSignalPropagation(int propagation)
		{m_store.SetAutoSignalPropagation(propagation);}
	int GetAutoSignalPropagation() const
		{return m_store.GetAutoSignalPropagation();}

protected:
	T m_store;
};

//! string-based implementation of Source interface
class CRYPTOPP_DLL StringSource : public SourceTemplate<StringStore>
{
public:
	StringSource(BufferedTransformation *attachment = NULL)
		: SourceTemplate<StringStore>(attachment) {}
	StringSource(const char *string, bool pumpAll, BufferedTransformation *attachment = NULL)
		: SourceTemplate<StringStore>(attachment) {SourceInitialize(pumpAll, MakeParameters("InputBuffer", ConstByteArrayParameter(string)));}
	StringSource(const byte *string, unsigned int length, bool pumpAll, BufferedTransformation *attachment = NULL)
		: SourceTemplate<StringStore>(attachment) {SourceInitialize(pumpAll, MakeParameters("InputBuffer", ConstByteArrayParameter(string, length)));}
	StringSource(const std::string &string, bool pumpAll, BufferedTransformation *attachment = NULL)
		: SourceTemplate<StringStore>(attachment) {SourceInitialize(pumpAll, MakeParameters("InputBuffer", ConstByteArrayParameter(string)));}
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_ARGNAMES_H
#define CRYPTOPP_ARGNAMES_H

//- #include "cryptlib.h"

NAMESPACE_BEGIN(CryptoPP)

DOCUMENTED_NAMESPACE_BEGIN(Name)

#define CRYPTOPP_DEFINE_NAME_STRING(name)	inline const char *name() {return #name;}

CRYPTOPP_DEFINE_NAME_STRING(ValueNames)			//!< string, a list of value names with a semicolon (';') after each name
CRYPTOPP_DEFINE_NAME_STRING(Version)			//!< int
CRYPTOPP_DEFINE_NAME_STRING(Seed)				//!< ConstByteArrayParameter
CRYPTOPP_DEFINE_NAME_STRING(Key)				//!< ConstByteArrayParameter
CRYPTOPP_DEFINE_NAME_STRING(IV)					//!< const byte *
CRYPTOPP_DEFINE_NAME_STRING(StolenIV)			//!< byte *
CRYPTOPP_DEFINE_NAME_STRING(Rounds)				//!< int
CRYPTOPP_DEFINE_NAME_STRING(FeedbackSize)		//!< int
CRYPTOPP_DEFINE_NAME_STRING(WordSize)			//!< int, in bytes
CRYPTOPP_DEFINE_NAME_STRING(BlockSize)			//!< int, in bytes
CRYPTOPP_DEFINE_NAME_STRING(EffectiveKeyLength)	//!< int, in bits
CRYPTOPP_DEFINE_NAME_STRING(KeySize)			//!< int, in bits
CRYPTOPP_DEFINE_NAME_STRING(ModulusSize)		//!< int, in bits
CRYPTOPP_DEFINE_NAME_STRING(SubgroupOrderSize)	//!< int, in bits
CRYPTOPP_DEFINE_NAME_STRING(PrivateExponentSize)//!< int, in bits
CRYPTOPP_DEFINE_NAME_STRING(Modulus)			//!< Integer
CRYPTOPP_DEFINE_NAME_STRING(PublicExponent)		//!< Integer
CRYPTOPP_DEFINE_NAME_STRING(PrivateExponent)	//!< Integer
CRYPTOPP_DEFINE_NAME_STRING(PublicElement)		//!< Integer
CRYPTOPP_DEFINE_NAME_STRING(SubgroupOrder)		//!< Integer
CRYPTOPP_DEFINE_NAME_STRING(Cofactor)			//!< Integer
CRYPTOPP_DEFINE_NAME_STRING(SubgroupGenerator)	//!< Integer, ECP::Point, or EC2N::Point
CRYPTOPP_DEFINE_NAME_STRING(Curve)				//!< ECP or EC2N
CRYPTOPP_DEFINE_NAME_STRING(GroupOID)			//!< OID
CRYPTOPP_DEFINE_NAME_STRING(PointerToPrimeSelector)		//!< const PrimeSelector *
CRYPTOPP_DEFINE_NAME_STRING(Prime1)				//!< Integer
CRYPTOPP_DEFINE_NAME_STRING(Prime2)				//!< Integer
CRYPTOPP_DEFINE_NAME_STRING(ModPrime1PrivateExponent)	//!< Integer
CRYPTOPP_DEFINE_NAME_STRING(ModPrime2PrivateExponent)	//!< Integer
CRYPTOPP_DEFINE_NAME_STRING(MultiplicativeInverseOfPrime2ModPrime1)	//!< Integer
CRYPTOPP_DEFINE_NAME_STRING(QuadraticResidueModPrime1)	//!< Integer
CRYPTOPP_DEFINE_NAME_STRING(QuadraticResidueModPrime2)	//!< Integer
CRYPTOPP_DEFINE_NAME_STRING(PutMessage)			//!< bool
CRYPTOPP_DEFINE_NAME_STRING(HashVerificationFilterFlags)		//!< word32
CRYPTOPP_DEFINE_NAME_STRING(SignatureVerificationFilterFlags)	//!< word32
CRYPTOPP_DEFINE_NAME_STRING(InputBuffer)		//!< ConstByteArrayParameter
CRYPTOPP_DEFINE_NAME_STRING(OutputBuffer)		//!< ByteArrayParameter
CRYPTOPP_DEFINE_NAME_STRING(XMACC_Counter)		//!< word32
CRYPTOPP_DEFINE_NAME_STRING(InputFileName)		//!< const char *
CRYPTOPP_DEFINE_NAME_STRING(InputStreamPointer)	//!< std::istream *
CRYPTOPP_DEFINE_NAME_STRING(InputBinaryMode)	//!< bool
CRYPTOPP_DEFINE_NAME_STRING(OutputFileName)		//!< const char *
CRYPTOPP_DEFINE_NAME_STRING(OutputStreamPointer)	//!< std::ostream *
CRYPTOPP_DEFINE_NAME_STRING(OutputBinaryMode)	//!< bool
CRYPTOPP_DEFINE_NAME_STRING(EncodingParameters)	//!< ConstByteArrayParameter
CRYPTOPP_DEFINE_NAME_STRING(KeyDerivationParameters)	//!< ConstByteArrayParameter
CRYPTOPP_DEFINE_NAME_STRING(Separator)			//< ConstByteArrayParameter
CRYPTOPP_DEFINE_NAME_STRING(Terminator)			//< ConstByteArrayParameter
CRYPTOPP_DEFINE_NAME_STRING(Uppercase)			//< bool
CRYPTOPP_DEFINE_NAME_STRING(GroupSize)			//< int
CRYPTOPP_DEFINE_NAME_STRING(Pad)				//< bool
CRYPTOPP_DEFINE_NAME_STRING(PaddingByte)		//< byte
CRYPTOPP_DEFINE_NAME_STRING(Log2Base)			//< int
CRYPTOPP_DEFINE_NAME_STRING(EncodingLookupArray)	//< const byte *
CRYPTOPP_DEFINE_NAME_STRING(DecodingLookupArray)	//< const byte *
CRYPTOPP_DEFINE_NAME_STRING(InsertLineBreaks)	//< bool
CRYPTOPP_DEFINE_NAME_STRING(MaxLineLength)		//< int

DOCUMENTED_NAMESPACE_END

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// pubkey.h - written and placed in the public domain by Wei Dai

#ifndef CRYPTOPP_PUBKEY_H
#define CRYPTOPP_PUBKEY_H

/** \file

	This file contains helper classes/functions for implementing public key algorithms.

	The class hierachies in this .h file tend to look like this:
<pre>
                  x1
                 / \
                y1  z1
                 |  |
            x2<y1>  x2<z1>
                 |  |
                y2  z2
                 |  |
            x3<y2>  x3<z2>
                 |  |
                y3  z3
</pre>
	- x1, y1, z1 are abstract interface classes defined in cryptlib.h
	- x2, y2, z2 are implementations of the interfaces using "abstract policies", which
	  are pure virtual functions that should return interfaces to interchangeable algorithms.
	  These classes have "Base" suffixes.
	- x3, y3, z3 hold actual algorithms and implement those virtual functions.
	  These classes have "Impl" suffixes.

	The "TF_" prefix means an implementation using trapdoor functions on integers.
	The "DL_" prefix means an implementation using group operations (in groups where discrete log is hard).
*/

//- #include "modarith.h"
//- #include "filters.h"
//- #include "eprecomp.h"
//- #include "fips140.h"
//- #include "argnames.h"
#include <memory>

// VC60 workaround: this macro is defined in shlobj.h and conflicts with a template parameter used in this file
#undef INTERFACE

NAMESPACE_BEGIN(CryptoPP)

//! _
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE TrapdoorFunctionBounds
{
public:
	virtual ~TrapdoorFunctionBounds() {}

	virtual Integer PreimageBound() const =0;
	virtual Integer ImageBound() const =0;
	virtual Integer MaxPreimage() const {return --PreimageBound();}
	virtual Integer MaxImage() const {return --ImageBound();}
};

//! _
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE RandomizedTrapdoorFunction : public TrapdoorFunctionBounds
{
public:
	virtual Integer ApplyRandomizedFunction(RandomNumberGenerator &rng, const Integer &x) const =0;
	virtual bool IsRandomized() const {return true;}
};

//! _
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE TrapdoorFunction : public RandomizedTrapdoorFunction
{
public:
	Integer ApplyRandomizedFunction(RandomNumberGenerator& /* rng */, const Integer &x) const
		{return ApplyFunction(x);}
	bool IsRandomized() const {return false;}

	virtual Integer ApplyFunction(const Integer &x) const =0;
};

//! _
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE RandomizedTrapdoorFunctionInverse
{
public:
	virtual ~RandomizedTrapdoorFunctionInverse() {}

	virtual Integer CalculateRandomizedInverse(RandomNumberGenerator &rng, const Integer &x) const =0;
	virtual bool IsRandomized() const {return true;}
};

//! _
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE TrapdoorFunctionInverse : public RandomizedTrapdoorFunctionInverse
{
public:
	virtual ~TrapdoorFunctionInverse() {}

	Integer CalculateRandomizedInverse(RandomNumberGenerator &rng, const Integer &x) const
		{return CalculateInverse(rng, x);}
	bool IsRandomized() const {return false;}

	virtual Integer CalculateInverse(RandomNumberGenerator &rng, const Integer &x) const =0;
};

// ********************************************************

//! message encoding method for public key encryption
class CRYPTOPP_NO_VTABLE PK_EncryptionMessageEncodingMethod
{
public:
	virtual ~PK_EncryptionMessageEncodingMethod() {}

	virtual bool ParameterSupported(const char* /* name */) const {return false;}

	//! max size of unpadded message in bytes, given max size of padded message in bits (1 less than size of modulus)
	virtual unsigned int MaxUnpaddedLength(unsigned int paddedLength) const =0;

	virtual void Pad(RandomNumberGenerator &rng, const byte *raw, unsigned int inputLength, byte *padded, unsigned int paddedBitLength, const NameValuePairs &parameters) const =0;

	virtual DecodingResult Unpad(const byte *padded, unsigned int paddedBitLength, byte *raw, const NameValuePairs &parameters) const =0;
};

// ********************************************************

//! _
template <class TFI, class MEI>
class CRYPTOPP_NO_VTABLE TF_Base
{
protected:
	virtual ~TF_Base() {};
	
	virtual const TrapdoorFunctionBounds & GetTrapdoorFunctionBounds() const =0;

	typedef TFI TrapdoorFunctionInterface;
	virtual const TrapdoorFunctionInterface & GetTrapdoorFunctionInterface() const =0;

	typedef MEI MessageEncodingInterface;
	virtual const MessageEncodingInterface & GetMessageEncodingInterface() const =0;
};

// ********************************************************

typedef std::pair<const byte *, unsigned int> HashIdentifier;

//! interface for message encoding method for public key signature schemes
class CRYPTOPP_NO_VTABLE PK_SignatureMessageEncodingMethod
{
public:
	virtual ~PK_SignatureMessageEncodingMethod() {}

	virtual unsigned int MaxRecoverableLength(unsigned int /* representativeBitLength */, unsigned int /* hashIdentifierLength */, unsigned int /* digestLength */) const
		{return 0;}

	bool IsProbabilistic() const 
		{return true;}
	bool AllowNonrecoverablePart() const
		{throw NotImplemented("PK_MessageEncodingMethod: this signature scheme does not support message recovery");}
	virtual bool RecoverablePartFirst() const
		{throw NotImplemented("PK_MessageEncodingMethod: this signature scheme does not support message recovery");}

	// for verification, DL
	virtual void ProcessSemisignature(HashTransformation& /* hash */, const byte* /* semisignature */, unsigned int /* semisignatureLength */) const {}

	// for signature
	virtual void ProcessRecoverableMessage(HashTransformation& /* hash */, 
		const byte* /* recoverableMessage */, unsigned int /* recoverableMessageLength */, 
		const byte* /* presignature */, unsigned int /* presignatureLength */,
		SecByteBlock& /* semisignature */) const
	{
		if (RecoverablePartFirst())
			assert(!"ProcessRecoverableMessage() not implemented");
	}

	virtual void ComputeMessageRepresentative(RandomNumberGenerator &rng, 
		const byte *recoverableMessage, unsigned int recoverableMessageLength,
		HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
		byte *representative, unsigned int representativeBitLength) const =0;

	virtual bool VerifyMessageRepresentative(
		HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
		byte *representative, unsigned int representativeBitLength) const =0;

	virtual DecodingResult RecoverMessageFromRepresentative(	// for TF
		HashTransformation& /* hash */, HashIdentifier /* hashIdentifier */, bool /* messageEmpty */,
		byte* /* representative */, unsigned int /* representativeBitLength */,
		byte* /* recoveredMessage */) const
		{throw NotImplemented("PK_MessageEncodingMethod: this signature scheme does not support message recovery");}

	virtual DecodingResult RecoverMessageFromSemisignature(		// for DL
		HashTransformation& /* hash */, HashIdentifier /* hashIdentifier */,
		const byte* /* presignature */, unsigned int /* presignatureLength */,
		const byte* /* semisignature */, unsigned int /* semisignatureLength */,
		byte* /* recoveredMessage */) const
		{throw NotImplemented("PK_MessageEncodingMethod: this signature scheme does not support message recovery");}

	// VC60 workaround
	struct HashIdentifierLookup
	{
		template <class H> struct HashIdentifierLookup2
		{
			static HashIdentifier Lookup()
			{
				return HashIdentifier(NULL, 0);
			}
		};
	};
};

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PK_DeterministicSignatureMessageEncodingMethod : public PK_SignatureMessageEncodingMethod
{
public:
	bool VerifyMessageRepresentative(
		HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
		byte *representative, unsigned int representativeBitLength) const;
};

class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PK_MessageAccumulatorBase : public PK_MessageAccumulator
{
public:
	PK_MessageAccumulatorBase() : m_empty(true) {}

	virtual HashTransformation & AccessHash() =0;

	void Update(const byte *input, unsigned int length)
	{
		AccessHash().Update(input, length);
		m_empty = m_empty && length == 0;
	}

	SecByteBlock m_recoverableMessage, m_representative, m_presignature, m_semisignature;
	Integer m_k, m_s;
	bool m_empty;
};

template <class HASH_ALGORITHM>
class PK_MessageAccumulatorImpl : public PK_MessageAccumulatorBase, protected ObjectHolder<HASH_ALGORITHM>
{
public:
	HashTransformation & AccessHash() {return this->m_object;}
};

//! _
template <class INTERFACE, class BASE>
class CRYPTOPP_NO_VTABLE TF_SignatureSchemeBase : public INTERFACE, protected BASE
{
public:
	unsigned int SignatureLength() const 
		{return this->GetTrapdoorFunctionBounds().MaxPreimage().ByteCount();}
	unsigned int MaxRecoverableLength() const 
		{return this->GetMessageEncodingInterface().MaxRecoverableLength(MessageRepresentativeBitLength(), GetHashIdentifier().second, GetDigestSize());}
	unsigned int MaxRecoverableLengthFromSignatureLength(unsigned int /* signatureLength */) const
		{return this->MaxRecoverableLength();}

	bool IsProbabilistic() const 
		{return this->GetTrapdoorFunctionInterface().IsRandomized() || this->GetMessageEncodingInterface().IsProbabilistic();}
	bool AllowNonrecoverablePart() const 
		{return this->GetMessageEncodingInterface().AllowNonrecoverablePart();}
	bool RecoverablePartFirst() const 
		{return this->GetMessageEncodingInterface().RecoverablePartFirst();}

protected:
	unsigned int MessageRepresentativeLength() const {return BitsToBytes(MessageRepresentativeBitLength());}
	unsigned int MessageRepresentativeBitLength() const {return this->GetTrapdoorFunctionBounds().ImageBound().BitCount()-1;}
	virtual HashIdentifier GetHashIdentifier() const =0;
	virtual unsigned int GetDigestSize() const =0;
};

//! _
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE TF_SignerBase : public TF_SignatureSchemeBase<PK_Signer, TF_Base<RandomizedTrapdoorFunctionInverse, PK_SignatureMessageEncodingMethod> >
{
public:
	void InputRecoverableMessage(PK_MessageAccumulator &messageAccumulator, const byte *recoverableMessage, unsigned int recoverableMessageLength) const;
	unsigned int SignAndRestart(RandomNumberGenerator &rng, PK_MessageAccumulator &messageAccumulator, byte *signature, bool restart=true) const;
};

//! _
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE TF_VerifierBase : public TF_SignatureSchemeBase<PK_Verifier, TF_Base<TrapdoorFunction, PK_SignatureMessageEncodingMethod> >
{
public:
	void InputSignature(PK_MessageAccumulator &messageAccumulator, const byte *signature, unsigned int signatureLength) const;
	bool VerifyAndRestart(PK_MessageAccumulator &messageAccumulator) const;
	DecodingResult RecoverAndRestart(byte *recoveredMessage, PK_MessageAccumulator &recoveryAccumulator) const;
};

// ********************************************************

//! _
template <class T1, class T2, class T3>
struct TF_CryptoSchemeOptions
{
	typedef T1 AlgorithmInfo;
	typedef T2 Keys;
	typedef typename Keys::PrivateKey PrivateKey;
	typedef typename Keys::PublicKey PublicKey;
	typedef T3 MessageEncodingMethod;
};

//! _
template <class T1, class T2, class T3, class T4>
struct TF_SignatureSchemeOptions : public TF_CryptoSchemeOptions<T1, T2, T3>
{
	typedef T4 HashFunction;
};

//! _
template <class KEYS>
class CRYPTOPP_NO_VTABLE PublicKeyCopier
{
public:
	virtual ~PublicKeyCopier() {};
	
	typedef typename KEYS::PublicKey KeyClass;
	virtual void CopyKeyInto(typename KEYS::PublicKey &key) const =0;
};

//! _
template <class KEYS>
class CRYPTOPP_NO_VTABLE PrivateKeyCopier
{
public:
	virtual ~PrivateKeyCopier() {};
	
	typedef typename KEYS::PrivateKey KeyClass;
	virtual void CopyKeyInto(typename KEYS::PublicKey &key) const =0;
	virtual void CopyKeyInto(typename KEYS::PrivateKey &key) const =0;
};

//! _
template <class BASE, class SCHEME_OPTIONS, class KEY>
class CRYPTOPP_NO_VTABLE TF_ObjectImplBase : public AlgorithmImpl<BASE, typename SCHEME_OPTIONS::AlgorithmInfo>
{
public:
	typedef SCHEME_OPTIONS SchemeOptions;
	typedef KEY KeyClass;

	PublicKey & AccessPublicKey() {return AccessKey();}
	const PublicKey & GetPublicKey() const {return GetKey();}

	PrivateKey & AccessPrivateKey() {return AccessKey();}
	const PrivateKey & GetPrivateKey() const {return GetKey();}

	virtual const KeyClass & GetKey() const =0;
	virtual KeyClass & AccessKey() =0;

	const KeyClass & GetTrapdoorFunction() const {return GetKey();}

	PK_MessageAccumulator * NewSignatureAccumulator(RandomNumberGenerator& /* rng */) const
	{
		return new PK_MessageAccumulatorImpl<CPP_TYPENAME SCHEME_OPTIONS::HashFunction>;
	}
	PK_MessageAccumulator * NewVerificationAccumulator() const
	{
		return new PK_MessageAccumulatorImpl<CPP_TYPENAME SCHEME_OPTIONS::HashFunction>;
	}

protected:
	const typename BASE::MessageEncodingInterface & GetMessageEncodingInterface() const 
		{return Singleton<CPP_TYPENAME SCHEME_OPTIONS::MessageEncodingMethod>().Ref();}
	const TrapdoorFunctionBounds & GetTrapdoorFunctionBounds() const 
		{return GetKey();}
	const typename BASE::TrapdoorFunctionInterface & GetTrapdoorFunctionInterface() const 
		{return GetKey();}

	// for signature scheme
	HashIdentifier GetHashIdentifier() const
	{
        typedef CPP_TYPENAME SchemeOptions::MessageEncodingMethod::HashIdentifierLookup::template HashIdentifierLookup2<CPP_TYPENAME SchemeOptions::HashFunction> L;
        return L::Lookup();
	}
	unsigned int GetDigestSize() const
	{
		typedef CPP_TYPENAME SchemeOptions::HashFunction H;
		return H::DIGESTSIZE;
	}
};

//! _
template <class BASE, class SCHEME_OPTIONS, class KEY_COPIER>
class CRYPTOPP_NO_VTABLE TF_ObjectImpl : public TF_ObjectImplBase<TwoBases<BASE, KEY_COPIER>, SCHEME_OPTIONS, typename KEY_COPIER::KeyClass>
{
public:
	typedef typename KEY_COPIER::KeyClass KeyClass;

	const KeyClass & GetKey() const {return m_trapdoorFunction;}
	KeyClass & AccessKey() {return m_trapdoorFunction;}

	void CopyKeyInto(typename SCHEME_OPTIONS::PrivateKey &key) const {key = GetKey();}
	void CopyKeyInto(typename SCHEME_OPTIONS::PublicKey &key) const {key = GetKey();}

private:
	KeyClass m_trapdoorFunction;
};

//! _
template <class SCHEME_OPTIONS>
class TF_SignerImpl : public TF_ObjectImpl<TF_SignerBase, SCHEME_OPTIONS, PrivateKeyCopier<typename SCHEME_OPTIONS::Keys> >
{
};

//! _
template <class SCHEME_OPTIONS>
class TF_VerifierImpl : public TF_ObjectImpl<TF_VerifierBase, SCHEME_OPTIONS, PublicKeyCopier<typename SCHEME_OPTIONS::Keys> >
{
};

// ********************************************************

CRYPTOPP_DLL void P1363_MGF1KDF2_Common(HashTransformation &hash, byte *output, unsigned int outputLength, const byte *input, unsigned int inputLength, const byte *derivationParams, unsigned int derivationParamsLength, bool mask, unsigned int counterStart);

// ********************************************************

//! _
template <class H>
class P1363_KDF2
{
public:
	static void DeriveKey(byte *output, unsigned int outputLength, const byte *input, unsigned int inputLength, const byte *derivationParams, unsigned int derivationParamsLength)
	{
		H h;
		P1363_MGF1KDF2_Common(h, output, outputLength, input, inputLength, derivationParams, derivationParamsLength, false, 1);
	}
};

// ********************************************************

//! A template implementing constructors for public key algorithm classes
template <class BASE>
class CRYPTOPP_NO_VTABLE PK_FinalTemplate : public BASE
{
public:
	PK_FinalTemplate() {}

	PK_FinalTemplate(const Integer &v1)
		{this->AccessKey().Initialize(v1);}

	PK_FinalTemplate(const typename BASE::KeyClass &key)  {this->AccessKey().operator=(key);}

	template <class T>
	PK_FinalTemplate(const PublicKeyCopier<T> &key)
		{key.CopyKeyInto(this->AccessKey());}

	template <class T>
	PK_FinalTemplate(const PrivateKeyCopier<T> &key)
		{key.CopyKeyInto(this->AccessKey());}

	PK_FinalTemplate(BufferedTransformation &bt) {this->AccessKey().BERDecode(bt);}

#if (defined(_MSC_VER) && _MSC_VER < 1300)

	template <class T1, class T2>
	PK_FinalTemplate(T1 &v1, T2 &v2)
		{this->AccessKey().Initialize(v1, v2);}

	template <class T1, class T2, class T3>
	PK_FinalTemplate(T1 &v1, T2 &v2, T3 &v3)
		{this->AccessKey().Initialize(v1, v2, v3);}
	
	template <class T1, class T2, class T3, class T4>
	PK_FinalTemplate(T1 &v1, T2 &v2, T3 &v3, T4 &v4)
		{this->AccessKey().Initialize(v1, v2, v3, v4);}

	template <class T1, class T2, class T3, class T4, class T5>
	PK_FinalTemplate(T1 &v1, T2 &v2, T3 &v3, T4 &v4, T5 &v5)
		{this->AccessKey().Initialize(v1, v2, v3, v4, v5);}

	template <class T1, class T2, class T3, class T4, class T5, class T6>
	PK_FinalTemplate(T1 &v1, T2 &v2, T3 &v3, T4 &v4, T5 &v5, T6 &v6)
		{this->AccessKey().Initialize(v1, v2, v3, v4, v5, v6);}

	template <class T1, class T2, class T3, class T4, class T5, class T6, class T7>
	PK_FinalTemplate(T1 &v1, T2 &v2, T3 &v3, T4 &v4, T5 &v5, T6 &v6, T7 &v7)
		{this->AccessKey().Initialize(v1, v2, v3, v4, v5, v6, v7);}

	template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
	PK_FinalTemplate(T1 &v1, T2 &v2, T3 &v3, T4 &v4, T5 &v5, T6 &v6, T7 &v7, T8 &v8)
		{this->AccessKey().Initialize(v1, v2, v3, v4, v5, v6, v7, v8);}

#else

	template <class T1, class T2>
	PK_FinalTemplate(const T1 &v1, const T2 &v2)
		{this->AccessKey().Initialize(v1, v2);}

	template <class T1, class T2, class T3>
	PK_FinalTemplate(const T1 &v1, const T2 &v2, const T3 &v3)
		{this->AccessKey().Initialize(v1, v2, v3);}
	
	template <class T1, class T2, class T3, class T4>
	PK_FinalTemplate(const T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4)
		{this->AccessKey().Initialize(v1, v2, v3, v4);}

	template <class T1, class T2, class T3, class T4, class T5>
	PK_FinalTemplate(const T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4, const T5 &v5)
		{this->AccessKey().Initialize(v1, v2, v3, v4, v5);}

	template <class T1, class T2, class T3, class T4, class T5, class T6>
	PK_FinalTemplate(const T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4, const T5 &v5, const T6 &v6)
		{this->AccessKey().Initialize(v1, v2, v3, v4, v5, v6);}

	template <class T1, class T2, class T3, class T4, class T5, class T6, class T7>
	PK_FinalTemplate(const T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4, const T5 &v5, const T6 &v6, const T7 &v7)
		{this->AccessKey().Initialize(v1, v2, v3, v4, v5, v6, v7);}

	template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
	PK_FinalTemplate(const T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4, const T5 &v5, const T6 &v6, const T7 &v7, const T8 &v8)
		{this->AccessKey().Initialize(v1, v2, v3, v4, v5, v6, v7, v8);}

	template <class T1, class T2>
	PK_FinalTemplate(T1 &v1, const T2 &v2)
		{this->AccessKey().Initialize(v1, v2);}

	template <class T1, class T2, class T3>
	PK_FinalTemplate(T1 &v1, const T2 &v2, const T3 &v3)
		{this->AccessKey().Initialize(v1, v2, v3);}
	
	template <class T1, class T2, class T3, class T4>
	PK_FinalTemplate(T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4)
		{this->AccessKey().Initialize(v1, v2, v3, v4);}

	template <class T1, class T2, class T3, class T4, class T5>
	PK_FinalTemplate(T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4, const T5 &v5)
		{this->AccessKey().Initialize(v1, v2, v3, v4, v5);}

	template <class T1, class T2, class T3, class T4, class T5, class T6>
	PK_FinalTemplate(T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4, const T5 &v5, const T6 &v6)
		{this->AccessKey().Initialize(v1, v2, v3, v4, v5, v6);}

	template <class T1, class T2, class T3, class T4, class T5, class T6, class T7>
	PK_FinalTemplate(T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4, const T5 &v5, const T6 &v6, const T7 &v7)
		{this->AccessKey().Initialize(v1, v2, v3, v4, v5, v6, v7);}

	template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
	PK_FinalTemplate(T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4, const T5 &v5, const T6 &v6, const T7 &v7, const T8 &v8)
		{this->AccessKey().Initialize(v1, v2, v3, v4, v5, v6, v7, v8);}

#endif
};

//! Base class for public key encryption standard classes. These classes are used to select from variants of algorithms. Note that not all standards apply to all algorithms.
struct EncryptionStandard {};

//! Base class for public key signature standard classes. These classes are used to select from variants of algorithms. Note that not all standards apply to all algorithms.
struct SignatureStandard {};

template <class STANDARD, class H, class KEYS, class ALG_INFO>	// VC60 workaround: doesn't work if KEYS is first parameter
class TF_SS;

//! Trapdoor Function Based Signature Scheme
template <class STANDARD, class H, class KEYS, class ALG_INFO = TF_SS<STANDARD, H, KEYS, int> >	// VC60 workaround: doesn't work if KEYS is first parameter
class TF_SS : public KEYS
{
public:
	//! see SignatureStandard for a list of standards
	typedef STANDARD Standard;
	typedef typename Standard::SignatureMessageEncodingMethod MessageEncodingMethod;
	typedef TF_SignatureSchemeOptions<ALG_INFO, KEYS, MessageEncodingMethod, H> SchemeOptions;

	static std::string StaticAlgorithmName() {return KEYS::StaticAlgorithmName() + "/" + MessageEncodingMethod::StaticAlgorithmName() + "(" + H::StaticAlgorithmName() + ")";}

	//! implements PK_Signer interface
	typedef PK_FinalTemplate<TF_SignerImpl<SchemeOptions> > Signer;
	//! implements PK_Verifier interface
	typedef PK_FinalTemplate<TF_VerifierImpl<SchemeOptions> > Verifier;
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_ITERHASH_H
#define CRYPTOPP_ITERHASH_H

//- #include "cryptlib.h"
//- #include "secblock.h"
//- #include "misc.h"
//- #include "simple.h"

NAMESPACE_BEGIN(CryptoPP)

//! _
template <class T, class BASE>
class CRYPTOPP_NO_VTABLE IteratedHashBase : public BASE
{
public:
	typedef T HashWordType;

	IteratedHashBase() : m_countLo(0), m_countHi(0) {}
	unsigned int BlockSize() const {return m_data.size() * sizeof(T);}
	unsigned int OptimalBlockSize() const {return BlockSize();}
	unsigned int OptimalDataAlignment() const {return sizeof(T);}
	void Update(const byte *input, unsigned int length);
	byte * CreateUpdateSpace(unsigned int &size);
	void Restart();

protected:
	void SetBlockSize(unsigned int blockSize) {m_data.resize(blockSize / sizeof(HashWordType));}
	void SetStateSize(unsigned int stateSize) {m_digest.resize(stateSize / sizeof(HashWordType));}

	T GetBitCountHi() const {return (m_countLo >> (8*sizeof(T)-3)) + (m_countHi << 3);}
	T GetBitCountLo() const {return m_countLo << 3;}

	virtual unsigned int HashMultipleBlocks(const T *input, unsigned int length);
	void PadLastBlock(unsigned int lastBlockSize, byte padFirst=0x80);
	virtual void Init() =0;
	virtual void HashBlock(const T *input) =0;

	SecBlock<T> m_data;			// Data buffer
	SecBlock<T> m_digest;		// Message digest

private:
	T m_countLo, m_countHi;
};


//! _
template <class T, class B, class BASE>
class CRYPTOPP_NO_VTABLE IteratedHashBase2 : public IteratedHashBase<T, BASE>
{
public:
	typedef B ByteOrderClass;
	typedef typename IteratedHashBase<T, BASE>::HashWordType HashWordType;

	inline static void CorrectEndianess(HashWordType *out, const HashWordType *in, unsigned int byteCount)
	{
		ConditionalByteReverse(B::ToEnum(), out, in, byteCount);
	}

	void TruncatedFinal(byte *digest, unsigned int size);

protected:
	void HashBlock(const HashWordType *input);
	virtual void HashEndianCorrectedBlock(const HashWordType *data) =0;
};

//! _
template <class T_HashWordType, class T_Endianness, unsigned int T_BlockSize, class T_Base = HashTransformation>
class CRYPTOPP_NO_VTABLE IteratedHash : public IteratedHashBase2<T_HashWordType, T_Endianness, T_Base>
{
public:
	enum {BLOCKSIZE = T_BlockSize};
	CRYPTOPP_COMPILE_ASSERT_GLOBAL((BLOCKSIZE & (BLOCKSIZE - 1)) == 0);		// blockSize is a power of 2

protected:
	IteratedHash() 
		: IteratedHashBase2<T_HashWordType, T_Endianness, T_Base>()
	{this->SetBlockSize(T_BlockSize);}
};

//! _
template <class T_HashWordType, class T_Endianness, unsigned int T_BlockSize, unsigned int T_StateSize, class T_Transform, unsigned int T_DigestSize = T_StateSize>
class CRYPTOPP_NO_VTABLE IteratedHashWithStaticTransform
	: public ClonableImpl<T_Transform, AlgorithmImpl<IteratedHash<T_HashWordType, T_Endianness, T_BlockSize>, T_Transform> >
{
public:
	enum {DIGESTSIZE = T_DigestSize};
	unsigned int DigestSize() const {return DIGESTSIZE;};

protected:
	IteratedHashWithStaticTransform()
	{
		this->SetStateSize(T_StateSize);
		Init();
	}
	void HashEndianCorrectedBlock(const T_HashWordType *data) {T_Transform::Transform(this->m_digest, data);}
	void Init() {T_Transform::InitState(this->m_digest);}
};

// *************************************************************

template <class T, class B, class BASE> void IteratedHashBase2<T, B, BASE>::TruncatedFinal(byte *digest, unsigned int size)
{
	this->ThrowIfInvalidTruncatedSize(size);

	PadLastBlock(this->BlockSize() - 2*sizeof(HashWordType));
	CorrectEndianess(this->m_data, this->m_data, this->BlockSize() - 2*sizeof(HashWordType));

	this->m_data[this->m_data.size()-2] = B::ToEnum() ? this->GetBitCountHi() : this->GetBitCountLo();
	this->m_data[this->m_data.size()-1] = B::ToEnum() ? this->GetBitCountLo() : this->GetBitCountHi();

	HashEndianCorrectedBlock(this->m_data);
	CorrectEndianess(this->m_digest, this->m_digest, this->DigestSize());
	memcpy(digest, this->m_digest, size);

	this->Restart();		// reinit for next use
}

template <class T, class B, class BASE> void IteratedHashBase2<T, B, BASE>::HashBlock(const HashWordType *input)
{
	if (NativeByteOrderIs(B::ToEnum()))
		HashEndianCorrectedBlock(input);
	else
	{
		ByteReverse(this->m_data.begin(), input, this->BlockSize());
		HashEndianCorrectedBlock(this->m_data);
	}
}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_SHA_H
#define CRYPTOPP_SHA_H

//- #include "iterhash.h"

NAMESPACE_BEGIN(CryptoPP)

/// <a href="http://www.weidai.com/scan-mirror/md.html#SHA-1">SHA-1</a>
class CRYPTOPP_DLL SHA : public IteratedHashWithStaticTransform<word32, BigEndian, 64, 20, SHA>
{
public:
	static void InitState(HashWordType *state);
	static void Transform(word32 *digest, const word32 *data);
	static const char *StaticAlgorithmName() {return "SHA-1";}
};

typedef SHA SHA1;

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_PKCSPAD_H
#define CRYPTOPP_PKCSPAD_H

//- #include "cryptlib.h"
//- #include "pubkey.h"

#ifdef CRYPTOPP_IS_DLL
//- #include "sha.h"
#endif

NAMESPACE_BEGIN(CryptoPP)

//! <a href="http://www.weidai.com/scan-mirror/ca.html#cem_PKCS1-1.5">EME-PKCS1-v1_5</a>
class PKCS_EncryptionPaddingScheme : public PK_EncryptionMessageEncodingMethod
{
public:
	static const char * StaticAlgorithmName() {return "EME-PKCS1-v1_5";}

	unsigned int MaxUnpaddedLength(unsigned int paddedLength) const;
	void Pad(RandomNumberGenerator &rng, const byte *raw, unsigned int inputLength, byte *padded, unsigned int paddedLength, const NameValuePairs &parameters) const;
	DecodingResult Unpad(const byte *padded, unsigned int paddedLength, byte *raw, const NameValuePairs &parameters) const;
};

template <class H> class PKCS_DigestDecoration
{
public:
	static const byte decoration[];
	static const unsigned int length;
};

// PKCS_DigestDecoration can be instantiated with the following
// classes as specified in PKCS#1 v2.0 and P1363a
class SHA;
// end of list

//! <a href="http://www.weidai.com/scan-mirror/sig.html#sem_PKCS1-1.5">EMSA-PKCS1-v1_5</a>
class CRYPTOPP_DLL PKCS1v15_SignatureMessageEncodingMethod : public PK_DeterministicSignatureMessageEncodingMethod
{
public:
	static const char * StaticAlgorithmName() {return "EMSA-PKCS1-v1_5";}

	void ComputeMessageRepresentative(RandomNumberGenerator &rng, 
		const byte *recoverableMessage, unsigned int recoverableMessageLength,
		HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
		byte *representative, unsigned int representativeBitLength) const;

	struct HashIdentifierLookup
	{
		template <class H> struct HashIdentifierLookup2
		{
			static HashIdentifier Lookup()
			{
				return HashIdentifier(PKCS_DigestDecoration<H>::decoration, PKCS_DigestDecoration<H>::length);
			}
		};
	};
};

//! PKCS #1 version 1.5, for use with RSAES and RSASS
/*! Only the following hash functions are supported by this signature standard:
	\dontinclude pkcspad.h
	\skip can be instantiated
	\until end of list
*/
struct PKCS1v15 : public SignatureStandard, public EncryptionStandard
{
	typedef PKCS_EncryptionPaddingScheme EncryptionMessageEncodingMethod;
	typedef PKCS1v15_SignatureMessageEncodingMethod SignatureMessageEncodingMethod;
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_ASN_H
#define CRYPTOPP_ASN_H

//- #include "filters.h"
//- #include "queue.h"
#include <vector>

NAMESPACE_BEGIN(CryptoPP)

// these tags and flags are not complete
enum ASNTag
{
	BOOLEAN 			= 0x01,
	INTEGER 			= 0x02,
	BIT_STRING			= 0x03,
	OCTET_STRING		= 0x04,
	TAG_NULL			= 0x05,
	OBJECT_IDENTIFIER	= 0x06,
	OBJECT_DESCRIPTOR	= 0x07,
	EXTERNAL			= 0x08,
	REAL				= 0x09,
	ENUMERATED			= 0x0a,
	UTF8_STRING			= 0x0c,
	SEQUENCE			= 0x10,
	SET 				= 0x11,
	NUMERIC_STRING		= 0x12,
	PRINTABLE_STRING 	= 0x13,
	T61_STRING			= 0x14,
	VIDEOTEXT_STRING 	= 0x15,
	IA5_STRING			= 0x16,
	UTC_TIME 			= 0x17,
	GENERALIZED_TIME 	= 0x18,
	GRAPHIC_STRING		= 0x19,
	VISIBLE_STRING		= 0x1a,
	GENERAL_STRING		= 0x1b
};

enum ASNIdFlag
{
	UNIVERSAL			= 0x00,
//	DATA				= 0x01,
//	HEADER				= 0x02,
	CONSTRUCTED 		= 0x20,
	APPLICATION 		= 0x40,
	CONTEXT_SPECIFIC	= 0x80,
	PRIVATE 			= 0xc0
};

inline void BERDecodeError() {throw BERDecodeErr();}

// unsigned int DERLengthEncode(unsigned int length, byte *output=0);
CRYPTOPP_DLL unsigned int DERLengthEncode(BufferedTransformation &out, unsigned int length);
// returns false if indefinite length
CRYPTOPP_DLL bool BERLengthDecode(BufferedTransformation &in, unsigned int &length);

CRYPTOPP_DLL void DEREncodeNull(BufferedTransformation &out);
CRYPTOPP_DLL void BERDecodeNull(BufferedTransformation &in);

CRYPTOPP_DLL unsigned int DEREncodeOctetString(BufferedTransformation &out, const byte *str, unsigned int strLen);
CRYPTOPP_DLL unsigned int DEREncodeOctetString(BufferedTransformation &out, const SecByteBlock &str);

// BER decode from source and DER reencode into dest
CRYPTOPP_DLL void DERReencode(BufferedTransformation &source, BufferedTransformation &dest);

//! Object Identifier
class CRYPTOPP_DLL OID
{
public:
	OID() {}
	OID(unsigned long v) : m_values(1, v) {}
	OID(BufferedTransformation &bt) {BERDecode(bt);}

	inline OID & operator+=(unsigned long rhs) {m_values.push_back(rhs); return *this;}

	void DEREncode(BufferedTransformation &bt) const;
	void BERDecode(BufferedTransformation &bt);

	// throw BERDecodeErr() if decoded value doesn't equal this OID
	void BERDecodeAndCheck(BufferedTransformation &bt) const;

	std::vector<unsigned long> m_values;

private:
	static void EncodeValue(BufferedTransformation &bt, unsigned long v);
	static unsigned int DecodeValue(BufferedTransformation &bt, unsigned long &v);
};

//! BER General Decoder
class CRYPTOPP_DLL BERGeneralDecoder : public Store
{
public:
	explicit BERGeneralDecoder(BufferedTransformation &inQueue, byte asnTag);
	explicit BERGeneralDecoder(BERGeneralDecoder &inQueue, byte asnTag);
	~BERGeneralDecoder();

	bool IsDefiniteLength() const {return m_definiteLength;}
	unsigned int RemainingLength() const {assert(m_definiteLength); return m_length;}
	bool EndReached() const;
	byte PeekByte() const;
	void CheckByte(byte b);

	unsigned int TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel=NULL_CHANNEL, bool blocking=true);
	unsigned int CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end=ULONG_MAX, const std::string &channel=NULL_CHANNEL, bool blocking=true) const;

	// call this to denote end of sequence
	void MessageEnd();

protected:
	BufferedTransformation &m_inQueue;
	bool m_finished, m_definiteLength;
	unsigned int m_length;

private:
	void Init(byte asnTag);
	void StoreInitialize(const NameValuePairs& /* parameters */) {assert(false);}
	unsigned int ReduceLength(unsigned int delta);
};

//! DER General Encoder
class CRYPTOPP_DLL DERGeneralEncoder : public ByteQueue
{
public:
	explicit DERGeneralEncoder(BufferedTransformation &outQueue, byte asnTag = SEQUENCE | CONSTRUCTED);
	explicit DERGeneralEncoder(DERGeneralEncoder &outQueue, byte asnTag = SEQUENCE | CONSTRUCTED);
	~DERGeneralEncoder();

	// call this to denote end of sequence
	void MessageEnd();

private:
	BufferedTransformation &m_outQueue;
	bool m_finished;

	byte m_asnTag;
};

//! BER Sequence Decoder
class CRYPTOPP_DLL BERSequenceDecoder : public BERGeneralDecoder
{
public:
	explicit BERSequenceDecoder(BufferedTransformation &inQueue, byte asnTag = SEQUENCE | CONSTRUCTED)
		: BERGeneralDecoder(inQueue, asnTag) {}
	explicit BERSequenceDecoder(BERSequenceDecoder &inQueue, byte asnTag = SEQUENCE | CONSTRUCTED)
		: BERGeneralDecoder(inQueue, asnTag) {}
};

//! DER Sequence Encoder
class CRYPTOPP_DLL DERSequenceEncoder : public DERGeneralEncoder
{
public:
	explicit DERSequenceEncoder(BufferedTransformation &outQueue, byte asnTag = SEQUENCE | CONSTRUCTED)
		: DERGeneralEncoder(outQueue, asnTag) {}
	explicit DERSequenceEncoder(DERSequenceEncoder &outQueue, byte asnTag = SEQUENCE | CONSTRUCTED)
		: DERGeneralEncoder(outQueue, asnTag) {}
};

//! key that can be ASN.1 encoded
/** derived class should override either BERDecodeKey or BERDecodeKey2 */
class CRYPTOPP_DLL ASN1Key : public ASN1CryptoMaterial
{
public:
	virtual OID GetAlgorithmID() const =0;
	virtual bool BERDecodeAlgorithmParameters(BufferedTransformation &bt)
		{BERDecodeNull(bt); return false;}
	virtual bool DEREncodeAlgorithmParameters(BufferedTransformation &bt) const
		{DEREncodeNull(bt); return false;}	// see RFC 2459, section 7.3.1
	//! decode subjectPublicKey part of subjectPublicKeyInfo, or privateKey part of privateKeyInfo, without the BIT STRING or OCTET STRING header
	virtual void BERDecodeKey(BufferedTransformation& /* bt */) {assert(false);}
	virtual void BERDecodeKey2(BufferedTransformation& bt, bool /* parametersPresent */, unsigned int /* size */)
		{BERDecodeKey(bt);}
	//! encode subjectPublicKey part of subjectPublicKeyInfo, or privateKey part of privateKeyInfo, without the BIT STRING or OCTET STRING header
	virtual void DEREncodeKey(BufferedTransformation &bt) const =0;
};

//! encodes/decodes subjectPublicKeyInfo
class CRYPTOPP_DLL X509PublicKey : virtual public ASN1Key, public PublicKey
{
public:
	void BERDecode(BufferedTransformation &bt);
	void DEREncode(BufferedTransformation &bt) const;
};

//! encodes/decodes privateKeyInfo
class CRYPTOPP_DLL PKCS8PrivateKey : virtual public ASN1Key, public PrivateKey
{
public:
	void BERDecode(BufferedTransformation &bt);
	void DEREncode(BufferedTransformation &bt) const;

	//! decode optional attributes including context-specific tag
	/*! /note default implementation stores attributes to be output in DEREncodeOptionalAttributes */
	virtual void BERDecodeOptionalAttributes(BufferedTransformation &bt);
	//! encode optional attributes including context-specific tag
	virtual void DEREncodeOptionalAttributes(BufferedTransformation &bt) const;

private:
	ByteQueue m_optionalAttributes;
};

// ********************************************************

//! DER Encode Unsigned
/*! for INTEGER, BOOLEAN, and ENUM */
template <class T>
unsigned int DEREncodeUnsigned(BufferedTransformation &out, T w, byte asnTag = INTEGER)
{
	byte buf[sizeof(w)+1];
	unsigned int bc;
	if (asnTag == BOOLEAN)
	{
		buf[sizeof(w)] = w ? 0xff : 0;
		bc = 1;
	}
	else
	{
		buf[0] = 0;
		for (unsigned int i=0; i<sizeof(w); i++)
			buf[i+1] = byte(w >> (sizeof(w)-1-i)*8);
		bc = sizeof(w);
		while (bc > 1 && buf[sizeof(w)+1-bc] == 0)
			--bc;
		if (buf[sizeof(w)+1-bc] & 0x80)
			++bc;
	}
	out.Put(asnTag);
	unsigned int lengthBytes = DERLengthEncode(out, bc);
	out.Put(buf+sizeof(w)+1-bc, bc);
	return 1+lengthBytes+bc;
}

//! BER Decode Unsigned
// VC60 workaround: std::numeric_limits<T>::max conflicts with MFC max macro
// CW41 workaround: std::numeric_limits<T>::max causes a template error
template <class T>
void BERDecodeUnsigned(BufferedTransformation &in, T &w, byte asnTag = INTEGER,
					   T minValue = 0, T maxValue = 0xffffffff)
{
	byte b;
	if (!in.Get(b) || b != asnTag)
		BERDecodeError();

	unsigned int bc;
	BERLengthDecode(in, bc);

	SecByteBlock buf(bc);

	if (bc != in.Get(buf, bc))
		BERDecodeError();

	const byte *ptr = buf;
	while (bc > sizeof(w) && *ptr == 0)
	{
		bc--;
		ptr++;
	}
	if (bc > sizeof(w))
		BERDecodeError();

	w = 0;
	for (unsigned int i=0; i<bc; i++)
		w = (w << 8) | ptr[i];

	if (w < minValue || w > maxValue)
		BERDecodeError();
}

inline bool operator==(const ::CryptoPP::OID &lhs, const ::CryptoPP::OID &rhs)
	{return lhs.m_values == rhs.m_values;}
inline bool operator!=(const ::CryptoPP::OID &lhs, const ::CryptoPP::OID &rhs)
	{return lhs.m_values != rhs.m_values;}
inline bool operator<(const ::CryptoPP::OID &lhs, const ::CryptoPP::OID &rhs)
	{return std::lexicographical_compare(lhs.m_values.begin(), lhs.m_values.end(), rhs.m_values.begin(), rhs.m_values.end());}
inline ::CryptoPP::OID operator+(const ::CryptoPP::OID &lhs, unsigned long rhs)
	{return ::CryptoPP::OID(lhs)+=rhs;}

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_RSA_H
#define CRYPTOPP_RSA_H

/** \file
	This file contains classes that implement the RSA
	ciphers and signature schemes as defined in PKCS #1 v2.0.
*/

//- #include "pkcspad.h"
//- #include "oaep.h"
//- #include "integer.h"
//- #include "asn.h"

NAMESPACE_BEGIN(CryptoPP)

//! _
class CRYPTOPP_DLL RSAFunction : public TrapdoorFunction, public X509PublicKey
{
	typedef RSAFunction ThisClass;

public:
	void Initialize(const Integer &n, const Integer &e)
		{m_n = n; m_e = e;}

	// X509PublicKey
	OID GetAlgorithmID() const;
	void BERDecodeKey(BufferedTransformation &bt);
	void DEREncodeKey(BufferedTransformation &bt) const;

	// CryptoMaterial
	bool Validate(RandomNumberGenerator &rng, unsigned int level) const;
	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;
	void AssignFrom(const NameValuePairs &source);

	// TrapdoorFunction
	Integer ApplyFunction(const Integer &x) const;
	Integer PreimageBound() const {return m_n;}
	Integer ImageBound() const {return m_n;}

	// non-derived
	const Integer & GetModulus() const {return m_n;}
	const Integer & GetPublicExponent() const {return m_e;}

	void SetModulus(const Integer &n) {m_n = n;}
	void SetPublicExponent(const Integer &e) {m_e = e;}

protected:
	Integer m_n, m_e;
};

//! _
class CRYPTOPP_DLL InvertibleRSAFunction : public RSAFunction, public TrapdoorFunctionInverse, public PKCS8PrivateKey
{
	typedef InvertibleRSAFunction ThisClass;

public:
	void Initialize(RandomNumberGenerator &rng, unsigned int modulusBits, const Integer &e = 17);
	void Initialize(const Integer &n, const Integer &e, const Integer &d, const Integer &p, const Integer &q, const Integer &dp, const Integer &dq, const Integer &u)
		{m_n = n; m_e = e; m_d = d; m_p = p; m_q = q; m_dp = dp; m_dq = dq; m_u = u;}
	//! factor n given private exponent
	void Initialize(const Integer &n, const Integer &e, const Integer &d);

	// PKCS8PrivateKey
	void BERDecode(BufferedTransformation &bt)
		{PKCS8PrivateKey::BERDecode(bt);}
	void DEREncode(BufferedTransformation &bt) const
		{PKCS8PrivateKey::DEREncode(bt);}
	void BERDecodeKey(BufferedTransformation &bt);
	void DEREncodeKey(BufferedTransformation &bt) const;

	// TrapdoorFunctionInverse
	Integer CalculateInverse(RandomNumberGenerator &rng, const Integer &x) const;

	// GeneratableCryptoMaterial
	bool Validate(RandomNumberGenerator &rng, unsigned int level) const;
	/*! parameters: (ModulusSize, PublicExponent (default 17)) */
	void GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &alg);
	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;
	void AssignFrom(const NameValuePairs &source);

	// non-derived interface
	const Integer& GetPrime1() const {return m_p;}
	const Integer& GetPrime2() const {return m_q;}
	const Integer& GetPrivateExponent() const {return m_d;}
	const Integer& GetModPrime1PrivateExponent() const {return m_dp;}
	const Integer& GetModPrime2PrivateExponent() const {return m_dq;}
	const Integer& GetMultiplicativeInverseOfPrime2ModPrime1() const {return m_u;}

	void SetPrime1(const Integer &p) {m_p = p;}
	void SetPrime2(const Integer &q) {m_q = q;}
	void SetPrivateExponent(const Integer &d) {m_d = d;}
	void SetModPrime1PrivateExponent(const Integer &dp) {m_dp = dp;}
	void SetModPrime2PrivateExponent(const Integer &dq) {m_dq = dq;}
	void SetMultiplicativeInverseOfPrime2ModPrime1(const Integer &u) {m_u = u;}

protected:
	Integer m_d, m_p, m_q, m_dp, m_dq, m_u;
};

//! RSA
struct CRYPTOPP_DLL RSA
{
	static std::string StaticAlgorithmName() {return "RSA";}
	typedef RSAFunction PublicKey;
	typedef InvertibleRSAFunction PrivateKey;
};

//! <a href="http://www.weidai.com/scan-mirror/sig.html#RSA">RSA signature scheme with appendix</a>
/*! See documentation of PKCS1v15 for a list of hash functions that can be used with it. */
template <class STANDARD, class H>
struct RSASS : public TF_SS<STANDARD, H, RSA>
{
};

// The three RSA signature schemes defined in PKCS #1 v2.0
typedef RSASS<PKCS1v15, SHA>::Signer RSASSA_PKCS1v15_SHA_Signer;
typedef RSASS<PKCS1v15, SHA>::Verifier RSASSA_PKCS1v15_SHA_Verifier;

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_BASECODE_H
#define CRYPTOPP_BASECODE_H

//- #include "filters.h"
//- #include "algparam.h"
//- #include "argnames.h"

NAMESPACE_BEGIN(CryptoPP)

//! base n encoder, where n is a power of 2
class CRYPTOPP_DLL BaseN_Encoder : public Unflushable<Filter>
{
public:
	BaseN_Encoder(BufferedTransformation *attachment=NULL)
		{Detach(attachment);}

	BaseN_Encoder(const byte *alphabet, int log2base, BufferedTransformation *attachment=NULL, int padding=-1)
	{
		Detach(attachment);
		IsolatedInitialize(MakeParameters(Name::EncodingLookupArray(), alphabet)
			(Name::Log2Base(), log2base)
			(Name::Pad(), padding != -1)
			(Name::PaddingByte(), byte(padding)));
	}

	void IsolatedInitialize(const NameValuePairs &parameters);
	unsigned int Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking);

private:
	const byte *m_alphabet;
	int m_padding, m_bitsPerChar, m_outputBlockSize;
	int m_bytePos, m_bitPos;
	SecByteBlock m_outBuf;
};

//! base n decoder, where n is a power of 2
class CRYPTOPP_DLL BaseN_Decoder : public Unflushable<Filter>
{
public:
	BaseN_Decoder(BufferedTransformation *attachment=NULL)
		{Detach(attachment);}

	BaseN_Decoder(const int *lookup, int log2base, BufferedTransformation *attachment=NULL)
	{
		Detach(attachment);
		IsolatedInitialize(MakeParameters(Name::DecodingLookupArray(), lookup)(Name::Log2Base(), log2base));
	}

	void IsolatedInitialize(const NameValuePairs &parameters);
	unsigned int Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking);

	static void InitializeDecodingLookupArray(int *lookup, const byte *alphabet, unsigned int base, bool caseInsensitive);

private:
	const int *m_lookup;
	int m_padding, m_bitsPerChar, m_outputBlockSize;
	int m_bytePos, m_bitPos;
	SecByteBlock m_outBuf;
};

//! filter that breaks input stream into groups of fixed size
class CRYPTOPP_DLL Grouper : public Bufferless<Filter>
{
public:
	Grouper(BufferedTransformation *attachment=NULL)
		{Detach(attachment);}

	Grouper(int groupSize, const std::string &separator, const std::string &terminator, BufferedTransformation *attachment=NULL)
	{
		Detach(attachment);
		IsolatedInitialize(MakeParameters(Name::GroupSize(), groupSize)
			(Name::Separator(), ConstByteArrayParameter(separator))
			(Name::Terminator(), ConstByteArrayParameter(terminator)));
	}

	void IsolatedInitialize(const NameValuePairs &parameters);
	unsigned int Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking);

private:
	SecByteBlock m_separator, m_terminator;
	unsigned int m_groupSize, m_counter;
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_BASE64_H
#define CRYPTOPP_BASE64_H

//- #include "basecode.h"

NAMESPACE_BEGIN(CryptoPP)

//! Base64 Encoder Class 
class Base64Encoder : public SimpleProxyFilter
{
public:
	Base64Encoder(BufferedTransformation *attachment = NULL, bool insertLineBreaks = true, int maxLineLength = 72)
		: SimpleProxyFilter(new BaseN_Encoder(new Grouper), attachment)
	{
		IsolatedInitialize(MakeParameters(Name::InsertLineBreaks(), insertLineBreaks)(Name::MaxLineLength(), maxLineLength));
	}

	void IsolatedInitialize(const NameValuePairs &parameters);
};

//! Base64 Decoder Class 
class Base64Decoder : public BaseN_Decoder
{
public:
	Base64Decoder(BufferedTransformation *attachment = NULL)
		: BaseN_Decoder(GetDecodingLookupArray(), 6, attachment) {}

	void IsolatedInitialize(const NameValuePairs& /* parameters */) {}

private:
	static const int *GetDecodingLookupArray();
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_FILES_H
#define CRYPTOPP_FILES_H

//- #include "cryptlib.h"
//- #include "filters.h"
//- #include "argnames.h"

#include <iostream>
#include <fstream>

NAMESPACE_BEGIN(CryptoPP)

//! file-based implementation of Store interface
class CRYPTOPP_DLL FileStore : public Store, private FilterPutSpaceHelper, public NotCopyable
{
public:
	class Err : public Exception
	{
	public:
		Err(const std::string &s) : Exception(IO_ERROR, s) {}
	};
	class OpenErr : public Err {public: OpenErr(const std::string &filename) : Err("FileStore: error opening file for reading: " + filename) {}};
	class ReadErr : public Err {public: ReadErr() : Err("FileStore: error reading file") {}};

	FileStore() : m_stream(NULL) {}
	FileStore(std::istream &in)
		{StoreInitialize(MakeParameters(Name::InputStreamPointer(), &in));}
	FileStore(const char *filename)
		{StoreInitialize(MakeParameters(Name::InputFileName(), filename));}

	std::istream* GetStream() {return m_stream;}

	unsigned long MaxRetrievable() const;
	unsigned int TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel=NULL_CHANNEL, bool blocking=true);
	unsigned int CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end=ULONG_MAX, const std::string &channel=NULL_CHANNEL, bool blocking=true) const;
	unsigned long Skip(unsigned long skipMax=ULONG_MAX);

private:
	void StoreInitialize(const NameValuePairs &parameters);
	
	member_ptr<std::ifstream> m_file;
	std::istream *m_stream;
	byte *m_space;
	unsigned int m_len;
	bool m_waiting;
};

//! file-based implementation of Source interface
class CRYPTOPP_DLL FileSource : public SourceTemplate<FileStore>
{
public:
	typedef FileStore::Err Err;
	typedef FileStore::OpenErr OpenErr;
	typedef FileStore::ReadErr ReadErr;

	FileSource(BufferedTransformation *attachment = NULL)
		: SourceTemplate<FileStore>(attachment) {}
	FileSource(std::istream &in, bool pumpAll, BufferedTransformation *attachment = NULL)
		: SourceTemplate<FileStore>(attachment) {SourceInitialize(pumpAll, MakeParameters(Name::InputStreamPointer(), &in));}
	FileSource(const char *filename, bool pumpAll, BufferedTransformation *attachment = NULL, bool binary=true)
		: SourceTemplate<FileStore>(attachment) {SourceInitialize(pumpAll, MakeParameters(Name::InputFileName(), filename)(Name::InputBinaryMode(), binary));}

	std::istream* GetStream() {return m_store.GetStream();}
};

//! file-based implementation of Sink interface
class CRYPTOPP_DLL FileSink : public Sink, public NotCopyable
{
public:
	class Err : public Exception
	{
	public:
		Err(const std::string &s) : Exception(IO_ERROR, s) {}
	};
	class OpenErr : public Err {public: OpenErr(const std::string &filename) : Err("FileSink: error opening file for writing: " + filename) {}};
	class WriteErr : public Err {public: WriteErr() : Err("FileSink: error writing file") {}};

	FileSink() : m_stream(NULL) {}
	FileSink(std::ostream &out)
		{IsolatedInitialize(MakeParameters(Name::OutputStreamPointer(), &out));}
	FileSink(const char *filename, bool binary=true)
		{IsolatedInitialize(MakeParameters(Name::OutputFileName(), filename)("OutputBinaryMode", binary));}

	std::ostream* GetStream() {return m_stream;}

	void IsolatedInitialize(const NameValuePairs &parameters);
	unsigned int Put2(const byte *inString, unsigned int length, int messageEnd, bool blocking);
	bool IsolatedFlush(bool hardFlush, bool blocking);

private:
	member_ptr<std::ofstream> m_file;
	std::ostream *m_stream;
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_RANDPOOL_H
#define CRYPTOPP_RANDPOOL_H

//- #include "cryptlib.h"
//- #include "filters.h"

NAMESPACE_BEGIN(CryptoPP)

//! Randomness Pool
/*! This class can be used to generate
	pseudorandom bytes after seeding the pool with
	the Put() methods */
class CRYPTOPP_DLL RandomPool : public RandomNumberGenerator,
				   public Bufferless<BufferedTransformation>
{
public:
	//! poolSize must be greater than 16
	RandomPool(unsigned int poolSize=384);

	unsigned int Put2(const byte *begin, unsigned int, int messageEnd, bool blocking);

	bool AnyRetrievable() const {return true;}
	unsigned long MaxRetrievable() const {return ULONG_MAX;}

	unsigned int TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel=NULL_CHANNEL, bool blocking=true);
	unsigned int CopyRangeTo2(BufferedTransformation& /* target */, unsigned long& /* begin */, unsigned long /* end */ = ULONG_MAX, const std::string& /* channel */ = NULL_CHANNEL, bool /* blocking */ = true) const
	{
		throw NotImplemented("RandomPool: CopyRangeTo2() is not supported by this store");
	}

	byte GenerateByte();
	void GenerateBlock(byte *output, unsigned int size);

	void IsolatedInitialize(const NameValuePairs& /* parameters */) {}

protected:
	void Stir();

private:
	SecByteBlock pool, key;
	unsigned int addPos, getPos;
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// seckey.h - written and placed in the public domain by Wei Dai

// This file contains helper classes/functions for implementing secret key algorithms.

#ifndef CRYPTOPP_SECKEY_H
#define CRYPTOPP_SECKEY_H

//- #include "cryptlib.h"
//- #include "misc.h"
//- #include "simple.h"

NAMESPACE_BEGIN(CryptoPP)

//! to be inherited by block ciphers with fixed block size
template <unsigned int N>
class FixedBlockSize
{
public:
	enum {BLOCKSIZE = N};
};

// ************** key length ***************

//! to be inherited by keyed algorithms with fixed key length
template <unsigned int N, unsigned int IV_REQ = SimpleKeyingInterface::NOT_RESYNCHRONIZABLE>
class FixedKeyLength
{
public:
	enum {KEYLENGTH=N, MIN_KEYLENGTH=N, MAX_KEYLENGTH=N, DEFAULT_KEYLENGTH=N};
	enum {IV_REQUIREMENT = IV_REQ};
	static unsigned int StaticGetValidKeyLength(unsigned int) {return KEYLENGTH;}
};

// ************** implementation helper for SimpledKeyed ***************

template <class T>
static inline void CheckedSetKey(T *obj, Empty empty, const byte *key, unsigned int length, const NameValuePairs &param)
{
	obj->ThrowIfInvalidKeyLength(length);
	obj->UncheckedSetKey(key, length);
}

template <class T>
static inline void CheckedSetKey(T *obj, CipherDir dir, const byte *key, unsigned int length, const NameValuePairs& /* param */)
{
	obj->ThrowIfInvalidKeyLength(length);
	obj->UncheckedSetKey(dir, key, length);
}

//! _
template <class BASE, class INFO = BASE>
class CRYPTOPP_NO_VTABLE SimpleKeyingInterfaceImpl : public BASE
{
public:
	unsigned int MinKeyLength() const {return INFO::MIN_KEYLENGTH;}
	unsigned int MaxKeyLength() const {return (unsigned int)INFO::MAX_KEYLENGTH;}
	unsigned int DefaultKeyLength() const {return INFO::DEFAULT_KEYLENGTH;}
	unsigned int GetValidKeyLength(unsigned int n) const {return INFO::StaticGetValidKeyLength(n);}
	typename BASE::IV_Requirement IVRequirement() const {return (typename BASE::IV_Requirement)INFO::IV_REQUIREMENT;}

protected:
	void AssertValidKeyLength(unsigned int length) {assert(GetValidKeyLength(length) == length);}
};

template <class INFO, class BASE = BlockCipher>
class CRYPTOPP_NO_VTABLE BlockCipherImpl : public AlgorithmImpl<SimpleKeyingInterfaceImpl<TwoBases<BASE, INFO> > >
{
public:
	unsigned int BlockSize() const {return this->BLOCKSIZE;}
};

//! _
template <CipherDir DIR, class BASE>
class BlockCipherFinal : public ClonableImpl<BlockCipherFinal<DIR, BASE>, BASE>
{
public:
 	BlockCipherFinal() {}
	BlockCipherFinal(const byte *key)
		{SetKey(key, this->DEFAULT_KEYLENGTH);}
	BlockCipherFinal(const byte *key, unsigned int length)
		{SetKey(key, length);}
	BlockCipherFinal(const byte *key, unsigned int length, unsigned int rounds)
		{this->SetKeyWithRounds(key, length, rounds);}

	bool IsForwardTransformation() const {return DIR == ENCRYPTION;}

	void SetKey(const byte *key, unsigned int length, const NameValuePairs &param = g_nullNameValuePairs)
	{
		CheckedSetKey(this, DIR, key, length, param);
	}
};

// ************** documentation ***************

/*! \brief Each class derived from this one defines two types, Encryption and Decryption, 
	both of which implement the SymmetricCipher interface. Two types of classes derive
	from this class: stream ciphers and block cipher modes. Stream ciphers can be used
	alone, cipher mode classes need to be used with a block cipher. See CipherModeDocumentation
	for more for information about using cipher modes and block ciphers. */
struct SymmetricCipherDocumentation
{
	//! implements the SymmetricCipher interface
	typedef SymmetricCipher Encryption;
	//! implements the SymmetricCipher interface
	typedef SymmetricCipher Decryption;
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_OSRNG_H
#define CRYPTOPP_OSRNG_H

//- #include "config.h"

#ifdef OS_RNG_AVAILABLE

//- #include "randpool.h"
//- #include "rng.h"
//- #include "des.h"
//- #include "fips140.h"

NAMESPACE_BEGIN(CryptoPP)

//! Exception class for Operating-System Random Number Generator.
class CRYPTOPP_DLL OS_RNG_Err : public Exception
{
public:
	OS_RNG_Err(const std::string &operation);
};

#ifdef NONBLOCKING_RNG_AVAILABLE

#ifdef CRYPTOPP_WIN32_AVAILABLE
class CRYPTOPP_DLL MicrosoftCryptoProvider
{
public:
	MicrosoftCryptoProvider();
	~MicrosoftCryptoProvider();
#if defined(_WIN64)
	typedef unsigned __int64 ProviderHandle;	// type HCRYPTPROV, avoid #include <windows.h>
#else
	typedef unsigned long ProviderHandle;
#endif
	ProviderHandle GetProviderHandle() const {return m_hProvider;}
private:
	ProviderHandle m_hProvider;
};

#pragma comment(lib, "advapi32.lib")
#endif

//! encapsulate CryptoAPI's CryptGenRandom or /dev/urandom
class CRYPTOPP_DLL NonblockingRng : public RandomNumberGenerator
{
public:
	NonblockingRng();
	~NonblockingRng();
	byte GenerateByte();
	void GenerateBlock(byte *output, unsigned int size);

protected:
#ifdef CRYPTOPP_WIN32_AVAILABLE
#	ifndef WORKAROUND_MS_BUG_Q258000
		MicrosoftCryptoProvider m_Provider;
#	endif
#else
	int m_fd;
#endif
};

#endif

#ifdef BLOCKING_RNG_AVAILABLE

//! encapsulate /dev/random
class CRYPTOPP_DLL BlockingRng : public RandomNumberGenerator
{
public:
	BlockingRng();
	~BlockingRng();
	byte GenerateByte();
	void GenerateBlock(byte *output, unsigned int size);

protected:
	int m_fd;
};

#endif

CRYPTOPP_DLL void OS_GenerateRandomBlock(bool blocking, byte *output, unsigned int size);

//! Automaticly Seeded Randomness Pool
/*! This class seeds itself using an operating system provided RNG. */
class CRYPTOPP_DLL AutoSeededRandomPool : public RandomPool
{
public:
	//! blocking will be ignored if the prefered RNG isn't available
	explicit AutoSeededRandomPool(bool blocking = false, unsigned int seedSize = 32)
		{Reseed(blocking, seedSize);}
	void Reseed(bool blocking = false, unsigned int seedSize = 32);
};

NAMESPACE_END

#endif

#endif
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
#ifndef CRYPTOPP_MD4_H
#define CRYPTOPP_MD4_H

//- #include "iterhash.h"

NAMESPACE_BEGIN(CryptoPP)

//! <a href="http://www.weidai.com/scan-mirror/md.html#MD4">MD4</a>
/*! \warning MD4 is considered insecure, and should not be used
	unless you absolutely need compatibility with a broken product. */
class MD4 : public IteratedHashWithStaticTransform<word32, LittleEndian, 64, 16, MD4>
{
public:
	static void InitState(HashWordType *state);
	static void Transform(word32 *digest, const word32 *data);
	static const char *StaticAlgorithmName() {return "MD4";}
};

NAMESPACE_END

#endif
////////////////////////////////////////////////////////////////////////////////

#endif
