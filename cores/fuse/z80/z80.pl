#!/usr/bin/perl -w

# z80.pl: generate C code for Z80 opcodes

# Copyright (c) 1999-2015 Philip Kendall

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

use Fuse;

# The status of which flags relates to which condition

# These conditions involve !( F & FLAG_<whatever> )
my %not = map { $_ => 1 } qw( NC NZ P PO );

# Use F & FLAG_<whatever>
my %flag = (

      C => 'C', NC => 'C',
     PE => 'P', PO => 'P',
      M => 'S',  P => 'S',
      Z => 'Z', NZ => 'Z',

);

# Generalised opcode routines

sub arithmetic_logical ($$$) {

    my( $opcode, $arg1, $arg2 ) = @_;

    unless( $arg2 ) { $arg2 = $arg1; $arg1 = 'A'; }

    if( length $arg1 == 1 ) {
	if( length $arg2 == 1 or $arg2 =~ /^REGISTER[HL]$/ ) {
	    print "      $opcode($arg2);\n";
	} elsif( $arg2 eq '(REGISTER+dd)' ) {
	    print << "CODE";
      {
	libspectrum_byte offset, bytetemp;
	offset = readbyte( PC );
	contend_read_no_mreq( PC, 1 ); contend_read_no_mreq( PC, 1 );
	contend_read_no_mreq( PC, 1 ); contend_read_no_mreq( PC, 1 );
	contend_read_no_mreq( PC, 1 ); PC++;
	z80.memptr.w = REGISTER + (libspectrum_signed_byte)offset;
	bytetemp = readbyte( z80.memptr.w );
	$opcode(bytetemp);
      }
CODE
	} else {
	    my $register = ( $arg2 eq '(HL)' ? 'HL' : 'PC' );
	    my $increment = ( $register eq 'PC' ? '++' : '' );
	    print << "CODE";
      {
	libspectrum_byte bytetemp = readbyte( $register$increment );
	$opcode(bytetemp);
      }
CODE
	}
    } elsif( $opcode eq 'ADD' ) {
	print << "CODE";
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      ${opcode}16($arg1,$arg2);
CODE
    } elsif( $arg1 eq 'HL' and length $arg2 == 2 ) {
	print << "CODE";
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      ${opcode}16($arg2);
CODE
    }
}

sub call_jp ($$$) {

    my( $opcode, $condition, $offset ) = @_;

    print << "CALL";
      z80.memptr.b.l = readbyte(PC++);
      z80.memptr.b.h = readbyte(PC);
CALL

    if( not defined $offset ) {
	print "      $opcode();\n";
    } else {
	my $condition_string;
	if( defined $not{$condition} ) {
	    $condition_string = "! ( F & FLAG_$flag{$condition} )";
	} else {
	    $condition_string = "F & FLAG_$flag{$condition}";
	}
	print << "CALL";
      if( $condition_string ) {
	$opcode();
      } else {
        PC++;
      }
CALL
    }
}

sub cpi_cpd ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'CPI' ? '++' : '--' );

    print << "CODE";
      {
	libspectrum_byte value = readbyte( HL ), bytetemp = A - value,
	  lookup = ( (        A & 0x08 ) >> 3 ) |
	           ( (  (value) & 0x08 ) >> 2 ) |
	           ( ( bytetemp & 0x08 ) >> 1 );
	contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
	contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
	contend_read_no_mreq( HL, 1 );
	HL$modifier; BC--;
	F = ( F & FLAG_C ) | ( BC ? ( FLAG_V | FLAG_N ) : FLAG_N ) |
	  halfcarry_sub_table[lookup] | ( bytetemp ? 0 : FLAG_Z ) |
	  ( bytetemp & FLAG_S );
	if(F & FLAG_H) bytetemp--;
	F |= ( bytetemp & FLAG_3 ) | ( (bytetemp&0x02) ? FLAG_5 : 0 );
	Q = F;
	z80.memptr.w$modifier;
      }
CODE
}

sub cpir_cpdr ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'CPIR' ? '++' : '--' );

    print << "CODE";
      {
	libspectrum_byte value = readbyte( HL ), bytetemp = A - value,
	  lookup = ( (        A & 0x08 ) >> 3 ) |
		   ( (  (value) & 0x08 ) >> 2 ) |
		   ( ( bytetemp & 0x08 ) >> 1 );
	contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
	contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
	contend_read_no_mreq( HL, 1 );
	BC--;
	F = ( F & FLAG_C ) | ( BC ? ( FLAG_V | FLAG_N ) : FLAG_N ) |
	  halfcarry_sub_table[lookup] | ( bytetemp ? 0 : FLAG_Z ) |
	  ( bytetemp & FLAG_S );
	if(F & FLAG_H) bytetemp--;
	F |= ( bytetemp & FLAG_3 ) | ( (bytetemp&0x02) ? FLAG_5 : 0 );
	Q = F;
	if( ( F & ( FLAG_V | FLAG_Z ) ) == FLAG_V ) {
	  contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
	  contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
	  contend_read_no_mreq( HL, 1 );
	  PC-=2;
	  z80.memptr.w = PC+1;
	} else {
	  z80.memptr.w$modifier;
	}
	HL$modifier;
      }
