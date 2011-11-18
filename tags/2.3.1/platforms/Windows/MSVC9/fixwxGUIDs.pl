#!perl
#
# This script is only useful if your source is in a SVN working copy!
#
# Problem: whenever you add a new wxWidgets version and convert the VC6 projects,
# they are converted to VC8 XML .vcproj . In the process, a new project GUID is created.
# When you then open your solution, all GUIDs are updated.
#
# To avoid this:
# 1) Open and close wx.dsw, creating the .vcproj
# 2) Run this script. It will replace the new GUIDs in the wx vcproj by the old ones from the aMule-MSVCE-ExtLibs.sln .
# 3) Now load aMule-MSVCE-ExtLibs.sln. It won't be changed anymore.
#

use strict;

open(sln, 'aMule-MSVCE-ExtLibs.sln');
#Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "wxregex", "..\..\wxWidgets\build\msw\wx_wxregex.vcproj", "{B4ACF599-824F-4806-8390-414B2DF64922}"
while (<sln>) {
	if (/^Project.*, \"(.+)\", (\".+\")/) {
		my ($path, $guid) = ($1, $2);
		next unless $path =~ /\\wxWidgets/;
		$path =~ s-\\-/-g;
		print "fix $path\n";
		open(prj, $path) or die "$path $!";
		my @content = <prj>;
		close prj;
		foreach (@content) {
			if (/ProjectGUID=/) {
				$_ = "\tProjectGUID=$guid\n";
				print $_;
				last;
			}
		}
		open(prj, ">$path") or die;
		print prj @content;
		close prj;
	}
}
