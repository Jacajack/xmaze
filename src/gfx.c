#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <stdio.h>

#include "gfx.h"

Display *display;
int screen;
Window window;
GC gfxc;
static const unsigned long evmask = ExposureMask | KeyPressMask;

void gfxInit( unsigned int resx, unsigned int resy )
{
	unsigned long black, white;

	//Basic init
	display = XOpenDisplay( (char *) 0 );
	screen = DefaultScreen( display );
	black = BlackPixel( display, screen );
	white = WhitePixel( display, screen );

	//Open window
	window = XCreateSimpleWindow( display, DefaultRootWindow( display ), 0, 0, resx, resy, 5, white, black );
	XSetStandardProperties( display, window, "xmaze", "xmaze", None, NULL, 0, NULL );

	//Select inputs
	XSelectInput( display, window, evmask );

	//Init graphics content
	gfxc = XCreateGC( display, window, 0, 0 );

	//Setup foreground and background
	XSetBackground( display, gfxc, black );
	XSetForeground( display, gfxc, white );

	//Clear window and bring it to the top
	XClearWindow( display, window );
	XMapRaised( display, window );
}

unsigned long int gfxColor( const char *cname )
{
	XColor tmp;
	XParseColor( display, DefaultColormap( display, screen ), cname, &tmp );
	XAllocColor( display, DefaultColormap( display, screen ), &tmp );
	return tmp.pixel;
}

void gfxRedraw( )
{
	XClearWindow( display, window );
}

void gfxUpdate( void ( *kbhandle )( char*, unsigned int ) )
{
	const int textSize = 255;

	XEvent event;
	KeySym key;	//Keypress handler
	char text[textSize]; //Keyboard buffer

	if ( !XCheckMaskEvent( display, evmask, &event ) ) return;

	switch ( event.type )
	{
		case Expose:
			if ( event.xexpose.count == 0 ) gfxRedraw( );
			break;

		case KeyPress:
			if (  XLookupString( &event.xkey, text, textSize, &key, 0 ) == 1 )
				( *kbhandle )( text, textSize );
			break;
	}
}

void gfxEnd( )
{
	XFreeGC( display, gfxc );
	XDestroyWindow( display, window );
	XCloseDisplay( display );
}
