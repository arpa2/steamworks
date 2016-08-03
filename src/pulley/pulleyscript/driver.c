/* driver.c -- Functions defined on output drivers.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include "types.h"
#include "bitset.h"
#include "driver.h"
#include "generator.h"
#include "variable.h"
#include "parser.h"
#include "resist.h"

#include "generator_int.h"


struct driverout {
	char *module;
	float weight;
	hash_t linehash;
	varnum_t *outputs;
	varnum_t outputs_count;
	varnum_t outputs_allocated;
	bitset_t *produced_vars;
	bitset_t *relevant_vars;
	bitset_t *explicit_guards;
	bitset_t *implicit_guards;
	bitset_t *all_guards;
	bitset_t *conditions;
	bitset_t *generators;
	path_t *path_of_least_resistence;
	driver_callback *user_callback;
	void *user_cbdata;
};

struct drvtab {
	type_t drvtype;
	type_t *vartype, *gentype, *cndtype;
	struct driverout *drvs;
	unsigned int allocated_drvs;
	unsigned int count_drvs;
};

static void *drvtab_index (void *data, unsigned int index);

struct drvtab *drvtab_new (void) {
	struct drvtab *retval = malloc (sizeof (struct drvtab));
	if (retval == NULL) {
		fatal_error ("Out of memory allocating output driver table");
	}
	retval->drvtype.data = (void *) retval;
	retval->drvtype.index = drvtab_index;
	retval->drvtype.name = "Driver";
	retval->drvs = NULL;
	retval->count_drvs = 0;
	retval->allocated_drvs = 0;
	return retval;
}

static void drv_cleanup (struct drvtab *tab, drvnum_t drvnum);

void drvtab_destroy (struct drvtab *tab) {
	int i;
	for (i=0; i<tab->count_drvs; i++) {
		drv_cleanup (tab, i);
	}
	free (tab);
}

void drv_set_hash (struct drvtab *tab, drvnum_t drvnum, hash_t drvhash) {
	tab->drvs [drvnum].linehash = drvhash;
}

hash_t drv_get_hash (struct drvtab *tab, drvnum_t drvnum) {
	return tab->drvs [drvnum].linehash;
}

type_t *drvtab_share_type (struct drvtab *tab) {
	return &tab->drvtype;
}

drvnum_t drvtab_count (struct drvtab *tab) {
	return tab->count_drvs;
}

void drvtab_print (struct drvtab *tab, char *head, FILE *stream, int indent) {
	drvnum_t i;
	fprintf (stream, "Driver outputs %s\n", head);
	for (i=0; i<tab->count_drvs; i++) {
		drv_print (tab, i, stream, indent);
	}
}

void drv_print (struct drvtab *tab, drvnum_t drv, FILE *stream, int indent) {
	int i;
	char bra;
	char *comma;
	fprintf (stream, "D%d, lexhash=", drv);
	hash_print (&tab->drvs [drv].linehash, stream);
	fprintf (stream, ", name=%s\n  produced_vars=",
		tab->drvs [drv].module?
			tab->drvs [drv].module:
			"(undef)");
	bitset_print (tab->drvs [drv].produced_vars, "V", stream);
	fprintf (stream, "\n  explicit_guards=");
	bitset_print (tab->drvs [drv].explicit_guards, "V", stream);
	fprintf (stream, "\n  implicit_guards=");
	bitset_print (tab->drvs [drv].implicit_guards, "V", stream);
	fprintf (stream, "\n  all_guards=");
	bitset_print (tab->drvs [drv].all_guards, "V", stream);
	fprintf (stream, "\n  relevant_vars=");
	bitset_print (tab->drvs [drv].relevant_vars, "V", stream);
	fprintf (stream, "\n  generators=");
	bitset_print (tab->drvs [drv].generators, "G", stream);
	fprintf (stream, "\n  conditions=");
	bitset_print (tab->drvs [drv].conditions, "C", stream);
	fprintf (stream, "\n  outputs=");
	bra = '[';
	comma = "";
	for (i=0; i<tab->drvs [drv].outputs_count; i++) {
		varnum_t outvar = tab->drvs [drv].outputs [i];
		if (outvar == VARNUM_BAD) {
			if (bra == ']') {
				fprintf (stream, "]");
				comma = ", ";
			} else {
				fprintf (stream, "%s[", comma);
				comma = "";
			}
			bra ^= ('[' ^ ']');
		} else {
			struct vartab *vt = vartab_from_type (tab->vartype);
			char *vn = var_get_name (vt, outvar);
			fprintf (stream, "%s%s", comma, vn);
			comma = ", ";
		}
	}
	fprintf (stream, "\n");
}

static void drv_cleanup (struct drvtab *tab, drvnum_t drvnum) {
	struct driverout *drv = &tab->drvs [drvnum];
	bitset_destroy (drv->produced_vars);
	bitset_destroy (drv->relevant_vars);
	bitset_destroy (drv->explicit_guards);
	bitset_destroy (drv->implicit_guards);
}

static void *drvtab_index (void *data, unsigned int index) {
	return (void *) (&(((struct drvtab *) data)->drvs [index]));
}

void drvtab_set_vartype (struct drvtab *tab, type_t *vartp) {
	tab->vartype = vartp;
}

type_t *drvtab_share_vartype (struct drvtab *tab) {
	return tab->vartype;
}

void drvtab_set_gentype (struct drvtab *tab, type_t *gentp) {
	tab->gentype = gentp;
}

type_t *drvtab_share_gentype (struct drvtab *tab) {
	return tab->gentype;
}

void drvtab_set_cndtype (struct drvtab *tab, type_t *cndtp) {
	tab->cndtype = cndtp;
}

type_t *drvtab_share_cndtype (struct drvtab *tab) {
	return tab->cndtype;
}

struct drvtab *drvtab_from_type (type_t *drvtype) {
	return (struct drvtab *) drvtype->data;
}

drvnum_t drv_new (struct drvtab *tab) {
	struct driverout *newdrv;
	if (tab->count_drvs >= tab->allocated_drvs) {
		int alloc = tab->allocated_drvs + 100;
		struct driverout *newdrvs;
		newdrvs = realloc (tab->drvs, alloc * sizeof (struct driverout));
		if (newdrvs == NULL) {
			fatal_error ("Out of memory allocating variable");
		}
		tab->drvs = newdrvs;
		tab->allocated_drvs = alloc;
	}
	newdrv = &tab->drvs [tab->count_drvs];	/* incremented when returned */
	newdrv->outputs = NULL;
	newdrv->outputs_allocated = 0;
	newdrv->outputs_count = 0;
	newdrv->module = NULL;
	newdrv->weight = 1.0;
	newdrv->produced_vars   = bitset_new (tab->vartype);
	newdrv->explicit_guards = bitset_new (tab->vartype);
	newdrv->relevant_vars   = bitset_new (tab->vartype);
	newdrv->implicit_guards = bitset_new (tab->vartype);
	newdrv->all_guards      = bitset_new (tab->vartype);
	newdrv->conditions      = bitset_new (tab->cndtype);
	newdrv->generators      = bitset_new (tab->gentype);
	newdrv->user_cbdata = NULL;
	newdrv->user_callback = NULL;
	return tab->count_drvs++;
}

