m4_define([DISABLE_PLASMAMULE], [MULE_ENABLEVAR([plasmamule])=disabled])

AC_DEFUN([PLASMAMULE_CHECKS],
[
	CHECK_MOC_VERSION
	AS_IF([test "${QT_MOC}" = "not found"], [DISABLE_PLASMAMULE])

	MULE_IF_ENABLED([plasmamule],
	[
		CHECK_QT_HEADERS
		AS_IF([test -z "${QT_CORE_CXXFLAGS}"], [DISABLE_PLASMAMULE])
	])

	MULE_IF_ENABLED([plasmamule],
	[
		KDE_CONFIG_CHECK
		AS_IF([test "${KDE4_CONFIG}" = "not found"], [DISABLE_PLASMAMULE])
	])

	MULE_IF_ENABLED([plasmamule],
	[
		KDE_HEADER_CHECK
		AS_IF([test -z ${KDE_HEADER_DIR}], [DISABLE_PLASMAMULE])
	])

	MULE_IF_ENABLED([plasmamule],
	[
		AS_IF([test -e `$BUILD_CC -print-file-name=libplasma.so` &&
			test -e `$BUILD_CC -print-file-name=libkdecore.so`],
			[
				KDE_APPLNK_PATH_CHECK
				KDE_SERVICE_PATH_CHECK
				KDE_MODULE_PATH_CHECK
				KDE_ICON_PATH_CHECK
				KDE_MIME_PATH_CHECK
			],
			[DISABLE_PLASMAMULE])
	])
	
	MULE_IF_ENABLED([plasmamule],
	[
		AS_IF([test -z ${DEB_HOST_ARCH}],
		[
			CHECK_HELPER_APPS
		])
	])

	MULE_IF_ENABLED([debug],
        [
		DEBUGFLAG="-D__DEBUG__"
		AC_SUBST(DEBUGFLAG)
	])
])
