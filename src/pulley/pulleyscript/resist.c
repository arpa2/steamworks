/* resist.c -- Management for the Path of Least Resistence.
 *
 * The "Path of Least Resistence" is the most efficient, or an estimate
 * thereof, method of driving output after a generator has produced a set
 * of variable value/fork additions or removals.
 *
 * In a slight variation, a driver itself can request its output to be
 * (re)generated, which is similar except that there is another source
 * of initial variables, namely the output driver's query.
 *
 * The determination of the path is based on cost estimations, for which
 * users can enter a "*float" annotation at the end of generator and
 * condition lines; generators are usually > 1.0 because they multiply
 * the number of forks, and conditions are usually < 1.0 because they
 * reduce the number of forks.  The number of forks counts as the cost
 * estimate, and it is always brought down as early as possible.
 *
 * Conditions rely on one of several generators to create variable values
 * for forks, so they may need to run, but then conditions should run as
 * soon as possible.  So we determine the lowest-cost conditions and which
 * are (probably) the cheapest generators to run beforehand.  This leads
 * to a path of zero or more generators and one condition, a sequence that
 * is repeated until no more conditions await execution.  Any remaining
 * generators are then finally run.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "bitset.h"
#include "generator.h"
#include "condition.h"
#include "driver.h"
#include "parser.h"

#include "qsort_fix.h"

enum legtype {
	LT_GENERATOR,
	LT_CONDITION,
	LT_DRIVEROUT
};


struct leg {
	enum legtype legtp;
	union {
		gennum_t lt_generator;
		cndnum_t lt_condition;
		drvnum_t lt_driverout;
	} legtyped;
};
#define legtyped_generator legtyped.lt_generator
#define legtyped_condition legtyped.lt_condition
#define legtyped_driverout legtyped.lt_driverout

/* The "Path of Least Resistence" structure, or path for short, contains
 * a number of entries that are known at the time of its creation.  This
 * means that those paths cannot be changed, once they are set; the total
 * number of generators, conditions, driverouts is fixed after all.
 *
 * Note that this is already typedef'd to path_t in types.h.
 */
struct path {
	unsigned int legs_count;
	unsigned int legs_sorted;
	struct leg legs [1];
	type_t *drvtype;
	bitset_t *vars_pre;
	bitset_t *vars_post;
};


/* Iterator structure for paths.
 */
struct path_iter {
	bool initialised;
};


/* Create a "Path of Least Resistence" with the given number of legs.
 */
struct path *path_new (unsigned int legs) {
	struct path *new = malloc (sizeof (struct path) + (legs - 1) * sizeof (struct leg));
	if (new == NULL) {
		fatal_error ("Out of memory allocating a Path of Least Resistence");
	}
	new->legs_count = 0;
	return new;
}


/* Destroy a Path of Least Resistence.
 */
void path_destroy (struct path *path) {
	free (path);
}


/* Add a single element to a "Path of Least Resistence".
 */
void path_add (struct path *plr, enum legtype ltp, unsigned int entry) {
	struct leg *new = &plr->legs [plr->legs_count];
	new->legtp = ltp;
	switch (ltp) {
	case LT_GENERATOR:
		new->legtyped_generator = (gennum_t) entry;
		break;
	case LT_CONDITION:
		new->legtyped_condition = (cndnum_t) entry;
		break;
	case LT_DRIVEROUT:
		new->legtyped_driverout = (drvnum_t) entry;
		break;
	default:
		fatal_error ("Unknown leg type offered");
	}
	plr->legs_count++;
}

/* Print the path */
void path_print (struct path *plr, FILE *stream) {
	char *comma = "";
	int i;
	fprintf (stream, "Path< ");
	for (i=0; i<plr->legs_count; i++) {
		struct leg *leg = &plr->legs [i];
		switch (leg->legtp) {
		case LT_GENERATOR:
			fprintf (stream, "%sG%d", comma, leg->legtyped_generator);
			break;
		case LT_CONDITION:
			fprintf (stream, "%sG%d", comma, leg->legtyped_condition);
			break;
		case LT_DRIVEROUT:
			fprintf (stream, "%sG%d", comma, leg->legtyped_driverout);
			break;
		default:
			fprintf (stream, "???legtp=%d???\n", leg->legtp);
		}
		comma = ", ";
	}
	fprintf (stream, " >");
}

/* Add multple elements as provided by a list to a "Path of Least Resistence".
 * The list is assumed to be in the proper order for adding, and it is setup
 * as an array of unsigned int values ("argv") with a given length ("argc").
 */
void path_add_multi (struct path *plr, enum legtype ltp,
		unsigned int entryc, unsigned int *entryv) {
	while (entryc-- > 0) {
		path_add (plr, ltp, *entryv++);
	}
}


