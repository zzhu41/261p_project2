
COMPILE = clang++
FLAGS = -std=c++11 -Wall -O

timing : hashes.hpp benchmark.cpp
	${COMPILE} ${FLAGS} benchmark.cpp -o benchmark

clean:
		rm -f benchmark
