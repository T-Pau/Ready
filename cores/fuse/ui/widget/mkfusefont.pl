#!/usr/bin/perl -w

# mkfusefont.pl: generate Fuse-format font from a .sbf file
#                (ASCII representation of a .psf (console font) file)
# Copyright (c) 2004 Darren Salt

# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.

# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

# Author contact information:

# E-mail: linux@youmustbejoking.demon.co.uk

use strict;
use POSIX;

die "sorry, I output binary data\n" if isatty( *STDOUT );

my $line;

MAIN: while( defined( $line = <> ) ) {
  chomp $line;
  next unless $line =~ /^\[U\+([[:xdigit:]]{4})]$/;

  my @codes = ( hex $1 );
  while( 1 ) {
    last MAIN unless defined( $line = <> );

    chomp $line;

    next MAIN if $line eq '' or $line =~ /^[[:space:]]*#/;
    last if $line =~ /^[.0]+$/;
    die unless $line =~ /^\[U\+([[:xdigit:]]{4})]$/;

    push @codes, hex $1;
  }
  @codes = grep { $_ < 0xD800 || ($_ >= 0xE000 && $_ < 0xFFFE) } @codes;

  my @pixmap = ( '','','','', '','','','', '','','','', '','','' );
  my $i;

  for( $i = 0; $i < 8; $i++ ) {
    chomp $line;
    die unless $line =~ /^[.0]+$/;
    $line =~ tr/.0/01/;
    die "pixmap line too long" if length ($line) > 15;
    $line .= '0' x ( 15 - length $line );
    for( my $j = 0; $j < 15; $j++ ) { $pixmap[$j] .= substr( $line, $j, 1 ); }

    die unless $i == 7 or defined( $line = <> );
  }

  my $left = 0;
  pop @pixmap while @pixmap && $pixmap[ -1 ] eq '00000000';
  while( @pixmap && $pixmap[0] eq '00000000' ) { $left++; shift @pixmap; }
  my @bitmap = map { oct ('0b'.$_) } @pixmap;

  print pack( "C*", $_ & 255, $_ >> 8, $left | @bitmap << 4, @bitmap )
    foreach @codes;
}
