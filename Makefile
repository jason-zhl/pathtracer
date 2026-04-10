PathTracer.exe: ./src/*.cpp ./src/*.h ./src/material/*.h ./src/IBL/ibl.cpp ./src/IBL/ibl.h
	g++ -O3 -Wall -std=c++23 -Isrc ./src/main.cpp ./src/IBL/ibl.cpp -o PathTracer.exe

all: PathTracer.exe

run: PathTracer.exe
	./pathtracer.exe