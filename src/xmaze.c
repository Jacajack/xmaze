#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "gfx.h"
#include "gen.h"
#include "xmaze.h"

struct status
{
	char active;
	char done;
	char gfxready;
	char paused;
} status;

unsigned int **map;
unsigned int flags = FLAG_GFX;
unsigned int width = 40, height = 40;
unsigned int tilesize = 10;
unsigned int sps = 40;
static unsigned long int wallcolor = 0x000000;
static unsigned long int pathcolor = 0xFFFFFF;
static unsigned long int frontcolor = 0x108760;
static char txtdumpchr = '#';
static char *outfname = NULL;
static FILE* outfile = NULL;
static char manualdump = 0;

void txtdump( FILE *f, char frame )
{
	unsigned int i, j;
	fseek( f, SEEK_SET, 0 );

	if ( frame )
	{
		for ( i = 0; i < width + 2; i++ ) fputc( txtdumpchr, f );
		fputc( '\n', f );
	}

	for ( i = 0; i < height; i++ )
	{
		if ( frame ) fputc( txtdumpchr, f );
		for ( j = 0; j < width; j++ )
			fprintf( f, "%c", flags & FLAG_KRUSKAL ? ( map[j][i] == 0 ? txtdumpchr : ' ' ) : ( map[j][i] & 128 ? txtdumpchr : ' ' ) );
		if ( frame ) fputc( txtdumpchr, f );
		fprintf( f, "\n" );
	}

	if ( frame )
	{
		for ( i = 0; i < width + 2; i++ ) fputc( txtdumpchr, f );
		fputc( '\n', f );
	}

	fprintf( f, "\n" );
	fflush( f );
}

void keyboard( char *text, unsigned int length )
{
	if ( !length ) return;

	switch ( text[0] )
	{
		//Q - exit program
		case 'q':
			gfxEnd( );
			status.active = 0;
			break;

		//P - pause
		case 'p':
			status.paused ^= 1;
			break;

		//D - dump frame
		case 'd':
			if ( outfname != NULL ) txtdump( outfile, flags & FLAG_FRAME );
			manualdump = 1;
			break;

		//S - dump frame (stdout)
		case 's':
			txtdump( stdout, flags & FLAG_FRAME );
			break;

		//F - dump with frame
		case 'f':
			flags ^= FLAG_FRAME;
			break;

		//C - toggle additional colors
		case 'c':
			flags ^= FLAG_COLORS;
			break;

		//+ - speed up
		case '+':
			sps++;
			break;

		//- - slow down
		case '-':
			if ( sps > 0 ) sps--;;
			break;

		default:
			break;
	}
}

void render( )
{
	unsigned int i, j;
	static ptrdiff_t laststack = 0;
	ptrdiff_t stacklen = stack.ptr - stack.bptr;
	static int stackdir = 0;

	for ( i = 0; i < width; i++ )
		for ( j = 0; j < height; j++ )
		{
			if ( flags & FLAG_KRUSKAL )
			{
				if ( map[i][j] )
				{
					if ( flags & FLAG_COLORS ) XSetForeground( display, gfxc, 1000 + map[i][j] * 1200000  );
					else XSetForeground( display, gfxc, pathcolor );
					XFillRectangle( display, window, gfxc, i * tilesize, j * tilesize, tilesize, tilesize );
				}
				else
				{
					XSetForeground( display, gfxc, wallcolor );
					XFillRectangle( display, window, gfxc, i * tilesize, j * tilesize, tilesize, tilesize );
				}
			}
			else
			{
				if ( map[i][j] & 1 )
				{
					XSetForeground( display, gfxc, pathcolor );
					XFillRectangle( display, window, gfxc, i * tilesize, j * tilesize, tilesize, tilesize );
				}

				if ( map[i][j] & 128 )
				{
					if ( flags & FLAG_COLORS ) XSetForeground( display, gfxc, frontcolor );
					else XSetForeground( display, gfxc, wallcolor );
					XFillRectangle( display, window, gfxc, i * tilesize, j * tilesize, tilesize, tilesize );
				}
			}
		}

	//Stack usage information
	char str[128];
	if ( !status.done ) sprintf( str, "stack size: %ld", (long) stacklen );
	else if ( status.paused ) sprintf( str, "Press P to unpause!" );
	else sprintf( str, "Done!" );
	XSetForeground( display, gfxc, gfxColor( "black" ) );
	XFillRectangle( display, window, gfxc, 0, height * tilesize, width * tilesize, 10 );
	XSetForeground( display, gfxc, gfxColor( stackdir > 0 || status.done ? "white" : "red" ) );
	XDrawString( display, window, gfxc, 0, height * tilesize + 10, str, strlen( str ) );

	if ( stacklen - laststack != 0 ) stackdir = stacklen - laststack;
	laststack = stacklen;
}


