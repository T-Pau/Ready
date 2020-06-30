/* startup_manager.c: handle Fuse's startup routines
   Copyright (c) 2016 Philip Kendall

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

#include "startup_manager.h"
#include "ui/ui.h"

typedef struct registered_module_t {
  startup_manager_module module;
  GArray *dependencies;
  startup_manager_init_fn init_fn;
  void *init_context;
  startup_manager_end_fn end_fn;
} registered_module_t;

static GArray *registered_modules;

static GArray *end_functions;

void
startup_manager_init( void )
{
  registered_modules =
    g_array_new( FALSE, FALSE, sizeof( registered_module_t ) );
  end_functions =
    g_array_new( FALSE, FALSE, sizeof( startup_manager_end_fn ) );
}

void
startup_manager_end( void )
{
  g_array_free( registered_modules, TRUE );
  registered_modules = NULL;

  g_array_free( end_functions, TRUE );
  end_functions = NULL;
}

void
startup_manager_register(
  startup_manager_module module, startup_manager_module *dependencies,
  size_t dependency_count, startup_manager_init_fn init_fn,
  void *init_context, startup_manager_end_fn end_fn )
{
  registered_module_t registered_module;

  registered_module.module = module;
  registered_module.dependencies =
    g_array_sized_new( FALSE, FALSE, sizeof( startup_manager_module ),
                       dependency_count );
  g_array_append_vals( registered_module.dependencies, dependencies,
                       dependency_count );
  registered_module.init_fn = init_fn;
  registered_module.init_context = init_context;
  registered_module.end_fn = end_fn;

  g_array_append_val( registered_modules, registered_module );
}

void
startup_manager_register_no_dependencies(
  startup_manager_module module, startup_manager_init_fn init_fn,
  void *init_context, startup_manager_end_fn end_fn )
{
  startup_manager_register( module, NULL, 0, init_fn, init_context, end_fn );
}

static void
remove_dependency( startup_manager_module module )
{
  guint i, j;

  for( i = 0; i < registered_modules->len; i++ ) {
    registered_module_t *registered_module =
      &g_array_index( registered_modules, registered_module_t, i );
    GArray *dependencies = registered_module->dependencies;

    for( j = 0; j < dependencies->len; j++ ) {
      startup_manager_module dependency =
        g_array_index( dependencies, startup_manager_module, j );

      if( dependency == module ) {
        g_array_remove_index_fast( dependencies, j );
        break;
      }
    }
  }
}

int
startup_manager_run( void )
{
  int progress_made;
  guint i;
  int error;

  /* Loop until we can't make any more progress; this will either be because
     we've called every function (good!) or because there's a logical error
     in the dependency graph (bad!) */
  do {
    i = 0;
    progress_made = 0;

    while( i < registered_modules->len ) {
      registered_module_t *registered_module =
        &g_array_index( registered_modules, registered_module_t, i );

      if( registered_module->dependencies->len == 0 ) {

        if( registered_module->init_fn ) {
          error = registered_module->init_fn(
            registered_module->init_context
          );
          if( error ) return error;
        }

        if( registered_module->end_fn )
          g_array_append_val( end_functions, registered_module->end_fn );

        remove_dependency( registered_module->module );

        g_array_free( registered_module->dependencies, TRUE );
        g_array_remove_index_fast( registered_modules, i );

        progress_made = 1;
      } else {
        i++;
      }
    }
  } while( progress_made && registered_modules->len );

  /* If there are still any modules left to be called, then that's bad */
  if( registered_modules->len ) {
    ui_error( UI_ERROR_ERROR, "%u startup modules could not be called",
              registered_modules->len );
    return 1;
  }

  return 0;
}

void
startup_manager_run_end( void )
{
  guint i;

  for( i = end_functions->len; i-- != 0; )
  {
    startup_manager_end_fn end_fn = 
      g_array_index( end_functions, startup_manager_end_fn, i );

    end_fn();
  }

  startup_manager_end();
}
