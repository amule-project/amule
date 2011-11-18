#! /bin/bash


#
# Copyright (c) 2004-2011 Marcelo Roberto Jimenez ( phoenix@amule.org )
#
# All rights reserved. This script is provided under the terms of the GPL.
#


#
# Example usage: copy all tarballs to a directory, cd to it and
# $ ./amule_build_install.sh
#


SCRIPT_VERSION="2.0.0"


USERHOME=$(echo ~)
DEFAULT_PREFIX="${USERHOME}/usr"
DEFAULT_TARDIR=$(pwd)
DEFAULT_UNTARDIR=${DEFAULT_TARDIR}/untars
JOBS=4


#
# This function uses three parameters:
#    $1 - an input parameter specifying a mask for the distribution, 
#         e.g., 'aMule-CVS-*' for cvs tarballs or 'aMule-*' for distro tarballs.
#    $2 - an output parameter with the name of the variable that will
#         receive the full file name of the tar archive
#    $3 - an output parameter with the name of the variable that will
#         receive the basename of the distribution
#
function lookup_distro {
    # assign the filename of distribution
    # the following is equivalent to execute XXX_FILENAME=$(ls ${TARDIR}/$1)
    eval $2=$(ls ${TARDIR}/$1)

    # Now we use indirection to dereference $2
    #echo $2    # evaluates to XXX_FILENAME
    #echo ${!2} # evaluates to /home/user/dir/xxx-y.z.t.tar.gz

    # remove the directory and the extension parts and assing it 
    # to XXX_DISTRO in $3
    case ${!2} in
	*.gz)
	    eval $3=$(basename ${!2} .tar.gz)
	    ;;
	
	*.bz2)
	    eval $3=$(basename ${!2} .tar.bz2)
	    ;;
	
	*.zip)
	    eval $3=$(basename ${!2} .zip)
	    ;;
    esac
}


function untar_distro {
    # $1 evaluates to /home/user/dir/xxx-y.z.t.tar.gz or .bz2
    local TARCMD=
    case $1 in
	*.gz)
	    TARCMD="tar zxf"
	    ;;
	*.bz2)
	    TARCMD="tar jxf"
	    ;;
	*.zip)
	    TARCMD="unzip -ao"
	    ;;
    esac
    $TARCMD $1
}


function init_package_versions {
    #
    # Put the software distributions in directory $TARDIR and the
    # script does the rest. There can even be a mixture of .tar.gz and
    # .tar.bz2 because the untarring routine tests it on the fly
    #
    # single quotes have to be used here to avoid expansion
    #
    lookup_distro 'cryptopp*'  CRYPTOPP_FILENAME CRYPTOPP_DISTRO
    lookup_distro 'libupnp-*' LIBUPNP_FILENAME LIBUPNP_DISTRO
    lookup_distro 'wxWidgets-*.*.*.tar.*' WXWIDGETS_FILENAME WXWIDGETS_DISTRO
    lookup_distro 'aMule-*' AMULE_FILENAME AMULE_DISTRO

    echo
    echo "Software packacge versions:"
    echo "    cryptopp  : $CRYPTOPP_DISTRO"
    echo "    libupnp   : $LIBUPNP_DISTRO"
    echo "    wxWidgets : $WXWIDGETS_DISTRO"
    echo "    aMule     : $AMULE_DISTRO"
    echo
}


function init_environment {
    echo
    echo aMule building script, version $SCRIPT_VERSION
    echo

    # only prompt if we're missing information
    if [ x$PREFIX = x ] || [ x$TARDIR = x ] || [ x$UNTARDIR = x ] ; then

	PREFIX=${DEFAULT_PREFIX}
	echo "Where to install?"
	read -p "[${PREFIX}]: "
	if [ x$REPLY != x ]; then PREFIX=${REPLY}; fi

	echo
	echo "Where are the tarballs?"
	TARDIR=${DEFAULT_TARDIR}
	read -p "[${TARDIR}]: "
	if [ x$REPLY != x ]; then TARDIR=${REPLY}; fi

	echo
	echo "Where to untar?"
	UNTARDIR=${DEFAULT_UNTARDIR}
	read -p "[${UNTARDIR}]: "
	if [ x$REPLY != x ]; then UNTARDIR=${REPLY}; fi

    fi

    # test that we have write permissions to the install dir
    TST_DIR=${PREFIX}/tmp
    TST_FILE=${TST_DIR}/test-if-has-write-permission
    mkdir -p ${TST_DIR}
    touch ${TST_FILE}
    if [ ! -w ${TST_FILE} ]; then
	echo "You don't appear to have write permissions to ${PREFIX}."
	echo "You must fix that before continuing."
	exit
    fi
    rm -f ${TST_FILE}

    echo
    echo "Building for:"
    echo "    --prefix=${PREFIX}"
    echo "    tarballs are at ${TARDIR}"
    echo "    tarballs will be untarred at ${UNTARDIR}"
    echo

    # Initialize package version variables
    init_package_versions
}


