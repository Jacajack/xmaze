#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#include "gen.h"
#include "xmaze.h"

extern unsigned int **map;
extern unsigned int width, height;
stck stack;

static unsigned char stckInit( stck *stack, size_t size )
{
	stack->size = size;
	return ( stack->ptr = stack->bptr = (unsigned int*) malloc( size * sizeof( unsigned int ) ) ) == NULL;
}

void stckFree( stck *stack )
{
	free( stack->bptr );
	stack->bptr = stack->ptr = NULL;
	stack->size = 0;
}

static unsigned int stckPush( stck *stack, unsigned int val )
{
	ptrdiff_t diff;
	if ( stack->ptr - stack->bptr >= stack->size )
	{
		diff = stack->ptr - stack->bptr;
		if ( ( stack->bptr = (unsigned int*) realloc( stack->bptr, ( stack->size + 32 ) * sizeof( unsigned int ) ) ) != NULL )
		{
			stack->size += 32;
			stack->ptr = stack->bptr + diff;
		}
		else
		{
			return 0;
		}
	}

	*( stack->ptr++ ) = val;

	return val;
}

static unsigned int stckPop( stck *stack )
{
	if ( stack->ptr - stack->bptr > 0 ) return *( --stack->ptr );
	else return 0;
}

static void mazemark( unsigned int x, unsigned int y )
{
	if ( x > 0 && !( map[x - 1][y] & 1 ) ) map[x - 1][y] |= 128;
	if ( y > 0 && !( map[x][y - 1] & 1 ) ) map[x][y - 1] |= 128;
	if ( x < width - 1 && !( map[x + 1][y] & 1 ) ) map[x + 1][y] |= 128;
	if ( y < height - 1 && !( map[x][y + 1] & 1 ) ) map[x][y + 1] |= 128;
}

static unsigned char mazecount( unsigned int x, unsigned int y )
{
	unsigned char count = 0;
	if ( x > 0 ) count += map[x - 1][y] & 1;
	if ( y > 0 ) count += map[x][y - 1] & 1;
	if ( x < width - 1 ) count += map[x + 1][y] & 1;
	if ( y < height - 1 ) count += map[x][y + 1] & 1;
	return count == 1;
}

static void floodfill( unsigned int x, unsigned int y, unsigned int old, unsigned int val )
{
	if ( x < 0 || y < 0 || x >= width || y >= height ) return;
	if ( map[x][y] != old ) return;

	map[x][y] = val;
	floodfill( x + 1, y, old, val );
	floodfill( x - 1, y, old, val );
	floodfill( x, y + 1, old, val );
	floodfill( x, y - 1, old, val );
}

void mazeinit( )
{
	unsigned int x = rand( ) % width, y = rand( ) % height;
	unsigned int i, j, c = 1;

	stckInit( &stack, ( width * height + width + height - 1 ) / 2 );

	if ( flags & FLAG_KRUSKAL )
	{
		//printf( "dim: %d, %d\n", width, height );
		for ( i = 0; i < width; i++ )
			for ( j = 0; j < height; j++ )
				if ( !( i % 2 + j % 2 ) )
				{
					map[i][j] = c++;
				}
				else
				{
					stckPush( &stack, i );
					stckPush( &stack, j );
					//printf( "wallon: %d, %d\n", i, j );
				}

		//printf( "wallcnt: %d\nexpected: %d\n", ( stack.ptr - stack.bptr ) / 2, ( width * height + width + height - 1 ) / 2 );
	}
	else
	{
		map[x][y] |= 1;
		mazemark( x, y );
		stckPush( &stack, x );
		stckPush( &stack, y );
	}
}

void mazefree( )
{
	unsigned int i;
	for ( i = 0; i < width; i++ ) free( map[i] );
	free( map );
}

int mazegen( )
{
	unsigned int x, y, wall, l = 0, r = 0, u = 0, d = 0;
	static unsigned char stepready, nostep = 0;

	if ( flags & FLAG_KRUSKAL ) //KRUSKAL
	{
		//Get random wall
		if ( stack.ptr - stack.bptr == 0 ) return 1;

		wall = ( rand( ) % ( ( stack.ptr - stack.bptr ) / 2 ) ) * 2;
		x = stack.bptr[wall];
		y = stack.bptr[wall + 1];
		stack.bptr[wall + 1] = stckPop( &stack );
		stack.bptr[wall] = stckPop( &stack );

		//printf( "%d, %d\n", x, y );
		if ( x > 0 ) l = map[x - 1][y];
		if ( x < width - 1 ) r = map[x + 1][y];
		if ( y > 0 ) u = map[x][y - 1];
		if ( y < height - 1 ) d = map[x][y + 1];

		if ( l != 0 && r != 0 && l != r )
		{
			map[x][y] = l;
			floodfill( x + 1, y, r, l );
		}

		if ( u != 0 && d != 0 && u != d )
		{
			map[x][y] = u;
			floodfill( x, y + 1, d, u );
		}

	}
	else //DFS
	{
		if ( stack.ptr - stack.bptr == 0 )
		{
			for ( x = 0; x < width; x++ )
				for ( y = 0; y < height; y++ )
					if ( !map[x][y] ) map[x][y] |= 128;
			return 1;
		}

		//Read, well, pretending that stack was not touched
		y = stckPop( &stack );
		x = stckPop( &stack );

		if ( !nostep )
		{
			stckPush( &stack, x );
			stckPush( &stack, y );
		}

		//Step
		nostep = stepready = 0;
		do
		{
			switch ( rand( ) % 4 )
			{
				case 0:
					if ( x + 1 < width && map[x + 1][y] & 128 && mazecount( x + 1, y ) )
					{
						map[x + 1][y] &= ~128;
						map[x + 1][y] |= 1;
						x = x + 1;
						stepready = 1;
					}
					nostep |= 1;
					break;

				case 1:
					if ( x > 0 && map[x - 1][y] & 128 && mazecount( x - 1, y ) )
					{
						map[x - 1][y] &= ~128;
						map[x - 1][y] |= 1;
						x = x - 1;
						stepready = 1;
					}
					nostep |= 2;
					break;

				case 2:
					if ( y + 1 < height && map[x][y + 1] & 128 && mazecount( x, y + 1 ) )
					{
						map[x][y + 1] &= ~128;
						map[x][y + 1] |= 1;
						y = y + 1;
						stepready = 1;
					}
					nostep |= 4;
					break;

				case 3:
					if ( y > 0 && map[x][y - 1] & 128 && mazecount( x , y - 1 ) )
					{
						map[x][y - 1] &= ~128;
						map[x][y - 1] |= 1;
						y = y - 1;
						stepready = 1;
					}
					nostep |= 8;
					break;
			}

			if ( nostep == 15 )
			{
				break;
			}
		}
		while ( !stepready );

		if ( nostep != 15 )
			nostep = 0;
		else
			nostep = 1;

		if ( stepready )
		{
			//Push position
			mazemark( x, y );
			stckPush( &stack, x );
			stckPush( &stack, y );
		}

	}

	return 0;
}
