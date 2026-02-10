How to build aMule with Visual Studio 2010 Express Edition

See also http://www.amule.org/wiki/index.php/HowTo_compile_with_Visual_Studio

This solution for Microsoft's Visual Studio 2010 has been configured to be as easy as possible to set up. 

However, given the size of the aMule project and the libraries it uses, along with limitations in the way Visual Studio works,
a couple of items must be setup separately. Additionally, some source code files might need patching due to incompatibilities 
with some generated files. 

The solution expects to find the following items:

* wxWidgets ( http://www.wxwidgets.org ) at a specific location: ..\..\wxWidgets\ from the solution directory.
* crypto++ ( http://www.cryptopp.com/ ) at a specific location: ..\..\cryptopp\ from the solution directory.


This means you must create a folder where you will compile aMule, and it must have this structure:

- <Root folder> - Your main folder. You can name it whatever you want.
 | 
 | - wxWidgets ( wxWidgets sources, no intermediate subfolder )
 |
 | - wxWidgets29 ( wxWidgets 2.9 sources, only required if you want to build against unstable wx 2.9 )
 |
 | - cryptopp ( Crypto++ sources, no intermediate subfolder )
 |
 | - <amule-sources> ( aMule sources, no intermediate subfolder, can have any name )


I hope this is simple enough to get you started. 

If wxWidgets fails to compile, please read http://wiki.wxwidgets.org/Microsoft_Visual_CPP_Guide

There is sometimes a problem with the cryptopp project in the release and debug build, if some projects fail to link
and give you a warning about redefined symbols, go to the properties on the cryptopp project, configuration "release" and change
the "Configuration Properties"->"C/C++"->"Code generation"->"Runtime library" to "Multithreaded DLL (/MD)"
 in the release build or "Multithreaded Debug DLL (/MDd)" for the debug build. 

There are 3 solutions:
1) aMule-MSVC10E.sln
   which includes aMule, aMule tools and aMule's internal libs
2) aMule-MSVC10E-ExtLibs.sln
   includes only wxWidgets and Crypto++ and builds a single library libext.lib from them. You must first build this solution before building aMule-MSVCE.sln
3) aMule-MSVC10E-ExtLibs29.sln
   same but using wxWidgets 2.9


The Debug/Release configs build against wxWidgets 2.8 (stable), Debug29/Release29 configs build against wxWidgets 2.9 (unstable)

To build aMule with GeoIP see libs\libGeoIP\readme.txt . 
Without it you get an error building libGeoIP when you build the full solution.
Just ignore it or unload the libGeoIP project.


If aMule fails to compile, here are some guidelines/patches to the most common problems:

[*] If Scanner.cpp fails to compile, apply this change around line 1545:

-        b->yy_is_interactive = file ? (isatty( fileno(file) ) > 0) : 0;
+        b->yy_is_interactive = 0;

if necessary.

[*] The file muuli_wdr.cpp can also fail if it hasn't been patched for MSVC in the current code. The most common problem comes 
from this kind of lines:

        _("For a film you can say its length, its story, language ...\n"
          "and if it's a fake, you can tell that to other users of aMule."),

Notice the opening and closing parentheses are on a different line, and the quotes opened again. While this is ok for gcc, Visual
Studio won't allow it. Change this lines to be a single-line quoted text. In the above example, the result will be:

	_("For a film you can say its length, its story, language ...\nand if it's a fake, you can tell that to other users of aMule."),

And that should be it. Feel free to ask in the aMule forum at forum.amule.org, "Compilation problems" section if you have further
doubts.

If you want to run aMule from inside Visual Studio, always set
_NO_DEBUG_HEAP=1
in Properties / Debugging / Environment!
