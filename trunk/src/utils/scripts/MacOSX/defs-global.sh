# Global definitions for MacOSX builds
# (c) 2011 Angel Vidal ( kry@amule.org )
# Public domain. Use at your own risk.

. defs-functions.sh

# Wouldn't I love to have bash4 on MacOSX (case fallthrough)

pn "-- Begin global def section --"

if [ "$SDKNUMBER" == "" ]; then
	pc $BLUE "\tAutomatically setting SDK to 10.4u (tiger with i386 and ppc, gcc 4.0) - set SDKNUMBER to your preferred SDK if you want to target it (10.5, 10.6) or \"default\" for the default SDK."
	SDKNUMBER=10.4
fi

case "$SDKNUMBER" in
"10.4")
	SDKRELEASE=10.4u
	CCVERSION="-4.0"
	;;
"10.5"|"10.6")
	SDKRELEASE=$SDKNUMBER
	;;
default)
	;;
*)
	pc $RED "Valid SDKNUMBER values are 10.4, 10.5, 10.6 and default."
	exit
	;;
esac

pc $GREEN "\tUsing $SDKNUMBER SDK"

if [ "$SDKNUMBER" != "default" ]; then
	SDK="-isysroot /Developer/SDKs/MacOSX${SDKRELEASE}.sdk -mmacosx-version-min=$SDKNUMBER"
fi

if [ "$UNIVERSAL" == "" ]; then
	pc $BLUE "\tAutomatically enabling universal (i386, ppc) build. Set UNIVERSAL=NO to build just for the current architecture."
	UNIVERSAL="YES"
fi

case "$UNIVERSAL" in
"NO"|"no")
	pc $GREEN "\tDisabling universal build"
	ARCHCPPFLAGS=""
	ARCHCONFIGFLAGS=""
	;;
"YES"|"yes")
	pc $GREEN "\tUsing universal build (i386, pcc)" 
	ARCHCPPFLAGS="-arch i386 -arch ppc"
	ARCHCONFIGFLAGS="--enable-universal_binary"
	;;
*)
	pc $RED "Only the values \"NO\" and \"YES\" are valid for UNIVERSAL"
	exit
	;;
esac

ROOT_FOLDER=`pwd`

pc $GREEN "\tBuild root folder is $ROOT_FOLDER"

SVN_REPOSITORY=$1

AMULE_FOLDER="amule-dev"

if [ "$STDOUT_FILE" == "" ]; then
	STDOUT_FILE=build_output
fi

if [ "$ERROR_FILE" == "" ]; then
	ERROR_FILE=error_output
fi

pc $GREEN "\tErrors will be redirected to $ERROR_FILE, normal build output to $STDOUT_FILE"

pn "-- End global def section --"
