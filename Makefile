all: rayroids
	LD_LIBRARY_PATH=./raylib/lib ./rayroids

rayroids: main.c
	cc -I./raylib/include -L./raylib/lib -o rayroids main.c -lraylib

bind: main.c
	cc -I./raylib/include -o rayroids main.c ./raylib/lib/libraylib.a -lm -lpthread -ldl -lrt -lX11