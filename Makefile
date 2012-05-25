membench: membench.cpp 
	g++ -O3 -march=native -mno-avx -o membench membench.cpp
all: membench
