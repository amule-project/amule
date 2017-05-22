#!/usr/bin/perl
# we register the script
# if someone knows how to unload it clean....do tell
IRC::register("xas", "1.9", "","XChat aMule stats");
# welcome message
IRC::print "\n\0033 Follow the \0034 white\0033 rabbit\0038...\003\n";
IRC::print "\n\0035 Use command \0038/xas\0035 to print out aMule statistics\003";
# we have no life. we are robots...and we hang around in here:
IRC::print "\0035 (#amule @ irc.freenode.net)\003";
# command that we use
IRC::add_command_handler("xas","xas");

#25.06.2005 - PleuR     : int_to_byte, ratio-fixer, fixed line 1 (is connected to online ?)
#16.12.2005 - stefanero : some kad cleanups
#12.12.2005 - fulgas	; made kad work with xas
#06.05.2005 - niet      : file handle change
#12.10.2004 - bisley    : added session/total ratios
#16.06.2004 - niet      : added support for memory usage and binary name
#05.05.2004 - Jacobo221 : fixed typos, sig 2 support, new outputs, crash detect
#29.04.2004 - niet      : renamed astats to xas (X-Chat Amule statistics)
#22.04.2004 - citroklar : added smp support
#      2004 - bootstrap : some hints on file opening
#      2004 - niet      : used some of cheetah.pl script and so astats was born

# Five status currently: online, connecting, offline, closed, crashed
sub xas
{
	#amule program name
	chomp(my $amulename = `ps --no-header -u $ENV{USER} -o ucmd --sort start_time|grep amule|head -n 1`);
	#amule binary date (because we still don't have date in CVS version). GRRRRRR
	#ls -lAF `ps --no-header -u j -o cmd --sort start_time|grep amule|head -n 1|awk '{print $2}'`
	# system uptime
	chomp(my $uptime = `uptime|cut -d " " -f 4- | tr -s " "`);
	# number of cpu's calculated from /proc/cpuinfo
	chomp(my $number_cpus = `cat /proc/cpuinfo | grep 'processor' -c`);
	# type of cpu
        chomp(my $cpu = `cat /proc/cpuinfo | grep 'model name' -m 1 | cut -f 2 -d ":" | cut -c 2-`);
	# cpu speed
	chomp(my $mhz = `cat /proc/cpuinfo | grep "cpu MHz" -m 1 | cut -f 2 -d ":" | cut -c 2-`);
	# what is the aMule's load on cpu
	chomp(my $amulecpu = `ps --no-header -C $amulename -o %cpu --sort start_time|head -n 1`);
	# how much memory is aMule using
	chomp(my $amulemem = (sprintf("%.02f", `ps --no-header -C $amulename  -o rss --sort start_time|head -n 1` / 1024 )));

	# bootstrap
	# there is no spoon...err.... signature
	open(AMULESIGFILE,"$ENV{'HOME'}/.aMule/amulesig.dat") or die "aMule online signature not found. Did you enable it ?";
	chomp(@amulesigdata = <AMULESIGFILE>);
	close AMULESIGFILE;

	# are we high or what ? :-Q
	if ($amulesigdata[4] eq "H")
		{$amuleid="high"}
	 else
		{$amuleid="low"};

	# are we online / offline / connecting
	#kad on
	if($amulesigdata[5]==2) {
		if ($amulesigdata[0]==0) {
			#$amulestatus="Not Connected";
			$amulextatus="Kad: ok";
		} elsif ($amulesigdata[0]==2) {	# Since aMule v2-rc4
			$amulestatus="connecting";
		    $amulextatus="| Kad: ok";
		} else {
			$amulestatus="online";
			$amulextatus="with $amuleid ID on server $amulesigdata[1] [ $amulesigdata[2]:$amulesigdata[3] ] | Kad: ok";
		}
	} elsif ($amulesigdata[5]==1) {
		if ($amulesigdata[0]==0) {
			#$amulestatus="Not Connected";
			$amulextatus="Kad: firewalled";
		} elsif ($amulesigdata[0]==2) {	# Since aMule v2-rc4
			$amulestatus="connecting";
			$amulextatus="| Kad: firewalled";
		} else {
			$amulestatus="online";
			$amulextatus="with $amuleid ID on server $amulesigdata[1] [ $amulesigdata[2]:$amulesigdata[3] ] | Kad: firewalled";
		}
	} else {
		if ($amulesigdata[0]==0) {
			$amulestatus="Not Connected";
			$amulextatus="| Kad: off";
		} elsif ($amulesigdata[0]==2) {	# Since aMule v2-rc4
			$amulestatus="connecting";
			$amulextatus="| Kad: off" ;
		} else {
			$amulestatus="online";
			$amulextatus="with $amuleid ID on server $amulesigdata[1] [ $amulesigdata[2]:$amulesigdata[3] ] | Kad: off";
		}
	}

	# total download traffic in bytes (int_to_byte *after* calculations)
	my $tdl = $amulesigdata[11];

	# total upload traffic in bytes
	my $tul = $amulesigdata[12];

	# session download traffic in bytes
	my $sdl = $amulesigdata[14];

	# session upload traffic in bytes
	my $sul = $amulesigdata[15];

	# ratio
	my $totalratio = calc_ratio($tdl,$tul);

	my $sessionratio = calc_ratio($sdl,$sul);

	# do int_to_bytes to make human-readable output
	$tdl = int_to_bytes($tdl);
	$tul = int_to_bytes($tul);
	$sdl = int_to_bytes($sdl);
	$sul = int_to_bytes($sul);

	# convert runtime from sec to string
	my $seconds = $amulesigdata[16];
	my $days    = pull_count($seconds, 86400);
        my $hours   = pull_count($seconds, 3600);
        my $minutes = pull_count($seconds, 60);

	my $runtime;

	if ($days > 0) {
		$runtime = sprintf "%02iD %02ih %02imin %02is", $days, $hours, $minutes, $seconds;
	}
        elsif ($hours > 0) {
		$runtime = sprintf "%02ih %02imin %02is", $hours, $minutes, $seconds;
	}
	elsif ($minutes > 0) {
		$runtime = sprintf "%02imin %02is", $minutes, $seconds;
	}
	else {
		$runtime = sprintf "%02is", $seconds;
	}

	# and display it

	# if current user isn't running aMule
		if ( ! `ps --no-header -u $ENV{USER} | grep amule`) {
		IRC::command "/say $amulesigdata[10] is not running";
		# Crash detection is implemented since v2-rc4, so XAS should be backwards compatible
		if ( grep(/^1./,$amulesigdata[13]) || $amulesigdata[13]=="2.0.0rc1" || $amulesigdata[13]=="2.0.0rc2" || $amulesigdata[13]=="2.0.0rc3" ) {
			IRC::command "/say aMule $amulesigdata[13] was closed after $runtime!" }
		elsif ( ! grep(/^00 /,$runtime)) {
			IRC::command "/say aMule $amulesigdata[13] crashed after $runtime!" }
		else {
			IRC::command "/say aMule $amulesigdata[13] was closed" };
		IRC::command "/say Total download traffic: $tdl";
		IRC::command "/say Total upload traffic: $tul" }
	# if aMule is running
	else {
		if ($amulesigdata[0]==0 && $amulesigdata[5]==0){
		IRC::command "/say $amulesigdata[10] is not connected";
		}
		else {
		IRC::command "/say $amulesigdata[10] is $amulestatus $amulextatus";}

		IRC::command "/say aMule $amulesigdata[13] is using $amulecpu% CPU, $amulemem MB of memory and it has been running for $runtime";

		# we only display "number of cpus" when we have more then one
		if ($number_cpus > 1) {
			IRC::command "/say on $number_cpus x $cpu @ $mhz up $uptime" }
		else {
			IRC::command "/say on $cpu @ $mhz MHz up $uptime" };

		IRC::command "/say Sharing $amulesigdata[9] files with $amulesigdata[8] clients in queue";
		IRC::command "/say Total download traffic: $tdl, total upload traffic: $tul, Total Ratio: $totalratio";
		IRC::command "/say Session download traffic: $sdl, session upload traffic: $sul, Session Ratio: $sessionratio";
		IRC::command "/say Current DL speed: $amulesigdata[6] KB/s, current UL speed:  $amulesigdata[7] KB/s" };
	return 1;
	# that's it
}

