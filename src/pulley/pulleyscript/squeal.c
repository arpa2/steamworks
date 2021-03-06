/* squeal.c -- engine for generator-to-driver mapping through SQL.
 *
 * While working on our Path of Least Resistence concept, it become more and more
 * obvious that large portions of the optimisation were in fact reinvention of
 * SQL optimisation concepts.  Although we believe that we can be more efficient
 * if we generate for multiple drivers at once, we believe that an SQL-based
 * implementation that generates for one driver at a time is quite useful and
 * reasonable to offer.  The maximum pay is linear in the number of drivers,
 * but something dramatic such as an order of magnitude.
 *
 * The SQL engine on which this is built, it SQLite3.  This is not a choice based
 * on the simplicity of the platform -- it is a full acceptance of the precise
 * conditions under which SQLite3 functions; with storage in the local filesystem,
 * an optimiser that does (at least) what we require, transaction support and a
 * general structure of set manipulation as in any RDBMS, we do not believe there
 * should ever be a reason to migrate to another, "more advanced" database.  It
 * is the local storage combined with precomputed optimisations that captured our
 * attention and focussed it on this engine.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "bitset.h"
#include "lexhash.h"
#include "variable.h"
#include "generator.h"
#include "condition.h"
#include "driver.h"
#include "squeal.h"

#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include <sqlite3.h>

// Offset for ?003 and alike -- they start from 001, not 000
#define PARAM_OFS 1

extern void write_logger(const char* logname, const char* message);

static const char logger[] = "steamworks.pulleyscript.squeal";
static char log_buffer[1024];

#define DEBUG(...) { snprintf(log_buffer, sizeof(log_buffer), __VA_ARGS__); log_buffer[sizeof(log_buffer)-1]=0; write_logger(logger, log_buffer); }
#define ERROR(...) DEBUG(__VA_ARGS__)

/* TODO: Make the database directory a configurable entity.
 */
#ifdef PULLEY_SQUEAL_DIR
static const char _default_dbdir[] = PULLEY_SQUEAL_DIR;
#else
static const char _default_dbdir[] = "/var/db/pulley/";
#endif

#ifdef ALLOW_INSECURE_DB
const char *squeal_use_dbdir = _default_dbdir;
#else
static const char const *squeal_use_dbdir = _default_dbdir;
#endif

/* Definitions for FNV-1a hashing */

typedef uint64_t fnv1a_t;
typedef fnv1a_t s3key_t;

#define FNV1A_PRIME 1099511628211U
#define FNV1A_INIT 14695981039346656037U
#define S3KEY_INIT FNV1A_INIT

/* Squeal engine runtime structures.  These are what's left after parsing the input,
 * and processing its semantics.  They connect three elements:
 *  1. The upstream LDAP provisioning of data and callback-from-ldap-to-engine interface
 *  2. The squeal engine's statements and further processing data
 *  3. The downstream driver data and callback-from-engine-to-driver interfaces
 */


/* The "s3ins_driver" structure is a shared description of a single driver.
 * It defines a hash that is prepared with the lexhash, as well as callback
 * structures.  The purpose of the prehash is to suppress callbacks in case
 * there already / still are other instances.  Callbacks only add output to
 * the driver upon first repeat of a new hash value, and they only delete
 * output from the driver upon last repeat of a hash value removal.  Use
 * the generic SQLite3 statements from "struct squeal" to fetch and modify
 * the repeat counter for a given output hash.  (The output hash starts from
 * the prehash, and repeatedly adds a 4-byte length in network byte order
 * plus that number of blob bytes, until all output variables are hashed.)
 *
 * Aside from the customary cbfun, cbdata fields, there also is a facility
 * cbnumparm, cbparm that offers a place for insertion of the output variables
 * as an array of blobs, in the order produced by the various output generators
 * from the prepared SQLite3 statements in "s3ins_gen2drv" structures.
 *
 * The callback function requires one additional flag, namely add_not_del which
 * is set to PULLEY_TUPLE_ADD or PULLEY_TUPLE_DEL to indicate how the
 * output variables are to be processed.
 */
struct s3ins_driver {
	s3key_t drvall_prehash;		// Already hashed driver's lexhash
	squeal_driverfun_t cbfun;	// Callback function for driver
	void *cbdata;			// First arg to cbfun
	int cbnumparm;			// Number of callback blob variables
	struct squeal_blob *cbparm;	// Array for callback blob variables
};

/* When a generator forks a tuple, this should be forward to the apropriate
 * drivers.  Each of the "s3ins_gen2drv" structures holds a link to shared data
 * for each driver, as well as a routine to produce output from a generator's
 * forked record.
 *
 * The arguments to this output routine are tuple variables from the generator
 * that hosts this structure.  The routine incorporates any co-generators as
 * needed (or none, it may simply quote the variables if that applies) and
 * while doing so it also applies any conditions of interest.  The output is
 * supposed to be iterated and delivered through callbacks, along with an
 * indication whether the output should be added or removed.  Note that drivers
 * also have a mechanism to deal with multiplicity.
 */
struct s3ins_gen2drv {
	struct s3ins_driver *driver;	// The description of the driver structure
	sqlite3_stmt *gen2drv_produce;	// Supply gen variables: ?x, ?y, ...
};

/* The "s3ins_generator" structure holds information for a generator.
 *
 * numrecvars is the number of variable names (or the number of variables)
 * expected in a forked record; 0 would be unexpected, since a generator
 * without variables is useless. The names of the variables are placed
 * sequentially into the char buffer recvars, NUL-separated (currently
 * not implemented).
 *
 * Prepared statements opt_gen_add_tuple and opt_gen_del_tuple are (if
 * not NULL) used to store variable (tuples) in the database, before
 * forking those variables.
 *
 * For co-generators, the opt_gen_{add,del}_tuple statements are used
 * to provide variable (tuples) to the co-generators.
 *
 * numdriveout is the number of output-driver tuples in the driveout
 * array. The driveout array may be indexed from 0 to numdriveout-1.
 */
struct s3ins_generator {
	int numrecvars;			  // Number of variable names in forked record
	char *recvars;			  // "gen_xxx", "x", "y", ..., "" (NUL splits)
		//TODO// Perhaps use ?001 and ?002 instead of ?x and ?y to simplify
		//TODO// With ?001 varnames, could we get in trouble after reloading?
	sqlite3_stmt *opt_gen_add_tuple;  // Supply ?hash + gen variables: ?x, ?y,...
	sqlite3_stmt *opt_gen_del_tuple;  // Supply ?hash + gen variables: ?x, ?y,...
	int numdriveout;		  // Number of driveout[] tuples
	struct s3ins_gen2drv* driveout;   // Driver instructions for this generator
};

