/* osname.c: Get a representation of the OS we're running on
   Copyright (c) 1999-2007 Philip Kendall

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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/expansion.h>

#include "ui/ui.h"

struct Library *ExpansionBase;
struct ExpansionIFace *IExpansion;

int compat_osname( char *buffer, size_t length )
{
  STRPTR machine;

  if( ExpansionBase = IExec->OpenLibrary( "expansion.library", 52 ) ) {
    if( IExpansion = ( struct ExpansionIFace * ) IExec->GetInterface( ExpansionBase,"main",1,NULL ) ) {

      IExpansion->GetMachineInfoTags( GMIT_MachineString, &machine,
                                      TAG_DONE );

      snprintf( buffer, length, "%s %s %s", "AmigaOS", machine, "4.0" );
      
      IExec->DropInterface( ( struct Interface * )IExpansion );
    }
    IExec->CloseLibrary( ExpansionBase );
  }
  else
  {
    return 1;
  }

  return 0;
}
