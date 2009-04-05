#!/bin/sh
# Helps bootstrapping 'aMule' when checked out from the source control system.
# Requires GNU autoconf, GNU automake and GNU which.

WANT_AUTOMAKE="1.7"
export WANT_AUTOMAKE
(autoconf --version) >/dev/null 2>/dev/null || (echo "You need GNU autoconf to install from sources (ftp://ftp.gnu.org/gnu/autoconf/)"; exit 1) || exit 1
(automake --version) >/dev/null 2>/dev/null || (echo "You need GNU automake 1.7 to install from sources (ftp://ftp.gnu.org/gnu/automake/)"; exit 1) || exit 1

# Do sanity checks.
# Directory check.
if [ ! -e src/SharedFileList.h ]; then
    echo "Run ./autogen.sh from the base directory of aMule."
    exit 1
fi

# Determine the version of automake.
automake_version=`automake --version | head -n 1 | sed -e 's/[^12]*\([12]\.[0-9]+[^ ]*\).*/\1/'`

# Require automake 1.7.
if expr "1.7" \> "$automake_version" >/dev/null; then
  automake --version | head -n 1
  echo "Fatal error: automake version 1.7 or higher is required."
  exit 1
fi

# Determine version of gettext.
gettext_version=`gettext --version | head -n 1 | sed -e 's/[^0]*\(0\.[0-9][^ ]*\).*/\1/'`
confver=`cat configure.in | grep '^AM_GNU_GETTEXT_VERSION(' | sed -e 's/^AM_GNU_GETTEXT_VERSION(\([^()]*\))/\1/p' | sed -e 's/^\[\(.*\)\]$/\1/' | sed -e 1q`

# Require version as specified in configure.in.
if expr "$confver" \> "$gettext_version" >/dev/null; then
  gettext --version | head -n 1
  echo "Fatal error: gettext version "$confver" or higher is required."
  exit 1
fi

# Force intl regenration to get last update from installed gettext templates
rm -rf intl/*
#if [ ! -d intl ]; then
    echo "Setting up internationalization files."
    autopoint --force
    if grep -q datarootdir po/Makefile.in.in; then
        echo autopoint honors dataroot variable, not patching.
    else 
	echo autopoint does not honor dataroot variable, patching.
        sed -e '/^datadir *=/a\
datarootdir = @datarootdir@' po/Makefile.in.in > po/Makefile.in.in.tmp && mv -f po/Makefile.in.in.tmp po/Makefile.in.in
        sed -e '/^datadir *=/a\
datarootdir = @datarootdir@' intl/Makefile.in > intl/Makefile.in.tmp && mv -f intl/Makefile.in.tmp intl/Makefile.in
    fi
    sed -e '/^clean:/a\
	rm -f *.gmo' po/Makefile.in.in > po/Makefile.in.in.tmp && mv -f po/Makefile.in.in.tmp po/Makefile.in.in
#    if [ -f Makefile -a -x config.status ]; then
#        CONFIG_FILES=intl/Makefile CONFIG_HEADERS= /bin/sh ./config.status
#    fi
#   gettextize --intl -f --no-changelog
#   echo "restoring Makefile.am and configure.in"
#   cp -f Makefile.am~ Makefile.am
#   cp -f configure.in~ configure.in
#fi

echo "Running aclocal -I m4"
aclocal -I m4

echo "Running autoheader"
autoheader

echo "Running autoconf"
autoconf

echo "Creating pixmaps Makefile.am"
OLDPWD="`pwd`"
cd src/pixmaps/flags_xpm
./makeflags.sh
cd "$OLDPWD"

echo "Running automake --foreign -a -c -f"
automake --foreign -a -c -f