/* The "squeal" structure holds the overall information for a SQLite3 engine instance.
 * It is defined as an opaque type for use by other modules in squeal.h.
 *
 * The operations on drv_all are global, but their use involves a prepared hash
 * to be taken from the s3ins_driver structure.  This hash has already walked
 * through the lexhash and is ready for further application of the blobs that
 * define the rule.
 */
struct squeal {
	sqlite3 *s3db;			// link to SQLite3 engine
	/* TODO: Uplink to LDAP */
	int numdrivers;			// Number of drivers[] tuples
	struct s3ins_driver *drivers;	// Array holding shared descriptions per driver
	sqlite3_stmt *get_drv_all;	// ?hash
	sqlite3_stmt *inc_drv_all;	// ?hash
	sqlite3_stmt *dec_drv_all;	// ?hash
	int numgens;			// Number of gens[] tuples
	struct s3ins_generator gens [1];// Array of generator descriptions
};


/* Write buffer state; consisting of a pointer, a current writing offset annex length,
 * and a buffer allocated size.
 */
struct sqlbuf {
	char *buf;
	size_t ofs;
	size_t siz;
};

#define BUF_GET 1
#define BUF_PUT 0


/********** RUNTIME SUPPORT UTILITIES **********/


/* Exchange write buffer: BUF_GET or BUF_SET the provided structure with an
 * internally held sqlbuf structure.  This facilitates reuse.
 */
static void sqlbuf_exchg (struct sqlbuf *wbuf, bool get_not_set) {
	static struct sqlbuf held = { NULL, 0, 0 };
	char *tofree;
	if (get_not_set == BUF_GET) {
		// get the internal buffer and wipe the internal one
		*wbuf = held;
		held.buf = NULL;
		held.ofs = 0;
		held.siz = 0;
	} else {
		// set the internal buffer
		if (wbuf->siz > held.siz) {
			tofree = held.buf;
			held = *wbuf;
			held.ofs = 0;
		} else {
			tofree = wbuf->buf;
		}
		if (tofree) {
			free (tofree);
		}
	}
}

/* Given a (char **) for an output buffer, write a (ptr,len) blob to it,
 * possibly adding to its size to make this possible.
 */
static void sqlbuf_writeblob (struct sqlbuf *buf, const char *blob, int len) {
	char *buf2;
	int   siz2;
	assert (buf->ofs <= buf->siz);
	if (buf->siz < buf->ofs + len) {
		siz2 = buf->siz + (buf->siz >> 2) + len;
		buf2 = realloc (buf->buf, siz2);
		if (buf2 == NULL) {
			ERROR("Out of memory while writing %d bytes to squeal buffer\n", len);
			exit (1);
		}
		memset(buf2 + buf->ofs, 0, siz2 - buf->ofs);
		buf->buf = buf2;
		buf->siz = siz2;
	}
	memcpy (buf->buf + buf->ofs, blob, len);
	buf->ofs += len;
}

/* Given a (char **) for an output buffer, write text to it, possibly adding to
 * its size to make this possible.
 */
static void sqlbuf_write (struct sqlbuf *buf, const char *addend) {
	sqlbuf_writeblob (buf, addend, strlen (addend));
}


/* Run an SQL statement from the given sqlbuf string.  Reset the buffer when done.
 * Return 0 on success, 1 on failure.
 */
static int sqlbuf_run (struct sqlbuf *sql, sqlite3 *s3db) {
	int retval = 0;
	int sqlretval = 0;
	sqlite3_stmt *s3in;
	DEBUG("exec sql>\n%.*s\n", (int) sql->ofs, sql->buf);
	sqlretval = sqlite3_prepare (s3db, sql->buf, sql->ofs, &s3in, NULL);
	if ((sqlretval != SQLITE_OK)) {
		/* TODO: Report error in more detail */
		DEBUG("SYNTAX ERROR in SQL %d\n", sqlretval);
	}
	sqlretval = sqlite3_step (s3in);
	if ((sqlretval != SQLITE_OK) && (sqlretval != SQLITE_DONE)) {
		/* TODO: Report error in more detail */
		DEBUG("RUNTIME ERROR in SQL %d\n", sqlretval);
	} else {
		DEBUG("DONE\n");
	}
	DEBUG("\n");
	sqlite3_finalize (s3in);
	sql->ofs = 0;
	return retval;
}


/* Derive a table name based on a prefix and a "lexhash" over a construct and
 * send it out through sqlbuf_write().
 * (The hash is not processed portably, as it uses local byte order.)
 */
static void sqlbuf_lexhash2name (struct sqlbuf *sql, char *prefix, hash_t lexhash) {
	char hexstr [3];
	int len = sizeof (hash_t);
	uint8_t *ptr = (uint8_t *) &lexhash;
	sqlbuf_write (sql, prefix);
	while (len-- > 0) {
		sprintf (hexstr, "%02x", *ptr++);
		sqlbuf_write (sql, hexstr);
	}
}


/* The FNV-1a hash algorithm on 64 bits.  This is not a secure hash, but it is
 * useful for scattering bits as a result of input of various sizes.  The output
 * size of 64 bits suffices to avoid accidental clashes, which is our purpose here,
 * as we have no reason to mistrust the data being passed to this hash algorithm.
 * Various routines are defined to simplify the use of this algorithm for our use
 * of it in the SQLite3 engine, where it serves to produce primary keys of our
 * own choosing, that is, indexes into the data.
 */

void fnv1a_add_bytes (fnv1a_t *hash, const uint8_t *data, size_t len) {
	fnv1a_t h = *hash;
	while (len-- > 0) {
		h ^= *data++;
		h *= FNV1A_PRIME;
	}
	*hash = h;
}

/* Scramble an FNV-1a hash by inserting the binary value of a lexhash.
 * (The hash is not processed portably, as they use local byte order.)
 */
void s3key_add_lexhash (s3key_t *hash, hash_t lexhash) {
	fnv1a_add_bytes (hash, (uint8_t *) &lexhash, sizeof (lexhash));
}

/* Add a blob to an FNV-1a hash.  Prefix a 4-byte length, so as to
 * avoid data fields to clash before being smashed to trash by the hash.
 */
void s3key_add_blob (s3key_t *hash, struct squeal_blob *blob) {
	uint32_t size2 = htonl (blob->size);
	fnv1a_add_bytes (hash, (uint8_t *) &size2, 4);
	fnv1a_add_bytes (hash, blob->data, blob->size);
}



/********** RUNTIME PROCEDURES **********/



/* Invoke a prepared SQL statement after filling out the hash for ?hash and the
 * other parameters for ?000, ?001 and so on.  The value returned is the value
 * from sqlite3_step() on the parameterised statement.  If multiple actions on
 * the statement are desired, then invoke sqlite3_step() and/or sqlite3_reset()
 * in addition after this call.  When this routine is called again, it sets up
 * the prepared statement with new parameters.
 */
