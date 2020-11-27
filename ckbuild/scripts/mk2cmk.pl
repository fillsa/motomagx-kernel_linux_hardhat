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

#
# script for producing a clearmake-compliant version of arch/*/Makefile
#
use strict;
use warnings;
use Text::Balanced qw(extract_bracketed);

my %funcs = (
    'call' => \&func_call,
);

sub log
{
    print STDERR "@_";
}

sub func_call
{
    my $text = shift;
    my ($func, @args) = sanitize_split($text);
    if ($func eq 'cc-option') {
        return '$(shell $(cc-option) "'.$args[0].'"'. ($args[1] ? ' "'.$args[1].'"' : '') .')';
    } elsif ($func eq 'cc-option-yn') {
        return '$(shell $(cc-option-yn) "'.$args[0].'")';
    } elsif ($func eq 'filechk') {
        return $args[0];
    } else {
        Carp::croak("cannot replace \$(call $text)");
    }
}

sub sanitize
{
    my ($text) = @_;
    defined($text) or $text = '';
    my ($orig_text, $val) = ($text, '');
    my $mylog = sub { &log("[sanitize($orig_text)] @_\n"); };
    $mylog->("SANITIZING : $text\n");
    while ($text)
    {
        # check whether there are still variables to sanitize
        if ($text =~ /(|.*?[^\\])(\$[\(\{].*)/)
        {
            # handle expression
            $val .= $1;
            $text = $2;
            (my $match, $text) = extract_bracketed($text, '(){}', '\$');
            $match or Carp::croak("unbalanced brackets in $text");
            # remove the outter brackets
            my ($b_open, $b_close);
            if ( ($match =~ /^(\{)(.*)(\})$/) || ($match =~ /^(\()(.*)(\))$/) )
            {
                ($b_open, $match, $b_close) = ($1, $2, $3);
            } else {
               Carp::croak("could not remove brackets from $match");
            }
            $mylog->("MATCH : $match\n");
            my $funcref;
            foreach my $func (keys %funcs)
            {
                if ($match =~ /^$func\s+(.*)/) {
                    my $args = $1;
                    $funcref = $funcs{$func};
                    Carp::croak("function '$func' is not implemented")
                        unless defined($funcref);
                    my $res = $funcref->($args);
                    &log("[$func($args)] $res\n");
                    $val .= $res if defined($res);
                    last;
                }
            }
            if (!$funcref) 
            {
                # recurse
                my $var = sanitize($match);
                # perform variable lookup
                $val .= "\$$b_open$var$b_close";
            }
            $mylog->("EXPR : $val\n");
        } else {
            # nothing more to sanitize
            $val .= $text;
            $text = '';
        }
    }
    $mylog->("FINAL : $val\n");
    return $val;
}


=item $p->expand_split( $args, $sep );

Splits string $args using $sep as a separator and expand each bit.

=cut

sub sanitize_split
{
    my ($args) = @_;
    my @xargs;
    while ($args)
    {
        (my $match, my $bit, $args) = extract_token($args, ',');
        $bit = sanitize($bit);
        push @xargs, $bit;
    }
    return @xargs;
}


=item $p->extract_token( $text, $stop )

Extract a token from $text delimited by the regular expression $stop.

This function takes care of skipping over bracketed expressions.

=cut

sub extract_token
{
    my ($text, $stop) = @_;
    my $token = '';
    my $mylog = sub { &log("[extract_token] @_\n"); };
    while (1)
    {
        my ($pre, $delim, $post);
        if ($text =~ /^(|.*?[^\\])($stop)(.*)/)
        {
            ($pre, $delim, $post) = ($1, $2, $3);
            $mylog->("PRE   : $pre\n");
            $mylog->("MATCH : $delim\n");
            $mylog->("POST  : $post\n");
        } else {
           # there is no match, return
           $mylog->("No match for `$stop'\n");
           return (undef, "$token$text");
        }

        # check the match we found is not in a variable
        if (($text =~ /^(|.*?[^\\])(\$[\(\{].*)/) && (length($1) < length($pre)))
        {
            $token .= $1;
            $text = $2;
            $mylog->("Found brackets first ($text)\n");
            ($delim, $text, $pre) = extract_bracketed($text, '(){}', '\$');
            $mylog->("MATCH : $delim\n");
            $mylog->("AFTER : $text\n");
            $mylog->("BEFOR : $pre\n");
            $delim or Carp::croak("unbalanced brackets in `$text`");
            $token .= "$pre$delim";
        } else {
            return ($delim, "$token$pre", $post);
        }
    }
}


sub main
{
    # check arguments
    if (@ARGV < 1)
    {
        print STDERR "Usage : $0 input_makefile\n";
        exit 1;
    }
    my $file = shift @ARGV;

    # read original Makefile
    open(FP, "< $file") or die("could not open $file");
    my @lines = <FP>;
    close(FP);

    # process all lines
    foreach my $line (@lines)
    {
        chomp($line);
        if ($line !~ /^\s+#/)
        {
            my $need_sanitize = 0;
            foreach my $func (keys %funcs) {
                if ($line =~ /^(.*)(\$\($func\s+.*)/) {
                    $need_sanitize =1;
                    last;
                }
            }
            $line = sanitize($line)
                if ($need_sanitize);
        }
        print "$line\n";
    }
}

&main;

