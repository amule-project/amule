#!/bin/bash
##################################################
#             aMule.app bundle creator.          #
##################################################

## This file is part of the aMule Project
##
## Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
## Copyright (c) 2003-2011 aMule Team     ( http://www.amule-project.net )
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

SRC_FOLDER=$1


if [ -z $SRC_FOLDER ]; then
	SRC_FOLDER="./"
fi

# Ensure empty directories exist
for pkg in aMule.app aMuleGUI.app ; do
	for d in Frameworks MacOS SharedSupport SharedSupport/locale ; do
		[ -d $pkg/Contents/$d ] || mkdir $pkg/Contents/$d
	done
done

echo ""
echo -n "Step 1: Cleaning bundles... "
rm aMule.app/Contents/Frameworks/libwx_* aMule.app/Contents/MacOS/* 1> /dev/null 2> /dev/null
rm aMuleGUI.app/Contents/Frameworks/libwx_* aMuleGUI.app/Contents/MacOS/* 1> /dev/null 2> /dev/null
rm -r aMule.app/Contents/SharedSupport 1> /dev/null 2> /dev/null
echo "Done"
echo ""
echo -n "Step 2.1: Copying aMule to app bundle... "
cp ${SRC_FOLDER}/src/amule aMule.app/Contents/MacOS/
cp ${SRC_FOLDER}/src/webserver/src/amuleweb aMule.app/Contents/MacOS/
cp ${SRC_FOLDER}/src/ed2k aMule.app/Contents/MacOS/
cp ${SRC_FOLDER}/src/amulecmd aMule.app/Contents/MacOS/
cp ${SRC_FOLDER}/platforms/MacOSX/aMule-Xcode/amule.icns aMule.app/Contents/Resources/
cp -R ${SRC_FOLDER}/src/webserver aMule.app/Contents/Resources
find aMule.app/Contents/Resources/webserver \( -name .svn -o -name "Makefile*" -o -name src \) -print0 | xargs -0 rm -rf
echo "Done"
echo -n "Step 2.2: Copying aMuleGUI to app bundle... "
cp ${SRC_FOLDER}/src/amulegui aMuleGUI.app/Contents/MacOS/
cp ${SRC_FOLDER}/platforms/MacOSX/aMule-Xcode/amule.icns aMuleGUI.app/Contents/Resources/
echo "Done"
echo ""
echo -n "Step 3: Installing translations to app bundle... "
orig_dir=`pwd`
pushd ${SRC_FOLDER}/po
make install datadir=$orig_dir/aMule.app/Contents/SharedSupport 1> /dev/null 2> /dev/null
make install datadir=$orig_dir/aMuleGUI.app/Contents/SharedSupport 1> /dev/null 2> /dev/null
popd
echo "Done"
echo ""
echo "Step 4: Copying libs to Framework"
echo "    wxWidgets..."
# wxWidgets libs to frameworks
for i in $( otool -L	aMule.app/Contents/MacOS/amule \
						aMule.app/Contents/MacOS/amuleweb \
						aMule.app/Contents/MacOS/ed2k \
						aMule.app/Contents/MacOS/amulecmd \
			| sort -u | grep libwx_ | cut -d " " -f 1 ); do
	cp $i aMule.app/Contents/Frameworks;
done
for i in $( otool -L	aMuleGUI.app/Contents/MacOS/amulegui \
			| sort -u | grep libwx_ | cut -d " " -f 1 ); do
	cp $i aMuleGUI.app/Contents/Frameworks;
done
echo "Libs copy done."
echo ""
echo "Step 5: Update libs info"
#then install_name_tool on them to fix the path on the shared lib
pushd aMule.app/Contents/
for i in $( ls Frameworks | grep -v CVS); do
	echo "    Updating $i"
	#update library id
	install_name_tool -id @executable_path/../Frameworks/$i Frameworks/$i
	#update library links
	for j in $( otool -L Frameworks/$i | grep libwx_ | cut -d " " -f 1 ); do
	        install_name_tool -change \
                $j @executable_path/../Frameworks/`echo $j | rev | cut -d "/" -f 1 | rev` \
		Frameworks/$i 1> /dev/null 2> /dev/null

	done
	echo "    Updating aMule lib info for $i"
	#update amule executable
	install_name_tool -change \
		`otool -L MacOS/amule | grep $i | cut -d " " -f 1` \
		@executable_path/../Frameworks/$i MacOS/amule 1> /dev/null 2> /dev/null
	install_name_tool -change \
		`otool -L MacOS/amuleweb | grep $i | cut -d " " -f 1` \
		@executable_path/../Frameworks/$i MacOS/amuleweb 1> /dev/null 2> /dev/null
	install_name_tool -change \
		`otool -L MacOS/ed2k | grep $i | cut -d " " -f 1` \
		@executable_path/../Frameworks/$i MacOS/ed2k 1> /dev/null 2> /dev/null
	install_name_tool -change \
		`otool -L MacOS/amulecmd | grep $i | cut -d " " -f 1` \
		@executable_path/../Frameworks/$i MacOS/amulecmd 1> /dev/null 2> /dev/null
done
popd
pushd aMuleGUI.app/Contents/
for i in $( ls Frameworks | grep -v CVS); do
	echo "    Updating $i"
	#update library id
	install_name_tool -id @executable_path/../Frameworks/$i Frameworks/$i
	#update library links
	for j in $( otool -L Frameworks/$i | grep libwx_ | cut -d " " -f 1 ); do
	        install_name_tool -change \
                $j @executable_path/../Frameworks/`echo $j | rev | cut -d "/" -f 1 | rev` \
		Frameworks/$i 1> /dev/null 2> /dev/null

	done
	echo "    Updating aMule lib info for $i"
	#update amule executable
	install_name_tool -change \
		`otool -L MacOS/amulegui | grep $i | cut -d " " -f 1` \
		@executable_path/../Frameworks/$i MacOS/amulegui 1> /dev/null 2> /dev/null
done
echo "Libs info updated, aMule.app and aMuleGUI.app are ready to package."
echo ""
popd
echo -n "Creating aMule.zip... "
zip -9 -r aMule.zip aMule.app/ > /dev/null
zip -9 -j ${SRC_FOLDER}/docs/README.Mac.txt ${SRC_FOLDER}/docs/COPYING > /dev/null
echo "Done"
echo -n "Creating aMuleGUI.zip... "
zip -9 -r aMuleGUI.zip aMuleGUI.app/ > /dev/null
zip -9 -j ${SRC_FOLDER}/docs/README.Mac.txt ${SRC_FOLDER}/docs/COPYING > /dev/null
echo "Done"
echo ""
