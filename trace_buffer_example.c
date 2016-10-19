#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "trace_buffer.h"

#define NSTEPS (100)

#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);	c += b; \
  b -= a;  b ^= rot(a, 6);	a += c; \
  c -= b;  c ^= rot(b, 8);	b += a; \
  a -= c;  a ^= rot(c,16);	c += b; \
  b -= a;  b ^= rot(a,19);	a += c; \
  c -= b;  c ^= rot(b, 4);	b += a; \
}

#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c, 4); \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}


int
hash_uint32(int k)
{
	register int a,
				b,
				c;

	a = b = c = 0x9e3779b9 + (int) sizeof(int) + 3923095;
	a += k;

	final(a, b, c);

	/* report the result */
	return c;
}


struct ArgData
{
	TraceBuffer tbuff;
	int 		nloops;
	int 		nsteps;
	int 		tid;
};

void *
do_some_work(void *argdata)
{
	TraceBuffer tbuff 	= ((struct ArgData *) argdata)->tbuff;
	int nloops 			= ((struct ArgData *) argdata)->nloops;
	int nsteps 			= ((struct ArgData *) argdata)->nsteps;
	int tid 			= ((struct ArgData *) argdata)->tid;
	int hash 			= 0;

	char buffer[256];

	sprintf(buffer, "BEFORE LOOP ---> ID: %d", tid);
	TraceBufferAddEntry(tbuff, strdup(buffer), true);

	for (int i = 0; i < nloops; i ++)
	{
		sprintf(buffer, "BEFORE ---> ID: %d >>>> %d step", tid, i + 1);
		TraceBufferAddEntry(tbuff, strdup(buffer), true);

		for (int j = 0; j < nsteps; j++)
			hash ^= hash_uint32(i * j);

		sprintf(buffer, "AFTER ---> ID: %d >>>> %d step", tid, i + 1);
		TraceBufferAddEntry(tbuff, strdup(buffer), true);
	}

	sprintf(buffer, "AFTER LOOP ---> ID: %d >>>> %x hash", tid, hash);
	TraceBufferAddEntry(tbuff, strdup(buffer), true);

	return NULL;
}

#define TRACE_BUFFER_SIZE (10000)


int main(int argc, char **argv)
{
	int nthreads, nloops, nsteps;
	TraceBuffer buff;
	struct ArgData *arg;
	pthread_t *threads;
	pthread_attr_t attr;
	int rc;
	void *status;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	if (argc != 4)
	{
		fprintf(stderr, "Usage: %s [nthreads] [nloops] [nsteps]\n", argv[0]);
		return EXIT_FAILURE;
	}

	sscanf(argv[1], "%d", &nthreads);
	sscanf(argv[2], "%d", &nloops);
	sscanf(argv[3], "%d", &nsteps);

	arg 	= malloc(sizeof(*arg) * nthreads);
	threads = malloc(sizeof(*threads) * nthreads);
	buff 	= TraceBufferAlloc(TRACE_BUFFER_SIZE);

	for (int i = 0; i < nthreads; i++)
	{
		arg[i].nsteps 	= nsteps;
		arg[i].nloops 	= nloops;
		arg[i].tbuff 	= buff;
		arg[i].tid		= i + 1;

		rc = pthread_create(&threads[i], &attr, do_some_work, (void *) &arg[i]);

		if (rc)
		{
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			return EXIT_FAILURE;
		}
	}

	pthread_attr_destroy(&attr);

	for(int i = 0; i < nthreads; i++)
	{
		rc = pthread_join(threads[i], &status);

		if (rc)
		{
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			return EXIT_FAILURE;
		}
	}

	TraceBufferSortByTime(buff);
	printf("%s\n", TraceBufferToString(buff, "\n"));

	return EXIT_SUCCESS;
}