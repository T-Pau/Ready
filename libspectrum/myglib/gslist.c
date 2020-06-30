/* gslist.c: Minimal replacement for GSList
   Copyright (c) 2001-2004 Matan Ziv-Av, Philip Kendall, Marek Januszewski

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

#ifndef HAVE_LIB_GLIB		/* Use this iff we're not using the
				   `proper' glib */

#include <stdlib.h>

#include "internals.h"

static
gint	first_function		(gconstpointer	 a,
				 gconstpointer	 b);

static
gint	last_function		(gconstpointer	 a,
				 gconstpointer	 b);

static int FREE_LIST_ALLOCATE_CHUNK = 1024;

GSList * free_list = NULL;
GSList * allocated_list = NULL;

#ifdef HAVE_STDATOMIC_H

static atomic_char atomic_locker = ATOMIC_VAR_INIT(0);

#define lock() atomic_lock( &atomic_locker )
#define unlock() atomic_unlock( &atomic_locker )

#else				/* #ifdef HAVE_STDATOMIC_H */

#define lock()
#define unlock()

#endif				/* #ifdef HAVE_STDATOMIC_H */

static
void    allocate_free   ( void ) {
    lock();
    if(!free_list) {
        int i;
        free_list=libspectrum_malloc(FREE_LIST_ALLOCATE_CHUNK*sizeof(GSList));
        allocated_list = free_list;
        for(i=0;i<FREE_LIST_ALLOCATE_CHUNK-1;i++)
            free_list[i].next=&free_list[i+1];
        free_list[FREE_LIST_ALLOCATE_CHUNK-1].next=NULL;
    }
    unlock();
}


GSList* g_slist_insert	(GSList		*list,
			 gpointer	 data,
			 gint		 position) {
    
  GSList *prev_list;
  GSList *tmp_list;
  GSList *new_list;

  if (position < 0)
    return g_slist_append (list, data);
  else if (position == 0)
    return g_slist_prepend (list, data);

  allocate_free();

  lock();
  new_list = free_list;
  free_list=free_list->next;
  unlock();
  new_list->data = data;
  new_list->next=NULL;

  if (!list)
    {
      return new_list;
    }

  prev_list = NULL;
  tmp_list = list;

  while ((position-- > 0) && tmp_list)
    {
      prev_list = tmp_list;
      tmp_list = tmp_list->next;
    }

  if (prev_list)
    {
      new_list->next = prev_list->next;
      prev_list->next = new_list;
    }
  else
    {
      new_list->next = list;
      list = new_list;
    }

  return list;
}

GSList* g_slist_insert_sorted	(GSList		*list,
				 gpointer	 data,
				 GCompareFunc	 func) {
    
  GSList *tmp_list = list;
  GSList *prev_list = NULL;
  GSList *new_list;
  gint cmp;

  allocate_free();

  if(!func) return list;

  if (!list)
    {
      lock();
      new_list = free_list;
      free_list=free_list->next;
      unlock();
      new_list->data = data;
      new_list->next=NULL;
      return new_list;
    }

  cmp = (*func) (data, tmp_list->data);

  while ((tmp_list->next) && (cmp > 0))
    {
      prev_list = tmp_list;
      tmp_list = tmp_list->next;
      cmp = (*func) (data, tmp_list->data);
    }

  lock();
  new_list = free_list;
  free_list=free_list->next;
  unlock();
  new_list->data = data;

  if ((!tmp_list->next) && (cmp > 0))
    {
      tmp_list->next = new_list;
      new_list->next=NULL;
      return list;
    }

  if (prev_list)
    {
      prev_list->next = new_list;
      new_list->next = tmp_list;
      return list;
    }
  else
    {
      new_list->next = list;
      return new_list;
    }
}

GSList* g_slist_append		(GSList		*list,
				 gpointer	 data) {

  return g_slist_insert_sorted(list, data, last_function);
}

GSList* g_slist_prepend		(GSList		*list,
				 gpointer	 data) {

  return g_slist_insert_sorted(list, data, first_function);
}

static
gint	first_function		(gconstpointer	 a,
				 gconstpointer	 b) {

  return -1;
}

static
gint	last_function		(gconstpointer	 a,
				 gconstpointer	 b) {

  return 1;
}

GSList* g_slist_remove		(GSList		*list,
				 gconstpointer	 data) {

  GSList *tmp;
  GSList *prev;

  prev = NULL;
  tmp = list;

  while (tmp)
    {
      if (tmp->data == data)
        {
          if (prev)
            prev->next = tmp->next;
          if (list == tmp)
            list = list->next;

          tmp->next = NULL;
          g_slist_free (tmp);

          break;
        }

      prev = tmp;
      tmp = tmp->next;
    }

  return list;
}

GSList* g_slist_delete_link	(GSList		*list,
				 GSList		*link) {

  GSList *tmp;
  GSList *prev;

  prev = NULL;
  tmp = list;

  while (tmp)
    {
      if (tmp == link)
        {
          if (prev)
            prev->next = tmp->next;
          if (list == tmp)
            list = list->next;

          tmp->next = NULL;
          g_slist_free (tmp);
          break;
        }

      prev = tmp;
      tmp = tmp->next;
    }

  return list;
}

GSList*
g_slist_last (GSList *list)
{
  if (list)
    {
      while (list->next)
      list = list->next;
    }

  return list;
}

guint
g_slist_length (GSList *list)
{
  guint length;

  length = 0;
  while (list)
    {
      length++;
      list = list->next;
    }

  return length;
}

void	g_slist_foreach		(GSList		*list,
				 GFunc		 func,
				 gpointer	 user_data) {

  while (list)
    {
      (*func) (list->data, user_data);
      list = list->next;
    }
}

void	g_slist_free		(GSList		*list) {
  if (list)
    {
      GSList *last_node = list;

      while( last_node->next )
	last_node = last_node->next;

      lock();
      last_node->next = free_list;
      free_list = list;
      unlock();
    }
}

GSList*
g_slist_reverse (GSList *list)
{
  GSList *prev = NULL;

  while (list)
    {
      GSList *next = list->next;

      list->next = prev;

      prev = list;
      list = next;
    }

  return prev;
}

GSList* g_slist_nth		(GSList		*list,
				 guint		n) {
  for( ; n; n-- ) {
    if( list == NULL ) return NULL;
    list = list->next;
  }

  return list;
}

GSList* g_slist_find_custom	(GSList		*list,
				 gconstpointer	data,
				 GCompareFunc	func ) {
  while (list)
    {
      if (!(*func) (list->data, data)) return list;
      list = list->next;
    }

  return NULL;
}

gint	g_slist_position	(GSList		*list,
				 GSList		*llink) {
  int n;

  n = 0;

  while( list ) {
    if( list == llink ) return n;
    list = list->next;
    n++;
  }

  return -1;
}

void
libspectrum_slist_cleanup( void )
{
  lock();
  libspectrum_free( allocated_list );
  allocated_list = NULL;
  free_list = NULL;
  unlock();
}

#endif				/* #ifndef HAVE_LIB_GLIB */
