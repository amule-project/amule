#!/usr/bin/perl
#

## This file is part of the aMule Project
##
## Copyright (c) 2006 Angel Vidal (Kry) ( kry@amule.org )
## Copyright (c) 2006 aMule Project     ( http://www.amule-project.net )
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

use File::Copy;
use warnings; 
use strict;

my $exit_with_help;

if (!($ARGV[0])) {
	print "You must specify at least one ip.\n";
	$exit_with_help = "true";
}

if ($exit_with_help) {
	die "Usage: kadnodescreate.pl [hash:ip:TCPport:UDPport:type]+\n";
}


print "Creating nodes.dat...\n";

#Open the new file
open(MET," > nodes.dat");
binmode MET;

my $contactcount = $#ARGV + 1;

print "\tContacts: " . $contactcount . "\n";

print MET &int32_string($contactcount);

my $contact;
my $hash;
my $ip;
my $tcpport;
my $udpport;
my $type;

my $contactnumber = 0;
foreach $contact (@ARGV) {
	$contactnumber++;
	if ($contact =~ /^(.*):(.*):(.*):(.*):(.*)$/) {

		$hash = &check_hash($1);
		if ($hash == 0) {
			die "Malformed hash, can't continue: " . $1 . "\n";
		}

		$ip = &check_ip($2);
		if ($ip == 0) {
			die "Malformed ip, can't continue: " . $2 . "\n";
		}

		my $tcpport = &check_port($3);
		if ($tcpport == 0) {
			die "Malformed tcp port, can't continue: " . $3 . "\n";
		}

		$udpport = &check_port($4);
		if ($udpport == 0) {
			die "Malformed udp port, can't continue: " . $4 . "\n";
		}

		$type = &check_type($5);
		if ($type == 9) {
			die "Malformed contact type, can't continue: " . $5 . "\n";
		}


		print "\t\tAdding Contact " . $contactnumber . ":\n";
		print "\t\t\tHash    : " . $1 . "\n";
		print "\t\t\tIP      : " . $ip . "\n";
		print "\t\t\tTCPPort : " . $tcpport . "\n";
		print "\t\t\tUDPPort : " . $udpport . "\n";
		print "\t\t\tType    : " . $type . "\n";

		print MET	&hash_string($1) . 
				&int32_string($ip) .	
				&int16_string($tcpport) . 
				&int16_string($udpport) . 
				&byte_string($type);
	} else {
		die "Malformed contact line, can't continue: " . $contact . "\n";
	} 
}	

print "Closing nodes.dat\n\n";
close(MET);


# Functions

sub check_ip {
	my $ipresult = 0;
        if ($_[0] =~ /^([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})$/) {
		$ipresult = ($1*16777216) + ($2*65536) + ($3*256) + $4;
	}
	$ipresult;
}

sub check_port {
	my $portresult = 0;
	if ($_[0] =~ /^([0-9]{1,5})$/) {
		if ($1 < 65535) {
			$portresult = $1;
		}
	}
	$portresult;
}

sub check_type {
	my $typeresult = -1;
	if ($_[0] =~ /^([0-9])$/) {
                $typeresult = $1;
        }
	$typeresult;
}

sub check_hash {
	my $hashresult = 0;
	if ($_[0] =~ /^([A-Z]|[0-9]|[a-z]){32}$/) {
		$hashresult = 1;
	}
	$hashresult;
}

#Hex write functions

sub byte_string {
	sprintf("%c",$_[0]);
}

sub int16_string {
	&byte_string($_[0] % 256) . &byte_string($_[0] / 256);
}

sub int32_string {
	&int16_string($_[0] % 65536) . &int16_string($_[0] / 65536);
}

sub int64_string {
	&int32_string($_[0] % 4294967296) . &int32_string($_[0] / 4294967296);
}

sub hash_string {
	my $i = 0;
	my $final_string = "";
	while ($i < 32) {
		$final_string = $final_string . &byte_string(hex(substr($_[0],$i,2)));
		$i += 2;
	}
	$final_string;
}

