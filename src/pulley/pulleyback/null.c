/*
Copyright (c) 2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "../pulleyback.h"

#include <stdlib.h>
#include <stdio.h>

struct squeal_blob {
	void *data;
	size_t size;
};


static const char logger[] = "steamworks.pulleyback.null";
static int num_instances = 0;

typedef struct {
	int instancenumber;
	int varc;
} handle_t;

void *pulleyback_open(int argc, char **argv, int varc)
{
	char ibuf[64];

	write_logger(logger, "NULL backend opened.");
	snprintf(ibuf, sizeof(ibuf), " .. %d parameters, expect %d vars later", argc, varc);
	write_logger(logger, ibuf);
	// snprintf(ibuf, sizeof(ibuf), " .. %d variables", varc);

	for (unsigned int i=0; i<argc; i++)
	{
		snprintf(ibuf, sizeof(ibuf), " arg %d=%s", i, argv[i]);
		write_logger(logger, ibuf);
	}

	handle_t* handle = malloc(sizeof(handle_t));
	if (handle == NULL)
	{
		snprintf(ibuf, sizeof(ibuf), "Could not allocate handle %ld (#%d)", sizeof(handle_t), num_instances+1);
		write_logger(logger, ibuf);
		/* assume malloc() has set errno */
		return NULL;
	}
	else
	{
		handle->instancenumber = ++num_instances;
		handle->varc = varc;

		snprintf(ibuf, sizeof(ibuf), "NULL backend handle %p (#%d)", (void *)handle, handle->instancenumber);
		write_logger(logger, ibuf);
		return handle;
	}
}

void pulleyback_close(void *pbh)
{
	char ibuf[64];
	handle_t* handle = pbh;

	snprintf(ibuf, sizeof(ibuf), "NULL backend close %p", (void *)handle);
	write_logger(logger, ibuf);
	snprintf(ibuf, sizeof(ibuf), " .. instance %d", handle->instancenumber);
	write_logger(logger, ibuf);

	free(pbh);
}

int pulleyback_add(void *pbh, der_t *forkdata)
{
	char ibuf[64];
	handle_t* handle = pbh;

	snprintf(ibuf, sizeof(ibuf), "NULL backend add %p", (void *)handle);
	write_logger(logger, ibuf);
	snprintf(ibuf, sizeof(ibuf), "  .. instance #%d add data @%p", handle->instancenumber, (void *)forkdata);
	write_logger(logger, ibuf);

	struct squeal_blob* p = (struct squeal_blob*)forkdata;
	for (unsigned int i = 0; i < handle->varc; i++)
	{
		snprintf(ibuf, sizeof(ibuf), "  .. arg %d len %ld\n", i, p->size);
	}

	return 1;
}

int pulleyback_del(void *pbh, der_t *forkdata)
{
	char ibuf[64];
	handle_t* handle = pbh;

	snprintf(ibuf, sizeof(ibuf), "NULL backend del %p", (void *)handle);
	write_logger(logger, ibuf);
	snprintf(ibuf, sizeof(ibuf), "  .. instance #%d del data @%p", handle->instancenumber, (void *)forkdata);
	write_logger(logger, ibuf);

	return 1;
}

int pulleyback_reset(void *pbh)
{
	char ibuf[64];
	handle_t* handle = pbh;

	snprintf(ibuf, sizeof(ibuf), "NULL backend reset %p", (void *)handle);
	write_logger(logger, ibuf);

	return 1;
}

int pulleyback_prepare(void *pbh)
{
	char ibuf[64];
	handle_t* handle = pbh;

	snprintf(ibuf, sizeof(ibuf), "NULL backend prepare %p", (void *)handle);
	write_logger(logger, ibuf);

	return 1;
}

int pulleyback_commit(void *pbh)
{
	char ibuf[64];
	handle_t* handle = pbh;

	snprintf(ibuf, sizeof(ibuf), "NULL backend commit %p", (void *)handle);
	write_logger(logger, ibuf);

	return 1;
}

void pulleyback_rollback(void *pbh)
{
	char ibuf[64];
	handle_t* handle = pbh;

	snprintf(ibuf, sizeof(ibuf), "NULL backend rollback %p", (void *)handle);
	write_logger(logger, ibuf);
}

int pulleyback_collaborate(void *pbh1, void *pbh2)
{
	char ibuf[64];
	handle_t* handle = pbh1;

	snprintf(ibuf, sizeof(ibuf), "NULL backend collaborate %p", (void *)handle);
	write_logger(logger, ibuf);

	snprintf(ibuf, sizeof(ibuf), "  .. joining @%p and @%p", (void *)handle, pbh2);
	write_logger(logger, ibuf);

	return 0;
}

