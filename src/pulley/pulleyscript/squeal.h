#ifndef PULLEYSCRIPT_SQUEAL_H
#define PULLEYSCRIPT_SQUEAL_H


#ifdef __cplusplus
extern "C" {
#endif

/* Declare the opaque structure that is used in the squeal engine.
 */
struct squeal;


/* The squeal_blob is a storage structure for a variable, such as it is deliverd
 * to external driver modules.  When registering, a module supplies an array of
 * these structures to serve for output.  The structure is also explicitly passed
 * to the driver upon callback.
 */
struct squeal_blob {
	void *data;
	size_t size;
};

/* The squeal_driverfun_t defines the callback function expected by squeal from
 * any driver.  It will be invoked with an opaque cbdata value to be registered
 * by the driver, a parameter add_not_del set to either PULLEY_RECORD_ADD or
 * PULLEY_RECORD_DEL and both the number and values of the blobs holding the
 * actual parameters to the driver.
 */
typedef void (*squeal_driverfun_t) (void *cbdata, int add_not_del,
			int numactpart, struct squeal_blob *actparm);

/* Values for the add_not_del function parameters:
 */
#define PULLEY_RECORD_ADD 1
#define PULLEY_RECORD_DEL 0


/* Open a SQLite3 engine for a given Pulley script.  The lexhash is used to name the
 * database in the configured database directory.  The database is created if it does
 * not exist yet.  The database directory is also created if it does not exist yet;
 * if so, its mode is set to 01777 (ugo+rwx with the sticky(8) bit set)
 * The return value NULL indicates an error opening the database.
 * TODO: Consider use of sticky bits, as are used for /tmp
 * TODO: Privileges of the database itself?
 */
struct squeal *squeal_open (hash_t lexhash, gennum_t numgens, drvnum_t numdrvs);
struct squeal *squeal_open_in_dbdir (hash_t lexhash, gennum_t numgens, drvnum_t numdrvs, const char *dbdir);

/* Close a SQLite3 engine using the handle that was returned by squeal_open().
 */
void squeal_close (struct squeal *s3db);

/* Unlink a SQLite3 engine for a given Pulley script lexhash.  As with any other
 * file, there is a chance that the file has a hard link and is kept around for that.
 */
void squeal_unlink (hash_t lexhash);
void squeal_unlink_in_dbdir (hash_t lexhash, const char *dbdir);

/* Create type descriptions in the present database.  Indicate whether pre-existing
 * tables may be reused.  If not, they will be dropped if they already exist.
 * Return 0 on success, 1 on failure.
 */
int squeal_have_tables (struct squeal *s3db, struct gentab *gentab, bool may_reuse);

/**
 * Prepare statements that manipulate the drv_all table;
 * these count the number of uses of each out_hash.
 */
int squeal_configure (struct squeal *squeal);

int squeal_configure_generators(struct squeal* squeal, struct gentab* gentab);

#ifdef __cplusplus
}
#endif

#endif

