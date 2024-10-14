emcc -O3 -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["cwrap"]' \
    -Ilib-3 \
    lib-3/lib-1.c \
    -c

emcc -O3 -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["cwrap"]' \
    -Ilib-3 \
    lib-3/main.c \
    lib-3/lib-1.c \
    -o main.js
echo python3 -m http.server



