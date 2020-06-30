/* enc28j60.c: SpeccyBoot Ethernet emulation
   
   Emulates SPI communication and (a minimal subset of) the
   functionality of the Microchip ENC28J60 Ethernet controller. Refer
   to the ENC28J60 data sheet for details.

   ENC28J60 data sheet:
     http://www.microchip.com/wwwproducts/Devices.aspx?dDocName=en022889

   Copyright (c) 2009-2015 Patrik Persson, Philip Kendall
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
   
   Author contact information:
   
   E-mail: philip-fuse@shadowmagic.org.uk
 
*/

#include <config.h>

#include <string.h>
#include <unistd.h>

#include "compat.h"
#include "enc28j60.h"
#include "fuse.h"
#include "settings.h"
#include "ui/ui.h"

/* ---------------------------------------------------------------------------
 * ENC28J60 emulation
 * ------------------------------------------------------------------------ */

#define ERDPTL(_x)            (_x)->registers[0][0x00]
#define ERDPTH(_x)            (_x)->registers[0][0x01]
#define EWRPTL(_x)            (_x)->registers[0][0x02]
#define EWRPTH(_x)            (_x)->registers[0][0x03]
#define ETXSTL(_x)            (_x)->registers[0][0x04]
#define ETXSTH(_x)            (_x)->registers[0][0x05]
#define ETXNDL(_x)            (_x)->registers[0][0x06]
#define ETXNDH(_x)            (_x)->registers[0][0x07]
#define ERXSTL(_x)            (_x)->registers[0][0x08]
#define ERXSTH(_x)            (_x)->registers[0][0x09]
#define ERXNDL(_x)            (_x)->registers[0][0x0a]
#define ERXNDH(_x)            (_x)->registers[0][0x0b]
#define ERXRDPTL(_x)          (_x)->registers[0][0x0c]
#define ERXRDPTH(_x)          (_x)->registers[0][0x0d]
#define ERXWRPTL(_x)          (_x)->registers[0][0x0e]
#define ERXWRPTH(_x)          (_x)->registers[0][0x0f]

/* Common registers (0x1b and up) are stored in bank 0 in this structure */
#define ESTAT(_x)             (_x)->registers[0][0x1d]
#define ECON2(_x)             (_x)->registers[0][0x1e]
#define ECON1(_x)             (_x)->registers[0][0x1f]

#define EPKTCNT(_x)           (_x)->registers[1][0x19]
#define MIRDH(_x)             (_x)->registers[2][0x19]

#define ECON1_RXEN            (0x04)
#define ECON1_TXRTS           (0x08)
#define ECON2_PKTDEC          (0x40)
#define ESTAT_CLKRDY          (0x01)

#define PHSTAT2_HI_LSTAT      (0x04)

/* Helpers for reading/writing 13-bit SRAM pointer registers */
#define GET_PTR_REG(_x, _nm)          (((_nm ## H(_x) & 0x1f) * 0x0100) + _nm ## L(_x))
#define SET_PTR_REG(_x, _nm, _v)      ((_nm ## H(_x) = HIBYTE(_v)),(_nm ## L(_x) = LOBYTE(_v)))

#ifndef LOBYTE
#define LOBYTE(x)                       ((x) & 0x00ff)
#endif

#ifndef HIBYTE
#define HIBYTE(x)                       (((x) >> 8) & 0x00ff)
#endif

/* ---------------------------------------------------------------------------
 * Ethernet frame layout
 * ------------------------------------------------------------------------ */

/* Maximal Ethernet frame we can receive */
#define ETH_MAX                         (0x600)

/* Status info before the frame (ENC28J60 data sheet, figure 7-3) */
#define ETH_STATUS_NEXT_LO              (0)
#define ETH_STATUS_NEXT_HI              (1)
#define ETH_STATUS_LENGTH               (6)

/* ------------------------------------------------------------------------- */

struct nic_enc28j60_t {

  /* Reserve 6 bytes before the Ethernet frame for next pointer + RSV */
  libspectrum_byte eth_rx_buf[ETH_STATUS_LENGTH + ETH_MAX];

