This solution for Microsoft's Visual Studio 2005 and higher has been configured to be as easy as possible to set up. 

However, given the size of the aMule project and the libraries it uses, along with limitations in the way Visual Studio works,
a couple of items must be setup separately. Additionally, some source code files might need patching due to incompatibilities 
with some generated files. 

The solution expects to find the following items:

* wxWidgets ( http://www.wxwidgets.org ) at a specific location: ..\..\wxWidgets\ from the solution directory.
* zlib ( http://www.zlib.net ) at a specific location: ..\..\zlib\ from the solution directory.
* crypto++ ( http://www.cryptopp.com/ ) at a specific location: ..\..\cryptopp\ from the solution directory.


This means you must create a folder where you will compile aMule, and it must have this structure:

- <Root folder> - Your main folder. You can name it whatever you want.
 | 
 | - wxWidgets ( wxWidgets sources, no intermediate subfolder )
 |
 | - zlib ( zlib sources, no intermediate subfolder )
 |
 | - cryptopp ( Crypto++ sources, no intermediate subfolder )
 |
 | - <amule-sources> ( aMule sources, no intermediate subfolder, can have any name )


I hope this is simple enough to get you started. 

If wxWidgets fails to compile, please read http://wiki.wxwidgets.org/Microsoft_Visual_CPP_Guide

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
