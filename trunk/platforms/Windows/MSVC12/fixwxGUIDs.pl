#!perl
#
# This script is only useful if your source is in a SVN working copy!
#
# Problem: whenever you add a new wxWidgets version and convert the VC6 projects,
# they are converted to VC8 XML .vcproj . In the process, a new project GUID is created.
# When you then open your solution, all GUIDs are updated.
#
# To avoid this:
# 1) Open and close wx.dsw with VS9 to convert to wx.sln (VS10 can't open them)
# 2) Open and close wx.sln, creating the .vcxproj
# 3) Run this script. It will replace the new GUIDs in the wx vcxproj by the old ones from the aMule-MSVC10E-ExtLibs.sln .
# 4) Now load aMule-MSVCE-ExtLibs.sln. It won't be changed anymore.
#

use strict;

open(sln, 'aMule-MSVC10E-ExtLibs.sln') or die $!;
#Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "wxregex", "..\..\..\..\wxWidgets\build\msw\wx_wxregex.vcxproj", "{A960D0AD-C20E-4FD6-B477-6509232C7D94}"
#    <ProjectGuid>{B499DD81-BCA2-42D8-986C-E0FBAAC25B13}</ProjectGuid>
while (<sln>) {
	if (/^Project.*, \"(.+)\", \"(.+)\"/) {
		my ($path, $guid) = ($1, $2);
		next unless $path =~ /\\wxWidgets/;
		$path =~ s-\\-/-g;
		print "fix $path\n";
		open(prj, $path) or die "$path $!";
		my @content = <prj>;
		close prj;
		foreach (@content) {
			if (/<ProjectGuid>/) {
				$_ = "    <ProjectGuid>$guid</ProjectGuid>\n";
				print $_;
				last;
			}
		}
		open(prj, ">$path") or die;
		print prj @content;
		close prj;
	}
}
