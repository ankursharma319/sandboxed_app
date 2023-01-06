#! /bin/sh

# run ./configure.sh first

cmake --build _build/release/ \
&& sudo setcap all=p ./_build/release/sandboxed_executable \
&& ./_build/release/sandboxed_executable

#cmake --install _build/ --prefix _install/
#_install/bin/MainExe

