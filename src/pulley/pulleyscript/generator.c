/* generator.c -- Pulley script administration for generators.
 *
 * Generators are the source of variable forking; the receive LDAP updates
 * and match those to form sets of variable values that can be deliverd as
 * added values/forks or as removals of previously created values/forks.
 *
 * Generators might have their own LDAP listening SyncRepl thread, but they
 * may also be addressed by a shared thread that informs them all in turn.
 *
 * When a generator processes an update, it must pass through the "path of
 * least resistence" to generate the driver output that might come about in
 * combination with other generators -- and conditions of course.  So, when
 * new variable values are added (or old ones removed) then a list of actions
 * takes place to produce all combinations of variables that need to be
 * driven out.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "generator.h"
#include "resist.h"
#include "parser.h"

#include "generator_int.h"


static void *gentab_index (void *data, unsigned int index);


struct gentab *gentab_new (void) {
	struct gentab *retval = malloc (sizeof (struct gentab));
	if (retval == NULL) {
		fatal_error ("Failed to allocate generator table");
	}
	retval->gentype.data = (void *) retval;
	retval->gentype.index = gentab_index;
	retval->gentype.name = "Generator";
	retval->gens = NULL;
	retval->allocated_gens = 0;
	retval->count_gens = 0;
	retval->vartype = NULL;
	retval->drvtype = NULL;
	return retval;
}

struct gentab *gentab_from_type (type_t *gentype) {
	return (struct gentab *) gentype->data;
}

void gen_cleanup (struct generator *gen);

void gentab_destroy (struct gentab *tab) {
	int i;
	for (i=0; i<tab->allocated_gens; i++) {
		gen_cleanup (&tab->gens [i]);
	}
	free (tab);
}

type_t *gentab_share_type (struct gentab *tab) {
	return &tab->gentype;
}

unsigned int gentab_count (struct gentab *tab) {
	return tab->count_gens;
}

void gentab_set_variable_type (struct gentab *tab, type_t *vartp) {
	tab->vartype = vartp;
}

void gentab_set_driverout_type (struct gentab *tab, type_t *drvtp) {
	tab->drvtype = drvtp;
}

type_t *gentab_share_variable_type (struct gentab *tab) {
	return tab->vartype;
}

type_t *gentab_share_driverout_type (struct gentab *tab) {
	return tab->drvtype;
}

static void *gentab_index (void *data, unsigned int index) {
	return (void *) (&(((struct gentab *) data)->gens [index]));
}

/* Fully initialize (to useless values) a generator structure, to avoid
 * uninitialized-pointers problems.
 */
static _gen_init (struct generator *gen)
{
	gen->weight = 0.0;
	gen->source = -1;
	gen->variables = NULL;
	gen->driverout = NULL;
	gen->path_of_least_resistence = NULL;
	gen->linehash = 0;
	gen->cogenerate = 0;
}

gennum_t gen_new (struct gentab *tab, varnum_t source) {
	struct generator *newgen;
	if (tab->count_gens >= tab->allocated_gens) {
		int alloc = tab->allocated_gens + 100;
		struct generator *newgens;
		newgens = realloc (tab->gens, alloc * sizeof (struct generator));
		if (newgens == NULL) {
			fatal_error ("Out of memory allocating generator");
		}
		for (int i = tab->allocated_gens; i < alloc; i++) {
			_gen_init(&newgens[i]);
		}
		tab->gens = newgens;
		tab->allocated_gens = alloc;
	}
	newgen = &tab->gens [tab->count_gens];	// Incremented on return
	newgen->weight = 100.0;	/* Default weight estimate */
	newgen->source = source;
	newgen->variables = bitset_new (tab->vartype);
	newgen->driverout = bitset_new (tab->drvtype);
	newgen->path_of_least_resistence = NULL;
	newgen->linehash = 0;
	newgen->cogenerate = 0;
	return tab->count_gens++;
}

void gen_cleanup (struct generator *gen) {
	bitset_destroy (gen->variables);
	bitset_destroy (gen->driverout);
	if (gen->path_of_least_resistence) {
		path_destroy (gen->path_of_least_resistence);
	}
	gen->variables = NULL;
	gen->driverout = NULL;
	gen->path_of_least_resistence = NULL;
}

