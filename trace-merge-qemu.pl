#!/usr/bin/perl
#
# Tool for merging and sorting trace data of a guest and host
#
# Created by Yoshihiro YUNOMAE <yoshihiro.yunomae.ez@hitachi.com>
#
# - How to use
# ./trace-merge.pl <-h host_data -g guest_data -t tsc_offset_value>
#
use strict;
use bigint;
use warnings;
use Getopt::Long qw(:config posix_default no_ignore_case);

my @merged_data = ();
my @sorted_data = ();

my ($opt_host, $opt_guest, $opt_offset);
GetOptions(
	"host_data|h=s"	=> \$opt_host,
	"guest_data|g=s"=> \$opt_guest,
	"tsc_offset|t=i"=> \$opt_offset
);

my $tsc_offset = 0;
my $MASK64 = (1 << 64) - 1;

&get_tsc_offset();
&read_all_data();

sub read_all_data {
	my $h_tsc = 0;
	my $g_comm = "";
	my $g_tsc = 0;
	my $g_event = "";
	my $h_line = "";
	my $g_line = "";

	open HOST_DATA, "<", $opt_host or die "Cannot open host file: $!";
	open GUEST_DATA, "<", $opt_guest or die "Cannot open guest file: $!";

	# skip header information of trace files
	while (!$h_tsc) {
		$h_line = <HOST_DATA>;
		if ($h_line =~ /\[[0-9]+\]\s.{4}\s([0-9]+):/) {
			$h_tsc = $1;
		}
	}

	# skip header information of trace files
	while (!$g_tsc) {
		$g_line = <GUEST_DATA>;
		if ($g_line =~ /^(.+\[[0-9]+\]\s.{4}\s)([0-9]+)(:.+)/) {
			$g_comm = $1;
			$g_tsc = ($2 - $tsc_offset) & $MASK64;
			$g_event = $3;
		}
	}

	# sort trace data by tsc
	while ($h_line) {
		if ($h_tsc < $g_tsc) {
			if($h_line =~ /tracing_mark_write/) {
				print "q $h_line";
			}
			else {
				print "h $h_line";
			}
			$h_line = <HOST_DATA>;
			if (!$h_line) {
				last;
			}
			if ($h_line =~ /\[[0-9]+\]\s.{4}\s([0-9]+):/) {
				$h_tsc = $1;
			}
		} else {
			print "g $g_comm$g_tsc$g_event\n";
			$g_line = <GUEST_DATA>;
			if (!$g_line) {
				last;
			}
			if ($g_line =~ /^(.+\[[0-9]+\]\s.{4}\s)([0-9]+)(:.+)/) {
				$g_comm = $1;
				$g_tsc = ($2 - $tsc_offset) & $MASK64;
				$g_event = $3;
			}
		}
	}

	#flush host data
	while ($h_line) {
		print "h $h_line";
		$h_line = <HOST_DATA>;
		if (!$h_line) {
			last;
		}
	}

	#flush guest data
	while ($g_line) {
		print "g $g_comm$g_tsc$g_event\n";
		$g_line = <GUEST_DATA>;
		if (!$g_line) {
			last;
		}
		if ($g_line =~ /^(.+\[[0-9]+\]\s.{4}\s)([0-9]+)(:.+)/) {
			$g_comm = $1;
			$g_tsc = ($2 - $tsc_offset) & $MASK64;
			$g_event = $3;
		}
	}

	close HOST_DATA;
	close GUEST_DATA;
}

sub get_tsc_offset {
	if (!$opt_offset) {
		$tsc_offset = 0;
	} else {
		$tsc_offset = &convert_tscoffset($opt_offset);
	}
}

sub convert_tscoffset {
	my $offset = shift;

	return $offset - (1 << 64);
}