static int s3ins_run (sqlite3 *s3db, sqlite3_stmt *s3in, s3key_t hash,
			int numparm, struct squeal_blob *parm) {
	char drvid [20];
	int i;
	int idx;
	int sqlret;
	assert (s3in != NULL);
	assert (numparm <= SQLITE_LIMIT_VARIABLE_NUMBER);
	//
	// Cleanup the prepared statement for fresh bindings (also before 1st call)
	sqlite3_reset (s3in);
	sqlite3_clear_bindings (s3in);
	//
	// Setup the prepared statement with the hash value, if it asks for it
	idx = sqlite3_bind_parameter_index (s3in, ":hash");
	if (idx != 0) {
		sqlite3_bind_int64 (s3in, idx, hash);
	}
	//
	// Setup the prepared statement with the output variable blobs
	for (i=1; i < numparm; i++) {
		snprintf (drvid, sizeof (drvid)-1, "?%03d", PARAM_OFS + idx);
		idx = sqlite3_bind_parameter_index (s3in, drvid);
		if (idx != 0) {
			sqlret = sqlite3_bind_blob64 (s3in, idx,
					parm [i].data, parm [i].size,
					SQLITE_STATIC);
			if (sqlret != SQLITE_OK) {
				ERROR ("Binding :hash yields %d %s\n", sqlret, sqlite3_errmsg (s3db));
			}
		}
	}
	//
	// Run the actual process and return the result
	return sqlite3_step (s3in);
}

/* Similar to s3ins_run() but using a textual UUID instead of FNV-1a hash.
 */
static int s3ins_run_uuid(sqlite3 *s3db, sqlite3_stmt *s3in, const char *uuid,
			int numparm, struct squeal_blob *parm) {
	//TODO// This code is never referenced
	char drvid [20];
	int i;
	int idx;
	int sqlret;

	assert (s3in != NULL);
	assert (numparm <= SQLITE_LIMIT_VARIABLE_NUMBER);
	//
	// Cleanup the prepared statement for fresh bindings (also before 1st call)
	sqlite3_reset (s3in);
	sqlite3_clear_bindings (s3in);
	//
	// Setup the prepared statement with the entryUUID value, if it asks for it
	idx = sqlite3_bind_parameter_index (s3in, ":uuid");
	if (idx != 0) {
		sqlret = sqlite3_bind_text(s3in, idx, uuid, -1, SQLITE_STATIC);
		if (sqlret != SQLITE_OK)
		{
			ERROR("Binding :uuid yields %d %s\n", sqlret, sqlite3_errmsg(s3db));
		}
	}
	//
	// Setup the prepared statement with the output variable blobs
	for (i=0; i < numparm; i++) {
		snprintf (drvid, sizeof (drvid)-1, "?%03d", PARAM_OFS + i+2);
		idx = sqlite3_bind_parameter_index (s3in, drvid);
		if (idx != 0) {
			sqlret = sqlite3_bind_blob64 (s3in, idx,
					parm [i].data, parm [i].size,
					SQLITE_STATIC);
			if (sqlret != SQLITE_OK)
			{
				ERROR("Binding %s yields %d %s\n", drvid, sqlret, sqlite3_errmsg(s3db));
			}
		}
	}
	//
	// Run the actual process and return the result
	return sqlite3_step (s3in);
}

/* Consider passing a series of blobs to a driver as output, either add or delete.
 * Avoid this when multiplicity forbids; that is, already added or not ready for
 * deletion yet, due to other existing instances.  Only when there are no other
 * instances, do we actually perform the callback.
 */
static void squeal_driver_callback_demult (struct squeal *squeal,
			struct s3ins_driver *drvback, int add_not_del) {
	int s3rv = SQLITE_OK;
	bool drive = false;
	int repeats = -1;
	int i;
	s3key_t hash;
	//
	// Setup hash from the prehash plus all the blobs of the parameters
	hash = drvback->drvall_prehash;
	for (i=0; i < drvback->cbnumparm; i++) {
		s3key_add_blob (&hash, &drvback->cbparm [i]);
	}
	//
	// Fetch the current number of values for the blobs
	s3rv = s3ins_run (squeal->s3db, squeal->get_drv_all,
			hash, drvback->cbnumparm, drvback->cbparm);
	if (s3rv == SQLITE_ROW) {
		repeats = sqlite3_column_int (squeal->get_drv_all, 0);
	} else {
		ERROR("  SQLERR %d %s\n", s3rv, sqlite3_errmsg(squeal->s3db));
	}

	//
	// See if the current number hints at not making the callback
	if (repeats < 0) {
		drive = 0;
	} else if (add_not_del == PULLEY_TUPLE_ADD) {
		drive = (repeats == 0);
	} else {
		drive = (repeats == 1);
	}
	//
	// Invoke either the increment or decrement procedure
	s3ins_run (squeal->s3db,
			(add_not_del == PULLEY_TUPLE_ADD)
				? squeal->inc_drv_all
				: squeal->dec_drv_all,
			hash, drvback->cbnumparm, drvback->cbparm);
	//
	// Possibly invoke the callback on the backend driver
	DEBUG("  DEMULT rpt=%d drive=%d cbfun=%p cbdata=%p\n", repeats, drive, (void *)drvback->cbfun, drvback->cbdata);
	if (drive && drvback->cbfun) {
		drvback->cbfun (drvback->cbdata, add_not_del,
				drvback->cbnumparm, drvback->cbparm);
	}
}


/* Generators iterate over drivers, and produce output in each.  This function does
 * the latter -- it runs an output production routine, into which the generator's
 * forked record is supplied, and from which a set of tuples is drawn that can
 * each be supplied separately as output tuples.  So we iterate over the output
 * tuples produced, and hand over each produced one to the driver backend.
 */
static void squeal_produce_output (struct squeal *squeal, int add_not_del,
				struct s3ins_gen2drv *gen2drv, s3key_t genhash,
				int numgenvars, struct squeal_blob *genvars) {
	struct s3ins_driver *drv = gen2drv->driver;
	sqlite3_stmt *s3in = gen2drv->gen2drv_produce;
	int s3rv;
	int i;
	s3rv = s3ins_run (squeal->s3db, s3in, genhash, numgenvars, genvars);
	while (s3rv != SQLITE_DONE) {
		if (s3rv != SQLITE_OK) {
			//TODO// Report SQLite3 error
			ERROR("SQLite3 ERROR while producing output\n");
		}
		assert (sqlite3_column_count (s3in) == drv->cbnumparm);
		for (i=0; i < drv->cbnumparm; i++) {
			drv->cbparm [i].data = (void *) sqlite3_column_blob  (s3in, i);
			drv->cbparm [i].size = (size_t) sqlite3_column_bytes (s3in, i);
		}
		squeal_driver_callback_demult (squeal, drv, add_not_del);
		sqlite3_step (s3in);
	}
}