#
# cryptopp
#
function build_cryptopp {
	CRYPTOPP_INSTALL_DIR="${PREFIX}/cryptopp"
	CRYPTOPP_SOURCES_DIR="cryptopp_tmpdir"

	rm -rf ${CRYPTOPP_SOURCES_DIR}
	mkdir -p ${CRYPTOPP_SOURCES_DIR}
	cd ${CRYPTOPP_SOURCES_DIR}

	untar_distro ${CRYPTOPP_FILENAME}

	make -f GNUmakefile -j${JOBS}
	PREFIX=${CRYPTOPP_INSTALL_DIR} make install
	cd ..
}


#
# libUPnP
#
function build_libupnp {
	untar_distro ${LIBUPNP_FILENAME}

	LIBUPNP_INSTALL_DIR="${PREFIX}/libupnp"
	cd libupnp-?.?.?
	./configure \
		--enable-debug \
		--prefix=${LIBUPNP_INSTALL_DIR} \
		&& \
		make -j${JOBS} > /dev/null && \
		make install > /dev/null
	cd ..
}


#
# wxWidgets
#
function build_wxwidgets {
	untar_distro ${WXWIDGETS_FILENAME}
	
	WXWIDGETS_INSTALL_DIR="${PREFIX}/wxWidgets"
	cd wxWidgets-?.?.?
	./configure \
		--enable-mem_tracing \
		--enable-debug \
		--disable-optimise \
		--enable-debug_flag \
		--enable-debug_info \
		--enable-debug_gdb \
		--with-opengl \
		--enable-gtk2 \
		--enable-unicode \
		--enable-largefile \
		--prefix=${WXWIDGETS_INSTALL_DIR} \
		&& \
		make -j${JOBS} > /dev/null &&
		make install > /dev/null
	cd ..
}


#
# aMule
#
function build_amule {
	untar_distro ${AMULE_FILENAME}

	AMULE_INSTALL_DIR="${PREFIX}/amule"
	cd amule-cvs
	./configure \
		--enable-ccache \
		--with-denoise-level=3 \
		--enable-debug \
		--disable-optimize \
		--enable-verbose \
		--enable-geoip \
		--enable-cas \
		--enable-wxcas \
		--enable-amule-gui \
		--enable-webserver \
		--enable-amulecmd \
		--enable-amule-daemon \
		--with-wx-config=${WXWIDGETS_INSTALL_DIR}/bin/wx-config \
		--prefix=${AMULE_INSTALL_DIR} \
		--with-crypto-prefix=${CRYPTOPP_INSTALL_DIR} \
		--with-libupnp-prefix=${LIBUPNP_INSTALL_DIR} \
		&& \
		LD_LIBRARY_PATH=${WXWIDGETS_INSTALL_DIR}/lib make -j${JOBS} && \
		LD_LIBRARY_PATH=${WXWIDGETS_INSTALL_DIR}/lib make install > /dev/null
}

#
# Here is where the fun begins...
#
init_environment

#
# Go to ${UNTARDIR}
#
mkdir -p ${UNTARDIR}
cd ${UNTARDIR}

#
# Build stuff
#
build_cryptopp
build_libupnp
build_wxwidgets
build_amule

#
# Leave ${UNTARDIR}
#
cd ..


echo 
echo Finished compilation.
echo You should run aMule like this:
echo '$LD_LIBRARY_PATH'=${WXWIDGETS_INSTALL_DIR}/lib:${LIBUPNP_INSTALL_DIR}/lib LANG=en_US.UTF-8 ${AMULE_INSTALL_DIR}/bin/amule
echo
echo Of course you may choose to use a different locale.
echo