CODE
}

sub inc_dec ($$) {

    my( $opcode, $arg ) = @_;

    my $modifier = ( $opcode eq 'INC' ? '++' : '--' );

    if( length $arg == 1 or $arg =~ /^REGISTER[HL]$/ ) {
	print "      $opcode($arg);\n";
    } elsif( length $arg == 2 or $arg eq 'REGISTER' ) {
	print << "CODE";
	contend_read_no_mreq( IR, 1 );
	contend_read_no_mreq( IR, 1 );
	${arg}$modifier;
CODE
    } elsif( $arg eq '(HL)' ) {
	print << "CODE";
      {
	libspectrum_byte bytetemp = readbyte( HL );
	contend_read_no_mreq( HL, 1 );
	$opcode(bytetemp);
	writebyte(HL,bytetemp);
      }
CODE
    } elsif( $arg eq '(REGISTER+dd)' ) {
	print << "CODE";
      {
	libspectrum_byte offset, bytetemp;
	offset = readbyte( PC );
	contend_read_no_mreq( PC, 1 ); contend_read_no_mreq( PC, 1 );
	contend_read_no_mreq( PC, 1 ); contend_read_no_mreq( PC, 1 );
	contend_read_no_mreq( PC, 1 ); PC++;
	z80.memptr.w = REGISTER + (libspectrum_signed_byte)offset;
	bytetemp = readbyte( z80.memptr.w );
	contend_read_no_mreq( z80.memptr.w, 1 );
	$opcode(bytetemp);
	writebyte(z80.memptr.w,bytetemp);
      }
CODE
    }

}

sub ini_ind ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'INI' ? '+' : '-' );

    print << "CODE";
      {
	libspectrum_byte initemp, initemp2;

	contend_read_no_mreq( IR, 1 );
	initemp = readport( BC );
	writebyte( HL, initemp );

	z80.memptr.w=BC $modifier 1;
        B--; HL$modifier$modifier;
        initemp2 = initemp + C $modifier 1;
	F = ( initemp & 0x80 ? FLAG_N : 0 ) |
            ( ( initemp2 < initemp ) ? FLAG_H | FLAG_C : 0 ) |
            ( parity_table[ ( initemp2 & 0x07 ) ^ B ] ? FLAG_P : 0 ) |
            sz53_table[B];
	Q = F;
      }
CODE
}

sub inir_indr ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'INIR' ? '+' : '-' );

    print << "CODE";
      {
	libspectrum_byte initemp, initemp2;

	contend_read_no_mreq( IR, 1 );
	initemp = readport( BC );
	writebyte( HL, initemp );

	z80.memptr.w=BC $modifier 1;
	B--;
        initemp2 = initemp + C $modifier 1;
	F = ( initemp & 0x80 ? FLAG_N : 0 ) |
            ( ( initemp2 < initemp ) ? FLAG_H | FLAG_C : 0 ) |
            ( parity_table[ ( initemp2 & 0x07 ) ^ B ] ? FLAG_P : 0 ) |
            sz53_table[B];
	Q = F;

	if( B ) {
	  contend_write_no_mreq( HL, 1 ); contend_write_no_mreq( HL, 1 );
	  contend_write_no_mreq( HL, 1 ); contend_write_no_mreq( HL, 1 );
	  contend_write_no_mreq( HL, 1 );
	  PC -= 2;
	}
        HL$modifier$modifier;
      }
CODE
}


sub ldi_ldd ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'LDI' ? '++' : '--' );

    print << "CODE";
      {
	libspectrum_byte bytetemp=readbyte( HL );
	BC--;
	writebyte(DE,bytetemp);
	contend_write_no_mreq( DE, 1 ); contend_write_no_mreq( DE, 1 );
	DE$modifier; HL$modifier;
	bytetemp += A;
	F = ( F & ( FLAG_C | FLAG_Z | FLAG_S ) ) | ( BC ? FLAG_V : 0 ) |
	  ( bytetemp & FLAG_3 ) | ( (bytetemp & 0x02) ? FLAG_5 : 0 );
	Q = F;
      }
CODE
}

sub ldir_lddr ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'LDIR' ? '++' : '--' );

    print << "CODE";
      {
	libspectrum_byte bytetemp=readbyte( HL );
	writebyte(DE,bytetemp);
	contend_write_no_mreq( DE, 1 ); contend_write_no_mreq( DE, 1 );
	BC--;
	bytetemp += A;
	F = ( F & ( FLAG_C | FLAG_Z | FLAG_S ) ) | ( BC ? FLAG_V : 0 ) |
	  ( bytetemp & FLAG_3 ) | ( (bytetemp & 0x02) ? FLAG_5 : 0 );
	Q = F;
	if(BC) {
	  contend_write_no_mreq( DE, 1 ); contend_write_no_mreq( DE, 1 );
	  contend_write_no_mreq( DE, 1 ); contend_write_no_mreq( DE, 1 );
	  contend_write_no_mreq( DE, 1 );
	  PC-=2;
	  z80.memptr.w = PC+1;
	}
        HL$modifier; DE$modifier;
      }
