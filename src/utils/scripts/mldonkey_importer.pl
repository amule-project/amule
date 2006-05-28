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

# Gimme a break, is my first perl app... (Kry)

use File::Copy;
use warnings; 
use strict;

my $exit_with_help;

if (!($ARGV[0])) {
	print "You must specify the mldonkey config folder (usually ~/.mldonkey).\n";
	$exit_with_help = "true";
}

if (!($ARGV[1])) {
	print "You must specify the aMule temp folder for output.\n";
	$exit_with_help = "true";
}

if ($exit_with_help) {
	die "Usage: importer2.pl mldonkey_config_folder amule_temp_folder.\n";
}


my $input_folder = $ARGV[0];

my $output_folder = $ARGV[1];

open(TEST,">" . $output_folder . "/test_file") or die "Unable to write to destination folder! Error: $!\n";
close(TEST);
unlink($output_folder . "/test_file");

open(INFO, $input_folder . "/files.ini") or die "Cannot open input file" . $input_folder . "/files.ini for reading: $!";		# Open the file

my $line="no";
while ($line !~ /^\s*files\s*=\s*\[\s*$/) {
	$line = <INFO>;
	if (!($line)) {
		die $input_folder . "/files.ini seems not to be a mldonkey files.ini\n";
	}
	chop $line;	
}

#We're at the start of the downloading files section.
# Read info for each file.

my $number = 1;

while ($line && ($line !~ /^.*};\].*$/)) {
	print "Reading info for file $number\n";
	&read_file_info;
	print "End reading\n\n";
	$number++;
}

close(INFO);

sub read_file_info {
	$line = <INFO>;

	my @md4_list = ();
	my @gap_list = ();
	my $file_size = 0;
	my $file_name = "";
	my $part_file = "";
	my $md4_hash = "";

	my $done = "false";

	while (($line) && ($line !~ /^\s*}.*/) && ($done ne "true")) {
		chop $line;
		if ($line =~ /.*file_network\s*=\s*(.*)$/) {
			print "Network is $1\n";
			if ($1 ne "Donkey") {
				print "Cannot import non-ed2k part file, skipping\n";
				while (($line) && ($line !~ /^\s*}.*/)) {
					$line = <INFO>;
					$done = "true";
				}
			}
		}
		if ($line =~ /^\s*file_size\s*=\s*(\d+)\s*$/) {
			$file_size = $1;
			print "File size: $file_size\n";
		}
		if ($line =~ /^\s*file_swarmer\s*=\s*\"(.*)\"\s*$/) {
			$part_file = $1;
			print "Part file to import: $part_file\n";
		}
		if ($line =~ /^\s*file_md4\s*=\s*\"(([A-Z]|[0-9])+)\"\s*$/) {
			$md4_hash = $1;
			print "File hash: $md4_hash\n";
		}
		if ($line =~ /^\s*file_filename\s*=\s*\"(.*)\"\s*$/) {
			$file_name = $1;
			print "File name: $file_name\n";
		}
		if ($line =~ /^\s*file_md4s\s*=\s*\[\s*$/) {
			# Read the MD4 list
			my $result = "";
			do {
				my $md4_line = <INFO>;
				if ($md4_line =~ /^\s*\"?(([A-Z]|[0-9])+)\"?;\]?\s*$/) {
					push(@md4_list,$1);
					if ($md4_line =~ /^.*;\].*$/) {
						$result = "done";
					}
				} else {
					print "Malformed md4 hash line $md4_line";
					@md4_list = ();
					$result = "error";
				}
			} while (!($result));
			if ($result eq "done") {
				print "MD4 list: @md4_list\n";
			}
			
			
		}

		if ($line =~ /^\s*file_present_chunks\s*=\s*\[\s*$/) {
			# Read the gaps list
			my $result = "";
			my @ml_gaps = ();
			do {
				my $gaps_line = <INFO>;
				if ($gaps_line =~ /^\s*\((\d+),\s*(\d+)\)(;|])\s*$/) {
					push(@ml_gaps,$1);
					push(@ml_gaps,$2);
					if ($gaps_line =~ /^.*\)\].*$/) {
						$result = "done";
					}
				} else {
					print "Malformed gaps line $gaps_line";
					$result = "error";
				}
			} while (!($result));

			if ($result eq "done") {
				# Process mldonkey gaps to aMule gaps
				print "ML Gaps list: @ml_gaps\n";
				
				@gap_list = &convert_gap_format($file_size,@ml_gaps);
				
				print "aMule Gaps list: @gap_list\n";
			}
			
			
		}
		
		if ($done ne "true") {
			$line = <INFO>;
		}
	}
	if ($done) {
		print "File import result: false\n";
	} else {
		if ($file_name && $file_size && $md4_hash && $part_file) {
			if (!(@md4_list)) {
				print "WARNING: File has no md4 hashes list, imported file will have 0 bytes downloaded\n";
			}
			
			my $first_free_number = &get_first_free_number;
			
			my $met_file = $output_folder . sprintf("/%03d.part.met",$first_free_number);

			&create_met_file($met_file,$file_name,$file_size,$md4_hash,@md4_list,"---",@gap_list);

			print "File $met_file imported successfully.\n";
			
			my $from = $input_folder . "/" . $part_file;
			my $destination = $output_folder . sprintf("/%03d.part",$first_free_number);
			copy($from, $destination) or die "CRITICAL: File $from cannot be copied to $destination. Error: $!\n";
			
		} else {
			print "Not enough info to import file, sorry.\n";
		}
	}
	$line;
}

