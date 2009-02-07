#!/bin/sh

# Create Makefile.am
echo > Makefile.am
echo 'EXTRA_DIST = \' >> Makefile.am
ls *.xpm | sed -e 's/^/	/' -e 's/$/ \\/' >> Makefile.am
echo '	makeflags.sh' >> Makefile.am

# Create CountryFlags.h and header
echo > CountryFlags.h
echo >> CountryFlags.h
echo '#ifndef COUNTRY_FLAGS_H' >> CountryFlags.h
echo '#define COUNTRY_FLAGS_H' >> CountryFlags.h
echo >> CountryFlags.h
echo 'namespace flags {' >> CountryFlags.h
echo >> CountryFlags.h

# Create include directives
ls *.xpm | sed -e 's/^/#include "/' -e 's/$/"/' >> CountryFlags.h
echo >> CountryFlags.h
echo >> CountryFlags.h

# Define the struct
echo 'struct FlagXPMCode' >> CountryFlags.h
echo '{' >> CountryFlags.h
echo '	const char **xpm;' >> CountryFlags.h
echo '	const char *code;' >> CountryFlags.h
echo '};' >> CountryFlags.h
echo >> CountryFlags.h
echo >> CountryFlags.h

# "do" is a reserved word, we can't use it
#sed -i -e 's/do\[\]/do_\[\]/' do.xpm

# Create the flag/Code vector
echo 'static struct FlagXPMCode flagXPMCodeVector[] = {'>> CountryFlags.h
ls *.xpm | sed -e 's/\.xpm$//;/\//s:.*/\([^/][^/]*\):\1:' | \
	sed -e 's/[A-Za-z]*/	{&, "&"},/' | \
	sed -e 's/do/do_/1' \
	>> CountryFlags.h
echo '};'>> CountryFlags.h
echo >> CountryFlags.h
echo >> CountryFlags.h

# Calculate the vector size
echo 'static const int FLAGS_XPM_SIZE = (sizeof flagXPMCodeVector) / (sizeof flagXPMCodeVector[0]);' >> CountryFlags.h
echo >> CountryFlags.h
echo >> CountryFlags.h

# Finish
echo '}	// namespace flags' >> CountryFlags.h
echo '#endif // COUNTRY_FLAGS_H' >> CountryFlags.h