CODE
}

sub otir_otdr ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'OTIR' ? '+' : '-' );

    print << "CODE";
      {
	libspectrum_byte outitemp, outitemp2;

	contend_read_no_mreq( IR, 1 );
	outitemp = readbyte( HL );
	B--;	/* This does happen first, despite what the specs say */
	z80.memptr.w = BC $modifier 1;
	writeport(BC,outitemp);

	HL$modifier$modifier;
        outitemp2 = outitemp + L;
	F = ( outitemp & 0x80 ? FLAG_N : 0 ) |
            ( ( outitemp2 < outitemp ) ? FLAG_H | FLAG_C : 0 ) |
            ( parity_table[ ( outitemp2 & 0x07 ) ^ B ] ? FLAG_P : 0 ) |
            sz53_table[B];
	Q = F;

	if( B ) {
	  contend_read_no_mreq( BC, 1 ); contend_read_no_mreq( BC, 1 );
	  contend_read_no_mreq( BC, 1 ); contend_read_no_mreq( BC, 1 );
	  contend_read_no_mreq( BC, 1 );
	  PC -= 2;
	}
      }
CODE
}

sub outi_outd ($) {

    my( $opcode ) = @_;

    my $modifier = ( $opcode eq 'OUTI' ? '+' : '-' );

    print << "CODE";
      {
	libspectrum_byte outitemp, outitemp2;

	contend_read_no_mreq( IR, 1 );
	outitemp = readbyte( HL );
	B--;	/* This does happen first, despite what the specs say */
	z80.memptr.w = BC $modifier 1;
	writeport(BC,outitemp);

	HL$modifier$modifier;
        outitemp2 = outitemp + L;
	F = ( outitemp & 0x80 ? FLAG_N : 0 ) |
            ( ( outitemp2 < outitemp ) ? FLAG_H | FLAG_C : 0 ) |
            ( parity_table[ ( outitemp2 & 0x07 ) ^ B ] ? FLAG_P : 0 ) |
            sz53_table[B];
	Q = F;
      }
CODE
}

sub push_pop ($$) {

    my( $opcode, $regpair ) = @_;

    my( $high, $low );

    if( $regpair eq 'REGISTER' ) {
	( $high, $low ) = ( 'REGISTERH', 'REGISTERL' );
    } else {
	( $high, $low ) = ( $regpair =~ /^(.)(.)$/ );
    }

    print "      ${opcode}16($low,$high);\n";
}

sub res_set_hexmask ($$) {

    my( $opcode, $bit ) = @_;

    my $mask = 1 << $bit;
    $mask = 0xff - $mask if $opcode eq 'RES';

    sprintf '0x%02x', $mask;
}

sub res_set ($$$) {

    my( $opcode, $bit, $register ) = @_;

    my $operator = ( $opcode eq 'RES' ? '&' : '|' );

    my $hex_mask = res_set_hexmask( $opcode, $bit );

    if( length $register == 1 ) {
	print "      $register $operator= $hex_mask;\n";
    } elsif( $register eq '(HL)' ) {
	print << "CODE";
      {
	libspectrum_byte bytetemp = readbyte( HL );
	contend_read_no_mreq( HL, 1 );
	writebyte( HL, bytetemp $operator $hex_mask );
      }
CODE
    } elsif( $register eq '(REGISTER+dd)' ) {
	print << "CODE";
      {
	libspectrum_byte bytetemp;
	bytetemp = readbyte( z80.memptr.w );
	contend_read_no_mreq( z80.memptr.w, 1 );
	writebyte( z80.memptr.w, bytetemp $operator $hex_mask );
      }
CODE
    }
}

sub rotate_shift ($$) {

    my( $opcode, $register ) = @_;

    if( length $register == 1 ) {
	print "      $opcode($register);\n";
    } elsif( $register eq '(HL)' ) {
	print << "CODE";
      {
	libspectrum_byte bytetemp = readbyte(HL);
	contend_read_no_mreq( HL, 1 );
	$opcode(bytetemp);
	writebyte(HL,bytetemp);
      }
CODE
    } elsif( $register eq '(REGISTER+dd)' ) {
	print << "CODE";
      {
	libspectrum_byte bytetemp = readbyte(z80.memptr.w);
	contend_read_no_mreq( z80.memptr.w, 1 );
	$opcode(bytetemp);
	writebyte(z80.memptr.w,bytetemp);
      }
CODE
    }
}

# Individual opcode routines

sub opcode_ADC (@) { arithmetic_logical( 'ADC', $_[0], $_[1] ); }

sub opcode_ADD (@) { arithmetic_logical( 'ADD', $_[0], $_[1] ); }

