/* module.c: API for Fuse modules
   Copyright (c) 2007 Philip Kendall

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

#ifdef HAVE_LIB_GLIB
#include <glib.h>
#endif				/* #ifdef HAVE_LIB_GLIB */

#include <libspectrum.h>

#include "compat.h"
#include "module.h"

static GSList *registered_modules = NULL;

int
module_register( module_info_t *module )
{
  registered_modules = g_slist_append( registered_modules, module );

  return 0;
}

void
module_end( void )
{
  g_slist_free( registered_modules );
  registered_modules = NULL;
}

static void
reset( gpointer data, gpointer user_data )
{
  const module_info_t *module = data;
  int hard_reset = GPOINTER_TO_INT( user_data );

  if( module->reset ) module->reset( hard_reset );
}

void
module_reset( int hard_reset )
{
  g_slist_foreach( registered_modules, reset, GINT_TO_POINTER( hard_reset ) );
}

static void
romcs( gpointer data, gpointer user_data GCC_UNUSED )
{
  const module_info_t *module = data;

  if( module->romcs ) module->romcs();
}

void
module_romcs( void )
{
  g_slist_foreach( registered_modules, romcs, NULL );
}

static void
snapshot_enabled( gpointer data, gpointer user_data )
{
  const module_info_t *module = data;
  libspectrum_snap *snap = user_data;

  if( module->snapshot_enabled ) module->snapshot_enabled( snap );
}

void
module_snapshot_enabled( libspectrum_snap *snap )
{
  g_slist_foreach( registered_modules, snapshot_enabled, snap );
}

static void
snapshot_from( gpointer data, gpointer user_data )
{
  const module_info_t *module = data;
  libspectrum_snap *snap = user_data;

  if( module->snapshot_from ) module->snapshot_from( snap );
}

void
module_snapshot_from( libspectrum_snap *snap )
{
  g_slist_foreach( registered_modules, snapshot_from, snap );
}

static void
snapshot_to( gpointer data, gpointer user_data )
{
  const module_info_t *module = data;
  libspectrum_snap *snap = user_data;

  if( module->snapshot_to ) module->snapshot_to( snap );
}

void
module_snapshot_to( libspectrum_snap *snap )
{
  g_slist_foreach( registered_modules, snapshot_to, snap );
}