void drv_set_module (struct drvtab *tab, drvnum_t drvnum, char *module) {
	if (tab->drvs [drvnum].module != NULL) {
		fatal_error ("Cannot change driver name, once it is set");
	}
	tab->drvs [drvnum].module = strdup (module);
	if (tab->drvs [drvnum].module == NULL) {
		fatal_error ("Out of memory while setting driver name");
	}
}

const char *drv_get_module (struct drvtab *tab, drvnum_t drvnum) {
	return tab->drvs [drvnum].module;
}

void drv_set_weight (struct drvtab *tab, drvnum_t drvnum, float weight) {
	tab->drvs [drvnum].weight = weight;
}

float drv_get_weight (struct drvtab *tab, drvnum_t drvnum) {
	return tab->drvs [drvnum].weight;
}

void drv_add_guardvar (struct drvtab *tab, drvnum_t drvnum, varnum_t varnum) {
	bitset_set (tab->drvs [drvnum].explicit_guards, varnum);
	bitset_set (tab->drvs [drvnum].relevant_vars,   varnum);
}

static void drv_output_extend (struct drvtab *tab, drvnum_t drvnum, varnum_t varnum) {
	struct driverout *drv = &tab->drvs [drvnum];
	if (drv->outputs_allocated <= drv->outputs_count) {
		drv->outputs_allocated += 25;
		drv->outputs = realloc (drv->outputs, sizeof (varnum_t) * drv->outputs_allocated);
		if (drv->outputs == NULL) {
			fatal_error ("Out of memory allocating driver output array");
		}
	}
	drv->outputs [drv->outputs_count++] = varnum;
}

