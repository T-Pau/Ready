/* enc28j60.h: SpeccyBoot Ethernet emulation
   
   Emulates SPI communication and (a minimal subset of) the
   functionality of the Microchip ENC28J60 Ethernet controller. Refer
   to the ENC28J60 data sheet for details.

   ENC28J60 data sheet:
     http://www.microchip.com/wwwproducts/Devices.aspx?dDocName=en022889

   Copyright (c) 2009-2010 Patrik Persson, Philip Kendall
   
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

#ifndef FUSE_ENC28J60_H
#define FUSE_ENC28J60_H

typedef enum nic_enc28j60_spi_state {
  SPI_IDLE = -2,
  SPI_CMD  = -1,  /* expect a command byte */

  SPI_RCR  = 0,   /* read command register */
  SPI_RBM  = 1,   /* read buffer memory */
  SPI_WCR  = 2,   /* write command register */
  SPI_WBM  = 3,   /* write buffer memory */
  SPI_BFS  = 4,   /* bit-field set */
  SPI_BFC  = 5,   /* bit-field clear */
  SPI_SRC  = 7    /* system reset command */
} nic_enc28j60_spi_state;

typedef struct nic_enc28j60_t nic_enc28j60_t;

nic_enc28j60_t* nic_enc28j60_alloc( void );
void nic_enc28j60_init( nic_enc28j60_t *self );
void nic_enc28j60_free( nic_enc28j60_t *self );

void nic_enc28j60_set_tap_fd( nic_enc28j60_t *self, int tap_fd );

void nic_enc28j60_poll( nic_enc28j60_t *self );
void nic_enc28j60_reset( nic_enc28j60_t *self );
void nic_enc28j60_set_spi_state( nic_enc28j60_t *self, nic_enc28j60_spi_state new_state );
int nic_enc28j60_spi_produce_bit( nic_enc28j60_t *self );
void nic_enc28j60_spi_consume_bit( nic_enc28j60_t *self, int bit );

#endif  /* #ifndef FUSE_ENC28J60_H */
