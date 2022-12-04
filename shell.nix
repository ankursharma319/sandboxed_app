
{ pkgs ? import (fetchTarball "https://github.com/NixOS/nixpkgs/archive/38e591dd05ffc8bdf79dc752ba78b05e370416fa.tar.gz") {}}:

pkgs.mkShell {
    nativeBuildInputs = [
        pkgs.cmake
    ];
    buildInputs = if pkgs.stdenv.isDarwin then [] else [pkgs.valgrind] ++ [
        pkgs.gdb
        pkgs.clang_13
	]; 
}

