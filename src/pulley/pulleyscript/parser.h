
#include "lexhash.h"
#include "bitset.h"

enum parser_type {
	PARSER_SCRIPT,
	PARSER_LINE,
	PARSER_LINE_GENERATOR,
	PARSER_LINE_CONDITION,
	PARSER_LINE_DRIVEROUT,
};

#define PARSER_VARIABLE_STACK_ENTRIES 4

struct parser {
	void *lexer;
	hashbuf_t scanhashbuf;
	hash_t scanhash;
	int bison_switch;
	int varstack_sp;
	bitset_t *varstack [PARSER_VARIABLE_STACK_ENTRIES];
	struct vartab *vartab;
	struct gentab *gentab;
	struct cndtab *cndtab;
	struct drvtab *drvtab;
	bool weight_set;
	float weight;
	drvnum_t newdrv;
	cndnum_t newcnd;
};

void fatal_error (const char* msg);


/* Initialise a parser buffer for collection of data.  This routine allocates
 * basic tables for variables, generators, conditions and drivers; and it
 * connects them.  Failure to allocate memory leads to halting the program.
 * Success is returned as a non-zero value.
 */
int pulley_parser_init (struct parser *prs);

/* Cleanup the (semantical) structures collected during parsing, and possibly
 * processed further.  The data for Bison and Flex are stored inside the
 * parser structure, and they will be cleaned up inasfar as necessary.
 *
 * It is assumed that pulley_parser_init() has succeeded, meaning that the
 * internal storage structures were all allocated successfully.
 *
 * The parsed semantics, notably vartab/gentab/cndtab/drvtab, will continue
 * to exist until these are separately cleaned up.
 */
void pulley_parser_cleanup_syntax (struct parser *prs);

/* Cleanup the semantical part of the parser structures.  This eradicates
 * the last traces of a parsing process.
 *
 * It is assumed that pulley_parser_init() and pulley_parser_cleanup_syntax()
 * have both run.
 */
void pulley_parser_cleanup_semantics (struct parser *prs);


/* Parse a (piece of a) Pulley script that is stored in a memory buffer.
 *
 * The memory buffer may be directly taken from a struct berval that has
 * arrived over LDAP.  They will be combined to form the larger whole of
 * the complete Pulley script.
 */
int pulley_parser_buffer (struct parser *prs, enum parser_type pt, char *buf, unsigned int buflen);

/* Parse a Pulley script that is stored in a file.  Assume that the file is
 * opened at the proper position to be parsed, and that parsing continues to
 * the end of file.
 *
 * Multiple files may be parsed.  They will be combined to form the larger
 * whole of the complete Pulley script.
 */
int pulley_parser_file (struct parser *prs, FILE *f);

/* Collect the hash value over the set of Pulley scripting lines collected
 * since program start or the last pulley_parse_cleanup().  Store the value
 * in prs->scanhash.
 *
 * Note that the hash_t value should be treated as an opaque byte array;
 * its size is to be determined with sizeof() and comparing or moving its
 * contents should be done with memcmp() and memcpy().  Do not assume that
 * the contents are printable, readable or NUL-terminated strings.
 */
void pulley_parser_hash (struct parser *prs);
