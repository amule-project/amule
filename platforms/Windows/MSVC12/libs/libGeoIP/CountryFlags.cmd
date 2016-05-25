@echo off

setlocal enableDelayedExpansion

set FLAGSDIR=..\..\..\..\..\src\pixmaps\flags_xpm
set COUNTRYFLAGSH=%FLAGSDIR%\CountryFlags.h

echo #ifndef COUNTRY_FLAGS_H>%COUNTRYFLAGSH%
echo #define COUNTRY_FLAGS_H>>%COUNTRYFLAGSH%
echo.>>%COUNTRYFLAGSH%
echo namespace flags {>>%COUNTRYFLAGSH%
echo.>>%COUNTRYFLAGSH%

rem Create #include directives
for /R %FLAGSDIR% %%G in (*.xpm) do (
	echo #include "%%~nxG">>%COUNTRYFLAGSH%
)
echo.>>%COUNTRYFLAGSH%

rem Define the struct
echo struct FlagXPMCode {>>%COUNTRYFLAGSH%
echo const char **xpm;>>%COUNTRYFLAGSH%
echo const char *code;>>%COUNTRYFLAGSH%
echo };>>%COUNTRYFLAGSH%
echo.>>%COUNTRYFLAGSH%

rem Create the flag/code vector
echo static struct FlagXPMCode flagXPMCodeVector[] = {>>%COUNTRYFLAGSH%
for /R %FLAGSDIR% %%G in (*.xpm) do (
	set ctry=%%~nG
	if "!ctry!"=="do" set ctry=do_
	echo { !ctry!, "%%~nG" },>>%COUNTRYFLAGSH%
)
echo };>>%COUNTRYFLAGSH%
echo.>>%COUNTRYFLAGSH%

rem Calculate the vector size
echo static const int FLAGS_XPM_SIZE = (sizeof flagXPMCodeVector) / (sizeof flagXPMCodeVector[0]);>>%COUNTRYFLAGSH%

rem Finish
echo.>>%COUNTRYFLAGSH%
echo } // namespace flags>>%COUNTRYFLAGSH%
echo.>>%COUNTRYFLAGSH%
echo #endif // COUNTRY_FLAGS_H>>%COUNTRYFLAGSH%
