#!/usr/bin/perl

use warnings;
use strict;

sub write_byte {

  my( $b ) = @_;

  printf "%c", $b;
}

sub write_word {

  my( $w ) = @_;

  write_byte( $w % 0x100 );
  write_byte( $w / 0x100 );
}

sub write_three {

  my( $three ) = @_;

  write_byte( $three % 0x100 );
  write_word( $three / 0x100 );

}

sub write_dword {

  my( $dw ) = @_;

  write_word( $dw % 0x10000 );
  write_word( $dw / 0x10000 );
}

sub write_header {

  print "ZXTape!\x1a";		# Signature
  write_byte(  1 );		# Major version number
  write_byte( 20 );		# Minor version number

}

sub write_standard_speed_data_block {

  my( $data, $pause ) = @_;

  write_byte( 0x10 );
  write_word( $pause );
  write_word( length $data );
  print $data;

}

sub write_turbo_speed_data_block {

  my( $pilot_length, $pilot_count, $sync1_length, $sync2_length,
      $zero_length, $one_length, $data, $bits_in_last_byte, $pause ) = @_;

  write_byte( 0x11 );
  write_word( $pilot_length );
  write_word( $sync1_length );
  write_word( $sync2_length );
  write_word( $zero_length );
  write_word( $one_length );
  write_word( $pilot_count );
  write_byte( $bits_in_last_byte );
  write_word( $pause );
  write_three( length $data );
  print $data;

}

sub write_pure_tone_block {

  my( $length, $count ) = @_;

  write_byte( 0x12 );
  write_word( $length );
  write_word( $count );

}

sub write_pulse_sequence_block {

  my( @data ) = @_;

  write_byte( 0x13 );
  write_byte( scalar @data );
  write_word( $_ ) foreach @data;

}

sub write_pure_data_block {

  my( $zero_length, $one_length, $data, $bits_in_last_byte, $pause ) = @_;

  write_byte( 0x14 );
  write_word( $zero_length );
  write_word( $one_length );
  write_byte( $bits_in_last_byte );
  write_word( $pause );
  write_three( length $data );
  print $data;

}

sub write_pause_block {

  my( $pause ) = @_;

  write_byte( 0x20 );
  write_word( $pause );

}

sub write_group_start_block {

  my( $name ) = @_;

  write_byte( 0x21 );
  write_byte( length $name );
  print $name;

}

sub write_group_end_block {

  my( $name ) = @_;

  write_byte( 0x22 );

}

sub write_jump_block {

  my( $offset ) = @_;

  write_byte( 0x23 );
  write_word( $offset );

}

sub write_loop_start_block {

  my( $iterations ) = @_;

  write_byte( 0x24 );
  write_word( $iterations );

}

sub write_loop_end_block {

  write_byte( 0x25 );

}

sub write_stop_tape_if_in_48k_mode_block {

  write_byte( 0x2a );
  write_dword( 0 );

}

sub write_text_description_block {

  my( $text ) = @_;

  write_byte( 0x30 );
  write_byte( length $text );
  print $text;

}

sub write_message_block {

  my( $text, $time ) = @_;

  write_byte( 0x31 );
  write_byte( $time );
  write_byte( length $text );
  print $text;

}

sub write_archive_info_block {

  my( @strings ) = @_;

  write_byte( 0x32 );

  my $data;

  foreach my $string ( @strings ) {
    $data .= chr( $string->{id} );
    $data .= chr( length $string->{text} );
    $data .= $string->{text};
  }

  write_word( length $data );
  write_byte( scalar @strings );
  print $data;

}

sub write_hardware_type_block {

  my( @info ) = @_;

  write_byte( 0x33 );
  write_byte( scalar @info );

  foreach my $info ( @info ) {
    write_byte( $info->{type} );
    write_byte( $info->{id} );
    write_byte( $info->{used} );
  }
}

sub write_custom_info_block {

  my( $id, $data ) = @_;

  write_byte( 0x35 );

  if( length $id < 16 ) {
    $id .= ' ' x ( 16 - length $id );
  } elsif( length $id > 16 ) {
    $id = substr( $id, 0, 16 );
  }
  print $id;
  write_dword( length $data );
  print $data;

}

write_header();

write_standard_speed_data_block( "\xaa", 2345 );

write_turbo_speed_data_block( 1000, 5, 123, 456, 789, 400, "\x00\xff\x55\xa0",
			      4, 987 );

write_pure_tone_block( 535, 666 );

write_pulse_sequence_block( 772, 297, 692 );

write_pure_data_block( 552, 1639, "\xff\x00\xfc", 6, 554 );

write_pause_block( 618 );

write_group_start_block( "Group Start" );

write_group_end_block();

write_jump_block( 2 );

write_pure_tone_block( 303, 678 );

write_loop_start_block( 3 );

write_pure_tone_block( 837, 185 );

write_loop_end_block();

write_stop_tape_if_in_48k_mode_block();

write_text_description_block( "Comment here" );

write_message_block( "A message", 1 );

write_archive_info_block( { id => 0x00, text => "Full title" },
			  { id => 0x03, text => "Year"       } );

write_hardware_type_block( { type => 0x00, id => 0x01, used => 0x00 },
			   { type => 0x02, id => 0x02, used => 0x03 } );

write_custom_info_block( "Complete TZX", "Arbitrary custom data" );

write_pure_tone_block( 820, 941 );
