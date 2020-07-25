/* dirname - return directory part of PATH.
   Copyright (C) 1996, 2000, 2001, 2002 Free Software Foundation, Inc.
   This file was part of the GNU C Library, version 2.3.2.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.
   Very slightly modified for use in Fuse by Philip Kendall, 2004.

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

#include "fuse.h"

static char *
__memrchr (char *s, int c, size_t n)
{
  for (s += n - 1; n; n--, s--)
    if (*s == c)
      return s;

  return NULL;
}

char *
dirname (char *path)
{
  static const char dot[] = ".";
  char *last_slash;

  /* Find last '/'.  */
  last_slash = path != NULL ? strrchr (path, FUSE_DIR_SEP_CHR) : NULL;

  if (last_slash != NULL && last_slash != path && last_slash[1] == '\0')
    {
      /* Determine whether all remaining characters are slashes.  */
      char *runp;

      for (runp = last_slash; runp != path; --runp)
	if (runp[-1] != FUSE_DIR_SEP_CHR)
	  break;

      /* The '/' is the last character, we have to look further.  */
      if (runp != path)
	last_slash = __memrchr (path, FUSE_DIR_SEP_CHR, runp - path);
    }

  if (last_slash != NULL)
    {
      /* Determine whether all remaining characters are slashes.  */
      char *runp;

      for (runp = last_slash; runp != path; --runp)
	if (runp[-1] != FUSE_DIR_SEP_CHR)
	  break;

      /* Terminate the path.  */
      if (runp == path)
	{
	  /* The last slash is the first character in the string.  We have to
	     return "/".  As a special case we have to return "//" if there
	     are exactly two slashes at the beginning of the string.  See
	     XBD 4.10 Path Name Resolution for more information.  */
	  if (last_slash == path + 1)
	    ++last_slash;
	  else
	    last_slash = path + 1;
	}
      else
	last_slash = runp;

      last_slash[0] = '\0';
    }
  else
    /* This assignment is ill-designed but the XPG specs require to
       return a string containing "." in any case no directory part is
       found and so a static and constant string is required.  */
    path = (char *) dot;

  return path;
}
