#!/usr/bin/perl -w

# keysyms.pl: generate keysyms.c from keysyms.dat
# Copyright (c) 2000-2013 Philip Kendall, Matan Ziv-Av, Russell Marks,
#			  Fredrick Meunier, Catalin Mihaila, Stuart Brady
# Copyright (c) 2015 Sergio Baldoví

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

use strict;

use lib '../../perl';

use Fuse;

my $ui = shift;
$ui = 'gtk' unless defined $ui;

die "$0: unrecognised user interface: $ui\n"
  unless 0 < grep { $ui eq $_ } ( 'gtk', 'x', 'svga', 'fb', 'sdl', 'win32', 'wii' );

sub fb_keysym ($) {

    my $keysym = shift;

    $keysym =~ tr/a-z/A-Z/;
    substr( $keysym, 0, 4 ) = 'WIN' if substr( $keysym, 0, 5 ) eq 'META_';

    return $keysym;
}

sub wii_keysym ($) {
    my $keysym = shift;

    $keysym =~ tr/a-z/A-Z/;
    return "WII_KEY_$keysym";
}

sub sdl_keysym ($) {

    my $keysym = shift;

    if ( $keysym =~ /[a-zA-Z][a-z]+/ ) {
	$keysym =~ tr/a-z/A-Z/;
    }
    $keysym =~ s/(.*)_L$/L$1/;
    $keysym =~ s/(.*)_R$/R$1/;
    
    # All the magic #defines start with `SDLK_'
    $keysym = "SDLK_$keysym";

    return $keysym;
}

sub sdl_unicode_keysym ($) {

    my $keysym = shift;

    if ( $keysym eq "'" ) {
        $keysym = "\\'";
    }
    $keysym = "'$keysym'";

    return $keysym;
}

sub svga_keysym ($) {
    
    my $keysym = shift;

    $keysym =~ tr/a-z/A-Z/;
    $keysym =~ s/(.*)_L$/LEFT$1/;
    $keysym =~ s/(.*)_R$/RIGHT$1/;
    $keysym =~ s/META$/WIN/;		# Fairly sensible mapping
    $keysym =~ s/^PAGE_/PAGE/;

    # All the magic #defines start with `SCANCODE_'
    $keysym = "SCANCODE_$keysym";
    
    # Apart from this one :-)
    $keysym = "127" if $keysym eq 'SCANCODE_MODE_SWITCH';

    return $keysym;
}

sub win32_keysym ($) {

    my $keysym = shift;

    # http://msdn.microsoft.com/en-us/library/dd375731(VS.85).aspx

    $keysym =~ tr/a-z/A-Z/;

    if ( $keysym =~ /^[A-Z0-9]$/ ) {
	$keysym = "'$keysym'";
    } else {
	$keysym =~ s/(.*)_L$/$1/;
	$keysym =~ s/META$/LWIN/;
	$keysym =~ s/ALT$/MENU/;	# Not a typo: the 'menu' key is VK_APPS

	# All the magic #defines start with `VK_'
	$keysym = "VK_$keysym";
    }

    return $keysym;
}

