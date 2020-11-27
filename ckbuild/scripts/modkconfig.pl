#!/usr/bin/perl
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Copyright 2006 Motorola
# 
# Motorola  2006-Nov-13 - Initial revision.

use strict;
use warnings;

# make sure we find the ModKconfig modules
BEGIN {
  use File::Basename;
  use lib dirname($0);
}

use Getopt::Std;
use ModKconfig;

sub usage
{
    print STDERR "Usage : $0 [options] operation\n\n".
    "Options:\n".
    "  -h         display help message\n".
    "  -a<arch>   arch\n".
    "  -f<file>   use <file> instead of default ModKconfig\n".
    "  -k<kernel> kernel source tree\n".
    "  -o<kernel> kernel build tree\n\n".
    "Operations:\n".
    "  path <field1> .. <fieldN>\n".
    "  defconfig <file>\n".
    "  gconfig\n".
    "  menuconfig\n".
    "  oldconfig\n".
    "  silentoldconfig\n".
    "  xconfig\n".
    "\n";
    exit 1;
}


sub main
{
    my %opts;

    if ( not getopts('a:hk:o:f:', \%opts) or $opts{'h'}) {
        &usage();
    }
    # common options
    $opts{'f'} = 'ModKconfig' unless $opts{'f'};
    &usage unless (@ARGV > 0);
    my $op = shift @ARGV;

    # check whether the operation is 'path'
    if ($op eq 'path') {
        my $spec = ModKconfig::Spec->new_from_file($opts{'f'});
        foreach my $var (@ARGV)
        {
            print $spec->$var."\n";
        }
        exit 0;
    }

    # otherwise, check required options are present for config operations
    if (!$opts{'a'}) {
        print STDERR "No architecture was specified\n";
        &usage;
    }
    if (!$opts{'k'}) {
        print STDERR "No kernel source tree was specified\n";
        &usage;
    }
    $opts{'o'} = $opts{'k'}
        unless defined($opts{'o'});
    my $defconfig;
    if ($op eq 'defconfig') {
        $defconfig = shift @ARGV;
        if (!$defconfig) {
            print STDERR "No default config was specified\n";
            &usage;
        }
    }
 
    # build the kernel spec
    my $kspec = ModKconfig::Spec->new({
        'tree' => $opts{'k'},
        'obj_tree' => $opts{'o'},
        'kconfig' => "arch/$opts{a}/Kconfig",
        'config' => '.config',
        'title' => 'Kernel',
    });

    # create ModKconfig instance for the current kernel
    my $modkconfig = ModKconfig->new($kspec);
   
    my $ok = $modkconfig->do_config($opts{'f'}, $op, $defconfig);
    exit(!$ok);

}

&main;
