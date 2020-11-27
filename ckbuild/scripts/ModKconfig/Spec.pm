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

package ModKconfig::Spec;

use strict;
use warnings;

use Carp;
use File::Basename;
use File::Spec;

=head1 NAME

ModKconfig::Spec - Class to manipulate ModKconfig module specifications

=head1 SYNOPSIS

  use ModKconfig::Spec;
  $s = ModKconfig::Spec->new({
    'tree' => '/path/linux',
    'obj_tree' => '/path/linux-build',
    'kconfig' => "arch/arm/Kconfig",
    'config' => '.config',
    'title' => 'Kernel',
  });
  $s = ModKconfig::Spec->new_from_file('/path/mymodule/ModKconfig');
  
=head1 DESCRIPTION

ModKconfig::Spec describes a ModKconfig module specification. Such a
specification contains information such as source directory, the build
directory, the module's Kconfig and .config files, C headers and
dependencies on other ModKconfig modules.

=over 6

=item ModKconfig::Spec->new( [$params] )

Creates a new ModKconfig::Spec, optionally setting some member variables.
All unspecified variables get set to their default values.

=cut

sub new
{
    my ($class, $params) = @_;
    $params = {} unless defined($params);
    my $self = {
        'tree' => '',
        'obj_tree' => '',
        'title' => "Unnamed Module",
        'depends' => [],
        'kconfig' => 'Kconfig',
        'config' => '.config',
        'autoconf_header' => 'modkconfig/autoconf.h',
        'config_header' => 'modkconfig/config.h',
        %$params, 
    };
    bless($self, $class);
}


=item ModKconfig::Spec->new_from_file( $file )

Creates a new ModKconfig::Spec from the contents of a spec file.

=cut

sub new_from_file
{
    my ($class, $file) = @_;
    my $self = $class->new({'tree' => dirname(File::Spec->rel2abs($file))});
    $self->{obj_tree} = $self->{tree};
    if ($file)
    {
        open(FP, "< $file") or
            Carp::croak("could not read spec '$file'");

        # process all lines in the file        
        while (my $line = <FP>)
        {
            # remove end of line, leading and trailing whitespace
            chomp($line);
            $line =~ s/^\s+//;
            $line =~ s/\s+$//;

            # skip empty lines
            next if ($line eq '');

            # skip comment lines, which start with a #
            next if ($line =~ /^#/);

            # check the line is of the form "key = value"
            if ($line =~ /^([a-z_]+)\s*=\s*(.*)/)
            {
                my ($key,$val) = ($1,$2);
                if ($key =~ /^(title|kconfig|config|autoconf_header|config_header)$/)
                {
                    # theses variables get set verbatim
                    $self->{$key} = $val;
                } elsif ($key =~ /^(obj_tree)$/) {
                    # if obj_tree is not absolute, take it relative to the
                    # source tree
                    $self->{$key} = File::Spec->rel2abs($val, $self->{tree});
                } elsif ($key eq 'depends') {
                    # dependencies are space-separated
                    push @{$self->{depends}}, (split /\s+/, $val);
                } else {
                    Carp::croak("unknown key '$key' in '$file'");
                }
            } else {
                 Carp::croak("bad line '$line' in '$file'");
            }
        }
        close(FP);
    }
    return $self;
}


=item $s->config

Returns the absolute path to the config file.

=cut

sub config
{
    my $self = shift;
    return File::Spec->rel2abs($self->{config}, $self->{obj_tree});
}


=item $s->kconfig

Returns the absolute path to the kconfig file.

=cut

sub kconfig
{
    my $self = shift;
    return File::Spec->rel2abs($self->{kconfig}, $self->{tree});
}


=item $s->autoconf_header

Returns the absolute path to the C header containing the configuration
for the current module only.

=cut

sub autoconf_header
{
    my $self = shift;
    return File::Spec->rel2abs($self->{autoconf_header}, $self->{obj_tree});
}


=item $s->config_header

Returns the absolute path to the C header containing the configuration
for the current module and all its dependencies.

=cut

sub config_header
{
    my $self = shift;
    return File::Spec->rel2abs($self->{config_header}, $self->{obj_tree});
}


=back

=cut

1;