/* Mentions varnum in produced_vars and in relevant_vars */
void drv_output_variable (struct drvtab *tab, drvnum_t drvnum, varnum_t varnum) {
	drv_output_extend (tab, drvnum, varnum);
	bitset_set (tab->drvs [drvnum].produced_vars, varnum);
	bitset_set (tab->drvs [drvnum].relevant_vars, varnum);
}

void drv_output_list_begin (struct drvtab *tab, drvnum_t drvnum) {
	// The beginning and end of a list are marked with VARNUM_BAD
	drv_output_extend (tab, drvnum, VARNUM_BAD);
}

void drv_output_list_end (struct drvtab *tab, drvnum_t drvnum) {
	// The beginning and end of a list are marked with VARNUM_BAD
	drv_output_extend (tab, drvnum, VARNUM_BAD);
}

bitset_t *drv_share_output_variables (struct drvtab *tab, drvnum_t drvnum) {
	return tab->drvs [drvnum].produced_vars;
}

void drv_share_output_variable_table (struct drvtab *tab, drvnum_t drvnum,
			varnum_t **out_array, varnum_t *out_count) {
	*out_array = tab->drvs [drvnum].outputs;
	*out_count = tab->drvs [drvnum].outputs_count;
}

bitset_t *drv_share_conditions (struct drvtab *tab, drvnum_t drvnum) {
	return tab->drvs [drvnum].conditions;
}

/* Collect all variables that impact the driver's output.  These include more
 * than just the produced output variables and explicit guards; any extra
 * ones are stored as implicit guards.  Such implicit guards are used for
 * other variables in the relevant variable's partitions (because these are
 * needed for determining conditions) and variables generated alongside in
 * the generators of these partitions (because these are needed to complete
 * the uniqueness of the lists of variables driven out).
 */

/***** COLLECTION PROCEDURES *********
 *
 * The following order is assumed:
 *  0. vartab_collect_varpartitions()
 *  1. drvtab_collect_varpartitions()
 *  2. drvtab_collect_conditions()
 *  3. drvtab_collect_generators()
 *  4. drvtab_collect_cogenerators()
 *  5. drvtab_collect_genvariables()
 *  6. drvtab_collect_guards()
 */

/* Collect variables that are not presently considered useful for the driver, but
 * that share a varpatition with others that are.  This is a requisite to be able
 * to calculate all conditions that may constrain visibility of variables
 * that are meant to be driven out.  Any extra variables will be incorporated
 * as implicit guards when these are collected.
 */
void drvtab_collect_varpartitions (struct drvtab *tab) {
	drvnum_t i;
	struct vartab *vartab = vartab_from_type (tab->vartype);
	for (i=0; i<tab->count_drvs; i++) {
		struct driverout *drv = &tab->drvs [i];
		var_partition_closure (vartab, drv->relevant_vars);
	}
}

/* Collect conditions for a driver.  This may be valuable during the generation
 * of an engine that takes variables from generator to drivers.
 */
void drvtab_collect_conditions (struct drvtab *tab) {
	drvnum_t i;
	struct driverout *drv = &tab->drvs [0];
	struct vartab *vartab = vartab_from_type (tab->vartype);
	bitset_iter_t vi;
	varnum_t v;
	bitset_t *cnds;
	for (i=0; i<tab->count_drvs; drv++, i++) {
		bitset_iterator_init (&vi, drv->relevant_vars);
		while (bitset_iterator_next_one (&vi, NULL)) {
			v = bitset_iterator_bitnum (&vi);
			cnds = var_share_conditions (vartab, v);
			bitset_union (drv->conditions, cnds);
		}
	}
}

