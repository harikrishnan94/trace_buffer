#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include <assert.h>

#include "trace_buffer.h"


#define MAX_TRACE_BUFFER_SIZE 		(2 * 1000 * 1000 * 1000)
#define DEFAULT_TRACE_BUFFER_SIZE 	(10000)

#ifndef Assert
#define Assert(cond) assert(cond)
#endif

#define palloc(size)	(malloc(size))
#define palloc0(size)	(calloc(1, size))
#define pfree(ptr)		(free(ptr))


typedef struct TraceBufferElement
{
	struct timespec timestamp;
	pthread_t 		threadId;
	char			*msg;
	bool			shouldFreeMsg;

} TraceBufferElement;

typedef struct TraceBufferImpl
{
	TraceBufferElement *elements;

	int traceBufferSize;
	int nextElement;

	char *traceBufferStr;
	bool sorted;

} TraceBufferImpl;



static int
diff(struct timespec *t1, struct timespec *t2)
{
	double diff = difftime(t1->tv_sec, t2->tv_sec);

	return diff > 0 ? 1 : (diff < 0 ? -1 : t1->tv_nsec - t2->tv_nsec);
}


/*
 * Compare two TraceBufferElements based on timestamp.
 * If an elements msg is null, it means it has no entry.
 */
static int
tracebuffer_timestamp_compare(const void *d1, const void *d2)
{
	TraceBufferElement *e1 = (TraceBufferElement *) d1;
	TraceBufferElement *e2 = (TraceBufferElement *) d2;

	if (e1->msg == NULL)
	{
		if (e2->msg == NULL)
			return 0;
		else
			return -1;
	}
	else if (e2->msg == NULL)
		return 1;
	else
		return diff(&e1->timestamp, &e2->timestamp);
}



TraceBuffer *
TraceBufferAlloc(int traceBufferSize)
{
	TraceBufferImpl *tbuff 			= NULL;
	TraceBufferElement *elements 	= NULL;

	Assert(traceBufferSize > 0);
	Assert(traceBufferSize <= MAX_TRACE_BUFFER_SIZE);

	elements 	= palloc0(sizeof(*elements) * traceBufferSize);
	tbuff 		= palloc(sizeof(*tbuff));

	tbuff->elements 		= elements;
	tbuff->traceBufferSize 	= traceBufferSize;
	tbuff->nextElement 		= 0;
	tbuff->traceBufferStr 	= NULL;

	return (TraceBuffer) tbuff;
}

void
TraceBufferDestroy(TraceBuffer t_buff)
{
	TraceBufferImpl *tbuff = t_buff;

	if (tbuff)
	{
		TraceBufferElement *elements 	= tbuff->elements;
		int size 						= tbuff->traceBufferSize;

		Assert(size > 0);
		Assert(tbuff->elements != NULL);

		for (int i = 0; i < size; i++)
		{
			if (elements[i].shouldFreeMsg)
			{
				Assert(elements[i].msg != NULL);
				pfree(elements[i].msg);
			}
		}

		pfree(elements);

		if (tbuff->traceBufferStr)
			pfree(tbuff->traceBufferStr);

		pfree(tbuff);
	}
}



void
TraceBufferAddEntry(TraceBuffer t_buff, char *msg, bool shouldFree)
{
	TraceBufferElement *element;
	TraceBufferImpl *tbuff 	= t_buff;
	int size 				= tbuff->traceBufferSize;
	int next 				= __sync_fetch_and_add(&tbuff->nextElement, 1) % size;

	Assert(msg != NULL);

	element = &tbuff->elements[next];

	element->threadId 		= pthread_self();
	element->msg 			= msg;
	element->shouldFreeMsg 	= shouldFree;
	tbuff->sorted			= false;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &element->timestamp);
}


void
TraceBufferSortByTime(TraceBuffer t_buff)
{
	TraceBufferImpl *tbuff 			= t_buff;
	TraceBufferElement *elements 	= tbuff->elements;
	int size 						= tbuff->traceBufferSize;

	if (tbuff->sorted)
		return;

	qsort(elements, size, sizeof(*elements), tracebuffer_timestamp_compare);
	tbuff->sorted = true;
}


#define TIMESTAMP_STR_LEN 	(11 * 3)
#define THREAD_ID_STR_LEN 	(11)

char *
TraceBufferToString(TraceBuffer t_buff, const char *sep)
{
	TraceBufferImpl *tbuff 			= t_buff;
	TraceBufferElement *elements 	= tbuff->elements;
	int size 						= tbuff->traceBufferSize;
	char *traceBufferStr 			= NULL;
	size_t traceBufferStrLen		= 0;
	size_t runningLen				= 0;
	size_t sepLen					= strlen(sep);

	if (tbuff->traceBufferStr)
		return tbuff->traceBufferStr;

	for (int i = 0; i < size; i++)
		if (elements[i].msg)
			traceBufferStrLen += strlen(elements[i].msg) + TIMESTAMP_STR_LEN + THREAD_ID_STR_LEN + 3 + sepLen;

	traceBufferStr 			= palloc(traceBufferStrLen + sizeof(char));
	tbuff->traceBufferStr 	= traceBufferStr;

	for (int i = 0; i < size; i++)
	{
		if (elements[i].msg)
		{
			sprintf(&traceBufferStr[runningLen], "0x%8.8lx| 0x%8.8lx-0x%8.8lx| %s%s", elements[i].threadId,
					elements[i].timestamp.tv_sec, elements[i].timestamp.tv_nsec, elements[i].msg, sep);

			while (traceBufferStr[runningLen])
				runningLen++;
		}
	}

	traceBufferStr[runningLen - 1] = '\0';

	return traceBufferStr;
}