all:
	clang simul.c -Ofast -fno-math-errno -o main
debug:
	clang simul.c -Ofast -D DEBUG -o main