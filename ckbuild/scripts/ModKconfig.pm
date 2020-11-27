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

package ModKconfig;

use strict;
use warnings;

use Cwd qw(cwd realpath);
use File::Basename;
use File::Copy;
use File::Path;
use File::Temp qw(tempdir);
use Getopt::Std;
use ModKconfig::Slurp;
use ModKconfig::Spec;

=head1 NAME

ModKconfig - Class to perform external kernel modules configuration

=head1 SYNOPSIS

  use ModKconfig;
  $m = ModKconfig->new( $kernel_spec );
  $m->do_config( 'ModKconfig', 'menuconfig' );
  
=head1 DESCRIPTION

ModKconfig allows you to use the kernel's Kconfig/.config system
for external modules.

=over 7

=item ModKconfig->new( $kernel_spec )

Creates a new ModKconfig for the kernel spec $kernel_spec.

=cut

sub new
{
    my ($class, $kernel_spec) = @_;
    Carp::croak("Invalid kernel spec")
        unless ref($kernel_spec);
    
    my $self = {
        'kernel' => $kernel_spec,
    };
    
    bless($self, $class);
}


=item $m->make_tempdir

Creates a temporary directory with the necessary symbolic links
 to invoke kconfig.

=cut

sub make_tempdir
{
    my $self = shift;
    my $kspec = $self->{kernel};
    
    # set directory names
    my $kconfig_dir = "scripts/kconfig";
    my $lxdialog_dir;
    if ( -d "$kspec->{tree}/scripts/kconfig/lxdialog" )
    {
        # newer kernels
        $lxdialog_dir = "scripts/kconfig/lxdialog";
    } elsif ( -d "$kspec->{tree}/scripts/lxdialog" ) {
        # older kernels
        $lxdialog_dir = "scripts/lxdialog";
    } else {
        print STDERR "Could not find 'lxdialog' directory\n";
        return;
    }

    # create temporary directory
    my $tempdir = tempdir();

    # create directory
    mkpath("$tempdir/include/linux");
    
    # create symlinks for kconfig tools, whether they exist or not
    my $targets = {
        "$kconfig_dir/conf" => $kspec->{obj_tree},
        "$kconfig_dir/mconf" => $kspec->{obj_tree},
        "$kconfig_dir/gconf" => $kspec->{obj_tree},
        "$kconfig_dir/gconf.glade" => $kspec->{tree},
        "$kconfig_dir/qconf" => $kspec->{obj_tree},
        "$lxdialog_dir/lxdialog" => $kspec->{obj_tree}
    };
    foreach my $script (keys %$targets)
    {
        my $source = "$targets->{$script}/$script";
        my $dest = "$tempdir/$script";
        my $destdir = dirname($dest);
        if (! -d $destdir)
        {
            if (!mkpath($destdir))
            {
                print STDERR "Could not create directory '$destdir'\n";
                rmtree($tempdir);
                return;
            }
        }
        if (!symlink($source, $dest))
        {
            print STDERR "Could not link '$dest' to '$source'\n";
            rmtree($tempdir);
            return;
        }
    }
    return $tempdir;
}


=item $m->merge_files( $output, @inputs )

Merges the inputs files into a single file $output.

=cut

sub merge_files
{
    my ($self, $output, @inputs) = @_;
    if (!open(OUT, "> $output"))
    {
        print STDERR "Could not open '$output' for writing\n";
        return;
    }
    foreach my $input (@inputs)
    {
        if (!open(IN, "< $input"))
        {
            print STDERR "Could not open '$input' for reading\n";
            close(OUT);
            return;
        }
        my @lines = <IN>;
        close(IN);
        print OUT @lines;
    }
    close(OUT);
    return 1;
}


=item $m->merge_kconfigs( $kconfig, $modspec, @depspecs )

Merges the kconfigs of the current module ($modspec) and its
 dependencies (@depspecs) into a single kconfig ($kconfig).

=cut

sub merge_kconfigs
{
    my ($self, $kconfig, $modspec, @depspecs) = @_;

    # open wrapper Kconfig for writing
    if (!open(FP, "> $kconfig"))
    {
        print STDERR "Could not open '$kconfig' for writing\n";
        return;
    }

    # current module
    print FP ModKconfig::Slurp->slurp($modspec->{kconfig}, $modspec->{tree});

    # dependencies, made read-only by enclosing them in
    # a lock/endlock block
    print FP "lock\n";
    foreach my $spec (@depspecs)
    {
        print FP 
            "menu \"$spec->{title} config (read-only)\"\n".
            ModKconfig::Slurp->slurp($spec->{kconfig}, $spec->{tree}).
            "endmenu\n";
    }
    print FP "endlock\n";
    close(FP);
    return 1;
}


=item $m->recurse_deps( $spec, [$seen] )

Returns all the dependencies for the module spec $spec.

$seen is a hash containing all specs that have already been
processed, in order to avoid duplicates.

=cut

sub recurse_deps
{
    my ($self, $spec, $seen) = @_;
    
    # if $seen is not set, initialise it to an empty hash
    $seen = {} unless defined($seen);

    # run over the current spec's dependencies, process them
    # and store the resulting ModKconfig::Spec's in @specs
    my @specs;
    foreach my $dep (@{$spec->{depends}})
    {
        my $depspecfile = File::Spec->rel2abs($dep, $spec->{tree});
        Carp::croak("Could not find spec file '$depspecfile'")
            unless ( -e $depspecfile );

        # cleanup path so we have unique names
        $depspecfile = realpath($depspecfile);

        # make sure we avoid duplicates
        next if $seen->{$depspecfile};
        $seen->{$depspecfile} = 1;
       
        # recurse 
        my $depspec = ModKconfig::Spec->new_from_file($depspecfile);
        push @specs, $self->recurse_deps($depspec, $seen);
        push @specs, $depspec;
    }
    return @specs;
}


