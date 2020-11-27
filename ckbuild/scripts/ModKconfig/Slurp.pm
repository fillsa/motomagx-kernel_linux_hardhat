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

package ModKconfig::Slurp;

use strict;
use warnings;

use Carp;
use Cwd;

our $STRIP_HELP = 0;
our $DEBUG = 0;
our $VERBOSE = 0;

=head1 NAME

ModKconfig::Slurp - Class to source Kconfig files

=head1 SYNOPSIS

  use ModKconfig::Slurp;
  $k = ModKconfig::Slup->slurp('/path/linux/arch/arm/Kconfig', '/path/linux');
  
=head1 DESCRIPTION

ModKconfig::Slurp allows you to read a Kconfig, follow all 'source'
directives, and produce a single merge Kconfig file.

=over 3

=item ModKconfig::Slurp->debug( $msg )

Print a debugging message if $ModKconfig::Slurp::DEBUG is set.

=cut

sub debug
{
    my ($self, $msg) = @_;
    print STDERR $msg
        if ($DEBUG);
}


=item ModKconfig::Slurp->space_level( $line )

Return the number of leading spaces in a line, expanding
all tabs to 8 spaces.

=cut

sub space_level
{
    my ($self, $line) = @_;
    my $ts = 0;
    for (my $i = 0; $i < length($line); $i++)
    {
        my $c = substr($line, $i, 1);
        if ($c eq "\t") {
            $ts = ($ts & ~7) + 8;
        } elsif ($c eq " ") {
            $ts++;
        } else {
            return $ts;
        }
    }
    return $ts;
}


=item ModKconfig::Slurp->slurp( $file, [$chdir] )

Read the Kconfig file $file, follow all the 'source' directives
and return the full contents.

If the optional $chdir parameter is specified, change to that
directory first.

=cut

sub slurp
{
    my ($self, $file, $chdir) = @_;
    my $curdir = cwd;
    chdir($chdir) if $chdir;

    my $output;
    print STDERR "Entering $file\n"
        if ($VERBOSE);

    open(FP, "< $file")
        or Carp::croak("Could not open '$file'");
    my @lines = <FP>;
    close(FP);

    # process all the lines in the Kconfig file
    while (my $line = shift @lines)
    {
        # if this is the beginning of a help block, process it
        if ($line =~ /^\s*(help|---\s*help)/)
        {
            $output .= $line
                unless $STRIP_HELP;
            $self->debug("HELP START : $line");

            # find first non-empty line
            while (($line = shift @lines) && ($line =~ /^\s*$/)) {
                $self->debug("HELP BLANK : $line");
                $output .= $line
                    unless $STRIP_HELP;
            }

            # determine the number of leading spaces of the first help line
            # the end of the help block is the first line with a smaller
            # indentation level
            my $help_level = $self->space_level($line);
            if ($help_level > 0)
            {
                $self->debug("HELP LEVEL : $help_level\n");
                $self->debug("HELP BODY : $line");
                $output .= $line
                    unless $STRIP_HELP;

                # find the end of the help
                while (($line = shift @lines) && (($line =~ /^\s*$/) || ($self->space_level($line) >= $help_level)))
                {
                    $self->debug("HELP BODY : $line");
                    $output .= $line
                        unless $STRIP_HELP;
                }
                $self->debug("HELP END : ". (defined($line) ? $line : "EOF\n"));
            }
        }

        # check we have not hit the end of file
        next if !defined($line);

        # if this line declares 'mainmenu', skip it
        if ($line =~ /^\s*(mainmenu)/)
        {
            next;
        }

        # if this line is a 'source' instruction, descend into the
        # specified file
        if ($line =~ /^\s*source\s+(.*)/)
        {
            my $source = $1;
            $source =~ s/\s+$//;
            $source =~ s/^"(.*)"/$1/;
            $output .= $self->slurp($source);
            next;
        }

        # if we reach this point, the line is included verbatim
        $output .= $line;
    }
   
    # if we changed directories, return to the original directory
    chdir($curdir) if $chdir;
    return $output;
}

=back

=cut

1;
