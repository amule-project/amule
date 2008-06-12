README for aMule on the Mac
===========================

Installation:
-------------
Just drag the aMule application to a folder of your choice (e.g., /Applications). To start aMule, double-click the application icon.


Documentation and help resources:
---------------------------------
The heart of aMule's documentation is the aMule Wiki, which you can find at http://www.amule.org/wiki/index.php/Main_Page. We suggest that you start with the Getting Started Guide (http://www.amule.org/wiki/index.php/Getting_Started) and the aMule FAQ (http://www.amule.org/wiki/index.php/FAQ_aMule). These pages should give you a good idea of how to setup and use aMule.

If you run into any problems you can drop by at the aMule forum (http://forum.amule.org). A lot of questions have already been answered there, so it is always a good idea to start by searching the forum for your particular problem. If you don't find an answer to your problem you can post your problem on the forum and we will do our best to help you out.

The forum and particularly the Mac subsection of the forum (http://forum.amule.org/board.php?boardid=49) are also good places to look for usage tips and to learn about recent aMule developments (especially look at the sticky threads).


Right-clicking & pop-up menus on the Mac:
-----------------------------------------
aMule contains a lot of features that are only accessible through pop-up menus, which you invoke by right-clicking a certain item. For instance, if you want to pause or cancel a download, change the upload or download priority of a file, see file details, etc. you need to do this via a right-click.

If you haven't bought an additional multi-button mouse with your Mac, you only have one mouse button (the one that corresponds to a left-click or "normal" click on a multi-button mouse). Fortunately, you can emulate the second button by HOLDING DOWN THE CONTROL-KEY ON THE KEYBOARD WHILE YOU CLICK. For example, do a "control-click" now on a file that you are downloading and see how a pop-up menu shows up with further option.

If you want to discover all of aMule's hidden functionalities just try to control-click everything that moves.  
This includes all the lists of files, clients & servers, all the labels on the top of these lists (e.g. "File Name"), the bar "all" at the top of the download window, ...

By the way, these emulated right-clicks work system-wide. Just try control-clicking a file in the Finder, an icon in the Dock, a marked word in TextEdit or MS Word.


Handling ed2k-Links on the Mac:
-------------------------------
There are basically four ways to download files with aMule:

(1) Search for files using aMule's search dialog and double click those search results that you want to download.

(2) Copy ed2k-links from a web page into the "ED2K-Link Handler" field at the bottom of the search window of aMule and press the commit button. If the ed2k-link is longer than that text field is wide you need to make the aMule window wider until the link fits completely into the field (you can make the aMule window wider than your screen if necessary). We are working on eliminating this issue.

(3) Import ed2k-links directly from your browser into aMule. See http://forum.amule.org/thread.php?threadid=5679 for more details.

(4) Use a text editor (e.g. TextEdit) to paste ed2k-links into a "ED2Links" file inside "~/Library/Application Support/aMule/" and aMule will automatically import the links.


Setting up aMule's video preview feature:
-----------------------------------------
You can use a video player like VLC or Mplayer to preview incomplete downloads of video files. To set up aMule properly for this, go to Preferences -> Directories. Under "Video Player", you have to enter "/usr/bin/open -a" together with the path of your video player program.

For example:
/usr/bin/open -a "/Applications/VLC.app"
/usr/bin/open -a "/Applications/vlc-0.8.4a/VLC.app"
/usr/bin/open -a "/Applications/MPlayer OS X 2.0b8r5/MPlayer OS X 2.app"


Getting up-to-date snapshot builds of aMule:
--------------------------------------------
If you are interested in running aMule versions with the latest bleeding edge features or if you want to help us test the most recent changes to the aMule code-base you download up-to-date snapshot builds of aMule. More information can be found at: http://forum.amule.org/thread.php?threadid=5051