sub create_met_file {

	print "Parameters: @_\n";

	#Open the new file
	open(MET," > $_[0]");
	binmode MET;

	my $large_file = "";

	# Met file version (1 byte)
	if ($_[2] < 4290048000) {
		# Not large file
		$large_file = "no";
		printf MET &byte_string(0xe0);
	} else {
		$large_file = "yes";
		printf MET &byte_string(0xe2);
	}
	# File modification time. 0 to force aMule rehash. (4 bytes)
	print MET &int32_string(0);

	# MD4 hash (16 bytes)
	print MET &hash_string($_[3]);

	#Calculate number of MD4 hashes
	my @md4_hashlist = ();
	
	my $i = 4;
	
	while ($_[$i] ne "---") {
		push (@md4_hashlist,$_[$i]);
		$i++;
	}
	
	$i++;
	
	my @gaps_list = ();
	while ($_[$i]) {
		push(@gaps_list,$_[$i]);
		$i++;
	}
	
	print "Write aMule gap list: @gaps_list\n";
	
	my $md4_hashsize = @md4_hashlist;
	
	print "MD4 hashlist size $md4_hashsize\n";
	
	#Number of MD4 hashes (2 bytes)
	print MET &int16_string($md4_hashsize);

	#Write MD4 hashes (16 bytes * number of hashes)
	my $md4_parthash = "";
	foreach $md4_parthash (@md4_hashlist) {
		print MET &hash_string($md4_parthash);
	}	

	#Number of tags (4 bytes)
	
	my $tags_number = 2; # Fixed tags (Name + Size)

	$tags_number = $tags_number + @gaps_list;
	
	print MET &int32_string($tags_number);
	
	#Name tag (x bytes)
	
	print MET &tag_string(2,0,0x01,$_[1]); # Tagtype string, id FT_FILENAME, value
	
	#Size tag (x bytes)
	
	if ($large_file eq "yes") {
		print MET &tag_string(0x0b,0,0x02,$_[2]); # Tagtype UINT64, id FT_FILESIZE, value	
	} else {
		print MET &tag_string(3,0,0x02,$_[2]); # Tagtype UINT32, id FT_FILESIZE, value
	}
	
	my $t = 0;

	my $tag_type;
	if ($large_file eq "yes") {
		$tag_type = 0x0b;
	} else {
		$tag_type = 0x03;		
	}

	while (@gaps_list[$t*2]) {
		my $gap_start = @gaps_list[$t*2];
		my $gap_end = @gaps_list[$t*2+1];
		
		print "Gap $t start $gap_start end $gap_end\n";
		
		print MET &tag_string($tag_type,1,sprintf("%c%d",0x09,$t),$gap_start);
		print MET &tag_string($tag_type,1,sprintf("%c%d",0x0a,$t),$gap_end);
		
		$t++;
	}

	close(MET);
}

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

sub tag_string {
	# ONLY STRINGS AND UINT32/64 SUPPORTED

	my $final_string = "";
	
	# Tag type
	$final_string = $final_string . &byte_string($_[0]);
	
	if ($_[1] == 0) {
		# Byte ID tag
		$final_string = $final_string . &int16_string(1);
		$final_string = $final_string . &byte_string($_[2]);
	} else {
		# String ID tag
		$final_string = $final_string . &int16_string(length $_[2]) . $_[2];
	}

	if ($_[0] == 2) {
		$final_string = $final_string . &int16_string(length $_[3]) . $_[3];
	} else {
		if ($_[0] == 3) {
			# UINT32
			$final_string = $final_string . &int32_string($_[3]);
		} else {
			if ($_[0] == 0x0b) {
				# UINT64
				$final_string = $final_string . &int64_string($_[3]);
			}
		}
	}
	$final_string;
}

sub convert_gap_format {
	my $total_size = $_[0];

	my @converted_gaps = ();

	my $n = 1;

	if ($_[1] != 0) {
		push(@converted_gaps,0);
		push(@converted_gaps,$_[1]);
	} 
	
	$n++;

	while ($_[$n+1]) {
		push(@converted_gaps,$_[$n]);
		push(@converted_gaps,$_[$n+1]);
		$n += 2;
	}
	
	if ($_[$n] != $total_size) {
		push(@converted_gaps,$_[$n]);
		push(@converted_gaps,$total_size);
	}
	
	@converted_gaps;
}

sub get_first_free_number {
	my $n = 1;
	my $result = 0;
	
	while (!$result && !($n>999)) {
		open(TEST, " <" . $output_folder . sprintf("/%03d.part.met",$n)) or $result=$n;
		close(TEST);
		$n++;
	}
	
	$result;
}