sub opcode_AND (@) { arithmetic_logical( 'AND', $_[0], $_[1] ); }

sub opcode_BIT (@) {

    my( $bit, $register ) = @_;

    if( length $register == 1 ) {
	print "      BIT( $bit, $register );\n";
    } elsif( $register eq '(REGISTER+dd)' ) {
	print << "BIT";
      {
	libspectrum_byte bytetemp = readbyte( z80.memptr.w );
	contend_read_no_mreq( z80.memptr.w, 1 );
	BIT_MEMPTR( $bit, bytetemp );
      }
BIT
    } else {
	print << "BIT";
      {
	libspectrum_byte bytetemp = readbyte( HL );
	contend_read_no_mreq( HL, 1 );
	BIT_MEMPTR( $bit, bytetemp );
      }
BIT
    }
}

sub opcode_CALL (@) { call_jp( 'CALL', $_[0], $_[1] ); }

sub opcode_CCF (@) {
    print << "CCF";
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
          ( ( F & FLAG_C ) ? FLAG_H : FLAG_C ) |
          ( ( IS_CMOS ? A : ( ( last_Q ^ F ) | A ) ) & ( FLAG_3 | FLAG_5 ) );
      Q = F;
CCF
}

sub opcode_CP (@) { arithmetic_logical( 'CP', $_[0], $_[1] ); }

sub opcode_CPD (@) { cpi_cpd( 'CPD' ); }

sub opcode_CPDR (@) { cpir_cpdr( 'CPDR' ); }

sub opcode_CPI (@) { cpi_cpd( 'CPI' ); }

sub opcode_CPIR (@) { cpir_cpdr( 'CPIR' ); }

sub opcode_CPL (@) {
    print << "CPL";
      A ^= 0xff;
      F = ( F & ( FLAG_C | FLAG_P | FLAG_Z | FLAG_S ) ) |
	( A & ( FLAG_3 | FLAG_5 ) ) | ( FLAG_N | FLAG_H );
      Q = F;
CPL
}

sub opcode_DAA (@) {
    print << "DAA";
      {
	libspectrum_byte add = 0, carry = ( F & FLAG_C );
	if( ( F & FLAG_H ) || ( ( A & 0x0f ) > 9 ) ) add = 6;
	if( carry || ( A > 0x99 ) ) add |= 0x60;
	if( A > 0x99 ) carry = FLAG_C;
	if( F & FLAG_N ) {
	  SUB(add);
	} else {
	  ADD(add);
	}
	F = ( F & ~( FLAG_C | FLAG_P ) ) | carry | parity_table[A];
	Q = F;
      }
DAA
}

sub opcode_DEC (@) { inc_dec( 'DEC', $_[0] ); }

sub opcode_DI (@) { print "      IFF1=IFF2=0;\n"; }

sub opcode_DJNZ (@) {
    print << "DJNZ";
      contend_read_no_mreq( IR, 1 );
      B--;
      if(B) {
	JR();
      } else {
	contend_read( PC, 3 );
        PC++;
      }
DJNZ
}

sub opcode_EI (@) {
    print << "EI";
      /* Interrupts are not accepted immediately after an EI, but are
	 accepted after the next instruction */
      IFF1 = IFF2 = 1;
      z80.interrupts_enabled_at = tstates;
      event_add( tstates + 1, z80_interrupt_event );
EI
}

sub opcode_EX (@) {

    my( $arg1, $arg2 ) = @_;

    if( $arg1 eq 'AF' and $arg2 eq "AF'" ) {
	print << "EX";
      /* Tape saving trap: note this traps the EX AF,AF\' at #04d0, not
	 #04d1 as PC has already been incremented */
      /* 0x76 - Timex 2068 save routine in EXROM */
      if( PC == 0x04d1 || PC == 0x0077 ) {
	if( tape_save_trap() == 0 ) break;
      }

      {
	libspectrum_word wordtemp = AF; AF = AF_; AF_ = wordtemp;
      }
EX
    } elsif( $arg1 eq '(SP)' and ( $arg2 eq 'HL' or $arg2 eq 'REGISTER' ) ) {

	my( $high, $low );

	if( $arg2 eq 'HL' ) {
	    ( $high, $low ) = qw( H L );
	} else {
	    ( $high, $low ) = qw( REGISTERH REGISTERL );
	}

	print << "EX";
      {
	libspectrum_byte bytetempl, bytetemph;
	bytetempl = readbyte( SP );
	bytetemph = readbyte( SP + 1 ); contend_read_no_mreq( SP + 1, 1 );
	writebyte( SP + 1, $high );
	writebyte( SP,     $low  );
	contend_write_no_mreq( SP, 1 ); contend_write_no_mreq( SP, 1 );
	$low=z80.memptr.b.l=bytetempl;
	$high=z80.memptr.b.h=bytetemph;
      }
EX
    } elsif( $arg1 eq 'DE' and $arg2 eq 'HL' ) {
	print << "EX";
      {
	libspectrum_word wordtemp=DE; DE=HL; HL=wordtemp;
      }
EX
    }
}

