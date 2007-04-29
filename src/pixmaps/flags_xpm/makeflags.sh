#! /bin/bash

# Create Makefile.am
echo > Makefile.am
echo 'EXTRA_DIST = \' >> Makefile.am
ls *.xpm | sed -e 's/^/\t/' | sed -e 's/$/ \\/' >> Makefile.am
echo '	makeflags.sh' >> Makefile.am

# Create CountryFlags.h and header
echo > CountryFlags.h
echo >> CountryFlags.h
echo '#ifndef COUNTRY_FLAGS_H' >> CountryFlags.h
echo '#define COUNTRY_FLAGS_H' >> CountryFlags.h
echo >> CountryFlags.h
echo >> CountryFlags.h

# Create include directives
ls *.xpm | sed -e 's/^/#include "/' | sed -e 's/$/"/' >> CountryFlags.h
echo >> CountryFlags.h
echo >> CountryFlags.h

# Define the struct
echo 'struct FlagXPMCode' >> CountryFlags.h
echo '{' >> CountryFlags.h
echo '	char **xpm;' >> CountryFlags.h
echo '	char *code;' >> CountryFlags.h
echo '};' >> CountryFlags.h
echo >> CountryFlags.h
echo >> CountryFlags.h

# "do" is a reserved word, we can't use it
sed -i -e 's/do\[\]/do_\[\]/' do.xpm

# globally used names by wx
sed -i -e 's/ht\[\]/ht_\[\]/' ht.xpm
sed -i -e 's/it\[\]/it_\[\]/' it.xpm
sed -i -e 's/sz\[\]/sz_\[\]/' sz.xpm

# Create the flag/Code vector
echo 'static struct FlagXPMCode flagXPMCodeVector[] = {'>> CountryFlags.h
ls *.xpm | xargs -I '{}' basename '{}' .xpm | \
	sed -e 's/[A-Za-z]*/\t{&, "&"},/' | \
	sed -e 's/do/do_/1' |
	sed -e 's/ht/ht_/1' |
	sed -e 's/it/it_/1' |
	sed -e 's/sz/sz_/1' \
	>> CountryFlags.h
echo '};'>> CountryFlags.h
echo >> CountryFlags.h
echo >> CountryFlags.h

# Calculate the vector size
echo 'static const int FLAGS_XPM_SIZE = (sizeof flagXPMCodeVector) / (sizeof flagXPMCodeVector[0]);' >> CountryFlags.h
echo >> CountryFlags.h
echo >> CountryFlags.h

# Finish
echo '#endif // COUNTRY_FLAGS_H' >> CountryFlags.h

