#!/usr/bin/perl

# cpp-perl.pl: trivial preprocessor with just about enough intelligence
# to parse config.h

# Copyright (c) 2004,2008 Philip Kendall, Gergely Szasz

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

# Author contact information:

# Philip Kendall <philip-fuse@shadowmagic.org.uk>

use warnings;
use strict;

use IO::File;

my %defines;
my @conditions;
my $condition;

sub parse_file ($;$) {
    my( $filename, $inhibit ) = @_;

    $condition = undef;

    $inhibit ||= 0;

    my $fh = new IO::File( '<' . $filename )
        or die "Couldn't open '$filename': $!";

    while( <$fh> ) {

        if( not defined $condition ) {
            $condition = 1;
            foreach ( @conditions ) {
                $condition = ( $condition and $_ );
            }
        }

        if( /^#\s*define\s+([[:alnum:]_]+)\s+(.*)\s*$/ ) {
            $defines{$1} = $2 if $condition;
            next;
        }

        if( /^#\s*undef\s+([[:alnum:]_]+)\s*$/ ) {
            delete $defines{$1} if $condition;
            next;
        }

        if( /^#\s*ifdef\s+([[:alnum:]_]+)/ or
            /^#\s*if\s+defined\s+([[:alnum:]_]+)/ ) {
            push @conditions, defined $defines{$1};
            $condition = undef;
            next;
        }

        if( /^#\s*ifndef\s+([[:alnum:]_]+)/ or
            /^#\s*if\s+!\s*defined\s+([[:alnum:]_]+)/ ) {
            push @conditions, not defined $defines{$1};
            $condition = undef;
            next;
        }
       
        if( /^#\s*else/ ) {
            $conditions[-1] = not $conditions[-1];
            $condition = undef;
            next;
        }
       
        if( /^#\s*elif\s+defined\s+([[:alnum:]_]+)/ ) {
            $conditions[-1] = $conditions[-1] ? not $conditions[-1] :
                                defined $defines{$1};
            $condition = undef;
            next;
        }
       
        if( /^#\s*elif\s+!\s*defined\s+([[:alnum:]_]+)/ ) {
            $conditions[-1] = $conditions[-1] ? not $conditions[-1] :
                                not defined $defines{$1};
            $condition = undef;
            next;
        }
       
        if( /^#\s*endif/ ) {
            pop @conditions;
            $condition = undef;
            next;
        }

        s/^#.*$//;


        print if $condition and not $inhibit;
    }
}
        
die "$0: usage: $0 /path/to/config.h /path/to/data/file\n" unless @ARGV == 2;

parse_file( $ARGV[0], 1 );
parse_file( $ARGV[1] ); 
