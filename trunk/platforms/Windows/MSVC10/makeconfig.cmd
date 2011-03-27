@echo off
echo #ifndef CONFIG_H>config.h
echo #define CONFIG_H>>config.h

rem Tarballs have a .svn-revision for identification
if not exist .svn-revision goto svn1
FOR /F %%i IN (.svn-revision) DO set svnv=%%i
goto :foundsvnversion

:svn1
rem Try to get version from a SVN working copy, svnversion must be in path
set svnv=exported
call svnversion >nul 2>&1
if errorlevel 1 goto nosvnversion
FOR /F %%i IN ('svnversion src') DO set svnv=%%i
if %svnv%==exported goto nosvnversion

:foundsvnversion
echo SVNDATE is %svnv%
echo #define SVNDATE "rev. %svnv%">>config.h
echo #define VERSION "SVN">>config.h
echo #define __PRERELEASE__>>config.h
goto finish

:nosvnversion
echo release build, version from ^<common/ClientVersion.h^>
echo #include ^<common/ClientVersion.h^> >>config.h

:finish
echo #define CRYPTOPP_INCLUDE_PREFIX	../cryptopp>>config.h
echo #define PACKAGE "amule">>config.h
echo #define HAVE_ERRNO_H>>config.h
echo #define HAVE_STRERROR>>config.h
echo #endif>>config.h