  libspectrum_byte sram[0x2000];
  libspectrum_byte registers[4][32];

  /* Current register for RCR and WCR commands */
  libspectrum_byte curr_register;
  libspectrum_byte curr_register_bank;

  /* TAP file descriptor */
  int tap_fd;

  /* ---------------------------------------------------------------------------
   * SPI state
   * ------------------------------------------------------------------------ */

  /* Number of valid bits currently held in MISO/MOSI shift registers (0..7) */
  libspectrum_byte miso_bits;
  libspectrum_byte miso_valid_bits;

  libspectrum_byte mosi_bits;
  libspectrum_byte mosi_valid_bits;

  nic_enc28j60_spi_state spi_state;

};

nic_enc28j60_t*
nic_enc28j60_alloc( void )
{
  nic_enc28j60_t *self = libspectrum_new( nic_enc28j60_t, 1 );

  self->tap_fd = -1;
  self->spi_state = SPI_IDLE;
  return self;
}

void
nic_enc28j60_init( nic_enc28j60_t *self )
{
  self->tap_fd = compat_get_tap( settings_current.speccyboot_tap );
}

void
nic_enc28j60_free( nic_enc28j60_t *self )
{
  libspectrum_free( self );
}

/* Poll for received frames. */
void
nic_enc28j60_poll( nic_enc28j60_t *self )
{
  ssize_t n;

  if ( (ECON1(self) & ECON1_RXEN)     /* Ethernet RX enabled? */
       && self->tap_fd > 0
       && (n = read( self->tap_fd,
                     self->eth_rx_buf + ETH_STATUS_LENGTH,
                     ETH_MAX )) > 0) {
    libspectrum_word erxwrpt = GET_PTR_REG( self, ERXWRPT );
    libspectrum_word erxst   = GET_PTR_REG( self, ERXST );
    libspectrum_word erxnd   = GET_PTR_REG( self, ERXND );

    /* Round total_length upwards to an even value */
    libspectrum_word total_length = (ETH_STATUS_LENGTH + n + 1) & 0x1ffe;
    libspectrum_word next_addr    = erxwrpt + total_length;

    /* Sanity check */
    if (erxwrpt > erxnd)
      return;

    if ( next_addr > erxnd ) {  /* FIFO wrap-around? */  
      libspectrum_word first_part = (erxnd - erxwrpt) + 1;

      next_addr = (next_addr - erxnd) + erxst;
          
      self->eth_rx_buf[ ETH_STATUS_NEXT_LO ] = LOBYTE( next_addr );
      self->eth_rx_buf[ ETH_STATUS_NEXT_HI ] = HIBYTE( next_addr );
      
      memcpy( self->sram + erxwrpt, self->eth_rx_buf, first_part );
      memcpy( self->sram + erxst, self->eth_rx_buf + first_part, total_length - first_part );
    } else {         
      self->eth_rx_buf[ ETH_STATUS_NEXT_LO ] = LOBYTE( next_addr );
      self->eth_rx_buf[ ETH_STATUS_NEXT_HI ] = HIBYTE( next_addr );

      memcpy( self->sram + erxwrpt, self->eth_rx_buf, total_length );
    }

    SET_PTR_REG( self, ERXWRPT, next_addr );

    ++EPKTCNT(self);
  }
}

/* Writing to some registers produces special side effects. */
static void
perform_side_effects_for_write( nic_enc28j60_t *self )
{
  if ( ECON1(self) & ECON1_TXRTS ) {     /* TXRTS: transmission request */
    libspectrum_word frame_start = (GET_PTR_REG(self, ETXST) & 0x1fff) + 1;
    libspectrum_word frame_end   = GET_PTR_REG(self, ETXND) & 0x1fff;

    if ( frame_end > frame_start && self->tap_fd >= 0) {
      ssize_t length = (frame_end - frame_start) + 1;
      if ( write( self->tap_fd, self->sram + frame_start, length ) != length )
        self->tap_fd = -1; /* write failed: disable TAP */
    }

    ECON1(self) &= ~ECON1_TXRTS;
  }

  if ( ECON2(self) & ECON2_PKTDEC ) {    /* PKTDEC: decrease EPKTCNT */
    --EPKTCNT(self);
    ECON2(self) &= ~ECON2_PKTDEC;
  }
}

