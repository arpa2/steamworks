#ifndef TYPES_H
#define TYPES_H

typedef struct {
	void *data;
	void * (*index) (void *data, unsigned int idx);
	char *name;
} type_t;


struct vartab;
typedef struct path path_t;


typedef unsigned int anynum_t;
typedef anynum_t varnum_t;
typedef anynum_t gennum_t;
typedef anynum_t cndnum_t;
typedef anynum_t drvnum_t;

#define BITNUM_BAD ((anynum_t) -1)
#define VARNUM_BAD ((varnum_t) -1)
#define GENNUM_BAD ((gennum_t) -1)
#define DRVNUM_BAD ((drvnum_t) -1)
#define CNDNUM_BAD ((cndnum_t) -1)

struct gentab *gentab_from_type (type_t *gentype);
struct vartab *vartab_from_type (type_t *vartype);
struct cndtab *cndtab_from_type (type_t *cndtype);
struct drvtab *drvtab_from_type (type_t *drvtype);

#endif /* TYPES_H */
