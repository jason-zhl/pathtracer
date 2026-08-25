PathTracer.exe: ./src/*.cpp ./src/*.h ./src/material/*.h ./src/environment/*.cpp ./src/environment/*.h
	g++ -O3 -Wall -std=c++23 -Isrc ./src/main.cpp ./src/environment/ibl.cpp -o PathTracer.exe

all: PathTracer.exe

run: PathTracer.exe
	./pathtracer.exe