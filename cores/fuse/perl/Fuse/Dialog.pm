# Fuse::Dialog: routines for creating Fuse dialog boxes
# Copyright (c) 2003-2005 Philip Kendall

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

# E-mail: philip-fuse@shadowmagic.org.uk

package Fuse::Dialog;

use strict;

use English;
use IO::File;

sub read (;$) {

    my $filename = shift;

    my $fh;
    if( defined $filename && $filename ne '-' ) {
	$fh = new IO::File( "< $filename" )
	    or die "Couldn't open '$filename': $!";
    } else {
	$fh = new IO::Handle;
	$fh->fdopen( fileno( STDIN ), "r" ) or die "Couldn't read stdin: $!";
    }

    local $INPUT_RECORD_SEPARATOR = ""; # Paragraph mode

    my @dialogs;
    while( <$fh> ) {

	my( $name, $title, @widgets ) = split /\n/;

	my @widget_data;
	my $postcheck;
	my $posthook;

	foreach( @widgets ) {

	    my( $widget_type, $text, $value, $key, $data1, $data2 ) =
		split /\s*,\s*/;

	    if( lc $widget_type eq 'postcheck' ) {
		$postcheck = $text;
		next;
	    }

	    if( lc $widget_type eq 'posthook' ) {
		$posthook = $text;
		next;
	    }

	    push @widget_data, { type => $widget_type,
				 text => $text,
				 value => $value,
				 key => $key,
				 data1 => $data1,
				 data2 => $data2,
			       };
	}

	push @dialogs, { name => $name,
			 title => $title,
			 postcheck => $postcheck,
			 posthook => $posthook,
			 widgets => \@widget_data };
    }

    return @dialogs;
}

1;
