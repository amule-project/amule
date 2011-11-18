# Definitions for MacOSX compilation of wxWidgets. 
# (c) 2011 Angel Vidal ( kry@amule.org )
# Public domain. Use at your own risk.

. defs-functions.sh

pn "-- Begin wxWidgets def section --"

if [ "$WXVERSION" == "" ]; then
	pc $BLUE "\tAutomatically setting wxWidgets version to 2.8.x - set WXVERSION to your desired version (accepted values are 2.8 and svn right now) if you want to use a different one."
	WXVERSION=2.8
fi

case "$WXVERSION" in
"2.8")
	WXSVNROOT="branches/WX_2_8_BRANCH"
	WXFOLDER="wxWidgets-2.8"
	AUTOPORT=carbon
	;;
"svn"|"SVN")
	WXSVNROOT="trunk/"
	WXFOLDER="wxWidgets"
	AUTOPORT=cocoa
	;;
*)
	pc $RED "Valid WXVERSION values are 2.8 and svn."
	exit
	;;
esac

pc $GREEN "\tUsing wxWidgets $WXVERSION"

if [ "$WXPORT" = "" ]; then
	pc $BLUE "\tAutomatically selecting the $AUTOPORT port for wxWidgets $WXVERSION - set WXPORT to your desired port (accepted values are carbon and cocoa right now) if you want to use a different one."
	WXPORT=$AUTOPORT
fi

case "$WXPORT" in
COCOA|cocoa)
	if [ "$WXVERSION" == "2.8" ]; then
		pc $RED "The $WXWIDGETS port on wxWidgets $WXVERSION is unusable for this application. Please select a different port, or a different wxWidgets version."
 		exit
	fi
	PORTFLAGS="--with-cocoa"
	;;
CARBON|carbon)
	PORTFLAGS="--with-carbon"
	;;
*)
	pc $RED "Valid WXPORT values are cocoa and carbon."
	exit
	;;
esac	

pc $GREEN "\tUsing wxWidgets $WXPORT port."


if [ "$SLIMWX" == "" ]; then
	pc $BLUE  "\tAutomatically selecting full wx compilation. Set SLIMWX to YES to compile only parts used by the application, which can result in a smaller binary (but can fail if the script hasn't been updated after adding new class usage)"
	SLIMWX=NO
fi

case "$SLIMWX" in
YES|yes)
	pc $GREEN "\tUsing minimal wxWidgets compilation."
	SLIM_WX_FLAGS="--without-odbc --without-expat --without-libtiff --without-libjpg \
	--without-libmspack --without-sdl --without-gnomeprint --without-gnomevfs --without-opengl \
	--without-dmalloc --without-themes --disable-sdltest --disable-gtktest \
	--disable-pnm --disable-iff --disable-tga --disable-pcx \
	--disable-dragimage --disable-metafiles --disable-joystick \
	--disable-splines --disable-miniframe --disable-wizarddlg \
	--disable-tipdlg --disable-numberdlg --disable-finddlg --disable-choicedlg \
	--disable-popupwin --disable-tipwindow --disable-toolbook \
	--disable-treebook --disable-tabdialog--disable-searchctrl \
	--disable-odcombobox --disable-listbook --disable-hyperlink \
	--disable-dataviewctrl --disable-grid  --enable-fontpicker \
	--disable-display --disable-datepick --disable-collpane --disable-choicebook \
	--disable-caret --disable-calendar --disable-bmpcombobox --disable-animatectrl \
	--disable-metafile --disable-postscript --disable-graphics_ctx --disable-richtext \
	--disable-webkit --disable-mdidoc --disable-mdi --disable-aui \
	--disable-htmlhelp --disable-html --disable-mshtmlhelp --disable-help --disable-variant \
	--disable-printfposparam --disable-gstreamer8 --disable-mediactrl \
	--disable-tarstream --disable-mslu  --disable-ole \
	--disable-dynamicloader --disable-dynlib --disable-dialupman --disable-apple_ieee \
	--disable-palette --disable-compat26 --disable-docview --disable-aboutdlg"
	;;
NO|no)
	pc $GREEN "\tUsing full wxWidgets compilation."
	SLIM_WX_FLAGS=""
	;;
*)
	pc $RED "Valid SLIMWX values are YES and NO."
	exit
	;;
esac

EXTRA_WXFLAGS="$PORTFLAGS $SLIMWXFLAGS --with-libpng --with-regex --enable-gif --enable-accesibility --enable-progressdlg \
	--enable-textdlg --enable-busyinfo --enable-fontdlg --enable-filedlg --enable-coldlg \
	--enable-treectrl  --enable-slider --enable-listbox --enable-gauge --enable-filepicker \
	--enable-dirpicker --enable-colourpicker --enable-checklst --enable-printarch \
	--enable-protocol --enable-protocol-http --enable-protocol-ftp --enable-protocol-file \
	--enable-sound --enable-unicode --enable-stopwatch --enable-snglinst --enable-ipc \
	--enable-sockets --enable-protocols --enable-ftp --enable-config --with-zlib --enable-textfile \
	--enable-textbuf --enable-url --enable-datetime --enable-http --enable-fileproto"


pn "-- End wxWidgets def section --"
