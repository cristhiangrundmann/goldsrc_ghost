.phony: all

all: prog.out
	./prog.out ./demos/run

prog.out: main.o
	cc -O3 main.c -o prog.out

main.o: main.c
	cc -c main.c -O3 -o main.o