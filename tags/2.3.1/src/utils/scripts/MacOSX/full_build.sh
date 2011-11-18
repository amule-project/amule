#!/bin/bash
##################################################
#             aMule.app bundle creator.          #
##################################################

## This file is part of the aMule Project
##
## Copyright (c) 2011 Angel Vidal ( kry@amule.org )
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either
## version 2 of the License, or (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

SCRIPTDIR=`dirname "$0"`
SCRIPTNAME=`basename "$0"`

## Get full path
SCRIPTDIR=`cd $SCRIPTDIR; pwd`

PATH="$SCRIPTDIR:$PATH"

. defs-global.sh $1
. defs-wx.sh
. defs-functions.sh

echo "Start" > $STDOUT_FILE
echo "Start" > $ERROR_FILE

REPEATSCRIPT=${ROOT_FOLDER}/repeat.sh
if [ -f $REPEATSCRIPT ]; then
	chmod 600 $REPEATSCRIPT
fi
echo "Save configuration commandline to ${REPEATSCRIPT} - execute that script to repeat this compilation with the same options."
echo "#!/bin/bash" > $REPEATSCRIPT
echo "echo \"Starting repeat, moving away current repeat.sh\"" >> $REPEATSCRIPT
echo "rm -rf \${0}.old 2>/dev/null; mv \$0 \${0}.old" >> $REPEATSCRIPT
if [ x"$SDKNUMBER" == x"" ]; then
	SDKSTRING="default"
else
	SDKSTRING=$SDKNUMBER
fi

echo "BUILDARCHS=\"${BUILDARCHS}\" SDKNUMBER=\"${SDKSTRING}\" UNIVERSAL=\"${UNIVERSAL}\" WXVERSION=\"${WXVERSION}\" WXPORT=\"${WXPORT}\" SLIMWX=\"${SLIMWX}\" BUILD_FOLDER=\"${BUILD_FOLDER}\" $0" >> $REPEATSCRIPT
echo "echo \"Repeat finished\"" >> $REPEATSCRIPT
chmod 500 $REPEATSCRIPT

echo "Starting build..."

echo -e "\tGetting aMule sources..."

#Get aMule first, because it may contain patches
if [ -d $AMULE_FOLDER/ ]; then
	if [ -d ${AMULE_FOLDER}/.svn/ ]; then
		echo -e "\t\tSources already exist, updating."
	        pushd $AMULE_FOLDER/ >> $STDOUT_FILE
	        svn up >> $STDOUT_FILE
	        popd >> $STDOUT_FILE
	else
		echo -e "\t\taMule sources at \"" $AMULE_FOLDER "\" are not from SVN checkout, so not updating."
	fi
else
	echo -e "\tFirst checkout."
	if [ "$SVN_REPOSITORY" == "public" ]; then
		SVN_REPOSITORY=http://amule.googlecode.com/svn/trunk/
		echo -e "\tUsing public SVN repository at ${SVN_REPOSITORY}."
	else
		echo -e "\tUsing provided SVN repository at ${SVN_REPOSITORY}."
	fi

	svn co $SVN_REPOSITORY $AMULE_FOLDER >> $STDOUT_FILE
	if [ ! -d $AMULE_FOLDER/ ]; then
		echo "ERROR: aMule sources could not be retrieved. Review your settings."
		exit
	fi
fi

pushd $ROOT_FOLDER >> $STDOUT_FILE

echo -e "\tDone"

echo -e "\tGetting wxWidgets sources..."

if [ -d ${WXFOLDER} ]; then
	pushd ${WXFOLDER} >> $STDOUT_FILE
	svn up >> $STDOUT_FILE 2>> $ERROR_FILE
	popd >> $STDOUT_FILE
