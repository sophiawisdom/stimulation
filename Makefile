all:
	clang main.c -Ofast -o main
debug:
	clang main.c -Ofast -D DEBUG -o main