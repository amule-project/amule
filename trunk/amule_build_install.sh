#! /bin/bash -x

#
# Example usage:
# $ ./amule_build_install.sh cryptopp552.zip libupnp-1.6.3.tar.bz2 wxWidgets-2.8.7.tar.gz aMule-CVS-20080111.tar.bz2
#
# Of course, all the above files must be in the current directory.
#

USERHOME=$(echo ~)
JOBS=4

#
# cryptopp
#
CRYPTOPP_INSTALL_DIR="${USERHOME}/usr/cryptopp"
CRYPTOPP_SOURCES_DIR="cryptopp_tmpdir"

rm -rf ${CRYPTOPP_SOURCES_DIR}
mkdir -p ${CRYPTOPP_SOURCES_DIR}
cd ${CRYPTOPP_SOURCES_DIR}

unzip ../$1
make -f GNUmakefile -j${JOBS}
PREFIX=${CRYPTOPP_INSTALL_DIR} make install
cd ..

#
# libUPnP
#
LIBUPNP_INSTALL_DIR="${USERHOME}/usr/libupnp"
tar jxf $2
cd libupnp-?.?.?
./configure \
	--enable-debug \
	--prefix=${LIBUPNP_INSTALL_DIR} \
	&& \
	make -j${JOBS} > /dev/null && \
	make install > /dev/null
cd ..

#
# wxWidgets
#
WXWIDGETS_INSTALL_DIR="${USERHOME}/usr/wxWidgets"
tar zxf $3
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

#
# aMule
#
AMULE_INSTALL_DIR="${USERHOME}/usr/amule"
tar jxf $4
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
	&& \
	LD_LIBRARY_PATH=${WXWIDGETS_INSTALL_DIR}/lib make -j${JOBS} && \
	LD_LIBRARY_PATH=${WXWIDGETS_INSTALL_DIR}/lib make install > /dev/null


echo 
echo Finished compilation
echo You should run aMule like this:
echo '$LD_LIBRARY_PATH'=${WXWIDGETS_INSTALL_DIR}/lib:${LIBUPNP_INSTALL_DIR}/lib LANG=en_US.UTF-8 ${AMULE_INSTALL_DIR}/bin/amule
echo
echo Of course you may use a different locale.
echo

