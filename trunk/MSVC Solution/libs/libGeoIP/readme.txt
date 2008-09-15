How to build aMule with GeoIP support

The MaxMind GeoIP library can detect the country an IP adress comes from. aMule uses this information
to display a little country flag for each source or ed2k server. 
Since this is an external library it is disabled by default. This is how to enable it:

Download the free GeoIP C library from http://www.maxmind.com/download/geoip/api/c/GeoIP-1.4.4.tar.gz
From the folder libGeoIP take GeoIP.h and GeoIP.c and put them into "MSVC Solution\libs\libGeoIP".
Apply GeoIP.c.patch to GeoIP.c . If you don't have a patch tool, you can easily apply it by hand. 
Just add the #defines to the beginning of the file and remove the 5 lines starting with #ifdef WIN32
at line 524.

Copy CountryFlags.h from "MSVC Solution\libs\libGeoIP" to "src/pixmaps/flags_xpm".

In aMule's project settings C/C++/Preprocessor/Preprocessor Definitions add ;ENABLE_IP2COUNTRY

In the solution explorer right-click on aMule, select Project dependencies and check libGeoIP.

Rebuild aMule. That's it. :-)