/* Process a new tuple fork in a generator.  This either reflects add or delete of
 * a tuple.  The generator will iterate over all the drivers that are interested,
 * and run the respective output generator on each.  In addition, if the generator
 * is/has co-generator, which means that it has the optional routines for adding
 * and removing the tuple to its own gen_xxx table, it will invoke the respective
 * routine to be of use for its future role as a co-generator.
 */
static void _squeal_generator_fork (struct squeal *squeal,
			struct s3ins_generator *genfront, int add_not_del,
			int numrecvars, struct squeal_blob *recvars) {
	int d;
	int i;
	assert (genfront->numrecvars == numrecvars);

	DEBUG("Generator fork @%p add?%d\n", (void *)genfront, add_not_del);
	DEBUG("  .. adding %d variables.", numrecvars);

	//
	// Produce the generator hash over the blobs holding the tuple variables
	s3key_t genhash = S3KEY_INIT;
	for (i=0; i < numrecvars; i++) {
		s3key_add_blob (&genhash, &recvars [i]);
	}
	//
	// If the tuple should be deleted, do that before producing output variables
	if ((add_not_del == PULLEY_TUPLE_DEL) && (genfront->opt_gen_del_tuple != NULL)) {
		s3ins_run (squeal->s3db, genfront->opt_gen_del_tuple,
			genhash, numrecvars, recvars);
	}
	//
	// Iterate over the generator's drivers, producing output for each in turn
	for (d=0; d < genfront->numdriveout; d++) {
		// Produce a set output variable tuples and present each to the driver
		squeal_produce_output (squeal, add_not_del,
				&genfront->driveout [d], genhash,
				numrecvars, recvars);
	}
	//
	// If the tuple should be added, do that after producing output variables
	if ((add_not_del == PULLEY_TUPLE_ADD) && (genfront->opt_gen_add_tuple != NULL)) {
		s3ins_run (squeal->s3db, genfront->opt_gen_add_tuple,
			genhash, numrecvars, recvars);
	}
}

void squeal_generator_fork(struct squeal *squeal, gennum_t gennum, int add_not_del, int numrecvars, struct squeal_blob *recvars)
{
	_squeal_generator_fork(squeal, &(squeal->gens[gennum]), add_not_del, numrecvars, recvars);
}

static void _squeal_fork(struct squeal *squeal, gennum_t gennum, const char *entryUUID, int add_not_del, int numrecvars, struct squeal_blob *recvars)
{
	//TODO// This code is never referenced
	int sqlret;
	unsigned int driveridx, columnidx;
	struct s3ins_generator* genfront = &(squeal->gens[gennum]);

	if (add_not_del)
	{
		assert (genfront->numrecvars == numrecvars);
		sqlret = s3ins_run_uuid(squeal->s3db, genfront->opt_gen_add_tuple, entryUUID, numrecvars, recvars);

		if ((sqlret != SQLITE_OK) && (sqlret != SQLITE_DONE))
		{
			ERROR("Can't insert fork SQL err %d %s\n", sqlret, sqlite3_errmsg(squeal->s3db));
		}
	}

	for (driveridx=0; driveridx < genfront->numdriveout; driveridx++)
	{
		sqlite3_stmt *statement = genfront->driveout[driveridx].gen2drv_produce;
		struct squeal_blob *params = genfront->driveout[driveridx].driver->cbparm;
		unsigned int rowcount = 1;
		sqlret = s3ins_run_uuid(squeal->s3db, statement, entryUUID, 0, NULL);
		while (sqlret != SQLITE_DONE) {
			if ((sqlret != SQLITE_OK) && (sqlret != SQLITE_ROW))
			{
				ERROR("SQLite3 ERROR while producing output for uuid %s sql %d %s\n", entryUUID, sqlret, sqlite3_errmsg(squeal->s3db));
				break;
			}

			DEBUG("Output row %d to driver %d\n", rowcount, driveridx);
			for (columnidx=0; columnidx < sqlite3_column_count(statement); columnidx++) {
				params[columnidx].data = (void *)sqlite3_column_blob(statement, columnidx);
				params[columnidx].size = sqlite3_column_bytes(statement, columnidx);
			}

			squeal_driver_callback_demult(squeal, genfront->driveout[driveridx].driver, add_not_del);
			sqlret = sqlite3_step(statement);

			rowcount++;
		}
	}

	if (!add_not_del)
	{
		int sqlret = s3ins_run_uuid(squeal->s3db, genfront->opt_gen_del_tuple, entryUUID, 0, NULL);

		if ((sqlret != SQLITE_OK) && (sqlret != SQLITE_DONE))
		{
			ERROR("Can't delete fork SQL err %d %s\n", sqlret, sqlite3_errmsg(squeal->s3db));
		}
	}
}

void squeal_insert_fork(struct squeal* squeal, gennum_t gennum, const char* entryUUID, int numrecvars, struct squeal_blob* recvars)
{
	//TODO// This code is never referenced
	_squeal_fork(squeal, gennum, entryUUID, PULLEY_TUPLE_ADD, numrecvars, recvars);
}

void squeal_delete_forks(struct squeal *squeal, gennum_t gennum, const char *entryUUID)
{
	//TODO// This code is never referenced
	_squeal_fork(squeal, gennum, entryUUID, PULLEY_TUPLE_DEL, 0, NULL);
}


/********** BACKEND STRUCTURE CREATION **********/


/* Produce the SQLite3 text form of an expression variable depending on its varkind:
 *  - VARIABLE not in param are inserted in ?003 form
 *  - VARIABLE in param such as x, y are inserted with a var_ prefix
 *  - CONSTANT INTEGER and FLOAT are printed by C
 *  - CONSTANT STRING is printed with single quotes; internal single quote is doubled
 *  - CONSTANT BLOB is printed as X'(hexdump)'
 *  - ATTRTYPE, PARAMETER, DRIVERNAME, BINDING should not occur in expressions
 */
