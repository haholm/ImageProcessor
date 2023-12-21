all: main

main: main.c
	gcc -g -O0 -Wall filterimage.c lodepng.c main.c -o main.out

clean: main
	rm main && make all