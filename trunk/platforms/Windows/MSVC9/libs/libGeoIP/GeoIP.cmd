@echo off
if exist geoip.h goto h_exist
echo creating GeoIP.h
echo #undef ENABLE_IP2COUNTRY > GeoIP.h
echo typedef void GeoIP; >> GeoIP.h
:h_exist
if exist geoip.c goto c_exist
echo creating GeoIP.c
echo // Dummy > GeoIP.c
:c_exist
if exist geoip_x.c goto finish
echo creating GeoIP_X.c
echo #pragma warning(disable:4996) > GeoIP_X.c
echo #include "GeoIP.c" >> GeoIP_X.c
:finish

