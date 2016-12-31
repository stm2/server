/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

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

#ifndef H_GC_MONSTER
#define H_GC_MONSTER
#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    struct region;
    struct faction;
    
    struct unit *spawn_seaserpent(struct region *r, struct faction *f);
    void spawn_dragons(void);
    void monsters_desert(struct faction *monsters);
    
    void monster_kills_peasants(struct unit *u);
    bool monster_is_waiting(const struct unit *u);
    struct faction *get_monsters(void);
    struct faction *get_or_create_monsters(void);
    void make_zombie(struct unit * u);

#define MONSTER_ID 666
#define is_monsters(f) ((f->flags & FFL_NPC) && f==get_monsters())

#ifdef __cplusplus
}
#endif
#endif