sub opcode_EXX (@) {
    print << "EXX";
      {
	libspectrum_word wordtemp;
	wordtemp = BC; BC = BC_; BC_ = wordtemp;
	wordtemp = DE; DE = DE_; DE_ = wordtemp;
	wordtemp = HL; HL = HL_; HL_ = wordtemp;
      }
EXX
}

sub opcode_HALT (@) { print "      z80.halted=1;\n      PC--;\n"; }

sub opcode_IM (@) {

    my( $mode ) = @_;

    print "      IM=$mode;\n";
}

sub opcode_IN (@) {

    my( $register, $port ) = @_;

    if( $register eq 'A' and $port eq '(nn)' ) {
	print << "IN";
      {
	libspectrum_word intemp;
	intemp = readbyte( PC++ ) + ( A << 8 );
        A=readport( intemp );
	/* TODO: is this correct if (nn) was 0xff? */
	z80.memptr.w = intemp + 1;
      }
IN
    } elsif( $register eq 'F' and $port eq '(C)' ) {
	print << "IN";
      {
	libspectrum_byte bytetemp;
	Z80_IN( bytetemp, BC );
      }
IN
    } elsif( length $register == 1 and $port eq '(C)' ) {
	print << "IN";
      Z80_IN( $register, BC );
IN
    }
}

sub opcode_INC (@) { inc_dec( 'INC', $_[0] ); }

sub opcode_IND (@) { ini_ind( 'IND' ); }

sub opcode_INDR (@) { inir_indr( 'INDR' ); }

sub opcode_INI (@) { ini_ind( 'INI' ); }

sub opcode_INIR (@) { inir_indr( 'INIR' ); }

sub opcode_JP (@) {

    my( $condition, $offset ) = @_;

    if( $condition eq 'HL' or $condition eq 'REGISTER' ) {
	print "      PC=$condition;\t\t/* NB: NOT INDIRECT! */\n";
	return;
    } else {
	call_jp( 'JP', $condition, $offset );
    }
}

sub opcode_JR (@) {

    my( $condition, $offset ) = @_;

    if( not defined $offset ) { $offset = $condition; $condition = ''; }

    if( !$condition ) {
	print "      JR();\n";
    } else {
	my $condition_string;
	if( defined $not{$condition} ) {
	    $condition_string = "! ( F & FLAG_$flag{$condition} )";
	} else {
	    $condition_string = "F & FLAG_$flag{$condition}";
	}
	print << "JR";
      if( $condition_string ) {
        JR();
      } else {
        contend_read( PC, 3 );
	PC++;
      }
JR
    }
}

