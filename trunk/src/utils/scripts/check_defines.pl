#!/usr/bin/perl
#

## This file is part of the aMule Project
##
## Copyright (c) 2004-2010 Stu Redman ( sturedman@amule.org )
## Copyright (c) 2003-2010 aMule Team ( admin@amule.org / http://www.amule.org )
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either
## version 2 of the License, or (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#
# To speed up compilation we compile some files used in amule, amuled, amulegui
# into libs which we link to the apps.
# This concept can only work if the files (or their includes) don't use 
# preprocessor macros used to tell these projects apart.
#
# This script checks for usage of these macros.
# It has to be run from the src dir after running configure to build the deps.
#

use warnings; 
use strict;
use Cwd;

die 'run from src dir after making dependencies' unless -f '.deps/amule-amule.Po';

my %checkedFiles;	# key: path  value: 0: ok  1: has AMULE_DAEMON  2: has CLIENT_GUI

sub checkDeps();

# libmuleappcommon, libmuleappcore, libmuleappgui
my @deps = glob('.deps/lib*.Po');
checkDeps();
# libcommon
chdir 'libs/common';
@deps = glob('.deps/*.Po');
checkDeps();
# libec
chdir '../ec/cpp';
@deps = glob('.deps/*.Po');
checkDeps();

sub checkDeps()
{
	printf("check %d dependencies in %s\n", scalar @deps, getcwd());
	foreach (@deps) {
		open(DEP, $_) or die "can't open $_ : $!";
		my @lines = <DEP>;
		close DEP;
		my $line = join(' ', @lines);
		$line =~ tr/:\\/  /;
		my @files = split(/\s+/, $line);
		my $obj = shift @files;
		foreach my $file (@files) {
			next if $file =~ m-^/usr/-;		# skip system and wx includes
			next if $file =~ m-/wx/-;
			my $status = $checkedFiles{$file};
			unless (defined $status) {
				$status = 0;
				open(SRC, $file) or die "can't open $file : $!";
				while (<SRC>) {
					$status |= 1 if /^\s*\#if.+AMULE_DAEMON/;
					$status |= 2 if /^\s*\#if.+CLIENT_GUI/;
				}
				close SRC;
				$checkedFiles{$file} = $status;
			}
			print "$obj: AMULE_DAEMON used in $file\n" if ($status & 1);
			print "$obj: CLIENT_GUI used in $file\n" if ($status & 2);
		}
	}
}