void *gfxthread( void *data )
{
	struct timespec timer;

	//Loop runs 30 times per second
	timer.tv_sec = 0;
	timer.tv_nsec = 1.0 / 30.0 * 1000000000.0;
	while ( status.active )
	{
		render( );
		gfxUpdate( keyboard );
		nanosleep( &timer, NULL );
		status.gfxready = 1;
	}

	return NULL;
}

int main( int argc, char **argv )
{
	unsigned char badarg = 1;
	unsigned long seed = time( NULL );
	int i;
	struct timespec timer;

	outfile = stdout;

	//Init
	if ( argc < 2 )
	{
		fprintf( stderr, "%s: no arguments given - assuming defaults (have you read help?)\n", argv[0] );
	}

	for ( i = 1; i < argc; i++ )
	{
		badarg = 1;

		//Help message
		if ( !strcmp( argv[i], "-h" ) || !strcmp( argv[i], "--help" ) )
		{
			fprintf( stderr, "%s - x maze generator " XMAZE_VERSION "\n", argv[0] );
			fprintf( stderr, "\t-h - display this help message\n" );
			fprintf( stderr, "\t-x <width> - maze width\n" );
			fprintf( stderr, "\t-y <height> - maze height\n" );
			fprintf( stderr, "\t-f <frequency> - steps per second\n" );
			fprintf( stderr, "\t-t <pixels> - tile size (in pixels)\n" );
			fprintf( stderr, "\t-s <seed> - random generator seed\n" );
			fprintf( stderr, "\t-k - use Kruskal's algorithm\n" );
			fprintf( stderr, "\t-c - show floodfill effects with colors\n" );
			fprintf( stderr, "\t-p - start paused\n" );
			fprintf( stderr, "\t-n - no graphics\n" );
			fprintf( stderr, "\t-b - add border to text dump\n" );
			fprintf( stderr, "\t-w <char> - wall character\n" );
			fprintf( stderr, "\t-W <hex color> - wall color\n" );
			fprintf( stderr, "\t-P <hex color> - path color\n" );
			fprintf( stderr, "\t-F <hex color> - frontier color\n" );
			fprintf( stderr, "\t-o <filename> - output file\n" );

			fprintf( stderr, "\nIn graphics mode:\n" );
			fprintf( stderr, "\tq - quit\n" );
			fprintf( stderr, "\tp - pause\n" );
			fprintf( stderr, "\tc - toggle additional colors\n" );
			fprintf( stderr, "\td - dump current maze into the output file\n" );
			fprintf( stderr, "\ts - dump current maze into the terminal\n" );
			fprintf( stderr, "\tf - toggle frame\n" );
			fprintf( stderr, "\t+ - speed up\n" );
			fprintf( stderr, "\t- - slow down\n" );

			badarg = 0;
			exit( 0 );
		}

		if ( !strcmp( argv[i], "-x" ) )
			if ( ++i >= argc || ( badarg = 0, !sscanf( argv[i], "%d", &width ) ) ) fprintf( stderr, "%s: bad value for '%s'\n", argv[0], argv[i - 1] ), exit( 1 );

		if ( !strcmp( argv[i], "-y" ) )
			if ( ++i >= argc || ( badarg = 0, !sscanf( argv[i], "%d", &height ) ) ) fprintf( stderr, "%s: bad value for '%s'\n", argv[0], argv[i - 1] ), exit( 1 );

		if ( !strcmp( argv[i], "-s" ) )
			if ( ++i >= argc || ( badarg = 0, !sscanf( argv[i], "%ld", &seed ) ) ) fprintf( stderr, "%s: bad value for '%s'\n", argv[0], argv[i - 1] ), exit( 1 );

		if ( !strcmp( argv[i], "-f" ) )
			if ( ++i >= argc || ( badarg = 0, !sscanf( argv[i], "%d", &sps ) ) ) fprintf( stderr, "%s: bad value for '%s'\n", argv[0], argv[i - 1] ), exit( 1 );

		if ( !strcmp( argv[i], "-t" ) )
			if ( ++i >= argc || ( badarg = 0, !sscanf( argv[i], "%d", &tilesize ) ) ) fprintf( stderr, "%s: bad value for '%s'\n", argv[0], argv[i - 1] ), exit( 1 );

		if ( !strcmp( argv[i], "-w" ) )
			if ( ++i >= argc || ( badarg = 0, !sscanf( argv[i], "%c", &txtdumpchr ) ) ) fprintf( stderr, "%s: bad value for '%s'\n", argv[0], argv[i - 1] ), exit( 1 );

		if ( !strcmp( argv[i], "-W" ) )
			if ( ++i >= argc || ( badarg = 0, !sscanf( argv[i], "%06lx", &wallcolor ) ) ) fprintf( stderr, "%s: bad value for '%s'\n", argv[0], argv[i - 1] ), exit( 1 );

		if ( !strcmp( argv[i], "-P" ) )
			if ( ++i >= argc || ( badarg = 0, !sscanf( argv[i], "%06lx", &pathcolor ) ) ) fprintf( stderr, "%s: bad value for '%s'\n", argv[0], argv[i - 1] ), exit( 1 );

		if ( !strcmp( argv[i], "-F" ) )
			if ( ++i >= argc || ( badarg = 0, !sscanf( argv[i], "%06lx", &frontcolor ) ) ) fprintf( stderr, "%s: bad value for '%s'\n", argv[0], argv[i - 1] ), exit( 1 );

		if ( !strcmp( argv[i], "-o" ) )
			if ( ++i >= argc || ( outfname = argv[i], badarg = 0 ) ) fprintf( stderr, "%s: bad value for '%s'\n", argv[0], argv[i - 1] ), exit( 1 );

		if ( !strcmp( argv[i], "-k" ) )
		{
			flags |= FLAG_KRUSKAL;
			badarg = 0;
		}

		if ( !strcmp( argv[i], "-c" ) )
		{
			flags |= FLAG_COLORS;
			badarg = 0;
		}

		if ( !strcmp( argv[i], "-p" ) )
		{
			status.paused = 1;
			badarg = 0;
		}

		if ( !strcmp( argv[i], "-n" ) )
		{
			flags &= ~FLAG_GFX;
			badarg = 0;
		}

		if ( !strcmp( argv[i], "-b" ) )
		{
			flags |= FLAG_FRAME;
			badarg = 0;
		}

		if ( badarg )
		{
			fprintf( stderr, "%s: unrecognized option '%s'\n", argv[0], argv[i] );
			exit( 1 );
		}
	}

	if ( width == 0 || height == 0 )
	{
		fprintf( stderr, "%s: zero size?\n", argv[0] );
		exit( 1 );
	}

	if ( flags & FLAG_GFX && ( width > 201 || height > 201 ) )
	{
		fprintf( stderr, "%s: maze too big\n", argv[0] );
		exit( 1 );
	}

	if ( flags & FLAG_KRUSKAL )
	{
		if ( width % 2 + width % 2 < 2 )
		{
			fprintf( stderr, "%s: when using Kruskal's algorithm, dimensions must me odd\n", argv[0] );
			exit( 1 );
		}
	}

	srand( seed );

	if ( outfname != NULL )
	{
		if ( ( outfile = fopen( outfname, "w" ) ) == NULL )
			fprintf( stderr, "%s: cannot open %s\n", argv[0], outfname );
	}

	map = (unsigned int**) calloc( width, sizeof( unsigned int* ) );
	for ( i = 0; i < (signed int) width; i++ )
		map[i] = (unsigned int*) calloc( height, sizeof( unsigned int ) );

	//Init graphics
	pthread_t gfxth;
	if ( flags & FLAG_GFX )
	{
		gfxInit( width * tilesize, height * tilesize + 10 );
		pthread_create( &gfxth, NULL, gfxthread, NULL );
	}

	status.active = 1;
	status.done = 0;

	//Wait for gfx
	while ( !status.gfxready && flags & FLAG_GFX );

	mazeinit( );



	while ( ( status.paused ? 1 : !mazegen( ) ) && status.active )
		if ( flags & FLAG_GFX )
		{
			if ( sps == 0 ) sps = 1;
			timer.tv_sec = 0;
			timer.tv_nsec = 1000000000.0 / sps;
			if ( sps == 1 )
			{
				timer.tv_sec = 1;
				timer.tv_nsec = 0;
			}
			nanosleep( &timer, NULL );
		}

	if ( outfname != NULL || !( flags & FLAG_GFX ) )
	{
		if ( !manualdump ) txtdump( outfile, flags & FLAG_FRAME );
	}

	if ( flags & FLAG_GFX )
	{
		status.done = 1;
		timer.tv_sec = 1;
		timer.tv_nsec = 0;
		while ( status.active ) nanosleep( &timer, NULL );
	}

	stckFree( &stack );
	mazefree( );

	if ( outfname != NULL ) fclose( outfile );
	return 0;
}
