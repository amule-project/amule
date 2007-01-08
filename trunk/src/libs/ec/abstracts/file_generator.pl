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

# Gimme a break, it's my second perl app... (Kry)

use File::Copy;
use Switch;
use warnings; 
use strict;

my $exit_with_help;

if ($#ARGV < 1) {
	print "You must specify at least the folder and one abstract file.\n";
	$exit_with_help = "true";
}

if ($exit_with_help) {
	die "Usage: file_generator file.abstract [file2.abstract ...]\n";
}


my $folder = $ARGV[0] . "/";

my $numArgs = $#ARGV;
print "Parsing $numArgs files\n";

foreach my $argnum (1 .. $#ARGV) {
	generate_files($folder, $ARGV[$argnum]);
}


sub generate_files {

	my $folder = $_[0];
	my $input_file = $_[1];

	open(INFO, $folder . $input_file) or die "Cannot open input file " . $input_file . " for reading: $!";		# Open the file

	my $line="no";
	while ($line !~ /^\[Section Definition\]$/) {
		$line = <INFO>;
		if (!($line)) {
			die $input_file . " seems not to have a Section Definition\n";
		}
		chop $line;
	}

	#We're at the start of the Section Definition.
	# Read the Definition Section.

	my $filename = "";

	my $nameline = <INFO>;
	if ($nameline =~ /^FileName\s+(.+)$/) {
		$filename = $1;
	} else {
		die "Section Definition must start with FileName item.";
	}

	my $filecontent = "";

	my $contentline = <INFO>;
	if ($contentline =~ /^FileContent\s+(.+)$/) {
		$filecontent = $1;
	} else {
		die "Section Definition must have a FileContent after the FileName item.";
	}

	#Skip the rest of the section, allow for extensions later.

	$line = "";
	while ($line && ($line !~ /^\[\/Section\]$/)) {
		$line = <INFO>;
	}


	print "FileName: " . $filename . "\n";
	print "FileContent: " . $filecontent . "\n";

	#Open language output files
	open(CPPFILE," > " . $folder . "../cpp/$filename" . ".h");
	#Open language output files
	open(JAVAFILE," > " . $folder . "../java/$filename" . ".java");


	# Print license on top.
	write_license_header($folder, *CPPFILE, "// ", "", $filecontent);
	write_license_header($folder, *JAVAFILE, "// ", "", $filecontent);
	#Example for a language that needs start/end:
	#write_license_header($folder, *CFILE, "/* ", " */", $filecontent);

	#Add top guards for each language
	write_cpp_top_guard(*CPPFILE, $filename);
	# JAVA doesn't need guards, but needs file type declaration
	print JAVAFILE "public interface " . $filename . " {\n\n";
	##Add other language guards


	read_content(*INFO, *CPPFILE, *JAVAFILE);


	#Add bottom guards for each language
	write_cpp_bottom_guard(*CPPFILE, $filename);
	# JAVA doesn't need guards, but we have to close the interface
	print JAVAFILE "}\n";
	##Add other language guards


	# Close language files
	close(CPPFILE);
	close(JAVAFILE);

	print "All info parsed\n";

	close(INFO);

}

################ Generic Subroutines #####################

sub read_content {

	local (*INFO) = $_[0];
	local (*CPPFILE) = $_[1];
	local (*JAVAFILE) = $_[2];

	my $stop = "";

	while (!($stop)) {
		my $line="no";

		#Skip till Content
		while (!(eof(INFO)) && $line !~ /^\[Section Content\]$/) {
			$line = <INFO>;
			chop $line;
		}

		if ($line =~ /^\[Section Content\]$/) {
			print "Reading content section...\n";
			read_content_section(*INFO, *CPPFILE, *JAVAFILE);
			print CPPFILE "\n";
			print JAVAFILE "\n";
		} else {
			print "No more content sections\n";
			$stop = "yes";
		}
	}
}

sub read_content_section {

	local (*INFO) = $_[0];
	local (*CPPOUTPUT) = $_[1];
	local (*JAVAOUTPUT) = $_[2];
	
	my $line = <INFO>;
	my $datatype = "";
	if ($line =~ /^Type\s+(.+)$/) {
		$datatype = $1;
		print "\tDatatype: " . $datatype . "\n";
	} else {
		die "Content section has a non-typed data stream\n";
	}

	switch ($datatype) {
		case "Define" {
			read_define_content(*INFO, *CPPOUTPUT, *JAVAOUTPUT);
		}
		case "Enum" { 
			read_enum_content(*INFO, *CPPOUTPUT, *JAVAOUTPUT);
		}
		case "TypeDef" {
			read_typedef_content(*INFO, *CPPOUTPUT, *JAVAOUTPUT);
		}
		else { die "Unknown type on content section\n" }
	}

}

