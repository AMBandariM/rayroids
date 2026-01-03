# Rayroids
A small Asteroids-style space shooter game written in C using [raylib](https://github.com/raysan5/raylib).

## Screenshots
<p align="center">
  <img src="shots/shot-00.png" height="400">
  <img src="shots/shot-01.png" height="400">
</p>

<p align="center">
  <img src="shots/shot-02.png" height="400">
  <img src="shots/shot-03.png" height="400">
</p>

## Requirements
- C compiler
- [raylib](https://github.com/raysan5/raylib)

The Makefile expects raylib at:
```
./raylib/include
./raylib/lib
```

## Quick Start (Linux)
Build and Run:
```console
$ make
```
Static Raylib Build:
```console
$ make bind
```
Run:
```console
$ ./rayroids
```

## Windows Export
Figure it out. This is what ChatGPT says you should do:
```console
$ x86_64-w64-mingw32-gcc main.c -Iraylib/include raylib/lib/libraylib.a -lopengl32 -lgdi32 -lwinmm -o SpaceRayShooter.exe
```