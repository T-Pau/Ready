/* ghash.c: Minimal replacement for GHashTable

   This code taken almost verbatim from glib 2.4.0's glib/ghash.c
   Copyright (C) 1995-1998  Peter Mattis, Spencer Kimball and Josh MacDonald

   Modified by the GLib Team and others 1997-2000.  See the AUTHORS
   file for a list of people on the GLib Team.  See the ChangeLog
   files for a list of changes.  These files are distributed with GLib
   at ftp://ftp.gtk.org/pub/gtk/.

   Modified by Philip Kendall 2004-2011.

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

   Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#include <config.h>

#ifndef HAVE_LIB_GLIB		/* Use this iff we're not using the
				   `proper' glib */
#include <string.h>

#include "internals.h"

#define HASH_TABLE_SIZE 241

typedef struct _GHashNode      GHashNode;

struct _GHashNode
{
  gpointer   key;
  gpointer   value;
  GHashNode *next;
};

struct _GHashTable
{
  gint          nnodes;
  GHashNode   **nodes;
  GHashFunc	hash_func;
  GCompareFunc	key_equal_func;
  GDestroyNotify	key_destroy_func;
  GDestroyNotify	value_destroy_func;
};

static GHashNode *node_free_list = NULL;
static GHashNode *node_allocated_list = NULL;

#ifdef HAVE_STDATOMIC_H

static atomic_char atomic_locker = ATOMIC_VAR_INIT(0);

#define lock() atomic_lock( &atomic_locker )
#define unlock() atomic_unlock( &atomic_locker )

#else				/* #ifdef HAVE_STDATOMIC_H */

#define lock()
#define unlock()

#endif				/* #ifdef HAVE_STDATOMIC_H */

static guint
g_direct_hash (gconstpointer v)
{
  return GPOINTER_TO_UINT (v);
}

GHashTable*
g_hash_table_new (GHashFunc	hash_func,
		  GCompareFunc	key_equal_func)
{
  return g_hash_table_new_full( hash_func, key_equal_func, NULL, NULL );
}

GHashTable*
g_hash_table_new_full (GHashFunc       hash_func,
		       GCompareFunc    key_equal_func,
		       GDestroyNotify  key_destroy_func,
		       GDestroyNotify  value_destroy_func)
{
  GHashTable *hash_table;
  guint i;

  hash_table = libspectrum_malloc (sizeof (GHashTable));

  hash_table->nnodes = 0;
  hash_table->hash_func = hash_func? hash_func : g_direct_hash;
  hash_table->key_equal_func = key_equal_func;
  hash_table->key_destroy_func   = key_destroy_func;
  hash_table->value_destroy_func = value_destroy_func;
  hash_table->nodes = libspectrum_malloc (HASH_TABLE_SIZE * sizeof (GHashNode*));

  for (i = 0; i < HASH_TABLE_SIZE; i++)
    hash_table->nodes[i] = NULL;

  return hash_table;
}

static void
g_hash_nodes_destroy (GHashNode *hash_node,
                      GFreeFunc  key_destroy_func,
                      GFreeFunc  value_destroy_func)
{
  if (hash_node)
    {
      GHashNode *node = hash_node;

      while (node->next) {
        if (key_destroy_func)
          key_destroy_func (node->key);
        if (value_destroy_func)
          value_destroy_func (node->value);

        node = node->next;
      }

      if (key_destroy_func)
        key_destroy_func (node->key);
      if (value_destroy_func)
        value_destroy_func (node->value);

      lock();
      node->next = node_free_list;
      node_free_list = hash_node;
      unlock();
    }
}

void
g_hash_table_destroy (GHashTable *hash_table)
{
  guint i;
  
  for (i = 0; i < HASH_TABLE_SIZE; i++)
    g_hash_nodes_destroy (hash_table->nodes[i],
                          hash_table->key_destroy_func,
                          hash_table->value_destroy_func);

  libspectrum_free (hash_table->nodes);
  libspectrum_free (hash_table);
}

static GHashNode**
g_hash_table_lookup_node (GHashTable    *hash_table,
                          gconstpointer  key)
{
  GHashNode **node;
  
  node = &hash_table->nodes
    [(* hash_table->hash_func) (key) % HASH_TABLE_SIZE];

  while( *node ) {
    if( hash_table->key_equal_func ) {
        if( hash_table->key_equal_func( (*node)->key, key ) ) break;
    } else if( (*node)->key == key ) {
      break;
    }

    node = &(*node)->next;
  }

  return node;
}

