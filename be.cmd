@echo off
python %~dp0\gen_manifest.py || exit /b 1
emcc code\build.c -o program.js -g -gseparate-dwarf=program.debug.wasm -s SEPARATE_DWARF_URL=program.debug.wasm -s USE_WEBGL2=1 -s FULL_ES3=1 -s ALLOW_MEMORY_GROWTH=1 -s ASSERTIONS=2 -s SAFE_HEAP=1 -s STACK_OVERFLOW_CHECK=2 -Wno-pointer-bool-conversion -Wno-deprecated-pragma %*
