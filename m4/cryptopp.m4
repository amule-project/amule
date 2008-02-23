dnl --------------------------------------------------------------------------
dnl Add cryptopp configure option
dnl --------------------------------------------------------------------------
dnl
dnl This macro sets and substitutes these variables:
AC_DEFUN([AM_OPTIONS_CRYPTO], [
	AC_ARG_WITH(
		[crypto-prefix],
		[AS_HELP_STRING(
			[--with-crypto-prefix=PREFIX],
			[prefix where crypto++ is installed])],
		[CRYPTO_PP_PREFIX="$withval"],
		[CRYPTO_PP_PREFIX=""])
])


dnl --------------------------------------------------------------------------
dnl Check for cryptopp library
dnl --------------------------------------------------------------------------
dnl
dnl This macro sets and substitutes these variables:
dnl - CRYPTO_PP_PREFIX
dnl 	This is the user or system directory where cryptopp is installed or sources
dnl - CRYPTO_PP_VERSION_STRING
dnl 	Something like "5.5.2"
dnl - CRYPTO_PP_VERSION_NUMBER
dnl 	Something like 5005002
dnl - CRYPTO_PP_STYLE
dnl 	"sources", "installed" or "gentoo_debian"
dnl - CRYPTO_PP_LIB_NAME
dnl 	"cryptopp" or "crypto++"
dnl - CRYPTO_PP_INCLUDE_PREFIX
dnl 	The string that goes here: #include <@CRYPTO_PP_INCLUDE_PREFIX@/rsa.h>
dnl - CRYPTO_PP_CXXFLAGS
dnl 	Flags to be added to CXXFLAGS
dnl - CRYPTO_PP_LDFLAGS
dnl 	Flags to be added to LDFLAGS
dnl
dnl Worth notice:
dnl - crypto_pp_include_i
dnl 	The string that goes in -I or -isystem on CXXFLAGS
dnl - crypto_pp_header_path
dnl 	The file we use to discover the version of cryptopp
dnl
AC_DEFUN([CHECK_CRYPTO], [

min_crypto_version=ifelse([$1], ,5.1,$1)
AC_MSG_CHECKING([for crypto++ version >= $min_crypto_version])

crypto_pp_file_with_version="cryptlib.h"

CRYPTO_PP_STYLE="unknown"
CRYPTO_PP_LIB_NAME="unknown"
crypto_pp_include_i="unknown"
CRYPTO_PP_INCLUDE_PREFIX="unknown"
CRYPTO_PP_DEFINE="unknown"
CRYPTO_PP_LIB="unknown"

#
# Set CRYPTO_PP_PREFIX if the user has not set it in the configure command line
# We don't use AC_CHECK_FILE to avoid caching.
#
if test x$CRYPTO_PP_PREFIX = x ; then
	CRYPTO_PP_PREFIX="/usr"
fi

#
# Find the Cryptopp header
#
if test -f $CRYPTO_PP_PREFIX/$crypto_pp_file_with_version; then
	CRYPTO_PP_STYLE="sources"
	CRYPTO_PP_LIB_NAME="cryptopp"
	crypto_pp_include_i="$CRYPTO_PP_PREFIX"
	CRYPTO_PP_INCLUDE_PREFIX="."
	CRYPTO_PP_DEFINE="__CRYPTO_SOURCE__"
	CRYPTO_PP_LIB="$CRYPTO_PP_PREFIX"
	AC_MSG_ERROR([
	Specifying the cryptopp source files directory for "--with-crypto-prefix="
	will not work because cryptopp uses headers with the same name of system
	headers (e.g. zlib.h) and you must be able to distinguish the system
	headers from cryptopp headers in an #include directive.
	Please run "PREFIX=/home/YourUserName/usr/cryptopp make install" on
	the cryptopp directory to properly install cryptopp in your system.])
elif test -f $CRYPTO_PP_PREFIX/include/cryptopp/$crypto_pp_file_with_version; then
	CRYPTO_PP_STYLE="installed"
	CRYPTO_PP_LIB_NAME="cryptopp"
	crypto_pp_include_i="$CRYPTO_PP_PREFIX/include"
	CRYPTO_PP_INCLUDE_PREFIX="$CRYPTO_PP_LIB_NAME"
	CRYPTO_PP_DEFINE="__CRYPTO_INSTALLED__"
	CRYPTO_PP_LIB="$CRYPTO_PP_PREFIX/lib"
elif test -f $CRYPTO_PP_PREFIX/include/crypto++/$crypto_pp_file_with_version; then
       	# Debian uses libcrypto++5.1 - it's not my fault, please soda patch the package
	CRYPTO_PP_STYLE="gentoo_debian"
	CRYPTO_PP_LIB_NAME="crypto++"
	crypto_pp_include_i="$CRYPTO_PP_PREFIX/include"
	CRYPTO_PP_INCLUDE_PREFIX="$CRYPTO_PP_LIB_NAME"
	CRYPTO_PP_DEFINE="__CRYPTO_SOURCE__"
	CRYPTO_PP_LIB="$CRYPTO_PP_PREFIX/lib"
fi

#
# Check for success in finding cryptopp
#
if test $CRYPTO_PP_STYLE = "unknown"; then
	#
	# If the execution reaches here, we have failed.
	#
	AC_MSG_ERROR([
	Could not find cryptopp header file "$crypto_pp_file_with_version".
	Please check if the path "$CRYPTO_PP_PREFIX" is valid.])
fi

#
# Find out the cryptopp version and check against the minimum required
#
crypto_pp_include_dir="$crypto_pp_include_i/$CRYPTO_PP_INCLUDE_PREFIX"
crypto_pp_header_path="$crypto_pp_include_dir/$crypto_pp_file_with_version"

CRYPTO_PP_VERSION_STRING=$(grep "Reference Manual" $crypto_pp_header_path | \
	sed -e ['s/[^0-9]*\([0-9.]*\).*/\1/'])

CRYPTO_PP_VERSION_NUMBER=$(echo $CRYPTO_PP_VERSION_STRING | \
	$AWK 'BEGIN { FS = "."; } { printf "%d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}')

minvers=$(echo $min_crypto_version | \
	$AWK 'BEGIN { FS = "."; } { printf "%d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}')

if test -n "$CRYPTO_PP_VERSION_NUMBER" && test "$CRYPTO_PP_VERSION_NUMBER" -ge $minvers; then
	result="yes (version $CRYPTO_PP_VERSION_STRING, $CRYPTO_PP_STYLE)"
else
	result="no"
fi
AC_MSG_RESULT([$result])

#
# FLAGS
#
CRYPTO_PP_CXXFLAGS="-isystem $crypto_pp_include_i -D$CRYPTO_PP_DEFINE"
CRYPTO_PP_LDFLAGS="-L$CRYPTO_PP_LIB"
AH_TEMPLATE([CRYPTOPP_INCLUDE_PREFIX], [Define this to the include prefix of crypto++])
AC_DEFINE_UNQUOTED([CRYPTOPP_INCLUDE_PREFIX], $CRYPTO_PP_INCLUDE_PREFIX)

#
# Exported symbols
#
AC_SUBST([CRYPTO_PP_PREFIX])
AC_SUBST([CRYPTO_PP_VERSION_STRING])
AC_SUBST([CRYPTO_PP_VERSION_NUMBER])

AC_SUBST([CRYPTO_PP_STYLE])
AC_SUBST([CRYPTO_PP_LIB_NAME])
AC_SUBST([CRYPTO_PP_INCLUDE_PREFIX])
AC_SUBST([CRYPTO_PP_CXXFLAGS])
AC_SUBST([CRYPTO_PP_LDFLAGS])

AC_MSG_NOTICE([Crypto++ version number is $CRYPTO_PP_VERSION_NUMBER])
])

