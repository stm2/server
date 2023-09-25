#ifndef H_RACES
#define H_RACES

#ifdef __cplusplus
extern "C" {
#endif
    struct unit;

    void register_races(void);
    void equip_newunits(struct unit *u);
    void show_items(struct unit *u, int flags, const char *name);

#ifdef __cplusplus
}
#endif
#endif