/* Collect all the generators that refer to one or more of the variables
 * contained in produced variables and guards.  Note that this should
 * include all the variables in all condition-driven partitions to be able
 * to calculate all the conditions.  Also note that this should not include
 * variables that happen to be generated at the same time; these must end up
 * in the implicit guards though, so this operation must occur before the
 * implicit guards are collected.
 *
 * Implementation note: We might have iterated over all relevant variables,
 * found their generators and in turn the variables involved in those, but
 * that would lead to a lot of overlapping results.  Instead, what we do
 * here is much more efficient; comparing each generators with each driver
 * and figuring out if there is overlap in their variable sets, and based
 * on that decide whether a generator should be included in the driver's
 * list of generators.
 *
 * While at it, this function also collects the set "drivers" in each of
 * the generators.  Lovely bulk work :)
 */
void drvtab_collect_generators (struct drvtab *drvtab) {
	drvnum_t i;
	struct gentab *gentab = gentab_from_type (drvtab->gentype);
	for (i=0; i<drvtab->count_drvs; i++) {
		struct driverout *drv = &drvtab->drvs [i];
		gennum_t j;
		for (j=0; j<gentab->count_gens; j++) {
			struct generator *gen = &gentab->gens [j];
			if (! bitset_disjoint (drv->relevant_vars,
					gen->variables)) {
				// The driver needs generator i:
				bitset_set (gen->driverout, i);
				// The generator impacts driver j:
				bitset_set (drv->generators, j);
			}
		}
	}
}

/* Collect cogenerators, meaning that generators are signalled when they are, or
 * equivalently put, when they have cogenerators.  This is a determining factor
 * for the question whether iteration over their values may be required, and so
 * if such values may need to be stored in particular backend drivers.
 */
void drvtab_collect_cogenerators (struct drvtab *tab) {
	drvnum_t i;
	bitset_iter_t gi;
	struct driverout *drv = &tab->drvs [0];
	struct gentab *gentab = gentab_from_type (tab->gentype);
	for (i=0; i<tab->count_drvs; drv++, i++) {
		if (bitset_count (drv->generators) > 1) {
			bitset_iterator_init (&gi, drv->generators);
			while (bitset_iterator_next_one (&gi, NULL)) {
				gennum_t g = bitset_iterator_bitnum (&gi);
				gen_set_cogeneration (gentab, g);
			}
		}
	}
}

/* Collect variables that are not presently considered relevant, but that
 * are generated by one of the generators responsible for variables that
 * we considere relevant.  Any extra variables will be incorporated as
 * implicit guards; we need them to ensure uniqueness of the total variable
 * set consisting of driven-out variables and guards.  Note that we are not
 * in any other way concerned about these variables, so we do not continue
 * with these variables in any other way (such as varpartition closures or
 * finding yet other generators for these variables) -- this is because
 * generators already generate their values, irrespective of who's listening.
 */
void drvtab_collect_genvariables (struct drvtab *tab) {
	drvnum_t i;
	bitset_t *genvars = bitset_new (tab->vartype);	// Temporary storage
	struct gentab *gentab = gentab_from_type (tab->gentype);
	for (i=0; i<tab->count_drvs; i++) {
		struct driverout *drv = &tab->drvs [i];
		bitset_iter_t gi;
		// Collect genvars separately, to avoid "contamination" of vi
		bitset_empty (genvars);
		bitset_iterator_init (&gi, drv->generators);
		while (bitset_iterator_next_one (&gi, NULL)) {
			gennum_t g = bitset_iterator_bitnum (&gi);
printf ("GENNUM considered for D%d is %d\n", i, g);
			bitset_t *gextra = gen_share_variables (gentab, g);
			bitset_union (genvars, gextra);
		}
		// Now add the generator variables to the relevant variables
		bitset_union (drv->relevant_vars, genvars);
	}
	bitset_destroy (genvars);
}

/* Collect the guards for this output driver.  Some variables may be setup to
 * be produced, others may have been setup as explicit guards; now use the
 * previously collected knowledge from drvtab_collect_genvariables() to
 * determine what extra variables are needed.  Sort the entries into implicit
 * guards if needed, and prefer mentioning in relevant_vars over mentioning
 * the same variable in explicit_guards (which should not happen, but it is
 * nice to establish that it cannot be true after this function has run).
 */
