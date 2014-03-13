#include "bind_eressea.h"

#include <platform.h>

#include "export.h"

#include <kernel/types.h>
#include <kernel/config.h>
#include <kernel/save.h>

#include <stream.h>
#include <filestream.h>


void eressea_free_game(void) {
  free_gamedata();
}

int eressea_read_game(const char * filename) {
  return readgame(filename, false);
} 

int eressea_write_game(const char * filename) {
  remove_empty_factions();
  return writegame(filename);
}

int eressea_read_orders(const char * filename) {
  return readorders(filename);
}

int eressea_export_json(const char * filename, unsigned int flags) {
    FILE *F = fopen(filename, "wt");
    if (F) {
        stream out = { 0 };
        int err;
        fstream_init(&out, F);
        err = export_json(&out, flags);
        fstream_done(&out);
        return err;
    }
    perror(filename);
    return -1;
}