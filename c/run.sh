#!/bin/sh

xxd -i file >file.h
gcc -std=c99 -o factoradic main.c -lgmp
./factoradic
