AC_DEFUN([QT_CONFIG_OPTIONS],
[
	AC_ARG_WITH(
		[moc],
		[AS_HELP_STRING([--with-moc=PATH],
			[Search in PATH for Qt's meta object compiler])],
		[QT_MOC=$withval], [QT_MOC=""])
])

AC_DEFUN([CHECK_MOC_VERSION],
[
	AS_IF([test -z ${QT_MOC}],
	[
		AC_PATH_PROGS(QT_MOC, moc-qt4 moc)
		AS_IF([test -z ${QT_MOC}],
		[
			echo moc not found
			exit 1
		])
	],
	[
		AC_MSG_CHECKING(for moc)
		AS_IF([test -x ${QT_MOC}],
		[
			AC_MSG_RESULT(${QT_MOC})
			AC_SUBST(QT_MOC)
		],
		[
			AC_MSG_RESULT("not found")
			QT_MOC="not found"
			exit 1
		])
	])


	AS_IF([test "${QT_MOC}" != "not found"],
	[
		AC_MSG_CHECKING(for moc version >= 4)
		QT_MOC_VERSION=`${QT_MOC} -v 2>&1 | sed -e 's/^.* (/(/'`
		AS_IF([test `echo ${QT_MOC_VERSION} | sed -e 's/^.* //' -e 's/\..*$//'` = 4],
		[
			AC_MSG_RESULT(found ${QT_MOC_VERSION})
		],
		[
			AC_MSG_RESULT(not found ${QT_MOC_VERSION} is too old)
			QT_MOC="not found"
			exit 1
		])
	])
])

AC_DEFUN([CHECK_QT_HEADERS],
[
	AC_REQUIRE([PKG_PROG_PKG_CONFIG])
	AC_MSG_CHECKING(for qt)
	AS_IF([test `${PKG_CONFIG} QtCore` --exists && `${PKG_CONFIG} QtGui --exists`],
	[
		AC_MSG_RESULT(found)
		AC_MSG_CHECKING(for qt core cflags)
		QT_CORE_CXXFLAGS=`${PKG_CONFIG} --cflags QtCore`
		AC_MSG_RESULT($QT_CORE_CFLAGS)
		AC_MSG_CHECKING(for qt core libs)
		QT_CORE_LIBS=`${PKG_CONFIG} --libs QtCore`
		AC_MSG_RESULT($QT_CORE_LIBS)
		AC_MSG_CHECKING(for qt gui cflags)
		QT_GUI_CXXFLAGS=`${PKG_CONFIG} --cflags QtGui`
		AC_MSG_RESULT($QT_GUI_CFLAGS)
		AC_MSG_CHECKING(for qt gui libs)
		QT_GUI_LIBS=`${PKG_CONFIG} --libs QtGui`
		AC_MSG_RESULT($QT_GUI_LIBS)
	],
		[AC_MSG_RESULT(not found)]
		exit 1
	)
	AC_SUBST(QT_CORE_CXXFLAGS)
	AC_SUBST(QT_CORE_LIBS)
	AC_SUBST(QT_GUI_CXXFLAGS)
	AC_SUBST(QT_GUI_LIBS)
])
