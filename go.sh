#!/bin/sh

./mads starter.asm -o:starter.bin
xxd -i starter.bin > starter.h
gcc -Wall -o atrdd2car atrdd2car.c

./atrdd2car Test.atr Test.car -c