sub opcode_LD (@) {

    my( $dest, $src ) = @_;

    if( length $dest == 1 or $dest =~ /^REGISTER[HL]$/ ) {

	if( length $src == 1 or $src =~ /^REGISTER[HL]$/ ) {

	    if( $dest eq 'R' or $src eq 'R' or $dest eq 'I' or $src eq 'I') {
		print "      contend_read_no_mreq( IR, 1 );\n"
	    }

	    if( $dest eq 'R' and $src eq 'A' ) {
		print << "LD";
      /* Keep the RZX instruction counter right */
      rzx_instructions_offset += ( R - A );
      R=R7=A;
LD
            } elsif( $dest eq 'A' and $src eq 'R' ) {
		print "      A=(R&0x7f) | (R7&0x80);\n";
	    } else {
		print "      $dest=$src;\n" if $dest ne $src;
	    }
            if( $dest eq 'A' and ( $src eq 'I' or $src eq 'R' ) ) {
		print << "LD";
      F = ( F & FLAG_C ) | sz53_table[A] | ( IFF2 ? FLAG_V : 0 );
      Q = F;
      z80.iff2_read = 1;
      event_add( tstates, z80_nmos_iff2_event );
LD
	    }
	} elsif( $src eq 'nn' ) {
	    print "      $dest = readbyte( PC++ );\n";
	} elsif( $src =~ /^\(..\)$/ ) {
	    my $register = substr $src, 1, 2;
	    if( $register eq 'BC' or $register eq 'DE') {
	        print << "LD";
      z80.memptr.w=$register+1;
LD
            }
	    print << "LD";
      $dest=readbyte($register);
LD
        } elsif( $src eq '(nnnn)' ) {
	    print << "LD";
      {
	z80.memptr.b.l = readbyte(PC++);
	z80.memptr.b.h = readbyte(PC++);
	A=readbyte(z80.memptr.w++);
      }
LD
        } elsif( $src eq '(REGISTER+dd)' ) {
	    print << "LD";
      {
	libspectrum_byte offset;
	offset = readbyte( PC );
	contend_read_no_mreq( PC, 1 ); contend_read_no_mreq( PC, 1 );
	contend_read_no_mreq( PC, 1 ); contend_read_no_mreq( PC, 1 );
	contend_read_no_mreq( PC, 1 ); PC++;
	z80.memptr.w = REGISTER + (libspectrum_signed_byte)offset;
	$dest = readbyte( z80.memptr.w );
      }
LD
        }

    } elsif( length $dest == 2 or $dest eq 'REGISTER' ) {

	my( $high, $low );

	if( $dest eq 'SP' or $dest eq 'REGISTER' ) {
	    ( $high, $low ) = ( "${dest}H", "${dest}L" );
	} else {
	    ( $high, $low ) = ( $dest =~ /^(.)(.)$/ );
	}

	if( $src eq 'nnnn' ) {

	    print << "LD";
      $low=readbyte(PC++);
      $high=readbyte(PC++);
LD
        } elsif( $src eq 'HL' or $src eq 'REGISTER' ) {
	    print << "LD";
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      SP = $src;
LD
        } elsif( $src eq '(nnnn)' ) {
	    print "      LD16_RRNN($low,$high);\n";
	}

    } elsif( $dest =~ /^\(..\)$/ ) {

	my $register = substr $dest, 1, 2;

	if( length $src == 1 ) {
	    if( $register eq 'BC' or $register eq 'DE' ) {
	        print << "LD";
      z80.memptr.b.l=$register+1;
      z80.memptr.b.h=A;
LD
            }
	    print << "LD";
      writebyte($register,$src);
LD
	} elsif( $src eq 'nn' ) {
	    print << "LD";
      writebyte($register,readbyte(PC++));
LD
        }

    } elsif( $dest eq '(nnnn)' ) {

	if( $src eq 'A' ) {
	    print << "LD";
      {
	libspectrum_word wordtemp = readbyte( PC++ );
	wordtemp|=readbyte(PC++) << 8;
	z80.memptr.b.l = wordtemp + 1;
	z80.memptr.b.h = A;
	writebyte(wordtemp,A);
      }
LD
        } elsif( $src =~ /^(.)(.)$/ or $src eq 'REGISTER' ) {

	    my( $high, $low );

	    if( $src eq 'SP' or $src eq 'REGISTER' ) {
		( $high, $low ) = ( "${src}H", "${src}L" );
	    } else {
		( $high, $low ) = ( $1, $2 );
	    }

	    print "      LD16_NNRR($low,$high);\n";
	}
    } elsif( $dest eq '(REGISTER+dd)' ) {

	if( length $src == 1 ) {
	  print << "LD";
      {
	libspectrum_byte offset;
	offset = readbyte( PC );
	contend_read_no_mreq( PC, 1 ); contend_read_no_mreq( PC, 1 );
	contend_read_no_mreq( PC, 1 ); contend_read_no_mreq( PC, 1 );
	contend_read_no_mreq( PC, 1 ); PC++;
	z80.memptr.w = REGISTER + (libspectrum_signed_byte)offset;
	writebyte( z80.memptr.w, $src );
      }
LD
        } elsif( $src eq 'nn' ) {
	  print << "LD";
      {
	libspectrum_byte offset, value;
	offset = readbyte( PC++ );
	value = readbyte( PC );
	contend_read_no_mreq( PC, 1 ); contend_read_no_mreq( PC, 1 ); PC++;
	z80.memptr.w = REGISTER + (libspectrum_signed_byte)offset;
	writebyte( z80.memptr.w, value );
      }
LD
        }
    }

}

sub opcode_LDD (@) { ldi_ldd( 'LDD' ); }

sub opcode_LDDR (@) { ldir_lddr( 'LDDR' ); }

sub opcode_LDI (@) { ldi_ldd( 'LDI' ); }

sub opcode_LDIR (@) { ldir_lddr( 'LDIR' ); }

sub opcode_NEG (@) {
    print << "NEG";
      {
	libspectrum_byte bytetemp=A;
	A=0;
	SUB(bytetemp);
      }
NEG
}

sub opcode_NOP (@) { }

sub opcode_OR (@) { arithmetic_logical( 'OR', $_[0], $_[1] ); }

sub opcode_OTDR (@) { otir_otdr( 'OTDR' ); }

sub opcode_OTIR (@) { otir_otdr( 'OTIR' ); }

sub opcode_OUT (@) {

    my( $port, $register ) = @_;

    if( $port eq '(nn)' and $register eq 'A' ) {
	print << "OUT";
      {
	libspectrum_byte nn = readbyte( PC++ );
	libspectrum_word outtemp = nn | ( A << 8 );
	z80.memptr.b.h = A;
	z80.memptr.b.l = (nn + 1);
	writeport( outtemp, A );
      }
OUT
    } elsif( $port eq '(C)' and length $register == 1 ) {
	if ( $register eq '0' ) {
	    print "      writeport( BC, IS_CMOS ? 0xff : 0 );\n";
	} else {
	    print "      writeport( BC, $register );\n";
	}
	print "      z80.memptr.w = BC + 1;\n";
    }
}


sub opcode_OUTD (@) { outi_outd( 'OUTD' ); }

sub opcode_OUTI (@) { outi_outd( 'OUTI' ); }

sub opcode_POP (@) { push_pop( 'POP', $_[0] ); }

