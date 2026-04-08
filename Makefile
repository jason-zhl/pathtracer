pathtracer.exe: *.cpp *.h
	g++ -O3 -Wall -std=c++23 main.cpp -o pathtracer.exe

all: pathtracer.exe

run: pathtracer.exe
	./pathtracer.exe