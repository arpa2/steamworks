/* driver.h -- Functions defined on output drivers.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */

#ifndef PULLEYSCRIPT_DRIVER_H
#define PULLEYSCRIPT_DRIVER_H

#include <stdio.h>

#include "bitset.h"
#include "types.h"
#include "lexhash.h"

#ifdef __cplusplus
extern "C" {
#endif

struct drvtab;

struct drvtab *drvtab_new (void);
void drvtab_destroy (struct drvtab *tab);
type_t *drvtab_share_type (struct drvtab *tab);

void drvtab_set_vartype (struct drvtab *tab, type_t *vartp);
void drvtab_set_gentype (struct drvtab *tab, type_t *gentp);
void drvtab_set_cndtype (struct drvtab *tab, type_t *cndtp);

type_t *drvtab_share_vartype (struct drvtab *tab);
type_t *drvtab_share_gentype (struct drvtab *tab);
type_t *drvtab_share_cndtype (struct drvtab *tab);

drvnum_t drv_new (struct drvtab *tab);
drvnum_t drvtab_count (struct drvtab *tab);

/* Hash handling */
void drv_set_hash (struct drvtab *tab, drvnum_t drvnum, hash_t drvhash);
hash_t drv_get_hash (struct drvtab *tab, drvnum_t drvnum);

void drvtab_print (struct drvtab *tab, char *head, FILE *stream, int indent);
void drv_print (struct drvtab *tab, drvnum_t drv, FILE *stream, int indent);
#ifdef DEBUG
#  define drvtab_debug(tab,head,stream,indent) drv_print(tab,head,stream,indent)
#  define drv_debug(tab,drv,stream,indent) drv_print(tab,drv,stream,indent)
#else
#  define drvtab_debug(tab,head,stream,indent)
#  define drv_debug(tab,drv,stream,indent)
#endif

void drv_set_module (struct drvtab *tab, drvnum_t drvnum, char *module);

void drv_set_weight (struct drvtab *tab, drvnum_t drvnum, float weight);
float drv_get_weight (struct drvtab *tab, drvnum_t drvnum);

void drv_add_guardvar (struct drvtab *tab, drvnum_t drvnum, varnum_t varnum);

void drv_output_variable (struct drvtab *tab, drvnum_t drvnum, varnum_t varnum);
void drv_output_list_begin (struct drvtab *tab, drvnum_t drvnum);
void drv_output_list_end (struct drvtab *tab, drvnum_t drvnum);
bitset_t *drv_share_output_variables (struct drvtab *tab, drvnum_t drvnum);
void drv_share_output_variable_table (struct drvtab *tab, drvnum_t drvnum,
			varnum_t **out_array, varnum_t *out_count);
bitset_t *drv_share_conditions (struct drvtab *tab, drvnum_t drvnum);

void drv_setup_param (struct drvtab *tab, drvnum_t drvnum, char *param, varnum_t const_varnum);

void drvtab_collect_varpartitions (struct drvtab *tab);
void drvtab_collect_genvariables (struct drvtab *tab);
void drvtab_collect_conditions (struct drvtab *tab);
void drvtab_collect_generators (struct drvtab *tab);
void drvtab_collect_cogenerators (struct drvtab *tab);
void drvtab_collect_guards (struct drvtab *tab);

bitset_t *drv_share_generators (struct drvtab *tab, drvnum_t drvnum);

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
bitset_t *drvtab_invariant_driverouts (struct drvtab *tab);

/* Manage a path of least resistence for this driver.  The path will assume
 * that all variables of interest to the driver are mentioned, and will
 * produce the guard variables belonging to it; possibly multiple responses
 * will be generated.
 */
void drv_add_path_of_least_resistence (struct drvtab *tab, drvnum_t drvnum, path_t *path);
path_t *drv_share_path_of_least_resistence (struct drvtab *tab, drvnum_t drvnum);


/* The API for user programs consists of register and unregister functions,
 * used to manage callbacks.  In addition, there are get/set functions for
 * userdata which is carried in and out of the callbacks.  Each of the
 * functions returns the previous setting.  Callback function and data may
 * be set to NULL; for the function, this causes printing the callback data.
 */
typedef void *driver_callback (struct drvtab *tab, drvnum_t drv, void *data);

void *drv_set_userdata (struct drvtab *tab, drvnum_t drv, void *data);
void *drv_share_userdata (struct drvtab *tab, drvnum_t drv);

driver_callback *drv_register_callback (struct drvtab *tab, drvnum_t drv, driver_callback *usercb);
driver_callback *drv_unregister_callback (struct drvtab *tab, drvnum_t drv);

int drv_callback (struct drvtab *tab, drvnum_t drv);

#ifdef __cplusplus
}
#endif

#endif

