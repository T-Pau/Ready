/* ide.c: Routines for handling HDF hard disk files
   Copyright (c) 2003-2004 Garry Lancaster,
		 2004,2015 Philip Kendall

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

   E-mail: Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "internals.h"

typedef enum libspectrum_ide_command {
  
  LIBSPECTRUM_IDE_COMMAND_READ_SECTOR_RETRY = 0x20,
  LIBSPECTRUM_IDE_COMMAND_READ_SECTOR_NORETRY = 0x21,
  LIBSPECTRUM_IDE_COMMAND_WRITE_SECTOR_RETRY = 0x30,
  LIBSPECTRUM_IDE_COMMAND_WRITE_SECTOR_NORETRY = 0x31,
  LIBSPECTRUM_IDE_COMMAND_IDENTIFY_DRIVE_ATA = 0xec,
  LIBSPECTRUM_IDE_COMMAND_IDENTIFY_DRIVE_ATAPI = 0xa1,
  LIBSPECTRUM_IDE_COMMAND_INITIALIZE_DEVICE_PARAMETERS = 0x91,

} libspectrum_ide_command;

typedef enum libspectrum_ide_phase {
  
  LIBSPECTRUM_IDE_PHASE_READY,
  LIBSPECTRUM_IDE_PHASE_PIO_OUT,
  LIBSPECTRUM_IDE_PHASE_PIO_IN,

} libspectrum_ide_phase;

typedef enum libspectrum_ide_statusreg {

  LIBSPECTRUM_IDE_STATUS_ERR = 0x01,
  LIBSPECTRUM_IDE_STATUS_DRQ = 0x08,
  LIBSPECTRUM_IDE_STATUS_DRDY = 0x40,
  LIBSPECTRUM_IDE_STATUS_BSY = 0x80,

} libspectrum_ide_statusreg;

typedef enum libspectrum_ide_headreg {

  LIBSPECTRUM_IDE_HEAD_HEAD = 0x0f,
  LIBSPECTRUM_IDE_HEAD_DEV = 0x10,
  LIBSPECTRUM_IDE_HEAD_LBA = 0x40,
  
} libspectrum_ide_headreg;

typedef enum libspectrum_ide_errorreg {
  
  LIBSPECTRUM_IDE_ERROR_OK = 0x00,
  LIBSPECTRUM_IDE_ERROR_ABRT = 0x04,
  LIBSPECTRUM_IDE_ERROR_IDNF = 0x10,
  LIBSPECTRUM_IDE_ERROR_UNC = 0x40, 

} libspectrum_ide_errorcode;

typedef enum libspectrum_ide_identityfield {
  
  LIBSPECTRUM_IDE_IDENTITY_NUM_CYLINDERS = 1,
  LIBSPECTRUM_IDE_IDENTITY_NUM_HEADS = 3,
  LIBSPECTRUM_IDE_IDENTITY_NUM_SECTORS = 6,
  LIBSPECTRUM_IDE_IDENTITY_CAPABILITIES = 49,
  LIBSPECTRUM_IDE_IDENTITY_FIELD_VALIDITY = 53,
  LIBSPECTRUM_IDE_IDENTITY_CURRENT_CYLINDERS = 54,
  LIBSPECTRUM_IDE_IDENTITY_CURRENT_HEADS = 55,
  LIBSPECTRUM_IDE_IDENTITY_CURRENT_SECTORS = 56,
  LIBSPECTRUM_IDE_IDENTITY_CURRENT_CAPACITY_LOW = 57,
  LIBSPECTRUM_IDE_IDENTITY_CURRENT_CAPACITY_HI = 58,
  LIBSPECTRUM_IDE_IDENTITY_TOTAL_SECTORS_LOW = 60,
  LIBSPECTRUM_IDE_IDENTITY_TOTAL_SECTORS_HI = 61,

} libspectrum_ide_identityfield;

/* Operations on identity fields.
   For reasons best known to Ramsoft, these (together with the disk
   data itself) are stored in Intel little-endian format rather than
   the way the ATA spec describes.
 */
#define GET_WORD( arr, index ) \
  ( ( (arr)[ ( (index) << 1 ) + 1 ] << 8 ) | ( (arr)[ (index) << 1 ] ) )
#define SET_WORD( arr, index, val ) \
  (arr)[ ( (index) << 1 ) + 1 ] = (val) >> 8; \
  (arr)[ (index) << 1 ] = (val) & 0xff;

