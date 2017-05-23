#ifndef GFX_H 
#define GFX_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

extern void gfxInit( unsigned int resx, unsigned int resy );
unsigned long int gfxColor( const char *cname );
extern void gfxRedraw( );
extern void gfxUpdate( void ( *kbhandle )( char*, unsigned int ) );
extern void gfxEnd( );

extern Display *display;
extern int screen;
extern Window window;
extern GC gfxc;

#endif
