PLATFORM_OS ?= UNKNOWN
ifeq ($(OS),Windows_NT)
	PLATFORM_OS = WINDOWS
else
	UNAMEOS = $(shell uname)
	ifeq ($(UNAMEOS),Linux)
		PLATFORM_OS = LINUX
	endif
endif

all: bind
ifeq ($(PLATFORM_OS),WINDOWS)
	./Rayroids.exe
endif
ifeq ($(PLATFORM_OS),LINUX)
	./rayroids
endif

test: main.c
	cc -I./raylib/include -L./raylib/lib -o rayroids main.c -lraylib
	LD_LIBRARY_PATH=./raylib/lib ./rayroids

bind: main.c
ifeq ($(PLATFORM_OS),WINDOWS)
	x86_64-w64-mingw32-gcc main.c -Iraylib/include raylib/lib/libraylib.a -lopengl32 -lgdi32 -lwinmm -o Rayroids.exe
endif
ifeq ($(PLATFORM_OS),LINUX)
	cc -I./raylib/include -o rayroids main.c ./raylib/lib/libraylib.a -lm -lpthread -ldl -lrt -lX11
endif

