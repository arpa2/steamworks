/* lexhash.h -- Hash lexemes that have any impact.
 *
 * These functions hash lexemes on input lines, and return the line's hashes
 * at the end of line.  The intention is to grab together a *set* of lines,
 * that is, without any ordering, and come to a complete overview of the
 * input with compatibile lexicographic output as a *set* of lines.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#ifndef PULLEYSCRIPT_LEXHASH_H
#define PULLEYSCRIPT_LEXHASH_H


#include <stdint.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


/* The hash outcome has its own type */
typedef uint32_t hash_t;

/* Use [0] to collect lines into a set of lines, use [1] within a line, [2] last line */
typedef hash_t hashbuf_t [3];

/* Management of hashes: starting, ending a line, finishing and harvesting */
void hash_start (hashbuf_t *buf);
void hash_endline (hashbuf_t *buf);
void hash_finish (hashbuf_t *buf, hash_t *output);
void hash_curline (hashbuf_t *buf, hash_t *output);
void hash_print (hash_t *output, FILE *stream);

/* Comparing hashes can be done with memcmp() based on the size of hash_t */

/* Insertion of tokens, mere text, and so on */
void hash_token (hashbuf_t *buf, int token);
void hash_text (hashbuf_t *buf, char *text);
void hash_token_text (hashbuf_t *buf, int token, char *text);
void hash_token_blob (hashbuf_t *buf, int token, void *data, int len);

#ifdef __cplusplus
}
#endif

#endif /* LEXHASH_H */
