How to build aMule with GeoIP support

The MaxMind GeoIP library can detect the country an IP adress comes from. aMule uses this information
to display a little country flag for each source or ed2k server. 
Since this is an external library it is disabled by default. This is how to enable it:

Download the free GeoIP C library from http://www.maxmind.com/download/geoip/api/c/GeoIP-1.4.6.tar.gz
From the folder libGeoIP take GeoIP.h and GeoIP.c and put them into "MSVC Solution\libs\libGeoIP"
(removing the dummy files there).

Rebuild aMule. That's it. :-)