sub opcode_PUSH (@) {

    my( $regpair ) = @_;

    print "      contend_read_no_mreq( IR, 1 );\n";
    push_pop( 'PUSH', $regpair );
}

sub opcode_RES (@) { res_set( 'RES', $_[0], $_[1] ); }

sub opcode_RET (@) {

    my( $condition ) = @_;

    if( not defined $condition ) {
	print "      RET();\n";
    } else {
	print "      contend_read_no_mreq( IR, 1 );\n";
	
	if( $condition eq 'NZ' ) {
	    print << "RET";
      if( PC==0x056c || PC == 0x0112 ) {
	if( tape_load_trap() == 0 ) break;
      }
RET
        }

	if( defined $not{$condition} ) {
	    print "      if( ! ( F & FLAG_$flag{$condition} ) ) { RET(); }\n";
	} else {
	    print "      if( F & FLAG_$flag{$condition} ) { RET(); }\n";
	}
    }
}

sub opcode_RETN (@) { 

    print << "RETN";
      IFF1=IFF2;
      RET();
      z80_retn();
RETN
}

sub opcode_RL (@) { rotate_shift( 'RL', $_[0] ); }

sub opcode_RLC (@) { rotate_shift( 'RLC', $_[0] ); }

sub opcode_RLCA (@) {
    print << "RLCA";
      A = ( A << 1 ) | ( A >> 7 );
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	( A & ( FLAG_C | FLAG_3 | FLAG_5 ) );
      Q = F;
RLCA
}

sub opcode_RLA (@) {
    print << "RLA";
      {
	libspectrum_byte bytetemp = A;
	A = ( A << 1 ) | ( F & FLAG_C );
	F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	  ( A & ( FLAG_3 | FLAG_5 ) ) | ( bytetemp >> 7 );
	Q = F;
      }
RLA
}

sub opcode_RLD (@) {
    print << "RLD";
      {
	libspectrum_byte bytetemp = readbyte( HL );
	contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
	contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
	writebyte(HL, (bytetemp << 4 ) | ( A & 0x0f ) );
	A = ( A & 0xf0 ) | ( bytetemp >> 4 );
	F = ( F & FLAG_C ) | sz53p_table[A];
	Q = F;
	z80.memptr.w=HL+1;
      }
RLD
}

sub opcode_RR (@) { rotate_shift( 'RR', $_[0] ); }

sub opcode_RRA (@) {
    print << "RRA";
      {
	libspectrum_byte bytetemp = A;
	A = ( A >> 1 ) | ( F << 7 );
	F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	  ( A & ( FLAG_3 | FLAG_5 ) ) | ( bytetemp & FLAG_C ) ;
	Q = F;
      }
RRA
}

sub opcode_RRC (@) { rotate_shift( 'RRC', $_[0] ); }

sub opcode_RRCA (@) {
    print << "RRCA";
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) | ( A & FLAG_C );
      A = ( A >> 1) | ( A << 7 );
      F |= ( A & ( FLAG_3 | FLAG_5 ) );
      Q = F;
RRCA
}

sub opcode_RRD (@) {
    print << "RRD";
      {
	libspectrum_byte bytetemp = readbyte( HL );
	contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
	contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
	writebyte(HL,  ( A << 4 ) | ( bytetemp >> 4 ) );
	A = ( A & 0xf0 ) | ( bytetemp & 0x0f );
	F = ( F & FLAG_C ) | sz53p_table[A];
	Q = F;
	z80.memptr.w=HL+1;
      }
RRD
}

sub opcode_RST (@) {

    my( $value ) = @_;

    printf "      contend_read_no_mreq( IR, 1 );\n      RST(0x%02x);\n", hex $value;
}

sub opcode_SBC (@) { arithmetic_logical( 'SBC', $_[0], $_[1] ); }

sub opcode_SCF (@) {
    print << "SCF";
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
          ( ( IS_CMOS ? A : ( ( last_Q ^ F ) | A ) ) & ( FLAG_3 | FLAG_5 ) ) |
          FLAG_C;
      Q = F;
SCF
}

sub opcode_SET (@) { res_set( 'SET', $_[0], $_[1] ); }

sub opcode_SLA (@) { rotate_shift( 'SLA', $_[0] ); }

sub opcode_SLL (@) { rotate_shift( 'SLL', $_[0] ); }

sub opcode_SRA (@) { rotate_shift( 'SRA', $_[0] ); }

sub opcode_SRL (@) { rotate_shift( 'SRL', $_[0] ); }

sub opcode_SUB (@) { arithmetic_logical( 'SUB', $_[0], $_[1] ); }

sub opcode_XOR (@) { arithmetic_logical( 'XOR', $_[0], $_[1] ); }

sub opcode_slttrap ($) {
    print "      slt_trap( HL, A );\n";
}