/* Sorting is done in two phases; first we store the current location with
 * path_pre_sort(), then we add elements (presumably of the same type), and
 * finally we sort them with path_run_sort().
 */
static void path_pre_sort (struct path *plr) {
	plr->legs_sorted = plr->legs_count;
}

static void path_run_sort (struct path *plr, void *context, qsort_cmp_fun_t cmp) {
	QSORT_R(&plr->legs [plr->legs_sorted],
		plr->legs_count - plr->legs_sorted,
		sizeof (struct leg),
		context,
		cmp);
}


/* Compare two legs that are assumed to be generators by their weight, in a
 * context being the gentab.  Return -1, 0, 1 for <, ==, >.
 */
static int path_cmp_by_genweight (struct gentab *context, struct leg *left_leg, struct leg *right_leg) {
	float wl = gen_get_weight (context,  left_leg->legtyped_generator);
	float wr = gen_get_weight (context, right_leg->legtyped_generator);
	if (wl < wr) {
		return -1;
	} else if (wl > wr) {
		return 1;
	} else {
		return 0;
	}
}

QSORT_CMP_FUN_DECL(gen_cmp_weight)

/* Schedule a "Path of Least Resistence" for the given needs.  Output in plr.
 *
 * The needed variables will be built up, the generators and conditions will
 * all be added to the "Path of Least Resistence" and the return value will
 * indicate if this is not possible.  The xxx_needed variables will be
 * emptied in the course of processing them.
 *
 * Usually, there will already be a beginning on the plr, namely the generator
 * that initiates the path when it produces forks to add/remove in response to
 * an LDAP update.
 *
 * This routine assumes that the cheapest generator for each variable has been
 * established, and that it was a success, that is there are no variables that
 * have no generators.
 *
 * TODO: This function currently empties cnds_needed and gens_needed.
 */
void path_schedule (struct path *plr,
			bitset_t *vars_needed,
			bitset_t *gens_needed,
			bitset_t *cnds_needed) {
	gennum_t *cand_genv, *min_genv;
	gennum_t  cand_genc,  min_genc;
	bitset_iter_t g;
	struct cndtab *cctab = cndtab_from_type (bitset_type (cnds_needed));
	//
	// Allocate room for the candidate suggestions
	cand_genc = bitset_count (vars_needed);
	cand_genv = calloc (cand_genc, sizeof (gennum_t));
	min_genv  = calloc (cand_genc, sizeof (gennum_t));
	if ((cand_genv == NULL) || (min_genv == NULL)) {
		fatal_error ("Out of memory while allocating generator lists during path scheduling");
	}
	//
	// Repeatedly add generator/generator/generator/condition sequences
	while (!bitset_isempty (cnds_needed)) {
		bool got_min;
		unsigned int min_cidx;
		float min_cost;
		bitset_iter_t c;
		int i;
		//
		// Find the minimum cost condition
		got_min = false;
		bitset_iterator_init (&c, cnds_needed);
		while (bitset_iterator_next_one (&c, NULL)) {
			float cand_cost;
			unsigned int cand_cidx = bitset_iterator_bitnum (&c);
			struct condition *cand_cond = bitset_iterator_element (&c, NULL);
			if (!cnd_estimate_total_cost (
					cctab, cand_cidx,
					vars_needed,
					&cand_cost,
					gentab_from_type (bitset_type (gens_needed)),
					&cand_genc, cand_genv)) {
				fatal_error ("Failed to plot a path for all variables needed");
			}
			if (got_min && (cand_cost > min_cost)) {
				continue;
			}
			got_min = true;
			min_cidx = cand_cidx;
			min_cost = cand_cost;
			min_genc = cand_genc;
			memcpy (min_genv, cand_genv, cand_genc * sizeof (gennum_t));
		}
		//
		// Schedule the minimum-cost condition's generators
		path_add_multi (plr, LT_GENERATOR, min_genc, min_genv);
		for (i=0; i<min_genc; i++) {
			bitset_clear (gens_needed, min_genv [i]);
		}
		//
		// Schedule the minimum-cost condition into the path
		path_add (plr, LT_CONDITION, min_cidx);
		bitset_clear (cnds_needed, min_cidx);
	}
	//
	// Schedule any remaining generators
	path_pre_sort (plr);
	bitset_iterator_init (&g, gens_needed);
	while (bitset_iterator_next_one (&g, NULL)) {
		path_add (plr, LT_GENERATOR, bitset_iterator_bitnum (&g));
	}
	path_run_sort (plr, gentab_from_type (bitset_type (gens_needed)), _qsort_gen_cmp_weight);
	bitset_empty (gens_needed);
	//
	// Cleanup temporary structures
	free (min_genv);
	free (cand_genv);
}