void squeal_produce_expression_variable (struct sqlbuf *sql, struct vartab *vartab, bitset_t *params, varnum_t vn) {
	varkind_t vk = var_get_kind (vartab, vn);
	struct var_value *vv = var_share_value (vartab, vn);
	char linebuf [90];
	char varid [20];
	int i;
	char *ptr;
	switch (vk) {
	//
	// Plain variables; may be parameters that are produced by the current generator or not
	case VARKIND_VARIABLE:
		if (!bitset_test (params, vn)) {
			/* No need to prefix var_ or anything, separate id space */
			snprintf (varid, sizeof (varid)-1, "?%03d", PARAM_OFS + vn);
			sqlbuf_write (sql, varid);
		} else {
			sqlbuf_write (sql, "var_");
			sqlbuf_write (sql, var_get_name (vartab, vn));
		}
		break;
	//
	// Constants; forms are INTEGER, FLOAT, STRING, BLOB
	case VARKIND_CONSTANT:
		switch (vv->type) {
		case VARTP_INTEGER:
			snprintf (linebuf, sizeof (linebuf)-1, "%d", vv->typed_integer);
			sqlbuf_write (sql, linebuf);
			break;
		case VARTP_FLOAT:
			snprintf (linebuf, sizeof (linebuf)-1, "%f", vv->typed_float);
			sqlbuf_write (sql, linebuf);
			break;
		case VARTP_STRING:
			// Format is 'How''s your mood today?'
			// Input is NUL-terminated, and flex already
			// took care of input quoting and escaping
			sqlbuf_write (sql, vv->typed_string);
			sqlbuf_write (sql, "'");
			ptr = vv->typed_string;
			while (1) {
				i = 0;
				while (ptr [i] && (ptr [i] != '\'')) {
					i++;
				}
				sqlbuf_writeblob (sql, ptr, i);
				if (ptr [i] == '\'') {
					sqlbuf_write (sql, "''");
					ptr += i + 1;
				} else {
					// ptr [i] == '\0'
					break;
				}
			}
			sqlbuf_write (sql, "'");
			break;
		case VARTP_BLOB:
			// Format is X'0123456789abcdef'
			sqlbuf_write (sql, "X'");
			ptr = vv->typed_blob.ptr;
			i   = vv->typed_blob.len;
			while (i-- > 0) {
				sprintf (linebuf, "%02X", *ptr++);
				sqlbuf_write (sql, linebuf);
			}
			sqlbuf_write (sql, "'");
			break;
		default:
			fprintf (stderr, "Unsupported variable type (%d)\n", vv->type);
			exit (1);
		}
		break;
	//
	// Misplaced ATTRTYPE, PARAMETER, DRIVERNAME, BINDING
	case VARKIND_ATTRTYPE:
	case VARKIND_PARAMETER:
	case VARKIND_DRIVERNAME:
	case VARKIND_BINDING:
		fprintf (stderr, "Unwanted variable kind in expression (%d)\n", vk);
		exit (1);
	//
	// We should never run into unknown variable kinds
	default:
		fprintf (stderr, "Unknown variable kind (%d)\n", vk);
		exit (1);
	}
}

/* TODO: Mockup code to produce expressions for driver output generation
 */
void squeal_produce_expression (struct sqlbuf *sql, struct vartab *vartab, bitset_t *params, int *exp, size_t explen) {
	int operator, operands;
	int *subexp;
	size_t subexplen;
	varnum_t v1, v2;
	cnd_parse_operation (exp, explen, &operator, &operands, &subexp, &subexplen);
	switch (operator) {
	case CND_TRUE:
		sqlbuf_write (sql, "1=1");
		break;
	case CND_FALSE:
		sqlbuf_write (sql, "1=0");
		break;
	case CND_NOT:
		sqlbuf_write (sql, "(NOT ");
		squeal_produce_expression (sql, vartab, params, subexp, subexplen);
		sqlbuf_write (sql, ")");
		break;
	case CND_AND:
	case CND_OR:
		sqlbuf_write (sql, "(");
		exp = subexp;
		explen = subexplen;
		while (operands-- > 0) {
			cnd_parse_operand (&exp, &explen, &subexp, &subexplen);
			squeal_produce_expression (sql, vartab, params, subexp, subexplen);
			if (operands > 0) {
				sqlbuf_write (sql, (operator==CND_AND)? " AND ": " OR ");
			}
		}
		sqlbuf_write (sql, ")");
		break;
	case CND_EQ:
	case CND_NE:
	case CND_LT:
	case CND_GT:
	case CND_LE:
	case CND_GE:
		v2 = cnd_parse_variable (&subexp, &subexplen);
		v1 = cnd_parse_variable (&subexp, &subexplen);
		squeal_produce_expression_variable (sql, vartab, params, v1);
		sqlbuf_write (sql,
			(operator == CND_EQ)? " = ":
			(operator == CND_NE)? " <> ":
			(operator == CND_LT)? " < ":
			(operator == CND_GT)? " > ":
			(operator == CND_LE)? " <= ":
			(operator == CND_GE)? " >= ":
			                      " ERROR ");
		squeal_produce_expression_variable (sql, vartab, params, v2);
		break;
	default:
		ERROR("Unknown operation code %d with %d operands\n", operator, operands);
		exit (1);
	}
}

/* Construct an SQL query that produces output for driver d when generator g
 * forks a tuple.  Note that addition or removal is not an issue yet; the
 * desired output from the query is a list of additional variables.  Return
 * the prepared statement for this SQL query.
 * TODO: Mockup code to produce expressions for driver output generation
 * TODO: Create generator's opt_gen_add_tuple / opt_gen_del_tuple code too
 */
