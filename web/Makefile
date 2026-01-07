this: ext
	python -m http.server 1234

add: add.c
#	clang --target=wasm32 -emit-llvm -c -S add.c
	clang --target=wasm32 -nostdlib -Wl,--no-entry -Wl,--export-all -o add.wasm add.c

opt: add.c
	clang --target=wasm32 -O3 -flto -nostdlib -Wl,--no-entry -Wl,--export-all -Wl,--lto-O3 -o add.wasm add.c

ext:
	clang --target=wasm32 --no-standard-libraries -Wl,--export-table -Wl,--no-entry -Wl,--allow-undefined -Wl,--export-all -o add.wasm add.c -DPLATFORM_WEB

ext-opt:
	clang --target=wasm32 -O3 -flto -nostdlib -Wl,--export-table -Wl,--no-entry -Wl,--allow-undefined -Wl,--export-all -o add.wasm add.c -DPLATFORM_WEB
