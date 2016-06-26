#ifndef PULLEYSCRIPT_RESIST_H
#define PULLEYSCRIPT_RESIST_H

#include <stdio.h>

#include "types.h"

struct path *path_new (unsigned int legs);
void path_destroy (struct path *path);

void path_print (struct path *plr, FILE *stream);
#ifdef DEBUG
#  define path_debug(plr,stream) path_print(plr,stream)
#else
#  define path_debug(plr,stream)
#endif

struct path *path_have_push (struct gentab *tab, gennum_t initiator);
struct path *path_have_pull (struct gentab *tab, gennum_t initiator);

bool path_run_push (struct gentab *tab, gennum_t initiator, bool add_notdel);
bool path_run_pull (struct gentab *tab, gennum_t initiator, bool add_notdel);

#endif /* RESIST_H */
