#ifndef GEN_H
#define GEN_H
#include <stdlib.h>

typedef struct
{
	ssize_t size;
	unsigned int *ptr;
	unsigned int *bptr;
} stck;

extern int mazegen( );
extern void mazeinit( );
extern void mazefree( );
extern void stckFree( stck *stack );
extern stck stack;

#endif