struct libspectrum_ide_channel {

  /* Interface bus width */
  libspectrum_ide_databus databus;
  
  /* 2 drives per channel */
  libspectrum_ide_drive drive[2];
  
  /* Last selection written to head register */
  libspectrum_ide_unit selected;
  
  /* Register values */
  libspectrum_byte feature;
  libspectrum_byte sector_count;
  libspectrum_byte sector;
  libspectrum_byte cylinder_low;
  libspectrum_byte cylinder_high;
  libspectrum_byte head;
  libspectrum_byte data2;
  
  /* Channel status */
  libspectrum_ide_phase phase;
  int datacounter;
  
  /* Sector buffer */
  libspectrum_byte buffer[512];
  int sector_number;

  /* One write cache for each drive */
  GHashTable *cache[2];

};

/* Private function prototypes */
static gboolean write_to_disk( gpointer key, gpointer value,
  gpointer user_data );
static gboolean clear_cache( gpointer key, gpointer value,
  gpointer user_data GCC_UNUSED );
static int read_hdf( libspectrum_ide_channel *chn );
static int write_hdf( libspectrum_ide_channel *chn );
static libspectrum_byte read_data( libspectrum_ide_channel *chn );
static void write_data( libspectrum_ide_channel *chn,
  libspectrum_byte data );
static libspectrum_error seek( libspectrum_ide_channel *chn );
static void identifydevice( libspectrum_ide_channel *chn );
static void readsector( libspectrum_ide_channel *chn );
static void writesector( libspectrum_ide_channel *chn );
static void init_device_params( libspectrum_ide_channel *chn );
static void execute_command( libspectrum_ide_channel *chn,
  libspectrum_byte data );


/* Initialise a libspectrum_ide_channel structure */
libspectrum_ide_channel*
libspectrum_ide_alloc( libspectrum_ide_databus databus )
{
  libspectrum_ide_channel *channel;

  channel = libspectrum_new( libspectrum_ide_channel, 1 );

  channel->databus = databus;
  channel->drive[ LIBSPECTRUM_IDE_MASTER ].disk = NULL;
  channel->drive[ LIBSPECTRUM_IDE_SLAVE  ].disk = NULL;

  channel->cache[ LIBSPECTRUM_IDE_MASTER ] =
    g_hash_table_new( g_int_hash, g_int_equal );
  channel->cache[ LIBSPECTRUM_IDE_SLAVE  ] =
    g_hash_table_new( g_int_hash, g_int_equal );

  return channel;
}

