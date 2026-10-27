set shell := ["sh", "-cu"]

CC := "clang"
CFLAGS := "-std=c23 -Wall -Wextra -Wpedantic"
PKGS := "ncurses libcurl"
OUTPUT := "nctikr"

build:
    {{CC}} {{CFLAGS}} src/main.c -o {{OUTPUT}} $(pkg-config --cflags --libs {{PKGS}})

check:
    {{CC}} {{CFLAGS}} -fsyntax-only src/main.c $(pkg-config --cflags {{PKGS}})


compile-db:
    bear -- just build