else
	echo -e "\tFirst checkout."
	svn checkout http://svn.wxwidgets.org/svn/wx/wxWidgets/${WXSVNROOT} ${WXFOLDER} >> $STDOUT_FILE 2>> $ERROR_FILE
	pushd ${WXFOLDER} >> $STDOUT_FILE
	echo -e "\tApplying patches."
	for i in $AMULE_FOLDER/src/utils/patches/wxWidgets/*.patch; do 
		echo -e "\t\tAppying \"$i\""
		patch -p0 < $i >> $STDOUT_FILE 2>> $ERROR_FILE
	done
	popd >> $STDOUT_FILE
fi	
echo -e "\tDone"

echo -e "\tConfiguring wxWidgets..."

pushd ${WXFOLDER} > $STDOUT_FILE

if [ "$SDKNUMBER" == "" ]; then
	WX_SDK_FLAGS=""	
else
	WX_SDK_FLAGS="--with-macosx-sdk=/Developer/SDKs/MacOSX${SDKRELEASE}.sdk \
	--with-macosx-version-min=$SDKNUMBER"
fi

if [ -e amulewxcompilation ]; then
	echo -e "\t\twxWidgets is already configured"
else
	make clean >> $STDOUT_FILE 2>/dev/null
	./configure CC=gcc$CCVERSION CXX=g++$CCVERSION CPP=cpp$CCVERSION LD=g++$CCVERSION \
	CFLAGS="$CFLAGS $ARCHCPPFLAGS" CXXFLAGS="$CXXFLAGS $ARCHCPPFLAGS" CPPFLAGS="$CPPFLAGS $ARCHCPPFLAGS" \
	LDFLAGS="$LDFLAGS $ARCHCPPFLAGS" \
	OBJCFLAGS="$OBJCFLAGS $ARCHCPPFLAGS" OBJCXXFLAGS="$OBJCXXFLAGS $ARCHCPPFLAGS" \
	--enable-debug --disable-shared \
	$EXTRA_WXFLAGS \
	$ARCHCONFIGFLAGS \
	$WX_SDK_FLAGS >> $STDOUT_FILE 2>> $ERROR_FILE
	touch amulewxcompilation >> $STDOUT_FILE
	echo -e "\t\tConfigured."
fi

echo -e "\t\tDone"

echo -e "\tCompiling wxWidgets..."

make >> $STDOUT_FILE 2>> $ERROR_FILE

echo -e "\tDone"

popd >> $STDOUT_FILE

CRYPTOPP_FOLDER="cryptopp-source"
CRYPTOPP_FOLDER_INST="cryptopp"
CRYPTOPP_URL=`curl -sS http://www.cryptopp.com/ | grep -oE "http://.*/cryptopp/cryptopp[0-9]+\.zip" | sort -r | head -1`

echo -e "\tGetting cryptopp sources..."

if [ -d $CRYPTOPP_FOLDER_INST ]; then
	echo -e "\t\t$CRYPTOPP_FOLDER_INST already exists, skipping (delete and rerun script to get new sources)"	
