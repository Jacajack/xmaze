all: src/xmaze.c src/gfx.c src/gen.c
	gcc -Wall $^ -lX11 -lpthread -o xmaze

clean:
	-rm xmaze
