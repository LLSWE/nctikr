set shell := ["sh", "-cu"]

CC := "clang"
CFLAGS := "-std=c23 -Wall -Wextra -Wpedantic -Iinclude" 
PKGS := "ncurses libcurl yyjson"
OUTPUT := "nctikr"


run:
    {{CC}} {{CFLAGS}} src/*.c -o {{OUTPUT}} $(pkg-config --cflags --libs {{PKGS}})
    ./{{OUTPUT}}


build:
    {{CC}} {{CFLAGS}} src/*.c -o {{OUTPUT}} $(pkg-config --cflags --libs {{PKGS}})

check:
    {{CC}} {{CFLAGS}} -fsyntax-only src/main.c $(pkg-config --cflags {{PKGS}})


bear:
    just clean
    bear -- just build

clean:
    rm -rf {{OUTPUT}}