sqlite3_stmt *squeal_produce_outputs (struct squeal *squeal, struct drvtab *drvtab, gennum_t gennum, drvnum_t drvnum) {
	sqlite3_stmt *retval = NULL;
	struct sqlbuf sql;
	char varid [20];
	int s3rv;
	char *comma;
	varnum_t *outarray;
	varnum_t  outcount;
	bitset_t *itbits;
	bitset_iter_t it;
	struct vartab *vartab;
	struct cndtab *cndtab;
	struct gentab *gentab;
	int *exp;
	size_t explen;
	gennum_t cogen;
	bitset_t *params;
	int i;
	//
	// Grab a write buffer
	sqlbuf_exchg (&sql, BUF_GET);
	//
	// Construct additional types
	vartab = vartab_from_type (drvtab_share_vartype (drvtab));
	cndtab = cndtab_from_type (drvtab_share_cndtype (drvtab));
	gentab = gentab_from_type (drvtab_share_gentype (drvtab));
	params = gen_share_variables (gentab, gennum);

	//
	// First, construct "SELECT v0,v1,v2" -- the driver's output but not guards
	//TODO: Use driver parameter names: "SELECT v0 AS p0,v1 AS p1, v2 AS p2"
	drv_share_output_variable_table (drvtab, drvnum, &outarray, &outcount);
	// There should always be at least one output variable:
	assert (outcount > 0);
	//
	// In passing, allocate space for the output parameters now we know
	// how many there will be.
	squeal->drivers[drvnum].cbparm = calloc(outcount, sizeof(struct squeal_blob));
	squeal->drivers[drvnum].cbnumparm = outcount;
	squeal->drivers[drvnum].drvall_prehash = drv_get_hash(drvtab, drvnum);
	//
	// Having collected data, produce the "SELECT ..." string
	comma = "SELECT ";
	for (i=0; i<outcount; i++) {
		sqlbuf_write (&sql, comma);
		if (!bitset_test (params, outarray [i])) {
			/* No need to prefix var_ or anything, separate id space */
			snprintf (varid, sizeof (varid)-1, "?%03d", PARAM_OFS + outarray [i]);
			sqlbuf_write (&sql, varid);
		} else {
			sqlbuf_write (&sql, "var_");
			sqlbuf_write (&sql, var_get_name (vartab, outarray [i]));
		}
		comma = ",";
	}

	//
	// Second, construct "FROM g0,g1,g2" -- the tables with co-generators
	itbits = drv_share_generators (drvtab, drvnum);
	//
	// We should have bits if there are co-generators
	// Without any co-generators, there will be no FROM clause in this SQL query
	assert (!bitset_isempty (itbits));
	sqlbuf_write(&sql, "\nFROM   ");
	sqlbuf_lexhash2name (&sql, "gen_", gen_get_hash(gentab, gennum));
	bitset_iterator_init (&it, itbits);
	while (bitset_iterator_next_one (&it, NULL)) {
		cogen = bitset_iterator_bitnum (&it);
		if (cogen != gennum) {
			//
			// We use NATURAL JOIN, and leave it to the optimiser to find
			// that no variable names are actually overlapping.  This may
			// seem silly, but if we (ever) decide to grant variable names
			// to occur in multiple generators, and their semantics would
			// be a match, then we benefit from a NATURAL JOIN.  (Checked
			// version 3.8.10.2, and indeed SQLite3 is clever enough.)
			sqlbuf_write (&sql, " NATURAL JOIN\n       ");
			sqlbuf_lexhash2name (&sql, "gen_", gen_get_hash (gentab, cogen));
		}
	}

	//
	// Third, iterate over varpartitions and find their associated conditions
	itbits = drv_share_conditions (drvtab, drvnum); /* 0 conditions is acceptable */
	sqlbuf_write(&sql, "\nWHERE  entryUUID = :uuid");
	comma = "\nAND    ";
	bitset_iterator_init (&it, itbits);
	while (bitset_iterator_next_one (&it, NULL)) {
		sqlbuf_write (&sql, comma);
		cnd_share_expression (cndtab, bitset_iterator_bitnum (&it), &exp, &explen);
		squeal_produce_expression (&sql, vartab, params, exp, explen);
	}

	//
	// Based on the generated SQL string, prepare a statement
	if (sqlite3_prepare (squeal->s3db, sql.buf, sql.ofs, &retval, NULL) != SQLITE_OK) {
		ERROR("Failed to construct production rule for SQLite3 engine: %s\n%.*s",
						sqlite3_errmsg (squeal->s3db),
						sql.ofs, sql.buf);
		retval = NULL;
		goto cleanup;
	}
	DEBUG("prep sql>\n%.*s\n\n", (int) sql.ofs, sql.buf);

cleanup:
	//
	// Release the SQL buffer
	sqlbuf_exchg (&sql, BUF_PUT);
	//
	// Return the prepared statement
	return retval;
}


/* Create type descriptions in the present database.  Indicate whether pre-existing
 * tables may be reused.  If not, they will be dropped if they already exist.
 * Return 0 on success, 1 on failure.
 * TODO: Mockup code to produce expressions for driver output generation
 */
int squeal_have_tables (struct squeal *squeal, struct gentab *gentab, bool may_reuse) {
	struct sqlbuf sql;
	int retval = 0;
	bitset_t *itbits;
	bitset_iter_t it;
	gennum_t numgens, g;
	varnum_t v;
	struct vartab *vartab;
	int varcount = 0;  // For each generator, the number of columns in gen_<hash> table

	//
	// Fetch a SQL write buffer
	sqlbuf_exchg (&sql, BUF_GET);
	//
	// Retrieve additional types
	vartab = vartab_from_type (gentab_share_variable_type (gentab));
	//
	// Create the drv_all table -- do not create entries due to no (new) output
	if (!may_reuse) {
		sqlbuf_write (&sql, "DROP TABLE IF EXISTS drv_all");
		retval = retval || sqlbuf_run (&sql, squeal->s3db);
	}
	sqlbuf_write (&sql, "CREATE TABLE IF NOT EXISTS drv_all (\n"
				"\tout_hash   INTEGER PRIMARY KEY NOT NULL,\n"
				"\tout_repeat INTEGER NOT NULL)");
	retval = retval || sqlbuf_run (&sql, squeal->s3db);
	//
	// Create the trigger that removes zero values for out_repeat from drv_all
	if (!may_reuse) {
		sqlbuf_write (&sql, "DROP TRIGGER IF EXISTS drv_all_dropzero");
		retval = retval || sqlbuf_run (&sql, squeal->s3db);
	}
	sqlbuf_write (&sql, "CREATE TRIGGER IF NOT EXISTS drv_all_dropzero\n"
				"AFTER UPDATE ON drv_all\n"
				"WHEN NEW.out_repeat = 0\n"
				"BEGIN DELETE FROM drv_all\n"
				"      WHERE out_repeat = 0;\n"
				"END");
	retval = retval || sqlbuf_run (&sql, squeal->s3db);
	//
	// Create a table holding the cookie for LDAP SyncRepl
	if (!may_reuse) {
		sqlbuf_write (&sql, "DROP TABLE IF EXISTS syncrepl_cookie");
		retval = retval || sqlbuf_run (&sql, squeal->s3db);
	}
	sqlbuf_write (&sql, "CREATE TABLE IF NOT EXISTS syncrepl_cookie (\n"
				"\ttimestamp INTEGER PRIMARY KEY NOT NULL,\n"
				"\tcookie BLOB)");
	retval = retval || sqlbuf_run (&sql, squeal->s3db);
	//
	// Create a table for each generator that is/has a co-generator
	numgens = gentab_count (gentab);
	for (g=0; g<numgens; g++) {
		itbits = gen_share_variables (gentab, g);
		varcount = 0;
#if 0
// In light of the considerations about the resync logic, store all generator tuples
// even when they are directly transformed into driver output and never used as/with
// co-generators.
		if (!gen_get_cogeneration (gentab, g)) {
			/* Not used as co-generator, so no use for storage */
			continue;
		}
#endif
		if (bitset_isempty (itbits)) {
			/* No variables generated, so no use for storage */
			continue;
		}
		if (!may_reuse) {
			sqlbuf_write (&sql, "DROP TABLE IF EXISTS ");
			sqlbuf_lexhash2name (&sql, "gen_", gen_get_hash (gentab, g));
			retval = retval || sqlbuf_run (&sql, squeal->s3db);
		}
		sqlbuf_write (&sql, "CREATE TABLE IF NOT EXISTS ");
		sqlbuf_lexhash2name (&sql, "gen_", gen_get_hash (gentab, g));
		sqlbuf_write (&sql, " (\n\t");
		sqlbuf_write (&sql, "entryUUID CHAR(36)");
		bitset_iterator_init (&it, itbits);
		while (bitset_iterator_next_one (&it, NULL)) {
			v = bitset_iterator_bitnum (&it);
			if (var_get_kind (vartab, v) != VARKIND_VARIABLE) {
				continue;
			}
			sqlbuf_write (&sql, ",\n\tvar_");
			sqlbuf_write (&sql, var_get_name (vartab, v));
			sqlbuf_write (&sql, " BLOB NOT NULL");
			varcount++;
		}
		sqlbuf_write (&sql, ")");
		retval = retval || sqlbuf_run (&sql, squeal->s3db);
		sqlbuf_write (&sql, "CREATE INDEX IF NOT EXISTS ");
		sqlbuf_lexhash2name (&sql, "idx_", gen_get_hash (gentab, g));
		sqlbuf_lexhash2name (&sql, "\n\tON gen_", gen_get_hash (gentab, g));
		sqlbuf_write (&sql, " (entryUUID)");
		retval = retval || sqlbuf_run (&sql, squeal->s3db);

		squeal->gens[g].numrecvars = varcount;
	}
	//
	// Release the SQL buffer
	sqlbuf_exchg (&sql, BUF_PUT);
	//
	// Provide the return value
	return retval;
}


