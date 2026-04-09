PathTracer.exe: ./src/*.cpp ./src/*.h ./src/material/*.h
	g++ -O3 -Wall -std=c++23 -Isrc ./src/main.cpp -o PathTracer.exe

all: PathTracer.exe

run: PathTracer.exe
	./pathtracer.exe