# usage: $count = pull_count(seconds, amount)
# remove from seconds the amount quantity, altering caller's version.
# return the integral number of those amounts so removed.
sub pull_count {
    my($answer) = int($_[0] / $_[1]);
    $_[0] -= $answer * $_[1];
    return $answer;
}

sub calc_ratio
{
	my $ratiomethod = 1;

	# total download traffic in bytes
	my $dl = $_[0];

	# total upload traffic in bytes
	my $ul = $_[1];

	my $ratiodown = 0;
	my $ratioup = 0;

	# calculate ratio
	if ($ul > 0) { # we don't want 'division by zero'-error
		$ratiodown = ($dl/$ul);
		$ratioup = 1;
	}

	if ($ratiodown == 0) { # we don't want 1:0 ratio's if down eq 0
		$ratioup = 0;
	}
	elsif ($ratiodown < 0.5) {
		if ($ratiomethod == 1) { # Set down to 1, calculate up
			$ratioup = ($ul/$dl);
			$ratiodown = 1;
		}
		if ($ratiomethod == 2) { # Increase by 10 until greater than 0.5
			while ($ratiodown < 0.5) {
				$ratiodown = $ratiodown * 10;
				$ratioup = $ratioup * 10;
			}
		}
	}
	$ratiodown = (sprintf("%0.1f",$ratiodown));
	$ratioup = (sprintf("%0.1f",$ratioup));

	return "$ratioup:$ratiodown";
}

sub int_to_bytes
{
	my($value) = int($_[0]);

	if ($value >= 1073741824) {
		$value = (sprintf("%0.2f",$value/1073741824));
		return "$value GiB";
	}
	elsif ($value >= 1048576) {
		$value = (sprintf("%0.2f",$value/1048576));
		return "$value MiB";
	}
	elsif ($value >= 1024) {
		$value = (sprintf("%0.2f",$value/1024));
		return "$value KiB";
	}
	else {
		$value = (sprintf("%0.2f",$value));
		return "$value B";
	}
}