sub read_define_content {

	local (*INFO) = $_[0];
	local (*CPPOUTPUT) = $_[1];
	local (*JAVAOUTPUT) = $_[2];

	my $line = <INFO>;
	while (!(eof) && ($line !~ /^\[\/Section\]$/)) {
		if ($line !~ /^(#.*|\s*)$/) {
			if ($line =~ /^(.+)\s+(.+)$/) {
				write_cpp_define_line(*CPPOUTPUT, $1, $2);
				write_java_define_line(*JAVAOUTPUT, $1, $2);
			} else {
				die "Malformed content section define line\n";
			}
		}
		$line = <INFO>;
	}
}

sub read_typedef_content {

	local (*INFO) = $_[0];
	local (*CPPOUTPUT) = $_[1];
	local (*JAVAOUTPUT) = $_[2];

	my $line = <INFO>;
	while (!(eof) && ($line !~ /^\[\/Section\]$/)) {
		if ($line !~ /^(#.*|\s*)$/) {
			if ($line =~ /^(.+)\s+(.+)$/) {
				write_cpp_typedef_line(*CPPOUTPUT, $1, $2);
				# Java doesn't support typedefs, ignore it.
				#write_java_typedef_line(*JAVAOUTPUT, $1, $2);
			} else {
				die "Malformed content section typedef line\n";
			}
		}
		$line = <INFO>;
	}
}

sub read_enum_content {

	local (*INFO) = $_[0];
	local (*CPPOUTPUT) = $_[1];
	local (*JAVAOUTPUT) = $_[2];

	my $line = <INFO>;
	my $dataname = "";
	if ($line =~ /^Name\s+(.+)$/) {
		$dataname = $1;
		print "\tDataname: " . $dataname . "\n";
	} else {
		die "Content section has a non-named data stream\n";
	}

	my $first = "yes";
	$line = <INFO>;
	while (!(eof) && ($line !~ /^\[\/Section\]$/)) {
		if ($line !~ /^(#.*|\s*)$/) {
			if ($line =~ /^(.+)\s+(.+)$/) {
				my $firstoperand = $1;
				my $secondoperand = $2;
	
				if ($first) {
					write_cpp_enum_start(*CPPOUTPUT, $dataname);
				}

				write_cpp_enum_line(*CPPOUTPUT, $firstoperand, $secondoperand, $first);
				write_java_define_line(*JAVAOUTPUT, $firstoperand, $secondoperand);
	
				if ($first) {
					$first = "";
				}
			} else {
				die "Malformed content section enum line\n";
			}
		}
		$line = <INFO>;
	}

	write_cpp_enum_end(*CPPOUTPUT);

}

# Takes a file handle, and the comment start/end character for that language
sub write_license_header {

	my $folder = $_[0];
	local (*OUTPUT) = $_[1];

	open(LICENSE, $folder . "License.abstract") or die "Cannot open license file";

	my $line = <LICENSE>;
	while (!(eof)) {
		printf OUTPUT $_[1] . $line . $_[2];
		$line = <LICENSE>;
	}

	print OUTPUT "\n";

	print OUTPUT $_[1] . "Purpose:" . $_[2] . "\n" . $_[1] . $_[3] . $_[2] . "\n\n";

	close(LICENSE);
}

################ CPP Specific Subroutines #####################

sub write_cpp_top_guard {

	local (*OUTPUT) = $_[0];

	my $guardname = uc($_[1]);

	print OUTPUT "#ifndef __" . $guardname . "_H__\n";
	print OUTPUT "#define __" . $guardname . "_H__\n\n";
}

sub write_cpp_bottom_guard {

	local (*OUTPUT) = $_[0];

	my $guardname = uc($_[1]);

	print OUTPUT "#endif // __" . $guardname . "_H__\n";

}

sub write_cpp_enum_start {

	local (*OUTPUT) = $_[0];

	print OUTPUT "enum " . $_[1] . " {\n";
}

sub write_cpp_enum_end {

	local (*OUTPUT) = $_[0];

	print OUTPUT "\n};\n"
}


sub write_cpp_enum_line {

	local (*OUTPUT) = $_[0];

	if ($_[3] !~ "yes") {
		print OUTPUT ",\n"
	}

	print OUTPUT "\t" . $_[1] . " = " . $_[2];
}

sub write_cpp_define_line {

	local (*OUTPUT) = $_[0];

	print OUTPUT "#define " . $_[1] . " " . $_[2] . "\n";

}

sub write_cpp_typedef_line {

	local (*OUTPUT) = $_[0];

	my $translated_type;

	my $preamble = "";

	switch ($_[2]) {
		case /^u?int(8|16|32|64)$/ { 
			$translated_type = $_[2] . "_t";
		}
		case "string" {
			$translated_type = "std::string"
		}
		else { 
			$preamble = "// ";
			$translated_type = $_[2]
		}
	}

	print OUTPUT $preamble . "typedef " . $translated_type . " " . $_[1] . ";\n";

}

################ JAVA Specific Subroutines #####################


sub write_java_define_line {

	local (*OUTPUT) = $_[0];

	my $datatype = "int";

	if ($_[2] =~ /^\".*\"$/) {
		$datatype = "String";
	}

	print OUTPUT "public final static " . $datatype . " " . $_[1] . " = " . $_[2] . ";\n";
}

sub write_java_typedef_line {

	die "Typedef not supported on java";

}