/* Free all memory used by a libspectrum_ide_channel structure */
libspectrum_error
libspectrum_ide_free( libspectrum_ide_channel *chn )
{
  /* Make sure all HDF files are ejected first */
  libspectrum_ide_eject( chn, LIBSPECTRUM_IDE_MASTER );
  libspectrum_ide_eject( chn, LIBSPECTRUM_IDE_SLAVE  );

  g_hash_table_destroy( chn->cache[ LIBSPECTRUM_IDE_MASTER ] );
  g_hash_table_destroy( chn->cache[ LIBSPECTRUM_IDE_SLAVE  ] );
  
  /* Free the channel structure */
  libspectrum_free( chn );

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_ide_insert_into_drive( libspectrum_ide_drive *drv,
                                   const char *filename )
{
  FILE *f;
  size_t l;

  /* Open the file */
  f = fopen( filename, "rb+" );
  if( !f ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "libspectrum_ide_insert: unable to open file '%s': %s", filename,
      strerror( errno )
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }
  
  /* Read the HDF header */
  l = fread( &drv->hdf, 1, sizeof( libspectrum_hdf_header ), f ); 
  if ( l != sizeof( libspectrum_hdf_header ) ) {
    fclose( f );
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "libspectrum_ide_insert: unable to read HDF header from '%s'", filename
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }
  
  /* Verify the validity of the header */
  if( memcmp( &drv->hdf.signature, "RS-IDE", 6 ) ||
      drv->hdf.id != 0x1a                           ) {
    fclose( f );
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_ide_insert: '%s' is not a valid HDF file", filename
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }
  
  /* Extract details from the header */
  drv->disk = f;
  drv->data_offset =
    ( drv->hdf.datastart_hi << 8 ) | ( drv->hdf.datastart_low );
  drv->sector_size = ( drv->hdf.flags & 0x01 ) ? 256 : 512;
  
  /* Extract drive geometry from the drive identity command */
  drv->cylinders = GET_WORD(
    drv->hdf.drive_identity, LIBSPECTRUM_IDE_IDENTITY_NUM_CYLINDERS );
  drv->heads = GET_WORD(
    drv->hdf.drive_identity, LIBSPECTRUM_IDE_IDENTITY_NUM_HEADS );
  drv->sectors = GET_WORD(
    drv->hdf.drive_identity, LIBSPECTRUM_IDE_IDENTITY_NUM_SECTORS );
  
  return LIBSPECTRUM_ERROR_NONE;
}

/* Insert a hard disk into a drive */
libspectrum_error
libspectrum_ide_insert( libspectrum_ide_channel *chn,
			libspectrum_ide_unit unit,
                        const char *filename )
{
  libspectrum_ide_drive *drv = &chn->drive[unit];

  libspectrum_ide_eject( chn, unit );
  if ( !filename ) return LIBSPECTRUM_ERROR_NONE;

  return libspectrum_ide_insert_into_drive( drv, filename );
}

static gboolean
write_to_disk( gpointer key, gpointer value, gpointer user_data )
{
  guint sector_number = *(guint*)key;
  libspectrum_byte *buffer = value;
  libspectrum_ide_drive *drv = user_data;

  long sector_position;

  sector_position = drv->data_offset + ( drv->sector_size * sector_number );

  if( fseek( drv->disk, sector_position, SEEK_SET ) )
    return FALSE;

  if( fwrite( buffer, 1, drv->sector_size, drv->disk ) != drv->sector_size )
    return FALSE;

  libspectrum_free( key ); libspectrum_free( value );

  return TRUE;	/* TRUE => remove key/value pair from hash */
}

void
libspectrum_ide_commit_drive( libspectrum_ide_drive *drv, GHashTable *cache )
{
  if( !drv->disk ) return;

  g_hash_table_foreach_remove( cache, write_to_disk, drv );
}

/* Commit any pending writes to disk */
libspectrum_error
libspectrum_ide_commit( libspectrum_ide_channel *chn,
			libspectrum_ide_unit unit )
{
  libspectrum_ide_commit_drive( &chn->drive[ unit ], chn->cache[ unit ] );

  return LIBSPECTRUM_ERROR_NONE;
}

static gboolean
clear_cache( gpointer key, gpointer value, gpointer user_data GCC_UNUSED )
{
  libspectrum_free( key );
  libspectrum_free( value );
  return TRUE;
}

/* Is there any dirty data for this disk? */
int
libspectrum_ide_dirty( libspectrum_ide_channel *chn,
		       libspectrum_ide_unit unit )
{
  return g_hash_table_size( chn->cache[ unit ] ) != 0;
}

/* Eject a hard disk from a drive and free its cache */
libspectrum_error
libspectrum_ide_eject_from_drive( libspectrum_ide_drive *drv,
                                  GHashTable *cache )
{
  if( !drv->disk ) return LIBSPECTRUM_ERROR_NONE;

  fclose( drv->disk );
  drv->disk = NULL;

  g_hash_table_foreach_remove( cache, clear_cache, NULL );
  
  return LIBSPECTRUM_ERROR_NONE;
}

/* Eject a hard disk from a channel / unit combination */
libspectrum_error
libspectrum_ide_eject( libspectrum_ide_channel *chn,
                       libspectrum_ide_unit unit )
{
  return libspectrum_ide_eject_from_drive( &chn->drive[ unit ],
                                           chn->cache[ unit ] );
}

/* Reset an IDE channel */
libspectrum_error
libspectrum_ide_reset( libspectrum_ide_channel *chn )
{
  /* Reset channel status */
  chn->selected = LIBSPECTRUM_IDE_MASTER;
  chn->phase = LIBSPECTRUM_IDE_PHASE_READY;
  
  if( chn->drive[LIBSPECTRUM_IDE_MASTER].disk ||
      chn->drive[LIBSPECTRUM_IDE_SLAVE].disk     ) {

    /* ATA reset signature for non-PACKET-supporting drives */
    chn->sector_count = 0x01;
    chn->sector = 0x01;
    chn->cylinder_low = 0x00;
    chn->cylinder_high = 0x00;
    chn->head = 0x00;

    /* Diagnostics passed */
    if (chn->drive[LIBSPECTRUM_IDE_MASTER].disk)
      chn->drive[LIBSPECTRUM_IDE_MASTER].error = 0x01;
    else
      chn->drive[LIBSPECTRUM_IDE_MASTER].error = 0xff;
    
    if (chn->drive[LIBSPECTRUM_IDE_SLAVE].disk)
      chn->drive[LIBSPECTRUM_IDE_SLAVE].error = 0x01;
    else
      chn->drive[LIBSPECTRUM_IDE_SLAVE].error = 0xff;
    
    
    /* Device ready, no PACKET support */
    if (chn->drive[LIBSPECTRUM_IDE_MASTER].disk)
      chn->drive[LIBSPECTRUM_IDE_MASTER].status = 0x40;
    else
      chn->drive[LIBSPECTRUM_IDE_MASTER].status = 0xff;
    
    if (chn->drive[LIBSPECTRUM_IDE_SLAVE].disk)
      chn->drive[LIBSPECTRUM_IDE_SLAVE].status = 0x40;
    else
      chn->drive[LIBSPECTRUM_IDE_SLAVE].status = 0xff;

    /* Feature is write-only */
    chn->feature = 0xff;

  } else {

    /* If no drive is present, set all registers to 0xff */
    chn->sector_count = 0xff;
    chn->sector = 0xff;
    chn->cylinder_low = 0xff;
    chn->cylinder_high = 0xff;
    chn->head = 0xff;
    chn->drive[LIBSPECTRUM_IDE_MASTER].error = 0xff;
    chn->drive[LIBSPECTRUM_IDE_SLAVE].error = 0xff;
    chn->drive[LIBSPECTRUM_IDE_MASTER].status = 0xff;
    chn->drive[LIBSPECTRUM_IDE_SLAVE].status = 0xff;
    chn->feature = 0xff;

  }
  
  return LIBSPECTRUM_ERROR_NONE;
}

int
libspectrum_ide_read_sector_from_hdf( libspectrum_ide_drive *drv,
    GHashTable *cache, libspectrum_dword sector_number, libspectrum_byte *dest )
{
  libspectrum_byte *buffer, packed_buf[512];

  /* First look in the write cache */
  buffer = g_hash_table_lookup( cache, &sector_number );

  /* If it's not in the write cache, read from the disk image */
  if( !buffer ) {

    long sector_position;

    sector_position =
      drv->data_offset + ( drv->sector_size * sector_number );

    /* Seek to the correct file position */
    if( fseek( drv->disk, sector_position, SEEK_SET ) ) {
      libspectrum_print_error(
          LIBSPECTRUM_ERROR_WARNING,
          "Couldn't seek in HDF file\n" );
      return 1;
    }

    /* Read the packed data into a temporary buffer */
    if ( fread( packed_buf, 1, drv->sector_size, drv->disk ) !=
	 drv->sector_size                                       ) {
      libspectrum_print_error(
          LIBSPECTRUM_ERROR_WARNING,
          "Couldn't read from HDF file\n" );
      return 1;
    }

    buffer = packed_buf;
  }

  /* Unpack or copy the data into the sector buffer */
  if( drv->sector_size == 256 ) {

    int i;
    
    for( i = 0; i < 256; i++ ) {
      dest[ i*2 ] = buffer[ i ];
      dest[ i*2 + 1 ] = 0xff;
    }

  } else {
    memcpy( dest, buffer, 512 );
  }

  return 0;
}

/* Read a sector from the HDF file */
static int
read_hdf( libspectrum_ide_channel *chn )
{
  return libspectrum_ide_read_sector_from_hdf(
      &chn->drive[ chn->selected ], chn->cache[ chn->selected ],
      chn->sector_number, chn->buffer );
}

void
libspectrum_ide_write_sector_to_hdf( libspectrum_ide_drive *drv, GHashTable *cache,
    libspectrum_dword sector_number, libspectrum_byte *src )
{
  libspectrum_byte *buffer;

  buffer = g_hash_table_lookup( cache, &sector_number );

  /* Add this sector to the write cache if it's not already present */
  if( !buffer ) {

    gint *key;

    key = libspectrum_new( gint, 1 );
    buffer = libspectrum_new( libspectrum_byte, drv->sector_size );

    *key = sector_number;
    g_hash_table_insert( cache, key, buffer );

  }

  /* Pack or copy the data into the write cache */
  if ( drv->sector_size == 256 ) {
    int i;
    for( i = 0; i < 256; i++ ) buffer[i] = src[ i * 2 ];
  } else {
    memcpy( buffer, src, 512 );
  }
}

/* Write a sector to the HDF file */
static int
write_hdf( libspectrum_ide_channel *chn )
{
  libspectrum_ide_write_sector_to_hdf( &chn->drive[ chn->selected ], chn->cache[ chn->selected ], chn->sector_number, chn->buffer );
  return 0;
}

/* Read the data register */
static libspectrum_byte
read_data( libspectrum_ide_channel *chn )
{
  libspectrum_byte data;
  libspectrum_ide_drive *drv = &chn->drive[ chn->selected ];
  
  /* Meaningful data is only returned in PIO input phase */
  if( chn->phase != LIBSPECTRUM_IDE_PHASE_PIO_IN ) return 0xff;

  switch( chn->databus ) {

  case LIBSPECTRUM_IDE_DATA8:
    /* 8-bit interfaces just read the low byte */
    data = chn->buffer[ chn->datacounter ];
    chn->datacounter += 2;
    break;

  case LIBSPECTRUM_IDE_DATA16:
    data = chn->buffer[ chn->datacounter++ ];
    break;

  case LIBSPECTRUM_IDE_DATA16_BYTESWAP:
    data = chn->buffer[ chn->datacounter ^ 1 ];
    chn->datacounter++;
    break;
        
  case LIBSPECTRUM_IDE_DATA16_DATA2:
    /* 16-bit interfaces using a secondary data register for the high byte */
    data = chn->buffer[ chn->datacounter++ ];
    chn->data2 = chn->buffer[ chn->datacounter++ ];
    break;

  default:
    data = 0xff; break;

  }

  /* Check for end of phase */
  if( chn->datacounter >= 512 ) {
    if( chn->sector_count ) {
      /* more sectors to read */
      readsector( chn );
    } else {
      /* all sectors done */
      chn->phase = LIBSPECTRUM_IDE_PHASE_READY;
      drv->status &= ~LIBSPECTRUM_IDE_STATUS_DRQ;
    }
  }

  return data;
}

/* Read the IDE interface */
libspectrum_byte
libspectrum_ide_read( libspectrum_ide_channel *chn,
		      libspectrum_ide_register reg )
{
  libspectrum_ide_drive *drv = &chn->drive[ chn->selected ];

    switch( reg ) {

    case LIBSPECTRUM_IDE_REGISTER_DATA:           return read_data( chn );
    case LIBSPECTRUM_IDE_REGISTER_DATA2:          return chn->data2;
    case LIBSPECTRUM_IDE_REGISTER_ERROR_FEATURE:  return drv->error;
    case LIBSPECTRUM_IDE_REGISTER_SECTOR_COUNT:   return chn->sector_count;
    case LIBSPECTRUM_IDE_REGISTER_SECTOR:         return chn->sector;
    case LIBSPECTRUM_IDE_REGISTER_CYLINDER_LOW:   return chn->cylinder_low;
    case LIBSPECTRUM_IDE_REGISTER_CYLINDER_HIGH:  return chn->cylinder_high;
    case LIBSPECTRUM_IDE_REGISTER_HEAD_DRIVE:     return chn->head;
    case LIBSPECTRUM_IDE_REGISTER_COMMAND_STATUS:
      if (!drv->disk) return 0x00;
      else return drv->status;
      break;

    }
  
  return 0xff;
}


/* Write the data register */
static void
write_data( libspectrum_ide_channel *chn, libspectrum_byte data )
{
  libspectrum_ide_drive *drv = &chn->drive[ chn->selected ];

  /* Data register can only be written in PIO output phase */
  if( chn->phase != LIBSPECTRUM_IDE_PHASE_PIO_OUT ) return;

  switch( chn->databus ) {
    
  case LIBSPECTRUM_IDE_DATA8:
    /* 8-bit interfaces just write the low byte */
    chn->buffer[ chn->datacounter ] = data;
    chn->datacounter += 2;
    break;

  case LIBSPECTRUM_IDE_DATA16:
    chn->buffer[ chn->datacounter++ ] = data;
    break;
    
  case LIBSPECTRUM_IDE_DATA16_BYTESWAP:
    chn->buffer[ chn->datacounter ^ 1 ] = data;
    chn->datacounter++;
    break;

  case LIBSPECTRUM_IDE_DATA16_DATA2:
    /* 16-bit interfaces using a secondary data register for the high byte */
    chn->buffer[ chn->datacounter++ ] = data;
    chn->buffer[ chn->datacounter++ ] = chn->data2;
    break;
  }
    
  /* Check for end of phase */
  if( chn->datacounter >= 512 ) {

    /* Write data to disk */
    if ( write_hdf( chn ) ) {
      drv->status |= LIBSPECTRUM_IDE_STATUS_ERR;
      drv->error = LIBSPECTRUM_IDE_ERROR_ABRT | LIBSPECTRUM_IDE_ERROR_UNC;
    }

    if( chn->sector_count ) {
      /* more sectors to write */
      writesector( chn );
    } else {
      /* all sectors done */
      chn->phase = LIBSPECTRUM_IDE_PHASE_READY;
      drv->status &= ~LIBSPECTRUM_IDE_STATUS_DRQ;
    }
  }

}

/* Seek to the addressed sector */
static libspectrum_error
seek( libspectrum_ide_channel *chn )
{
  libspectrum_ide_drive *drv = &chn->drive[ chn->selected ];
  int sectornumber;
  int next_head;

  /* Calculate sector number, depending upon LBA/CHS mode. */
  if( chn->head & LIBSPECTRUM_IDE_HEAD_LBA ) {

    sectornumber = ( chn->head & LIBSPECTRUM_IDE_HEAD_HEAD << 24 ) +
                   ( chn->cylinder_high << 16 )			   +
                   ( chn->cylinder_low << 8 )			   +
                   ( chn->sector );

  } else {

    int cylinder = ( chn->cylinder_high << 8 ) | ( chn->cylinder_low );
    int head = ( chn->head & LIBSPECTRUM_IDE_HEAD_HEAD );
    int sector = ( chn->sector - 1 );

    if( cylinder >= drv->cylinders || head >= drv->heads     ||
        sector < 0                 || sector >= drv->sectors    ) {
      sectornumber = -1;
    } else {
      sectornumber =
	( ( ( cylinder * drv->heads ) + head ) * drv->sectors ) + sector;
    }

  }

  /* Seek to the correct position */
  if( sectornumber < 0                                               ||
      sectornumber >= ( drv->cylinders * drv->heads * drv->sectors )    ) {  
    drv->status |= LIBSPECTRUM_IDE_STATUS_ERR;
    drv->error = LIBSPECTRUM_IDE_ERROR_ABRT | LIBSPECTRUM_IDE_ERROR_IDNF;
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  chn->sector_number = sectornumber;
  
  /* advance registers to next sector, for multiple sector accesses */
  chn->sector_count--;
  if (!chn->sector_count) return LIBSPECTRUM_ERROR_NONE;

  if( chn->head & LIBSPECTRUM_IDE_HEAD_LBA ) {

    /* increment using LBA scheme */
    chn->sector = ( chn->sector + 1 ) & 0xff;
    if( !chn->sector ) {
      chn->cylinder_low = ( chn->cylinder_low + 1 ) & 0xff;
      if( !chn->cylinder_low ) {
        chn->cylinder_high = ( chn->cylinder_high + 1 ) & 0xff;
        if( !chn->cylinder_high ) {
          next_head = ( ( chn->head & LIBSPECTRUM_IDE_HEAD_HEAD ) + 1 ) &
            LIBSPECTRUM_IDE_HEAD_HEAD;
          chn->head = ( chn->head & ~LIBSPECTRUM_IDE_HEAD_HEAD ) | next_head;
        }
      }
    }

  } else {

    /* increment using CHS scheme */
    chn->sector = ( chn->sector % drv->sectors ) + 1;
    /* NB sector number is 1-based */
    if( chn->sector == 1 ) {
      next_head = ( ( chn->head & LIBSPECTRUM_IDE_HEAD_HEAD ) + 1 )
        % drv->heads;
      chn->head = ( chn->head & ~LIBSPECTRUM_IDE_HEAD_HEAD ) | next_head;
      if( !next_head ) {
        chn->cylinder_low = ( chn->cylinder_low + 1 ) & 0xff;
        if( !chn->cylinder_low ) {
          chn->cylinder_high++;
        }
      }
    }

  }
  
  return LIBSPECTRUM_ERROR_NONE;
}

/* Execute the IDENTIFY DEVICE command */
static void
identifydevice( libspectrum_ide_channel *chn )
{
  libspectrum_ide_drive *drv = &chn->drive[ chn->selected ];
  unsigned long sector_count = drv->cylinders * drv->heads * drv->sectors;

  /* Clear sector buffer and copy in HDF identity information */
  memset( &chn->buffer[0], 0, 512 );
  memcpy( &chn->buffer[0], &drv->hdf.drive_identity[0], 0x6a );
    
  /* Fill in fields that lie beyond the end of the HDF header */
  /* Field validity */
  /* TODO: handle drives that exceed the limits of the CHS scheme
     (possibly determining their actual size from HDF file size);
     in this case, words 54-58 will be flagged as invalid */
  /* 0x01 = 'words 54-58 are valid' */
  SET_WORD( chn->buffer, LIBSPECTRUM_IDE_IDENTITY_FIELD_VALIDITY, 0x01 );
  /* Number of current logical cylinders */
  SET_WORD( chn->buffer, LIBSPECTRUM_IDE_IDENTITY_CURRENT_CYLINDERS,
	    drv->cylinders );
  /* Number of current logical heads */
  SET_WORD( chn->buffer, LIBSPECTRUM_IDE_IDENTITY_CURRENT_HEADS,
	    drv->heads );
  /* Number of current logical sectors per logical track */
  SET_WORD( chn->buffer, LIBSPECTRUM_IDE_IDENTITY_CURRENT_SECTORS,
	    drv->sectors );
  /* Current capacity in sectors */
  SET_WORD( chn->buffer, LIBSPECTRUM_IDE_IDENTITY_CURRENT_CAPACITY_LOW,
	    ( sector_count & 0x0000ffff ) );
  SET_WORD( chn->buffer, LIBSPECTRUM_IDE_IDENTITY_CURRENT_CAPACITY_HI,
	    ( sector_count & 0xffff0000 ) >> 16 );

  /* Total number of user addressable sectors;
     only defined if LBA supported */
  if( GET_WORD( chn->buffer, LIBSPECTRUM_IDE_IDENTITY_CAPABILITIES ) &
      0x0200 ) {
    SET_WORD( chn->buffer, LIBSPECTRUM_IDE_IDENTITY_TOTAL_SECTORS_LOW,
	      ( sector_count & 0x0000ffff ) );
    SET_WORD( chn->buffer, LIBSPECTRUM_IDE_IDENTITY_TOTAL_SECTORS_HI,
	      ( sector_count & 0xffff0000 ) >> 16 );
  }

  /* prevent read_data from trying to read from disk after identity block
     is completely read in */
  chn->sector_count = 0;

  /* Initiate the PIO input phase */
  chn->phase = LIBSPECTRUM_IDE_PHASE_PIO_IN;
  drv->status |= LIBSPECTRUM_IDE_STATUS_DRQ;
  chn->datacounter = 0;
}

/* Execute the READ SECTOR command */
static void
readsector( libspectrum_ide_channel *chn )
{
  libspectrum_ide_drive *drv = &chn->drive[ chn->selected ];

  if( seek( chn ) ) return;

  /* Read data from disk */
  if( read_hdf( chn ) ) {
    
    drv->status |= LIBSPECTRUM_IDE_STATUS_ERR;
    drv->error = LIBSPECTRUM_IDE_ERROR_ABRT | LIBSPECTRUM_IDE_ERROR_UNC;

  } else {

    /* Initiate the PIO input phase */
    chn->phase = LIBSPECTRUM_IDE_PHASE_PIO_IN;
    drv->status |= LIBSPECTRUM_IDE_STATUS_DRQ;
    chn->datacounter = 0;

  }
}

/* Execute the WRITE SECTOR command */
static void
writesector( libspectrum_ide_channel *chn )
{
  libspectrum_ide_drive *drv = &chn->drive[ chn->selected ];

  if( seek( chn ) ) return;

  /* Initiate the PIO output phase */
  chn->phase = LIBSPECTRUM_IDE_PHASE_PIO_OUT;
  drv->status |= LIBSPECTRUM_IDE_STATUS_DRQ;
  chn->datacounter = 0;
}

/* Execute the INITIALIZE DEVICE PARAMETERS command */
static void
init_device_params( libspectrum_ide_channel *chn )
{
  libspectrum_ide_drive *drv = &chn->drive[ chn->selected ];
  int size;

  if ( !chn->sector_count ) {
    drv->status |= LIBSPECTRUM_IDE_STATUS_ERR;
    drv->error = LIBSPECTRUM_IDE_ERROR_ABRT;
    return;
  }

  size = drv->heads * drv->sectors * drv->cylinders;

  if ( size > 16514064 ) size = 16514064;

  drv->heads = ( chn->head & LIBSPECTRUM_IDE_HEAD_HEAD ) + 1;
  drv->sectors = chn->sector_count;
  drv->cylinders = size / (drv->heads * drv->sectors);

  /* maybe this would be better moved to identify device */
  if ( drv->cylinders > 65535 ) drv->cylinders = 65535;

  chn->phase = LIBSPECTRUM_IDE_PHASE_READY;

  drv->error = LIBSPECTRUM_IDE_ERROR_OK;
  drv->status &= ~( LIBSPECTRUM_IDE_STATUS_ERR | LIBSPECTRUM_IDE_STATUS_BSY |
                    LIBSPECTRUM_IDE_STATUS_DRQ );
  drv->status |= LIBSPECTRUM_IDE_STATUS_DRDY;
}

/* Execute a command */
static void
execute_command( libspectrum_ide_channel *chn, libspectrum_byte data )
{
  libspectrum_ide_drive *drv = &chn->drive[ chn->selected ];

  if( !drv->disk ) return;
  chn->phase = LIBSPECTRUM_IDE_PHASE_READY;

  /* Clear error conditions */
  drv->error = LIBSPECTRUM_IDE_ERROR_OK;
  drv->status &= ~(LIBSPECTRUM_IDE_STATUS_ERR | LIBSPECTRUM_IDE_STATUS_BSY);
  drv->status |= LIBSPECTRUM_IDE_STATUS_DRDY;

  /* Perform command */
  switch( data ) {

  case LIBSPECTRUM_IDE_COMMAND_READ_SECTOR_RETRY:    readsector( chn );     break;
  case LIBSPECTRUM_IDE_COMMAND_READ_SECTOR_NORETRY:  readsector( chn );     break;
  case LIBSPECTRUM_IDE_COMMAND_WRITE_SECTOR_RETRY:   writesector( chn );    break;
  case LIBSPECTRUM_IDE_COMMAND_WRITE_SECTOR_NORETRY: writesector( chn );    break;
  case LIBSPECTRUM_IDE_COMMAND_IDENTIFY_DRIVE_ATA:   identifydevice( chn ); break;
  case LIBSPECTRUM_IDE_COMMAND_IDENTIFY_DRIVE_ATAPI: identifydevice( chn ); break;
  case LIBSPECTRUM_IDE_COMMAND_INITIALIZE_DEVICE_PARAMETERS:
    init_device_params( chn ); break;
      
    /* Unknown/unsupported commands */
  default:
    drv->status |= LIBSPECTRUM_IDE_STATUS_ERR;
    drv->error = LIBSPECTRUM_IDE_ERROR_ABRT;
  }
}


/* Write the IDE interface */
void
libspectrum_ide_write( libspectrum_ide_channel *chn,
		       libspectrum_ide_register reg, libspectrum_byte data )
{
  switch( reg ) {
  case LIBSPECTRUM_IDE_REGISTER_DATA:          write_data( chn, data ); break;
  case LIBSPECTRUM_IDE_REGISTER_DATA2:         chn->data2 = data; break;
  case LIBSPECTRUM_IDE_REGISTER_ERROR_FEATURE: chn->feature = data; break;
  case LIBSPECTRUM_IDE_REGISTER_SECTOR_COUNT:  chn->sector_count = data; break;
  case LIBSPECTRUM_IDE_REGISTER_SECTOR:        chn->sector = data; break;
  case LIBSPECTRUM_IDE_REGISTER_CYLINDER_LOW:  chn->cylinder_low = data; break;
  case LIBSPECTRUM_IDE_REGISTER_CYLINDER_HIGH:
    chn->cylinder_high = data; break;
      
  case LIBSPECTRUM_IDE_REGISTER_HEAD_DRIVE:
    chn->head = data;
    chn->selected = chn->head & LIBSPECTRUM_IDE_HEAD_DEV ?
                    LIBSPECTRUM_IDE_SLAVE : LIBSPECTRUM_IDE_MASTER;
    break;

  case LIBSPECTRUM_IDE_REGISTER_COMMAND_STATUS:
    execute_command( chn, data ); break;
  }
}
