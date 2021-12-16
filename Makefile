.phony: all

all: prog.out
	./prog.out ./demos/run

prog.out: main.o
	c++ -O3 main.cpp -o prog.out

main.o: main.cpp
	c++ -c main.cpp -O3 -o main.o

clean:
	-@rm *.o *.out