=item $m->do_config( $modspecfile, $operation, [$defconfig] )

The main entry point for ModKconfig. It takes care of preparing
a temporary directory in which kconfig can be run, generates
wrapper kconfig and config files, and runs the kconfig command
for the specified $operation with the wrapper kconfig as its argument.

If the command modified the config file, the module's config file and
config C headers will be copied back.

If the requested operation is 'defconfig', the $defconfig parameter
needs to be set.

=cut

sub do_config
{
    my ($self, $modspecfile, $operation, $defconfig) = @_;

    # check the specified arguments are correct, and determine
    # the kconfig command that will be run
    my $commands = {
        'defconfig'  => "scripts/kconfig/conf -D defconfig",
        'gconfig'    => "scripts/kconfig/gconf",
        'menuconfig' => "scripts/kconfig/mconf",
        'oldconfig'  => "scripts/kconfig/conf -o",
        'silentoldconfig'  => "scripts/kconfig/conf -s",
        'xconfig'    => "scripts/kconfig/qconf",
    };
    if (!defined($commands->{$operation}))
    {
        print STDERR "Unknown operation : $operation\n";
        return;
    }
    if (($operation eq 'defconfig') and (!defined($defconfig)))
    {
        print STDERR "Operation '$operation' requires a defconfig\n";
        return;
    }
    my $command = $commands->{$operation};

    # current module spec
    my $modspec = ModKconfig::Spec->new_from_file($modspecfile);

    # all module dependencies, excluding the kernel
    my @depspecs = $self->recurse_deps($modspec);
    
    # set up the temporary directory
    my $tempdir = $self->make_tempdir
        or return;
    my $tmpkconfig = "$tempdir/Kconfig";
    my $tmpconfig = "$tempdir/.config";
    my $tmpautoconf = "$tempdir/include/linux/autoconf.h";

    # merge kconfig files
    if ( !$self->merge_kconfigs($tmpkconfig, $modspec, $self->{kernel}, @depspecs) )
    {
        rmtree($tempdir);
        return;
    }

    # start building the list of dotconfig files to merge
    my $tmpinconfig = $tmpconfig;
    my @dotconfigs = ( $self->{kernel}->config );
    foreach my $spec (@depspecs) {
        push @dotconfigs, $spec->config;
    }
    # if the operation is 'defconfig', we use $defconfig for the module's
    # configuration values, otherwise we use its current configuration
    # if it exists
    if ($operation eq 'defconfig') {
        $tmpinconfig = "$tempdir/defconfig";
        push @dotconfigs, $defconfig;
    } elsif ( -f $modspec->config ) {
        push @dotconfigs, $modspec->config;
    }
    # merge the dotconfig files
    if ( !$self->merge_files($tmpinconfig, @dotconfigs) )
    {
        rmtree($tempdir);
        return;
    }
   
    # record the modification time of the output dotconfig if it exists,
    # then run config command
    my $mtime = ( -f $tmpconfig ) ? (stat($tmpconfig))[9] : 0;
    my $curdir = cwd();
    chdir($tempdir);
    my $ret = system("$command $tmpkconfig");
    chdir($curdir);

    if ($ret != 0)
    {
        print STDERR "The following command failed:\n$command $tmpkconfig\n"; 
        rmtree($tempdir);
        return;
    }
    
    my $newmtime = (stat($tmpconfig))[9];

    # figure out whether we should write back changes
    #
    # this is the case if the output dotconfig exists AND
    # either the module lacks config files OR the dotconfig 
    # was updated
    if ( ( -f $tmpconfig ) &&
         ( ( ! -f $modspec->config ) ||
           ( ! -f $modspec->autoconf_header ) ||
           ( ! -f $modspec->config_header ) ||
           ( $newmtime != $mtime )
         ) )
    {
        print STDERR "writing config files\n";
        # remove #define AUTOCONF_INCLUDED from autoconf.h
        if (!open(IN, "< $tmpautoconf"))
        {
            print STDERR "Could not read '$tmpautoconf'\n";
            rmtree($tempdir);
            return;
        }
        my @lines = grep(!/^#define AUTOCONF_INCLUDED/, <IN>);
        close(IN);
        if (!open(OUT, "> $tmpautoconf"))
        {
            print STDERR "Could not write '$tmpautoconf'\n";
            rmtree($tempdir);
            return;
        }
        print OUT @lines;
        close(OUT);
 
        # copy back .config file
        mkpath(dirname($modspec->config));
        if (!copy($tmpconfig, $modspec->config))
        {
            print STDERR "Could not write '".$modspec->config."'\n";
            rmtree($tempdir);
            return;
        }

        # copy back autoconf.h
        mkpath(dirname($modspec->autoconf_header));
        if (!copy($tmpautoconf, $modspec->autoconf_header))
        {
            print STDERR "Could not write '".$modspec->autoconf_header."'\n";
            rmtree($tempdir);
            return;
        }

        # generate merged config header
        mkpath(dirname($modspec->config_header));
        my @autoconfs;
        foreach my $spec (@depspecs) {
            push @autoconfs, $spec->autoconf_header;
        }
        push @autoconfs, $tmpautoconf;
        if (!$self->merge_files($modspec->config_header, @autoconfs))
        {
            print STDERR "Could not write '".$modspec->config_header."'\n";
            rmtree($tempdir);
            return;
        }
    }
    
    rmtree($tempdir);
    return 1; 
}


=back

=cut

1;