# Parameters for each UI
my %ui_data = (

    fb   => { headers => [ ],
	      # max_length not used
	      skips => { },
	      translations => { 
	          Mode_switch => 'MENU',
	      },
	      function => \&fb_keysym
	    },

    wii => { headers => [ 'ui/wii/wiikeysyms.h' ],
	      max_length => 24,
	      skips => { map { $_ => 1 } ( 'numbersign',
					   'Shift_L', 'Shift_R',
					   'Control_L', 'Control_R',
					   'Alt_L', 'Alt_R',
					   'Meta_L', 'Meta_R',
					   'Hyper_L','Hyper_R',
					   'Super_L','Super_R',
					   'KP_Enter',
					   'Mode_switch' ) },
	      function => \&wii_keysym
	    },

    gtk  => { headers => [ 'gdk/gdkkeysyms.h', 'ui/gtk/gtkcompat.h' ],
	      max_length => 16,
	      skips => { },
	      translations => { },
	      function => sub ($) { "GDK_KEY_$_[0]" },
    	    },

    sdl  => { headers => [ 'SDL.h' ],
	      max_length => 18,
	      skips => { map { $_ => 1 } ( 'Hyper_L','Hyper_R','Caps_Lock',
                         'A' .. 'Z', 'asciitilde', 'bar', 'dead_circumflex',
                         'braceleft', 'braceright', 'percent' ) },
	      unicode_skips => { map { $_ => 1 } qw( Hyper_L Hyper_R Caps_Lock
                         Escape F1 F2 F3 F4 F5 F6 F7 F8 F9 F10 F11 F12
                         BackSpace Tab Caps_Lock Return Shift_L Shift_R
                         Control_L Control_R Alt_L Alt_R Meta_L Meta_R
                         Super_L Super_R Mode_switch Up Down Left Right
                         Insert Delete Home End Page_Up Page_Down KP_Enter
                         dead_circumflex ) },
	      translations => {
		  apostrophe  => 'QUOTE',
		  asciicircum => 'CARET',
		  bracketleft => 'LEFTBRACKET',
		  bracketright => 'RIGHTBRACKET',
		  exclam      => 'EXCLAIM',
		  Control_L   => 'LCTRL',	 
		  Control_R   => 'RCTRL',	 
		  equal       => 'EQUALS',
		  numbersign  => 'HASH',
		  Mode_switch => 'MENU',
		  Page_Up     => 'PAGEUP',
		  Page_Down   => 'PAGEDOWN',
		  parenleft   => 'LEFTPAREN',
		  parenright  => 'RIGHTPAREN',
	      },
	      unicode_translations => {
                  space       => ' ',
                  exclam      => '!',
                  dollar      => '$',
                  numbersign  => '#',
                  ampersand   => "&",
                  apostrophe  => "'",
                  asciitilde  => "~",
                  at          => "@",
                  backslash   => "\\\\",
                  braceleft   => "{",
                  braceright  => "}",
                  bracketleft => "[",
                  bracketright => "]",
                  parenleft   => "(",
                  parenright  => ")",
                  percent     => "%",
                  question    => "?",
                  quotedbl    => '\\"',
                  asterisk    => "*",
                  plus        => "+",
                  comma       => ',',
                  minus       => '-',
                  period      => '.',
                  slash       => '/',
                  colon       => ':',
                  semicolon   => ';',
                  less        => '<',
                  equal       => '=',
                  greater     => '>',
                  asciicircum => '^',
                  bar         => '|',
                  underscore  => '_',
	      },
	      function => \&sdl_keysym,
	      unicode_function => \&sdl_unicode_keysym,
	    },

    svga => { headers => [ 'vgakeyboard.h' ],
	      max_length => 26,
	      skips => { map { $_ => 1 } qw( Hyper_L Hyper_R Super_L Super_R
                 dollar less greater exclam ampersand parenleft parenright
                 asterisk plus colon asciicircum dead_circumflex bar ) },
	      translations => {
		  Caps_Lock  => 'CAPSLOCK',
		  numbersign => 'BACKSLASH',
		  Return     => 'ENTER',
		  Delete     => 'REMOVE',
		  Left       => 'CURSORBLOCKLEFT',
		  Down       => 'CURSORBLOCKDOWN',
		  Up         => 'CURSORBLOCKUP',
		  Right      => 'CURSORBLOCKRIGHT',
		  KP_Enter   => 'KEYPADENTER',
	      },
	      function => \&svga_keysym,
	    },

    x    => { headers => [ 'X11/keysym.h' ],
	      max_length => 15,
	      skips => { },
	      translations => { },
	      function => sub ($) { "XK_$_[0]" },
	    },

    win32 => { headers => [ 'windows.h' ],
	      max_length => 16,
	      skips => { map { $_ => 1 } ( 'Shift_R','Control_R',
					   'Alt_R','Meta_R',
					   'Hyper_L','Hyper_R',
					   'Super_L','Super_R',
					   'KP_Enter',
					   'dollar','less','greater','exclam',
					   'ampersand','parenleft','parenright',
					   'asterisk','plus','colon','bar',
					   'braceleft','braceright','bracketleft','bracketright',
					   'apostrophe','asciicircum','dead_circumflex','asciitilde',
					   'at','backslash','comma','equal','minus','numbersign',
					   'percent','period','question','quotedbl',
					   'semicolon','slash','underscore',
					   'A' .. 'Z' ) },
	      translations => { 
		  BackSpace   => 'BACK',
		  Page_Up     => 'PRIOR',
		  Page_Down   => 'NEXT',
		  Caps_Lock   => 'CAPITAL',
	          Mode_switch => 'APPS',
	      },
	      function => \&win32_keysym
	    },
);