void gen_set_hash (struct gentab *tab, gennum_t gennum, hash_t genhash) {
	tab->gens [gennum].linehash = genhash;
}

hash_t gen_get_hash (struct gentab *tab, gennum_t gennum) {
	return tab->gens [gennum].linehash;
}

void gen_print (struct gentab *tab, gennum_t gennum, FILE *stream, int indent) {
	fprintf (stream, "G%d, lexhash=", gennum);
	hash_print (&tab->gens [gennum].linehash, stream);
	// fprintf (stream, "<- %s", var_get_name (
	// 			tab->gens->vartype, tab->gens [gennum].source));
	fprintf (stream, "\nvariables=");
	bitset_print (tab->gens [gennum].variables, "V", stream);
	fprintf (stream, "\ndriverout=");
	bitset_print (tab->gens [gennum].driverout, "D", stream);
	if (tab->gens [gennum].path_of_least_resistence != NULL) {
		fprintf (stream, "\npath_of_least_resistence=");
		path_print (tab->gens [gennum].path_of_least_resistence, stream);
	} else {
		fprintf (stream, "\npath_of_least_resistence == NULL");
	}
	fprintf (stream, "\n");
}

void gentab_print (struct gentab *tab, char *head, FILE *stream, int indent) {
	gennum_t i;
	fprintf (stream, "Generator listing %s\n", head);
	for (i=0; i<tab->count_gens; i++) {
		gen_print (tab, i, stream, indent);
	}
}

varnum_t gen_get_source (struct gentab *tab, gennum_t gennum) {
	return tab->gens [gennum].source;
}

void gen_set_cogeneration (struct gentab *tab, gennum_t gennum) {
	tab->gens [gennum].cogenerate = 1;
}

bool gen_get_cogeneration (struct gentab *tab, gennum_t gennum) {
	return tab->gens [gennum].cogenerate;
}

void gen_set_weight (struct gentab *tab, gennum_t gennum, float weight) {
	if (weight < 1.0) {
		fprintf (stderr, "WARNING: Obscure generator weight %f is under 1.0\n", weight);
	}
	tab->gens [gennum].weight = weight;
}

void gen_add_guardvar (struct gentab *tab, gennum_t gennum, varnum_t varnum) {
	fatal_error ("You should not set guardvars in a generator; its semantics are undefined");
}

float gen_get_weight (struct gentab *tab, gennum_t gennum) {
	return tab->gens [gennum].weight;
}

int gen_cmp_weight (void *gentab, const void *left, const void *right) {
	float wl = gen_get_weight ((struct gentab *) gentab, * (gennum_t *) left );
	float wr = gen_get_weight ((struct gentab *) gentab, * (gennum_t *) right);
	if (wl < wr) {
		return -1;
	} else if (wl > wr) {
		return 1;
	} else {
		return 0;
	}
}

void gen_add_variable  (struct gentab *tab, gennum_t gennum, varnum_t varnum) {
	bitset_set (tab->gens [gennum].variables, varnum);
}

void gen_add_driverout (struct gentab *tab, gennum_t gennum, drvnum_t drvnum) {
	bitset_set (tab->gens [gennum].driverout, drvnum);
}

bool gen_ask_variable  (struct gentab *tab, gennum_t gennum, varnum_t varnum) {
	return bitset_test (tab->gens [gennum].variables, varnum);
}

bool gen_ask_driverout (struct gentab *tab, gennum_t gennum, drvnum_t drvnum) {
	return bitset_test (tab->gens [gennum].driverout, drvnum);
}

bitset_t *gen_share_variables (struct gentab *tab, gennum_t gennum) {
	return tab->gens [gennum].variables;
}

bitset_t *gen_share_driverout (struct gentab *tab, gennum_t gennum) {
	return tab->gens [gennum].driverout;
}

void gen_add_path_of_least_resistence (struct gentab *tab, gennum_t gennum, path_t *path) {
	struct generator *gen = &tab->gens [gennum];
	if (gen->path_of_least_resistence == NULL) {
		gen->path_of_least_resistence = path;
	} else {
		fatal_error ("Cannot replace path_of_least_resistence");
	}
}

path_t *gen_share_path_of_least_resistence (struct gentab *tab, gennum_t gennum) {
	return tab->gens [gennum].path_of_least_resistence;
}
