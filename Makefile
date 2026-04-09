PathTracer.exe: ./src/*.cpp ./src/*.h
	g++ -O3 -Wall -std=c++23 ./src/main.cpp -o PathTracer.exe

all: PathTracer.exe

run: PathTracer.exe
	./pathtracer.exe