/* pokemem.h: help with handling pokes
   Copyright (c) 2011 Philip Kendall, Sergio Baldoví

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

#ifndef POKEMEM_H
#define POKEMEM_H

#include <config.h>

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#endif				/* #ifdef HAVE_LIB_GLIB */

#include <libspectrum.h>

extern GSList *trainer_list;

typedef struct trainer_t {
  char *name;
  int disabled;               /* malformed trainer or pokes? */
  int ask_value;              /* request user for a value */
  libspectrum_byte value;     /* user's custom value */
  int active;                 /* pokes applied? */
  GSList *poke_list;
} trainer_t;

typedef struct poke_t {
  libspectrum_byte bank;      /* 8 means ignore bank */
  libspectrum_word address;   /* address to poke */
  libspectrum_word value;     /* 256 means request to user */
  libspectrum_byte restore;   /* original value before poke */
} poke_t;

void pokemem_clear( void );
void pokemem_end( void );
int pokemem_set_pokfile( const char *filename );
int pokemem_find_pokfile( const char *filename );
int pokemem_autoload_pokfile( void );
int pokemem_read_from_file( const char *filename );

trainer_t *pokemem_trainer_list_add( libspectrum_byte bank,
                                     libspectrum_word address,
                                     libspectrum_word value );

int pokemem_trainer_activate( trainer_t *trainer );
int pokemem_trainer_deactivate( trainer_t *trainer );

#endif