else
	mkdir $CRYPTOPP_FOLDER
	mkdir $CRYPTOPP_FOLDER_INST
	curl -L -o cryptopp.zip $CRYPTOPP_URL >> $STDOUT_FILE 2>> $ERROR_FILE
	unzip cryptopp.zip -d $CRYPTOPP_FOLDER >> $STDOUT_FILE 2>> $ERROR_FILE
	pushd $CRYPTOPP_FOLDER >> $STDOUT_FILE 
	#./configure 
	for i in $AMULE_FOLDER/src/utils/patches/cryptopp/*.patch; do 
		echo -e "\t\tAppying \"$i\"" 
		patch -p0 < $i >> $STDOUT_FILE 2>> $ERROR_FILE
	done
	#cp ../GNUMakefile .
	echo -e "\t\tCompiling cryptopp..."
	CC=gcc$CCVERSION CXX=g++$CCVERSION CPP=cpp$CCVERSION LD=g++$CCVERSION \
		CXXFLAGS="-pthread $ARCHCPPFLAGS $SDK" CFLAGS="-pthread $ARCHCPPFLAGS $SDK" LDFLAGS="-pthread $SDK" make > $STDOUT_FILE 2> $ERROR_FILE
	PREFIX=${ROOT_FOLDER}/$CRYPTOPP_FOLDER_INST make install >> $STDOUT_FILE 2>> $ERROR_FILE
	popd >> $STDOUT_FILE 
	rm cryptopp.zip >> $STDOUT_FILE 2>> $ERROR_FILE
	rm -rf $CRYPTOPP_FOLDER >> $STDOUT_FILE 2>> $ERROR_FILE
fi

echo -e "\tDone"

# Gettext

echo -e "\tGetting gettext sources..."

GETTEXT_FOLDER="gettext-source"
GETTEXT_FOLDER_INST="gettext-inst"
GETTEXT_URL=`curl -sS http://www.gnu.org/software/gettext/ | grep -m 1 -oE "http://[^\"]+/gettext-([0-9]+\.)+tar.gz" | head -1`

if [ -d $GETTEXT_FOLDER_INST ]; then
	echo -e "\t\t$GETTEXT_FOLDER_INST already exists, skipping"
else
	mkdir $GETTEXT_FOLDER
	mkdir $GETTEXT_FOLDER_INST
	curl -L -o gettext.tar.gz $GETTEXT_URL >> $STDOUT_FILE 2>> $ERROR_FILE
	pushd $GETTEXT_FOLDER >> $STDOUT_FILE 2>> $ERROR_FILE
	echo -e "\t\tCompiling gettext..."
	tar --strip-components 1 -zxf ../gettext.tar.gz >> $STDOUT_FILE 2>> $ERROR_FILE
	./configure CC=gcc$CCVERSION CXX=g++$CCVERSION CPP=cpp$CCVERSION LD=g++$CCVERSION CXXFLAGS="-pthread $ARCHCPPFLAGS $SDK" \
		CFLAGS="-pthread $ARCHCPPFLAGS $SDK" LDFLAGS="-pthread $SDK" \
		--disable-debug --disable-shared --prefix=${ROOT_FOLDER}/$GETTEXT_FOLDER_INST >> $STDOUT_FILE 2>> $ERROR_FILE
	make >> $STDOUT_FILE 2>> $ERROR_FILE
	make install >> $STDOUT_FILE 2>> $ERROR_FILE
	popd >> $STDOUT_FILE 2>> $ERROR_FILE
	rm gettext.tar.gz >> $STDOUT_FILE 2>> $ERROR_FILE
	rm -rf $GETTEXT_FOLDER >> $STDOUT_FILE 2>> $ERROR_FILE
fi

echo -e "\tDone."

#libupnp

echo -e "\tGetting libupnp sources..."

LIBUPNP_FOLDER="libupnp-source"
LIBUPNP_FOLDER_INST="libupnp-inst"
LIBUPNP_URL=`curl -sS http://sourceforge.net/projects/pupnp/files/ | grep -m 1 -ioE "http://sourceforge.net/[^\"]+/libupnp-([0-9]+\.)+tar.bz2/download" | head -1`

if [ -d $LIBUPNP_FOLDER_INST ]; then
	echo -e "\t\t$LIBUPNP_FOLDER_INST already exists, skipping"	
else
	mkdir $LIBUPNP_FOLDER >> $STDOUT_FILE 2>> $ERROR_FILE
	mkdir $LIBUPNP_FOLDER_INST >> $STDOUT_FILE 2>> $ERROR_FILE
	curl -L -o libupnp.tar.bz2 $LIBUPNP_URL  >> $STDOUT_FILE 2>> $ERROR_FILE
	pushd $LIBUPNP_FOLDER >> $STDOUT_FILE 2>> $ERROR_FILE
	echo -e "\t\tCompiling libupnp..."	
	tar --strip-components 1 -jxf ../libupnp.tar.bz2 >> $STDOUT_FILE 2>> $ERROR_FILE
	./configure CC=gcc$CCVERSION CXX=g++$CCVERSION CPP=cpp$CCVERSION LD=g++$CCVERSION CXXFLAGS="-pthread $ARCHCPPFLAGS $SDK" \
		CFLAGS="-pthread $ARCHCPPFLAGS $SDK" LDFLAGS="-pthread $SDK" --disable-dependency-tracking \
		--disable-debug --disable-shared --prefix=${ROOT_FOLDER}/$LIBUPNP_FOLDER_INST >> $STDOUT_FILE 2>> $ERROR_FILE
	make >> $STDOUT_FILE 2>> $ERROR_FILE
	make install >> $STDOUT_FILE 2>> $ERROR_FILE
	popd >> $STDOUT_FILE 2>> $ERROR_FILE
	rm libupnp.tar.bz2 >> $STDOUT_FILE 2>> $ERROR_FILE
	rm -rf $LIBUPNP_FOLDER >> $STDOUT_FILE 2>> $ERROR_FILE
fi

echo -e "\tDone."

#geoip

echo -e "\tGetting GeoIP sources..."

LIBGEOIP_FOLDER="libgeoip-source"
LIBGEOIP_FOLDER_INST="libgeoip-inst"
LIBGEOIP_URL="http://geolite.maxmind.com/download/geoip/api/c/GeoIP.tar.gz"

if [ -d $LIBGEOIP_FOLDER_INST ]; then
	echo -e "\t\t$LIBGEOIP_FOLDER_INST already exists, skipping"	
else
	mkdir $LIBGEOIP_FOLDER >> $STDOUT_FILE 2>> $ERROR_FILE
	mkdir $LIBGEOIP_FOLDER_INST >> $STDOUT_FILE 2>> $ERROR_FILE
	curl -L -o libgeoip.tar.gz $LIBGEOIP_URL >> $STDOUT_FILE 2>> $ERROR_FILE
	pushd $LIBGEOIP_FOLDER >> $STDOUT_FILE 2>> $ERROR_FILE
	echo -e "\t\tCompiling GeoIP..." 
	tar --strip-components 2 -zxf ../libgeoip.tar.gz >> $STDOUT_FILE 2>> $ERROR_FILE
	./configure CC=gcc$CCVERSION CXX=g++$CCVERSION CPP=cpp$CCVERSION LD=g++$CCVERSION CXXFLAGS="-pthread $ARCHCPPFLAGS $SDK" \
		CFLAGS="-pthread $ARCHCPPFLAGS $SDK" LDFLAGS="-pthread $SDK" --disable-dependency-tracking \
		--disable-debug --disable-shared --prefix=${ROOT_FOLDER}/$LIBGEOIP_FOLDER_INST >> $STDOUT_FILE 2>> $ERROR_FILE
	make >> $STDOUT_FILE 2>> $ERROR_FILE
	make install >> $STDOUT_FILE 2>> $ERROR_FILE
	popd >> $STDOUT_FILE 2>> $ERROR_FILE
	rm libgeoip.tar.gz >> $STDOUT_FILE 2>> $ERROR_FILE
	rm -rf $LIBGEOIP_FOLDER >> $STDOUT_FILE 2>> $ERROR_FILE
fi

echo -e "\tDone."

#pkg-config

echo -e "\tGetting pkg-config sources..."

PKGCFG_FOLDER="pkgcfg-source"
PKGCFG_FOLDER_INST="pkgcfg-inst"
# pkgconfig introduced a dependency on glib to build on 0.26, and I refuse to build the whole glib for this. 
# On top of it, glib uses pkgconfig to configure itself... 
#PKGCFG_FILE=`curl -sS http://pkgconfig.freedesktop.org/releases/ | grep -ioE "pkg-config-([0-9]+\.)+tar.gz" | uniq | sort -r | head -1`
#PKGCFG_URL="http://pkgconfig.freedesktop.org/releases/${PKGCFG_FILE}"
PKGCFG_URL="http://pkgconfig.freedesktop.org/releases/pkg-config-0.25.tar.gz"

if [ -f $PKGCFG_FOLDER_INST/bin/pkg-config ]; then
	echo -e "\t\t$PKGCFG_FOLDER_INST already exists, skipping"	
else
	mkdir $PKGCFG_FOLDER >> $STDOUT_FILE 2>> $ERROR_FILE
	mkdir $PKGCFG_FOLDER_INST >> $STDOUT_FILE 2>> $ERROR_FILE
	curl -L -o pkgcfg.tar.gz $PKGCFG_URL >> $STDOUT_FILE 2>> $ERROR_FILE
	pushd $PKGCFG_FOLDER >> $STDOUT_FILE 2>> $ERROR_FILE
	echo -e "\t\tCompiling pkg-config..."	
	tar --strip-components 1 -zxf ../pkgcfg.tar.gz >> $STDOUT_FILE 2>> $ERROR_FILE
	./configure CC=gcc$CCVERSION CXX=g++$CCVERSION CPP=cpp$CCVERSION LD=g++$CCVERSION CXXFLAGS="-pthread $ARCHCPPFLAGS $SDK" \
		CFLAGS="-pthread $ARCHCPPFLAGS $SDK" LDFLAGS="-pthread $SDK" --disable-dependency-tracking \
		--disable-debug --disable-shared --prefix=${ROOT_FOLDER}/$PKGCFG_FOLDER_INST >> $STDOUT_FILE 2>> $ERROR_FILE
	make >> $STDOUT_FILE 2>> $ERROR_FILE
	make install >> $STDOUT_FILE 2>> $ERROR_FILE
	popd >> $STDOUT_FILE 2>> $ERROR_FILE
	if [ -f $PKGCFG_FOLDER_INST/bin/pkg-config ]; then
		rm -f pkgcfg.tar.gz >> $STDOUT_FILE 2>> $ERROR_FILE
		rm -rf $PKGCFG_FOLDER >> $STDOUT_FILE 2>> $ERROR_FILE
	else
		echo -e " ERROR: check output $STDOUT_FILE and $ERROR_FILE for details. "
	fi
fi


echo -e "\tDone."

# aMule

echo -e "\tFINALLY compiling aMule..."

pushd $AMULE_FOLDER >> $STDOUT_FILE 2>> $ERROR_FILE

if [ -f configure ]; then
	echo -e "\t\tConfigure already exists"
else
	PATH="${PATH}:${ROOT_FOLDER}/${GETTEXT_FOLDER_INST}/bin" ./autogen.sh >> $STDOUT_FILE 2>> $ERROR_FILE
fi

if [ -d intl/ ]; then
	echo -e "\t\tGood: intl folder already exists."
else
	mkdir intl >> $STDOUT_FILE 2>> $ERROR_FILE
	touch intl/Makefile.in >> $STDOUT_FILE 2>> $ERROR_FILE
	echo "all:" > intl/Makefile >> $STDOUT_FILE 2>> $ERROR_FILE
	echo "clean:" >> intl/Makefile >> $STDOUT_FILE 2>> $ERROR_FILE
	echo "" >> intl/Makefile >> $STDOUT_FILE 2>> $ERROR_FILE
fi

if [ ! -f src/amule ]; then
	MULECLEAN=YES
fi

rm -rf ${ROOT_FOLDER}/amule-inst/
mkdir -p ${ROOT_FOLDER}/amule-inst/

if [ "$MULECLEAN" == "YES" ]; then
	echo -e "\t\tRunning configure"

	PATH="${PATH}:${ROOT_FOLDER}/${GETTEXT_FOLDER_INST}/bin/:${ROOT_FOLDER}/${PKGCFG_FOLDER_INST}/bin/" \
	 ./configure CC=gcc$CCVERSION CXX=g++$CCVERSION CPP=cpp$CCVERSION LD=g++$CCVERSION \
	CXXFLAGS="-pthread $ARCHCPPFLAGS $SDK" CFLAGS="-pthread $ARCHCPPFLAGS $SDK" LDFLAGS="-pthread $SDK" \
	--enable-nls --disable-dependency-tracking --enable-ccache \
	--with-wxdir=${ROOT_FOLDER}/${WXFOLDER}/ \
	--with-crypto-prefix=${ROOT_FOLDER}/$CRYPTOPP_FOLDER_INST \
	--with-libintl-prefix=${ROOT_FOLDER}/${GETTEXT_FOLDER_INST} \
	--with-libupnp-prefix=${ROOT_FOLDER}/${LIBUPNP_FOLDER_INST} \
	--with-geoip-static --with-geoip-headers=${ROOT_FOLDER}/${LIBGEOIP_FOLDER_INST}/include --with-geoip-lib=${ROOT_FOLDER}/${LIBGEOIP_FOLDER_INST}/lib/ \
	--disable-cas --disable-webserver --disable-amulecmd --disable-amule-gui --disable-wxcas --disable-alc --disable-alcc --disable-amule-daemon \
	--prefix=${ROOT_FOLDER}/amule-inst/ >> $STDOUT_FILE 2>> $ERROR_FILE

	echo -e "\t\tCleaning compilation"

	make clean >> $STDOUT_FILE 2>> $ERROR_FILE
fi

echo -e "\t\tCompiling aMule"

make >> $STDOUT_FILE 2>> $ERROR_FILE

echo -e "\tDone."

echo -e "\t\tFaking install"

make install >> $STDOUT_FILE 2>> $ERROR_FILE

echo -e "\tDone."

popd >> $STDOUT_FILE 2>> $ERROR_FILE

echo -e "Getting application bundle and packaging"

rm -rf aMule.app aMule.zip >> $STDOUT_FILE 2>> $ERROR_FILE

cp -R ${AMULE_FOLDER}/aMule.app . >> $STDOUT_FILE 2>> $ERROR_FILE

find aMule.app \( -name .svn -o -name "Makefile*" -o -name src \) -print0 | xargs -0 rm -rf >> $STDOUT_FILE 2>> $ERROR_FILE

echo -e "Copying i18n files..."
cp -r amule-inst/share/locale aMule.app/Contents/SharedSupport/
echo -e "Done."

. application_packager.sh ${AMULE_FOLDER}/ >> $STDOUT_FILE 2>> $ERROR_FILE

if [ ! -f aMule.zip ]; then
	echo "ERROR: aMule.zip was not created. Please review the output files"
else
	echo "All Done"
fi

# Pop root folder.
popd >> $STDOUT_FILE