sub opcode_shift (@) {

    my( $opcode ) = @_;

    my $lc_opcode = lc $opcode;

    if( $opcode eq 'DDFDCB' ) {

	print << "shift";
      {
	libspectrum_byte opcode3;
	contend_read( PC, 3 );
	z80.memptr.w =
	    REGISTER + (libspectrum_signed_byte)readbyte_internal( PC );
	PC++; contend_read( PC, 3 );
	opcode3 = readbyte_internal( PC );
	contend_read_no_mreq( PC, 1 ); contend_read_no_mreq( PC, 1 ); PC++;
#ifdef HAVE_ENOUGH_MEMORY
	switch(opcode3) {
#include "z80_ddfdcb.c"
	}
#else			/* #ifdef HAVE_ENOUGH_MEMORY */
	z80_ddfdcbxx(opcode3);
#endif			/* #ifdef HAVE_ENOUGH_MEMORY */
      }
shift
    } else {
	print << "shift";
      {
	libspectrum_byte opcode2;
	contend_read( PC, 4 );
	opcode2 = readbyte_internal( PC ); PC++;
	R++;
#ifdef HAVE_ENOUGH_MEMORY
	switch(opcode2) {
shift

    if( $opcode eq 'DD' or $opcode eq 'FD' ) {
	my $register = ( $opcode eq 'DD' ? 'IX' : 'IY' );
	print << "shift";
#define REGISTER  $register
#define REGISTERL ${register}L
#define REGISTERH ${register}H
#include "z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
shift
        } elsif( $opcode eq 'CB' or $opcode eq 'ED' ) {
	    print "#include \"z80_$lc_opcode.c\"\n";
        }

        print << "shift"
	}
#else			/* #ifdef HAVE_ENOUGH_MEMORY */
	if( z80_${lc_opcode}xx(opcode2) ) goto end_opcode;
#endif			/* #ifdef HAVE_ENOUGH_MEMORY */
      }
shift
    }
}

# Description of each file

my %description = (

    'opcodes_cb.dat'     => 'z80_cb.c: Z80 CBxx opcodes',
    'opcodes_ddfd.dat'   => 'z80_ddfd.c Z80 {DD,FD}xx opcodes',
    'opcodes_ddfdcb.dat' => 'z80_ddfdcb.c Z80 {DD,FD}CBxx opcodes',
    'opcodes_ed.dat'     => 'z80_ed.c: Z80 CBxx opcodes',
    'opcodes_base.dat'   => 'opcodes_base.c: unshifted Z80 opcodes',

);

# Main program

( my $data_file = $ARGV[0] ) =~ s!.*/!!;

print Fuse::GPL( $description{ $data_file }, '1999-2003 Philip Kendall' );

print << "COMMENT";

/* NB: this file is autogenerated by '$0' from '$data_file',
   and included in 'z80_ops.c' */

COMMENT

while(<>) {

    # Remove comments
    s/#.*//;

    # Skip (now) blank lines
    next if /^\s*$/;

    chomp;

    my( $number, $opcode, $arguments, $extra ) = split;

    if( not defined $opcode ) {
	print "    case $number:\n";
	next;
    }

    $arguments = '' if not defined $arguments;
    my @arguments = split ',', $arguments;

    print "    case $number:\t\t/* $opcode";

    print ' ', join ',', @arguments if @arguments;
    print " $extra" if defined $extra;

    print " */\n";

    # Handle the undocumented rotate-shift-or-bit and store-in-register
    # opcodes specially

    if( defined $extra ) {

	my( $register, $opcode ) = @arguments;

	if( $opcode eq 'RES' or $opcode eq 'SET' ) {

	    my( $bit ) = split ',', $extra;

	    my $operator = ( $opcode eq 'RES' ? '&' : '|' );
	    my $hexmask = res_set_hexmask( $opcode, $bit );

	    print << "CODE";
      $register = readbyte(z80.memptr.w) $operator $hexmask;
      contend_read_no_mreq( z80.memptr.w, 1 );
      writebyte(z80.memptr.w, $register);
      break;
CODE
	} else {

	    print << "CODE";
      $register=readbyte(z80.memptr.w);
      contend_read_no_mreq( z80.memptr.w, 1 );
      $opcode($register);
      writebyte(z80.memptr.w, $register);
      break;
CODE
	}
	next;
    }

    {
	no strict qw( refs );

	if( defined &{ "opcode_$opcode" } ) {
	    "opcode_$opcode"->( @arguments );
	}
    }

    print "      break;\n";
}

if( $data_file eq 'opcodes_ddfd.dat' ) {

    print << "CODE";
    default:		/* Instruction did not involve H or L, so backtrack
			   one instruction and parse again */
      PC--;
      R--;
      opcode = opcode2;
#ifdef HAVE_ENOUGH_MEMORY
      goto end_opcode;
#else			/* #ifdef HAVE_ENOUGH_MEMORY */
      return 1;
#endif			/* #ifdef HAVE_ENOUGH_MEMORY */
CODE

} elsif( $data_file eq 'opcodes_ed.dat' ) {
    print << "NOPD";
    default:		/* All other opcodes are NOPD */
      break;
NOPD
}