/* The idempotent function path_have_push() ensures a Path of Least Resistence
 * exists, starting from the generator provided.  If one has been created
 * already, it won't be generated anew.  This means that path_have_push()
 * can be run at any time before run_path() and as often as desired.
 *
 * The path is used in "push mode", which means that the generator takes the
 * initiative and drivers eventually receive variables sets pushed out.
 * In push mode, one generator may send information out of many drivers.
 * See path_have_pull() for the other mode.
 *
 * This function returns the Path of Least Resistence for the generator, and
 * will also store it in the generator for future lookup.
 */
struct path *path_have_push (struct gentab *tab, gennum_t initiator) {
	struct path *retval;
	retval = gen_share_path_of_least_resistence (tab, initiator);
	if (retval == NULL) {
		//TODO// retval = path_schedule (retval, ...vars/gens/conds..._needed);
		retval->drvtype = gentab_share_driverout_type (tab);
		//TODO// Setup vars_pre, vars_post
		gen_add_path_of_least_resistence (tab, initiator, retval);
	}
	return retval;
}

#ifdef IMPLEMENT_PULL_PATH_TODO
/* The idempotent function path_have_pull() ensures a Path of Least Resistence
 * exists, starting from the generator provided.  If one has been created
 * already, it won't be generated anew.  This means that path_have_push()
 * can be run at any time before run_path() and as often as desired.
 *
 * The path is used in "pull mode", which means that a driver takes the
 * initiative to pull information out of generators.  In pull mode, there
 * will not be any impact on other drivers because no new data is generated.
 * See path_have_push() for the other mode.
 *
 * This function returns the Path of Least Resistence for the driver, and
 * will also store it in the driver for future lookup.
 */
struct path *path_have_pull (struct drvtab *tab, drvnum_t initiator) {
	struct path *retval;
	retval = drv_share_path_of_least_resistence (tab, initiator);
	if (retval == NULL) {
		//TODO// retval = path_schedule (retval, ...vars/gens/conds..._needed);
		retval->drvtype = drvtab_share_type (tab));
		//TODO// Setup vars_pre, vars_post
		drv_add_path_of_least_resistence (tab, initiator, retval);
	}
	return retval;
}
#endif /* IMPLEMENT_PULL_PATH_TODO */


/* Iterate to the next generator state.  (Internal support for path_run)
 *
 * This function iterates over generators, conditions, drivers; this
 * leads to new, or changed, values for variables.  One could say that
 * the informal "forks" of variable values are generated here.  They may
 * also be squashed, if so desired by conditions.  While iterating, new
 * variable combinations are found that aligns with all applicable
 * conditions and that fills a complete set of variables as supported by
 * the Path of Least Resistence over which the iterator runs.
 *
 * Since a Path of Least Resistence consists of nested for-loops, and since
 * not all variables iterated are of interest to all output drivers, the
 * big question now is which output drivers should run.  Generally, this
 * is only done when the complete set of variables that determine a
 * unique output (so, parameters and both explicit and implicit guards)
 * has been created.  A set to hold the generators to run is passed into
 * this function, and filled in while the iterator runs.
 *
 * In terms of the iterator, the nested for-loops may iterate on variables
 * that are not of interest to a particular output driver; the determining
 * question is whether they have an impact on the unique variable combination
 * driven out.  This can be resolved by looking at the outermost variable
 * that has changed.  Initially, that will be the initiating variable and
 * later on it will be determined by looking at control moving from inner
 * loops to outer loops, and picking the outermost loop that generates.
 *
 * The set of output drivers that are impacted by such an outermost
 * generator is subject to static analysis, which is assumed to have taken
 * place before using the iterator functions on paths.
 *
 * Note that a condition may cut short the availability of a unique set
 * of variable values to an output driver.  In such situations, a future
 * iteration may still lead to driving an output, even if the outermost
 * generator would not suggest this.
 *
 * The general mechanism therefore becomes a set of output drivers for each
 * of the generators in the Path of Least Resistence; and the driver set
 * returned is the union of all these sets.  Generators that are driven will
 * be removed from all the sets along the path upon return from this function,
 * but not before.  When a generator is run, its set of driven generators will
 * be replaced, which comes down to the same thing as union'ed, with the set
 * of drivers for that generator.  When a loop terminates, its set of output
 * drivers will be reset.  Finally, the initiating generator will be present
 * in all this * with its set, which will be used as soon as possible; all the
 * other generators in the path will be initialised with empty driver sets, to
 * reflect that they haven't generated and so are not yet causing drivers
 * to act up.
 *
 * Individual (internal) functions follow below:
 *  - path_iterator_init ()
 *  - path_iterator_next ()
 *  - path_iterator_cleanup ()
 */