# Translation table for any UI which uses keyboard mode K_MEDIUMRAW
my @cooked_keysyms = (
    # 0x00
    undef, 'ESCAPE', '1', '2', '3', '4', '5', '6',
    '7', '8', '9', '0', 'MINUS', 'EQUAL', 'BACKSPACE', 'TAB',
    # 0x10
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', 'BRACKETLEFT', 'BRACKETRIGHT', 'RETURN', 'CONTROL_L', 'A', 'S',
    # 0x20
    'D', 'F', 'G', 'H', 'J', 'K', 'L', 'SEMICOLON',
    'APOSTROPHE', 'GRAVE', 'SHIFT_L', 'NUMBERSIGN', 'Z', 'X', 'C', 'V',
    # 0x30
    'B', 'N', 'M', 'COMMA', 'PERIOD', 'SLASH', 'SHIFT_R', 'KB_MULTIPLY',
    'ALT_L', 'SPACE', 'CAPS_LOCK', 'F1', 'F2', 'F3', 'F4', 'F5',
    # 0x40
    'F6', 'F7', 'F8', 'F9', 'F10', 'NUM_LOCK', 'SCROLL_LOCK', 'KP_7',
    'KP_8', 'KP_9', 'KP_MINUS', 'KP_4', 'KP_5', 'KP_6', 'KP_PLUS', 'KP_1',
    # 0x50
    'KP_2', 'KP_3', 'KP_0', 'KP_DECIMAL', undef, undef, 'BACKSLASH', 'F11',
    'F12', undef, undef, undef, undef, undef, undef, undef,
    # 0x60
    'KP_ENTER', 'CONTROL_R', 'KP_DIVIDE', 'PRINT', 'ALT_R', undef, 'HOME','UP',
    'PAGE_UP', 'LEFT', 'RIGHT', 'END', 'DOWN', 'PAGE_DOWN', 'INSERT', 'DELETE',
    # 0x70
    undef, undef, undef, undef, undef, undef, undef, 'BREAK',
    undef, undef, undef, undef, undef, 'WIN_L', 'WIN_R', 'MENU'
);

my @keys;
while(<>) {

    next if /^\s*$/;
    next if /^\s*\#/;

    chomp;

    my( $keysym, $key1, $key2 ) = split /\s*,\s*/;

    push @keys, [ $keysym, $key1, $key2 ]

}

my $define = uc $ui;

print Fuse::GPL(
    'keysyms.c: UI keysym to Fuse input layer keysym mappings',
    "2000-2007 Philip Kendall, Matan Ziv-Av, Russell Marks\n" .
    "                           Fredrick Meunier, Catalin Mihaila, Stuart Brady" ),
    << "CODE";

/* This file is autogenerated from keysyms.dat by keysyms.pl.
   Do not edit unless you know what you're doing! */

#include <config.h>

CODE

# Comment to unbreak Emacs' perl mode

print << "CODE";

#include "input.h"
#include "keyboard.h"

CODE

# Comment to unbreak Emacs' perl mode

foreach my $header ( @{ $ui_data{$ui}{headers} } ) {
    print "#include <$header>\n";
}

print "\nkeysyms_map_t keysyms_map[] = {\n\n";

