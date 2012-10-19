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
if %svnv%==Unversioned goto nosvnversion

:foundsvnversion
echo SVNDATE is %svnv%
echo #define SVNDATE "rev. %svnv%">>config.h
echo #define VERSION "SVN">>config.h
echo #define __PRERELEASE__>>config.h
goto boost1

:nosvnversion
echo release build, version from ^<common/ClientVersion.h^>
echo #include ^<src/include/common/ClientVersion.h^> >>config.h

:boost1
if not exist ..\boost\boost\asio.hpp goto boost2
if not exist ..\boost\libs\system\src\error_code.cpp goto boost2
echo Boost detected, using Asio sockets
echo #define ASIO_SOCKETS>>config.h
echo #define HAVE_BOOST_SOURCES>>config.h
goto finish
:boost2
echo no Boost found, using wx sockets

:finish
echo #define CRYPTOPP_INCLUDE_PREFIX	../cryptopp>>config.h
echo #define PACKAGE "amule">>config.h
echo #define HAVE_ERRNO_H>>config.h
echo #define HAVE_STRERROR>>config.h
echo #endif>>config.h