int squeal_configure_generators(struct squeal* squeal, struct gentab* gentab, struct drvtab* drvtab)
{
	int gennum, drvnum;
	int varnum;
	int sqlretval;
	char paramstr[20];

	struct sqlbuf sql;
	for (gennum=0; gennum < squeal->numgens; gennum++)
	{
		struct s3ins_generator* gen = &(squeal->gens[gennum]);

		sqlbuf_exchg (&sql, BUF_GET);

		sqlbuf_write (&sql, "DELETE FROM ");
		sqlbuf_lexhash2name(&sql, "gen_", gen_get_hash(gentab, gennum));
		sqlbuf_write(&sql, " WHERE entryUUID = :uuid");

		if ((sqlretval = sqlite3_prepare(squeal->s3db, sql.buf, sql.ofs, &gen->opt_gen_del_tuple, NULL)) != SQLITE_OK)
		{
			ERROR("PREP ERROR delete in generator SQL %d: %s\n", sqlretval, sqlite3_errmsg (squeal->s3db));
			goto fail;
		}

		sql.ofs = 0;
		sqlbuf_write(&sql, "INSERT INTO ");
		sqlbuf_lexhash2name(&sql, "gen_", gen_get_hash(gentab, gennum));
		sqlbuf_write(&sql, " VALUES (:uuid");  // One parameter is always the entryUUID
		for (varnum=0; varnum < gen->numrecvars; varnum++)
		{
			snprintf (paramstr, sizeof (paramstr)-1, ",?%03d", PARAM_OFS + varnum+2);
			sqlbuf_write(&sql, paramstr);
		}
		sqlbuf_write(&sql, ")");

		if ((sqlretval = sqlite3_prepare(squeal->s3db, sql.buf, sql.ofs, &gen->opt_gen_add_tuple, NULL)) != SQLITE_OK)
		{
			ERROR("PREP ERROR insert in generator SQL %d: %s\n", sqlretval, sqlite3_errmsg (squeal->s3db));
			sqlite3_finalize(gen->opt_gen_del_tuple);
			gen->opt_gen_del_tuple = NULL;
			goto fail;
		}

		bitset_t *drvs = gen_share_driverout(gentab, gennum);
		unsigned int numdrivers = bitset_count(drvs);
		gen->driveout = calloc(numdrivers, sizeof(struct s3ins_gen2drv));
		gen->numdriveout = numdrivers;

		bitset_iter_t it;
		unsigned int drvindex = 0;
		bitset_iterator_init(&it, drvs);
		while (bitset_iterator_next_one (&it, NULL)) {
			gen->driveout[drvindex].driver = &(squeal->drivers[bitset_iterator_bitnum(&it)]);
			gen->driveout[drvindex].gen2drv_produce = squeal_produce_outputs(squeal, drvtab, gennum, bitset_iterator_bitnum(&it));
			drvindex++;
		}
	}

	sqlbuf_exchg (&sql, BUF_PUT);
	return 0;

fail:
	sqlbuf_exchg (&sql, BUF_PUT);
	// TODO: cleanup already-prepared statements up to gennum-1
	return 1;
}

int squeal_configure_driver(struct squeal* squeal, drvnum_t drv, squeal_driverfun_t cbfun, void* cbdata)
{
	DEBUG("DRV configured %d %p %p\n", drv, (void *)cbfun, cbdata);
	squeal->drivers[drv].cbdata = cbdata;
	squeal->drivers[drv].cbfun = cbfun;
	return 0;
}

int squeal_configure (struct squeal *squeal) {
	struct sqlbuf sql;
	int retval = 0;
	int sqlretval = -1;

	//
	// Grab an SQL buffer
	sqlbuf_exchg (&sql, BUF_GET);
	//
	// Fetch a drv_all's out_repeat for a given out_hash, defaulting to 0:
	sqlbuf_write (&sql, "SELECT max (out_repeat)\n"
			    "FROM ( SELECT out_repeat\n"
			    "       FROM drv_all\n"
			    "       WHERE out_hash = :hash\n"
			    "       UNION VALUES (0) )");
	if ((sqlretval = sqlite3_prepare (squeal->s3db, sql.buf, sql.ofs, &squeal->get_drv_all, NULL)) != SQLITE_OK) {
		ERROR("PREP ERROR select in SQL %d: %s\n", sqlretval, sqlite3_errmsg (squeal->s3db));
		retval = 1;
		goto cleanup;
	}
	sql.ofs = 0;
	//
	// Increment a drv_all's out_repeat for a given out_hash:
	sqlbuf_write (&sql, "INSERT OR REPLACE INTO drv_all\n"
			    "SELECT :hash, 1 + max (out_repeat)\n"
			    "FROM ( SELECT out_repeat\n"
			    "       FROM drv_all\n"
			    "       WHERE out_hash = :hash\n"
			    "       UNION VALUES (0) )");
	if ((sqlretval = sqlite3_prepare (squeal->s3db, sql.buf, sql.ofs, &squeal->inc_drv_all, NULL)) != SQLITE_OK) {
		ERROR("PREP ERROR insert in SQL %d\n", sqlretval);
		retval = 1;
		goto cleanup;
	}
	sql.ofs = 0;
	//
	// Decrement a drv_all's out_repeat for a given out_hash:
	sqlbuf_write (&sql, "UPDATE drv_all\n"
				"SET out_repeat = out_repeat - 1\n"
				"WHERE out_hash = :hash");
	if ((sqlretval = sqlite3_prepare (squeal->s3db, sql.buf, sql.ofs, &squeal->dec_drv_all, NULL)) != SQLITE_OK) {
		ERROR("PREP ERROR update in SQL %d\n", sqlretval);
		retval = 1;
		goto cleanup;
	}
	sql.ofs = 0;
cleanup:
	//
	// Release the SQL buffer
	sqlbuf_exchg (&sql, BUF_GET);

	//
	// Provide the return value
	return retval;
}