KEY:
foreach( @keys ) {

    my( $keysym ) = @$_;

    next if $ui_data{$ui}{skips}{$keysym};

    my $ui_keysym = $keysym;

    $ui_keysym = $ui_data{$ui}{translations}{$keysym} if
	$ui_data{$ui}{translations}{$keysym};

    $ui_keysym = $ui_data{$ui}{function}->( $ui_keysym );

    if( $ui eq 'svga' and $ui_keysym =~ /WIN$/ ) {
	print "#ifdef $ui_keysym\n";
    }

    if( $ui eq 'fb' ) {

	for( my $i = 0; $i <= $#cooked_keysyms; $i++ ) {
	    next unless defined $cooked_keysyms[$i] and
			$cooked_keysyms[$i] eq $ui_keysym;
	    printf "  { %3i, INPUT_KEY_%-12s },\n", $i, $keysym;
	    last;
	}

    } else {

	printf "  { %-$ui_data{$ui}{max_length}s INPUT_KEY_%-12s },\n",
            "$ui_keysym,", $keysym;

    }

    if( $ui eq 'svga' and $ui_keysym =~ /WIN$/ ) {
	print "#endif                          /* #ifdef $keysym */\n";
    }

}

print << "CODE";

  { 0, 0 }			/* End marker: DO NOT MOVE! */

};

CODE

if( $ui eq 'sdl' ) {

print "\nkeysyms_map_t unicode_keysyms_map[] = {\n\n";

    foreach( @keys ) {

        my( $keysym ) = @$_;

        next if $ui_data{$ui}{unicode_skips}{$keysym};

        my $ui_keysym = $keysym;

        $ui_keysym = $ui_data{$ui}{unicode_translations}{$keysym} if
            $ui_data{$ui}{unicode_translations}{$keysym};

        $ui_keysym = $ui_data{$ui}{unicode_function}->( $ui_keysym );

	printf "  { %-$ui_data{$ui}{max_length}s INPUT_KEY_%-12s },\n",
            "$ui_keysym,", $keysym;

    }

print << "CODE";

  { 0, 0 }			/* End marker: DO NOT MOVE! */

};

CODE
}

if( $ui eq 'win32' ) {

print << "CODE";
keysyms_map_t oem_keysyms_map[] = {

  { '&',             INPUT_KEY_ampersand    },
  { '\\'',            INPUT_KEY_apostrophe   },
  { '~',             INPUT_KEY_asciitilde   },
  { '*',             INPUT_KEY_asterisk     },
  { '\@',             INPUT_KEY_at           },
  { '\\\\',            INPUT_KEY_backslash    },
  { '|',             INPUT_KEY_bar          },
  { '{',             INPUT_KEY_braceleft    },
  { '}',             INPUT_KEY_braceright   },
  { '[',             INPUT_KEY_bracketleft  },
  { ']',             INPUT_KEY_bracketright },
  { ':',             INPUT_KEY_colon        },
  { ',',             INPUT_KEY_comma        },
  { '\$',             INPUT_KEY_dollar       },
  { '=',             INPUT_KEY_equal        },
  { '!',             INPUT_KEY_exclam       },
  { '>',             INPUT_KEY_greater      },
  { '<',             INPUT_KEY_less         },
  { '-',             INPUT_KEY_minus        },
  { '#',             INPUT_KEY_numbersign   },
  { '(',             INPUT_KEY_parenleft    },
  { ')',             INPUT_KEY_parenright   },
  { '%',             INPUT_KEY_percent      },
  { '.',             INPUT_KEY_period       },
  { '+',             INPUT_KEY_plus         },
  { '?',             INPUT_KEY_question     },
  { '"',             INPUT_KEY_quotedbl     },
  { ';',             INPUT_KEY_semicolon    },
  { '/',             INPUT_KEY_slash        },
  { '_',             INPUT_KEY_underscore   },

  { 0, 0 }			/* End marker: DO NOT MOVE! */

};

CODE
}