gpointer
g_hash_table_lookup (GHashTable   *hash_table,
		     gconstpointer key)
{
  GHashNode *node;

  node = *g_hash_table_lookup_node (hash_table, key);

  return node ? node->value : NULL;
}

static GHashNode*
g_hash_node_new (gpointer key,
                 gpointer value)
{
  GHashNode *hash_node;
  guint i;

  lock();
  if (!node_free_list)
    {
      node_free_list = libspectrum_malloc (1024 * sizeof (GHashNode));
      node_allocated_list = node_free_list;

      for(i = 0; i < 1023; i++ )
	node_free_list[i].next = &node_free_list[i+1];
      node_free_list[1023].next = NULL;
    }

  
  hash_node = node_free_list;
  node_free_list = node_free_list->next;
  unlock();

  hash_node->key = key;
  hash_node->value = value;
  hash_node->next = NULL;
  
  return hash_node;
}

void
g_hash_table_insert (GHashTable *hash_table,
                     gpointer    key,
                     gpointer    value)
{
  GHashNode **node;
  
  node = g_hash_table_lookup_node (hash_table, key);
  
  if (*node)
    {
      /* free the passed key */
      if (hash_table->key_destroy_func)
        hash_table->key_destroy_func (key);
      
      if (hash_table->value_destroy_func)
        hash_table->value_destroy_func ((*node)->value);

      (*node)->value = value;
    }
  else
    {
      *node = g_hash_node_new (key, value);
      hash_table->nnodes++;
    }
}

static void
g_hash_node_destroy (GHashNode *hash_node,
		     GDestroyNotify  key_destroy_func,
		     GDestroyNotify  value_destroy_func)
{
  if (key_destroy_func)
    key_destroy_func (hash_node->key);
  if (value_destroy_func)
    value_destroy_func (hash_node->value);

  lock();
  hash_node->next = node_free_list;
  node_free_list = hash_node;
  unlock();
}

guint
g_hash_table_foreach_remove (GHashTable *hash_table,
                             GHRFunc     func,
                             gpointer    user_data)
{
  GHashNode *node, *prev;
  guint i;
  guint deleted = 0;
  
  for (i = 0; i < HASH_TABLE_SIZE; i++)
    {
    restart:
      
      prev = NULL;
      
      for (node = hash_table->nodes[i]; node; prev = node, node = node->next)
        {
          if ((* func) (node->key, node->value, user_data))
            {
              deleted += 1;

              hash_table->nnodes -= 1;
              
              if (prev)
                {
                  prev->next = node->next;
                  g_hash_node_destroy (node,
                                       hash_table->key_destroy_func,
                                       hash_table->value_destroy_func);
                  node = prev;
                }
              else
                {
                  hash_table->nodes[i] = node->next;
                  g_hash_node_destroy (node,
                                       hash_table->key_destroy_func,
                                       hash_table->value_destroy_func);
                  goto restart;
                }
            }
        }
    }
  
  return deleted;
}

void
g_hash_table_foreach (GHashTable *hash_table,
                      GHFunc      func,
                      gpointer    user_data)
{
  GHashNode *node;
  gint i;

  for (i = 0; i < HASH_TABLE_SIZE; i++)
    for (node = hash_table->nodes[i]; node; node = node->next)
      (* func) (node->key, node->value, user_data);
}

guint
g_hash_table_size (GHashTable *hash_table)
{
  return hash_table->nnodes;
}

guint
g_int_hash (gconstpointer v)
{
  return *(const gint*) v;
}

gboolean
g_int_equal (gconstpointer v1,
             gconstpointer v2)
{
  return *((const gint*) v1) == *((const gint*) v2);
}

guint
g_str_hash (gconstpointer v)
{
  /* 31 bit hash function */
  const signed char *p = v;
  libspectrum_dword h = *p;

  if (h)
    for (p += 1; *p != '\0'; p++)
      h = (h << 5) - h + *p;

  return h;
}

gboolean
g_str_equal (gconstpointer v1,
             gconstpointer v2)
{
  const gchar *string1 = v1;
  const gchar *string2 = v2;

  return strcmp (string1, string2) == 0;
}

void
libspectrum_hashtable_cleanup( void )
{
  lock();
  libspectrum_free( node_allocated_list );
  node_allocated_list = NULL;
  node_free_list = NULL;
  unlock();
}
#endif				/* #ifndef HAVE_LIB_GLIB */
