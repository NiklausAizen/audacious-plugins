/* 
 * Audacious Monkey's Audio plugin, an APE tag reading stuff
 *
 * Copyright (C) Eugene Zagidullin 2007
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version. 
 *  
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details. 
 *  
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA
 *
 */

#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 

#include <glib.h>
#include <audacious/vfs.h>
#include <audacious/plugin.h> 

#include "apev2.h"


static int read_uint32(VFSFile *vfd, guint32* x) {
    unsigned char tmp[4];
    int n;

    n = aud_vfs_fread(tmp, 1, 4, vfd);

    if (n != 4)
        return -1;
    
    /* convert to native endianness */
    *x = tmp[0] | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24);

    return 0;
}

#define TMP_BUFSIZE 256
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

static gboolean strcase_equal (gconstpointer v1, gconstpointer v2) {
  return (g_ascii_strcasecmp((gchar *)v1, (gchar *)v2) == 0);
}

static guint strcase_hash (gconstpointer v) {
  gchar *tmp;
  gchar buf[TMP_BUFSIZE+1];
  gchar *p = buf;
  
  for(tmp=(gchar*)v; (*tmp && (p < buf+TMP_BUFSIZE)); tmp++)
    *(p++) = g_ascii_toupper(*tmp);
  *p = '\0';
  return g_str_hash((gconstpointer)buf);
}

GHashTable* parse_apev2_tag(VFSFile *vfd) {
  unsigned char tmp[TMP_BUFSIZE+1];
  unsigned char tmp2[TMP_BUFSIZE+1];
  guint32 tag_version;
  guint32 tag_size, item_size, item_flags;
  guint32 tag_items;
  guint32 tag_flags;
  GHashTable *hash;

  aud_vfs_fseek(vfd, -32, SEEK_END);
  aud_vfs_fread(tmp, 1, 8, vfd);
  if ((tmp[0]!='A')||(tmp[1]!='P')||(tmp[2]!='E')||(tmp[3]!='T')||
     (tmp[4]!='A')||(tmp[5]!='G')||(tmp[6]!='E')||(tmp[7]!='X')) {
#ifdef DEBUG
    fprintf(stderr, "** demac: apev2.c: APE tag not found\n");
#endif
    return NULL;
  }
  
  read_uint32(vfd, &tag_version);
  read_uint32(vfd, &tag_size);
  read_uint32(vfd, &tag_items);
  read_uint32(vfd, &tag_flags);
#ifdef DEBUG
  fprintf(stderr, "** demac: apev2.c: found APE tag version %d contains %d items, flags %08x\n", tag_version, tag_items, tag_flags);
#endif
  if(tag_items == 0) {
#ifdef DEBUG
    fprintf(stderr, "** demac: apev2.c: found empty tag\n");
#endif
    return NULL;
  }
  
  hash = g_hash_table_new_full(strcase_hash, strcase_equal, g_free, g_free); /* string-keyed table with dynamically allocated keys and items */

  aud_vfs_fseek(vfd, -tag_size, SEEK_END);
  int i;
  unsigned char *p;
  for(i=0; i<tag_items; i++) {
      read_uint32(vfd, &item_size);
      read_uint32(vfd, &item_flags);
      
      /* read key */
      for(p = tmp; p <= tmp+TMP_BUFSIZE; p++) {
        aud_vfs_fread(p, 1, 1, vfd);
	if(*p == '\0') break;
      }
      *(p+1) = '\0';

      /* read item */
      aud_vfs_fread(tmp2, 1, MIN(item_size, TMP_BUFSIZE), vfd);
      tmp2[item_size] = '\0';
#ifdef DEBUG
      fprintf(stderr, "%s: \"%s\", f:%08x\n", tmp, tmp2, item_flags);
#endif
      /* APEv2 stores all items in utf-8 */
      gchar *item = ((tag_version == 1000 ) ? aud_str_to_utf8((gchar*)tmp2) : g_strdup((gchar*)tmp2));
      
      g_hash_table_insert (hash, g_strdup((gchar*)tmp), item);
  }
  
  return hash;
}