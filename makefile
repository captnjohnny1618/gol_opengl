CC=g++

INCLUDE=-I/usr/local/cuda/include/

all: main.o

main.o: main.cpp
	$(CC) $(INCLUDE) main.cpp -o test -lGL -lglut -lGLEW