void drvtab_collect_guards (struct drvtab *tab) {
	drvnum_t i;
	for (i=0; i<tab->count_drvs; i++) {
		struct driverout *drv = &tab->drvs [i];
		bitset_copy (drv->implicit_guards, drv->relevant_vars);
		bitset_subtract (drv->explicit_guards, drv->produced_vars);
		bitset_subtract (drv->implicit_guards, drv->produced_vars);
		bitset_copy     (drv->all_guards,      drv->implicit_guards);
		bitset_subtract (drv->implicit_guards, drv->explicit_guards);
	}
}


/* Verify that all driven output depends on at least one variable; it would
 * be senseless to have an output driver that does not depend on any variable.
 * Moreover, it might invalidate assumptions of our model, which centers on
 * changes being passed through (this could lead to the assumption that one
 * output is generated, but the framework might not actually result in that
 * because constants fall out in differentation, probably even in code).
 *
 * This function returns NULL if no problem is found, or otherwise a bitset_t
 * with the driver outputs that have a problem.
 */
bitset_t *drvtab_invariant_driverouts (struct drvtab *tab) {
	bitset_t *retval = NULL;
	drvnum_t i;
	bitset_iter_t out;
	int novarsyet;
	struct vartab *vartab = vartab_from_type (tab->vartype);
	for (i=0; i<tab->count_drvs; i++) {
		bitset_iterator_init (&out, tab->drvs [i].produced_vars);
		novarsyet = 1;
		while (novarsyet && bitset_iterator_next_one (&out, NULL)) {
			varnum_t j = bitset_iterator_bitnum (&out);
			if (var_get_kind (vartab, j) == VARKIND_VARIABLE) {
				novarsyet = 0;
			}
		}
		if (novarsyet) {
			// Error found
			if (retval == NULL) {
				retval = bitset_new (&tab->drvtype);
			}
			bitset_set (retval, i);
		}
	}
	return retval;
}

bitset_t *drv_share_generators (struct drvtab *tab, drvnum_t drvnum) {
	return tab->drvs [drvnum].generators;
}

/* Set userdata for a driver, and return the previous value. */
void *drv_set_userdata (struct drvtab *tab, drvnum_t drv, void *data) {
	void *retval = tab->drvs [drv].user_cbdata;
	tab->drvs [drv].user_cbdata = data;
	return retval;
}

/* Return the current value of userdata without changing it. */
void *drv_share_userdata (struct drvtab *tab, drvnum_t drv) {
	return tab->drvs [drv].user_cbdata;
}

/* Register a new callback function, and return the old.  Both on going in
 * and going out, NULL represents the drv_default_callback() function which
 * is not externally available.
 */
driver_callback *drv_register_callback (struct drvtab *tab, drvnum_t drv, driver_callback *usercb) {
	driver_callback *retval = tab->drvs [drv].user_callback;
	tab->drvs [drv].user_callback = usercb;
	return retval;
}

/* Replace the current callback with the internal default callback, and return
 * the previous setting (or NULL if that too was the default callback).
 */
driver_callback *drv_unregister_callback (struct drvtab *tab, drvnum_t drv) {
	return drv_register_callback (tab, drv, NULL);
}

/* Internal default callback function.  TODO: This prints out the variables
 * and guards that serve it as parameter.
 */
static void *drv_default_callback (struct drvtab *tab, drvnum_t drv, void *data) {
	fprintf (stderr, "Default driver output with no callback to handle the TODO:following variables:\n");
	return data;
}

int drv_callback (struct drvtab *tab, drvnum_t drv) {
	struct driverout *d = &tab->drvs [drv];
	driver_callback *cb = d->user_callback;
	if (cb == NULL) {
		cb = drv_default_callback;
	}
	d->user_cbdata = (*cb) (tab, drv, d->user_cbdata);
	return true;
}

void drv_add_path_of_least_resistence (struct drvtab *tab, drvnum_t drvnum, path_t *path) {
	struct driverout *drv = &tab->drvs [drvnum];
	if (drv->path_of_least_resistence == NULL) {
		drv->path_of_least_resistence = path;
	} else {
		fatal_error ("Cannot replace path_of_least_resistence");
	}
}

path_t *drv_share_path_of_least_resistence (struct drvtab *tab, drvnum_t drvnum) {
	return tab->drvs [drvnum].path_of_least_resistence;
}
