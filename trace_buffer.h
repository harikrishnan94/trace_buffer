#ifndef TRACE_BUFFER_H
#define TRACE_BUFFER_H

#ifndef _PTHREAD_H
	#error "pthread support not found"
#endif


typedef void *TraceBuffer;
typedef unsigned char bool;

#define true 	1
#define false 	0


extern TraceBuffer *TraceBufferAlloc(int traceBufferSize);
extern void TraceBufferDestroy(TraceBuffer tbuff);

extern void TraceBufferSortByTime(TraceBuffer tbuff);
extern char *TraceBufferToString(TraceBuffer tbuff, const char *sep);

extern void TraceBufferAddEntry(TraceBuffer tbuff, char *msg, bool shouldFree);
extern void TraceBufferAddEntryFmt(TraceBuffer t_buff, const char *fmt, ...);

#endif