void
nic_enc28j60_set_spi_state( nic_enc28j60_t *self, nic_enc28j60_spi_state new_state )
{
  self->spi_state = new_state;
  self->miso_valid_bits = self->mosi_valid_bits = 0;
}

void
nic_enc28j60_reset( nic_enc28j60_t *self )
{
  nic_enc28j60_set_spi_state( self, SPI_IDLE );

  memset( self->registers, 0, sizeof( self->registers ) );

  MIRDH(self) = PHSTAT2_HI_LSTAT;  /* Assume PHSTAT2 is mapped to MIRDH */
  ESTAT(self) = ESTAT_CLKRDY;
}

/* Produce one bit for MISO for the next IN I/O operation */
int
nic_enc28j60_spi_produce_bit( nic_enc28j60_t *self )
{
  int bit;

  libspectrum_word erdpt = GET_PTR_REG(self, ERDPT);

  if ( self->miso_valid_bits-- == 0 ) {  /* Load another byte */
    switch ( self->spi_state ) {

    case SPI_RCR:
      self->miso_bits = self->registers[ self->curr_register_bank ][ self->curr_register ];
      break;

    case SPI_RBM:
      self->miso_bits = self->sram[ erdpt ];
      /* Assume ECON2:AUTOINC to be set, wrap at ERXND */
      erdpt = (erdpt == GET_PTR_REG( self, ERXND )) ? GET_PTR_REG( self, ERXST )
                                                    : (erdpt + 1);
      SET_PTR_REG( self, ERDPT, erdpt );
      break;

    default:
      break;
    }

    self->miso_valid_bits = 7;  /* 8 bits in total, one shifted out below */
  }

  bit = self->miso_bits & 0x80 ? 1 : 0;
  self->miso_bits <<= 1;

  return bit;
}

/* Consume one bit from MOSI */
void
nic_enc28j60_spi_consume_bit( nic_enc28j60_t *self, int bit )
{
  self->mosi_bits = (self->mosi_bits << 1) | bit;

  if ( ++self->mosi_valid_bits == 8 ) {
    libspectrum_word ewrpt = GET_PTR_REG(self, EWRPT);

    switch ( self->spi_state ) {

    case SPI_CMD:
      nic_enc28j60_set_spi_state( self, (self->mosi_bits >> 5) & 0x07 );

      if ( self->spi_state == SPI_SRC )
        nic_enc28j60_reset( self );

      self->curr_register = (self->mosi_bits & 0x1f);    
      self->curr_register_bank = (self->curr_register >= 0x1b) ? 0 : (ECON1(self) & 0x03);
      break;

    case SPI_WCR:
      self->registers[ self->curr_register_bank ][ self->curr_register ] = self->mosi_bits;
      perform_side_effects_for_write( self );
      nic_enc28j60_set_spi_state( self, SPI_IDLE );
      break;

    case SPI_WBM:
      self->sram[ ewrpt++ ] = self->mosi_bits;      /* Assume ECON2:AUTOINC to be set */
      SET_PTR_REG( self, EWRPT, ewrpt );
      break;

    case SPI_BFS:
      self->registers[ self->curr_register_bank ][ self->curr_register ] |= self->mosi_bits;
      perform_side_effects_for_write( self );
      nic_enc28j60_set_spi_state( self, SPI_IDLE );
      break;

    case SPI_BFC:
      self->registers[ self->curr_register_bank ][ self->curr_register ] &= ~self->mosi_bits;
      perform_side_effects_for_write( self );
      nic_enc28j60_set_spi_state( self, SPI_IDLE );
      break;

    default:
      break;
    }

    self->mosi_valid_bits = 0;
  }
}
