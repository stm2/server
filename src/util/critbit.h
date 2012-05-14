/*
Copyright (c) 2012, Enno Rehling <enno@eressea.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifndef _CRITBITT_H
#define _CRITBITT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct critbit_tree {
  void * root;
} critbit_tree;

#define CB_ENOMORE 0
#define CB_SUCCESS 1

#define CRITBIT_TREE() { 0 }

int cb_insert(critbit_tree * cb, const void * key, size_t keylen);
int cb_find(critbit_tree * cb, const void * key, size_t keylen);
int cb_erase(critbit_tree * cb, const void * key, size_t keylen);
int cb_find_prefix(critbit_tree * cb, const void * key, size_t keylen, const char ** results, int numresults, int offset);
void cb_clear(critbit_tree * cb);

#define cb_insert_str(cb, key) \
  cb_insert(cb, (void *)key, strlen(key)+1)
#define cb_find_str(cb, key) \
  cb_find(cb, (void *)key, strlen(key)+1)
#define cb_erase_str(cb, key) \
  cb_erase(cb, (void *)key, strlen(key)+1)
#define cb_find_prefix_str(cb, key, results, numresults, offset) \
  cb_find_prefix(cb, (void *)key, strlen(key), results, numresults, offset)



#ifdef __cplusplus
}
#endif
#endif