/* Initialise an iterator for the given Path of Least Resistence.  (internal)
 * Note that the iterator must still be run to get to the first work set.
 *
 * The return value, which MUST be checked, indicates success.  Failure
 * to construct an iterator is a passing condition if the current running
 * process continues to run until completion, so a later retry would
 * be possible.  Invoking path_iterator_next() on an iterator that failed
 * to initialise is a fatal error.
 *
 * Implementation note: Only one iterator may be active at any one time.
 * This is due to the implementation which stores bitsets in generators and
 * values in variables.  It is probably even a conceptual requirement; we
 * do not see, at the moment, how generators can run in parallel, even in
 * various orders as determined by various paths, since they influence
 * each other and could therefore cause race conditions.
 */
static bool singular_iterator_welcomed = true;
static bool path_iterator_init (struct path_iter *it, struct path *plr) {
	// When using threading, lock() the flag here
	bool retval = singular_iterator_welcomed;
	if (retval) {
		singular_iterator_welcomed = false;
	}
	// When using threading, unlock() the flag here
	it->initialised = retval;
	//TODO// Setup the sets for all generators, only non-empty on initiator
	return retval;
}

/* Cleanup an iterator for a Path of Least Resistence.  (internal)
 */
static void path_iterator_cleanup (struct path_iter *it) {
	// Note: This does not need any lock() but it signal a queue
	if (it->initialised) {
		singular_iterator_welcomed = true;
		it->initialised = false;
	}
}

/* Continue to the next iteration of a Path of Least Resistence.  (internal)
 * The function fills the drivers_todo variable with the set of drivers
 * that should be run with the delivered set of variables.  The return value
 * of the function is false when iteration is done (in which case drivers_todo
 * will be an empty set).
 */
static bool path_iterator_next (struct path_iter *it, bitset_t *drivers_todo) {
	if (!it->initialised) {
		fatal_error ("Attempt to iterate along a Path of Least Resistence with a failed-to-initialise iterator");
	}
	bitset_empty (drivers_todo);
	return false; //TODO// Actually iterate!  Collect drivers_todo!
}


/* Run the path's iterators and drive outputs.  The input to this phase is
 * the generator that takes the initiative to generated new variable values.
 * The new values are assumed to have been stored in the variables already.
 * TODO: Actually present variable values in some form.
 *
 * The Path of Least Resistence must first be obtained with path_have_push()
 * or path_have_pull(), depending on whether the path runs in push mode
 * (initiated by a generator) or in pull mode (initiated by a driver).
 *
 * The variables bound by a generator or driver must be set before path_run()
 * is invoked.  The set of variables is defined in path_share_vars_pre().
 *
 * The parameter add_notdel is true when elements are added, or false when
 * they are deleted.  TODO: Move this alongside the variable values being
 * forked / removed by the generator / driver.
 *
 * This function returns false on failure, in which case rescheduling it
 * is advised.
 */
bool path_run (struct gentab *tab, path_t *plr, bool add_notdel) {
	struct path_iter it;
	bitset_t *drivers_todo;
	bitset_t *vars_todo;
	bitset_t *vars_done;
	bool ok = true;
	//
	// Try to claim an iterator.
	// This is a resource, which may be (temporarily) rejected.
	// When assigned, the first iteration will move in, and next
	// iterations increment counters "from the inside out", just
	// like nested for loops.
	ok = ok && path_iterator_init (&it, plr);
	if (ok) {
		vars_done = plr->vars_pre;
		vars_todo = bitset_clone (plr->vars_post);
		drivers_todo = bitset_new (plr->drvtype);
		struct drvtab *drvtab = drvtab_from_type (plr->drvtype);
		bitset_subtract (vars_todo, vars_done);
		//
		// Iterate, returning a list of drivers to invoke with
		// the next combination of variables that were set and
		// approved by conditions.  The drivers_todo list knows
		// which drivers need to have their callbacks invoked
		// based on whether one or more of the variables that
		// are aware of has changed, so this set may differ from
		// iteration to iteration.
		while (ok && path_iterator_next (&it, drivers_todo)) {
			bitset_iter_t di;
			bitset_iterator_init (&di, drivers_todo);
			while (ok && bitset_iterator_next_one (&di, NULL)) {
				drvnum_t drv = bitset_iterator_bitnum (&di);
				ok = drv_callback (drvtab, drv);
				bitset_clear (drivers_todo, drv);
				if (!ok) {
					fprintf (stderr, "Error returned from driver output callback function, inconsistent driver state!!\n");
				}
			}
		}
		path_iterator_cleanup (&it);
		bitset_destroy (vars_todo);
		bitset_destroy (drivers_todo);
	}
	return ok;
}