void errorLogCallback(void *pArg, int iErrCode, const char *zMsg){
	ERROR("(%d) %s\n", iErrCode, zMsg);
}

void traceLogCallback(void *pData, const char *stmt)
{
	DEBUG("(%p) %s\n", pData, stmt);
}

/**
 * Reset @p buf and write dbdir in it; ensure that it ends
 * with a / and is NUL terminated (which is not what you'd usually
 * get with sqlbuf).
 */
void _copy_dbdir(struct sqlbuf* buf, const char *dbdir)
{
	buf->ofs = 0;
	sqlbuf_write(buf, dbdir);
	if (buf->buf[buf->ofs-1] == '/')
	{
		sqlbuf_write(buf, "0");
	}
	else
	{
		sqlbuf_write(buf, "/0");
	}
	buf->buf[--(buf->ofs)] = 0;  /* NUL terminated */
}

/* Open a SQLite3 engine for a given Pulley script.  The lexhash is used to name the
 * database in the configured database directory.  The database is created if it does
 * not exist yet.  The database directory is also created if it does not exist yet;
 * if so, its mode is set to 01777 (ugo+rwx with the sticky(8) bit set)
 * The return value NULL indicates an error opening the database.
 *
 * The database is placed in the directory named @p dbdir; if @p dbdir is NULL,
 * then an in-memory database is used instead. This is not recommended.
 *
 * TODO: Consider use of sticky bits, as are used for /tmp
 * TODO: Privileges of the database itself?
 */
struct squeal *squeal_open_in_dbdir (hash_t lexhash, gennum_t numgens, drvnum_t numdrvs, const char *dbdir) {
	struct squeal *retval = NULL, *work = NULL;
	sqlite3 *s3db = NULL;
	struct sqlbuf dbname;

	DEBUG("squeal_open with %d gen %d drv in '%s'", numgens, numdrvs, dbdir);
	//
	// Allocate the memory structures for the squeal backend
	work = calloc (1, sizeof (struct squeal)
				+ (numgens-1) * sizeof (struct s3ins_generator));
	if (work == NULL) {
		return NULL;
	}
	work->numgens = numgens;
	work->drivers = calloc (numdrvs, sizeof (struct s3ins_driver));
	work->numdrivers = numdrvs;
	if (work->drivers == NULL) {
		free (work);
		return NULL;
	}
	//
	// Fetch a buffer for fun and play
	sqlbuf_exchg (&dbname, BUF_GET);
	//
	// Construct the filename of the database, possibly create the directory
	if (dbdir) {
		_copy_dbdir(&dbname, dbdir);
		mkdir (dbname.buf, 01777);		// Best effort.  Mode u+rwx, g+rx, o+
		sqlbuf_lexhash2name (&dbname, "pulley_", lexhash);
		sqlbuf_write (&dbname, ".sqlite30");
		dbname.buf [dbname.ofs-1] = '\0';	// Setup  trailing NUL for use with C
	} else {
		sqlbuf_write (&dbname, ":memory:0");
		dbname.buf [dbname.ofs-1] = '\0';	// Setup  trailing NUL for use with C
	}
	//
	// Open the SQLite3 database file
	//
	// TODO: may want to add SQLITE_OPEN_URI flag to allow file: and similar URIs
	int s3rv = sqlite3_open_v2 (dbname.buf, &s3db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, NULL);
	DEBUG("SQLite3 open (\"%s\", &s3db) returned %d, hoped for %d\n", dbname.buf, s3rv, SQLITE_OK);
	if (s3rv != SQLITE_OK) {
		/* TODO: Report detailed error */
		if (s3db != NULL) {
			sqlite3_close(s3db);
		}
		free (work->drivers);
		free (work);
		retval = NULL;
	} else {
		work->s3db = s3db;
		retval = work;
		sqlite3_config(SQLITE_CONFIG_LOG, errorLogCallback, NULL);
		// sqlite3_trace(s3db, traceLogCallback, NULL);
	}
	sqlbuf_exchg (&dbname, BUF_PUT);
	//
	// Return the allocated structure
	return retval;
}

struct squeal *squeal_open (hash_t lexhash, gennum_t numgens, drvnum_t numdrvs) {
	return squeal_open_in_dbdir(lexhash, numgens, numdrvs, squeal_use_dbdir);
}

/* Close a SQLite3 engine using the handle that was returned by squeal_open().
 */
void squeal_close (struct squeal *squeal) {
	sqlite3_close (squeal->s3db);
	for (unsigned int drvnum = 0; drvnum < squeal->numdrivers; drvnum++)
	{
		free(squeal->drivers[drvnum].cbparm);
		squeal->drivers[drvnum].cbparm = NULL;
	}
	free (squeal->drivers);
	for (unsigned int i=0; i<squeal->numgens; i++)
	{
		free(squeal->gens[i].driveout);
		squeal->gens[i].driveout = NULL;
	}
	free (squeal);
}

/* Unlink a SQLite3 engine for a given Pulley script lexhash.  As with any other
 * file, there is a chance that the file has a hard link and is kept around for that.
 */
void squeal_unlink_in_dbdir (hash_t lexhash, const char *dbdir) {
	if (!dbdir) {
		return;
	}
	struct sqlbuf dbname;
	sqlbuf_exchg (&dbname, BUF_GET);
	_copy_dbdir(&dbname, dbdir);
	sqlbuf_lexhash2name (&dbname, "pulley_", lexhash);
	sqlbuf_write (&dbname, "0");
	dbname.buf [dbname.ofs-1] = '\0';	// Modify for use as a C string
	unlink (dbname.buf);			// Best effort
	sqlbuf_exchg (&dbname, BUF_PUT);
}

void squeal_unlink (hash_t lexhash) {
	squeal_unlink_in_dbdir(lexhash, squeal_use_dbdir);
}

