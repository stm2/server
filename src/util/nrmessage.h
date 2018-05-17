/*
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */

#ifndef H_UTIL_NRMESSAGE
#define H_UTIL_NRMESSAGE

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct locale;
    struct message;
    struct message_type;
    struct nrmessage_type;

    typedef struct nrsection {
        char *name;
        struct nrsection *next;
    } nrsection;

    extern nrsection *sections;

    void free_nrmesssages(void);

    void nrt_register(const struct message_type *mtype,
        const struct locale *lang, const char *script,
        int level, const char *section);
    struct nrmessage_type *nrt_find(const struct locale *,
        const struct message_type *);
    const char *nrt_string(const struct nrmessage_type *type);
    const char *nrt_section(const struct nrmessage_type *mt);

    size_t nr_render(const struct message *msg, const struct locale *lang,
        char *buffer, size_t size, const void *userdata);
    int nr_level(const struct message *msg);
    const char *nr_section(const struct message *msg);

    void export_messages(const struct locale * lang, FILE *F, const char *context);

#ifdef __cplusplus
}
#endif
#endif
