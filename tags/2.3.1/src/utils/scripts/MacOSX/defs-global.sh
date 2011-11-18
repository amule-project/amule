# Global definitions for MacOSX builds
# (c) 2011 Angel Vidal ( kry@amule.org )
# Public domain. Use at your own risk.

. defs-functions.sh

# Wouldn't I love to have bash4 on MacOSX (case fallthrough)

pn "-- Begin global def section --"

SVN_REPOSITORY=$1

if [ -z $SVN_REPOSITORY ]; then
        AMULE_FOLDER=`cd ${SCRIPTDIR}/../../../../; pwd`
	if [ -f "${AMULE_FOLDER}/src/amule.cpp" ]; then
		pc $GREEN "\tSVN repository has not been specified, using detected local sources at \"${AMULE_FOLDER}\""
	else
		pc $RED "\tSVN repository has not been specified, and no local sources found at \"${AMULE_FOLDER}\".\n\tPlease specify your aMule source folder by setting the AMULE_FOLDER variable, or call the script with the SVN repository of your choice as a first parameter, or \"public\" to download from our public SVN repository."
		exit
	fi
fi

if [ -z "$BUILD_FOLDER" ]; then
	if [ -z $SVN_REPOSITORY ]; then
		BUILD_FOLDER="$AMULE_FOLDER/build/"
		mkdir $BUILD_FOLDER 2>/dev/null
	else
		BUILD_FOLDER=`pwd`
	fi

	pc $BLUE "\tAutomatically setting build folder to ${BUILD_FOLDER} - set BUILD_FOLDER to your preferred folder to change this setting."
else
	pc $GREEN "\tSetting build folder to $BUILD_FOLDER"
fi

ROOT_FOLDER=`cd $BUILD_FOLDER; pwd`

if [ -z $AMULE_FOLDER ]; then
        AMULE_FOLDER="${ROOT_FOLDER}/amule-dev"
fi

pc $GREEN "\tBuild root absolute path is $ROOT_FOLDER"

if [ "$SDKNUMBER" == "" ]; then
	pc $BLUE "\tAutomatically setting SDK to 10.4u (tiger with i386 and ppc, gcc 4.0) - set SDKNUMBER to your preferred SDK if you want to target it (10.5, 10.6, 10.7) or \"default\" for the default SDK."
	SDKNUMBER=10.4
fi

case "$SDKNUMBER" in
"10.4")
	SDKRELEASE=10.4u
	CCVERSION="-4.0"
	;;
"10.5"|"10.6"|"10.7")
	SDKRELEASE=$SDKNUMBER
	;;
default)
	;;
*)
	pc $RED "Valid SDKNUMBER values are 10.4, 10.5, 10.6, 10.7 and default."
	exit
	;;
esac

pc $GREEN "\tUsing $SDKNUMBER SDK"

if [ "$SDKNUMBER" != "default" ]; then
	SDK="-isysroot /Developer/SDKs/MacOSX${SDKRELEASE}.sdk -mmacosx-version-min=$SDKNUMBER"
else
	SDKNUMBER=""
fi

if [ "$UNIVERSAL" == "" ]; then
	pc $BLUE "\tAutomatically enabling universal (i386, ppc) build. Set UNIVERSAL=NO to build just for the current architecture."
	UNIVERSAL="YES"
fi

case "$UNIVERSAL" in
"NO"|"no")
	pc $GREEN "\tDisabling universal build"
	ARCHCPPFLAGS="$BUILDARCHS"
	ARCHCONFIGFLAGS=""
	;;
"YES"|"yes")
	pc $GREEN "\tUsing universal build (i386, pcc)" 
	ARCHCPPFLAGS="-arch i386 -arch ppc"
	ARCHCONFIGFLAGS="--enable-universal_binary"
	CCVERSION="-4.0"
	;;
*)
	pc $RED "Only the values \"NO\" and \"YES\" are valid for UNIVERSAL"
	exit
	;;
esac

if [ "$STDOUT_FILE" == "" ]; then
	STDOUT_FILE=${ROOT_FOLDER}/build_output
fi

if [ "$ERROR_FILE" == "" ]; then
	ERROR_FILE=${ROOT_FOLDER}/error_output
fi

pc $GREEN "\tErrors will be redirected to $ERROR_FILE, normal build output to $STDOUT_FILE"

pn "-- End global def section --"
