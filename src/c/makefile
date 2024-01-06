all: main

GCC_OPTIONS = -O3 -Wall -g -framework OpenCL #-fsanitize=address 
OBJECTS = main.o filterimage.o lodepng.o
EXEC_NAME = main.out

main: main.o filterimage.o lodepng.o
	gcc $(GCC_OPTIONS) -o $(EXEC_NAME) $(OBJECTS)

main.o: main.c
	gcc -c -g main.c

lodepng.o: lodepng.c lodepng.h
	gcc -c -g lodepng.c

filterimage.o: filterimage.c filterimage.h filterimage_types.h
	gcc -c -g filterimage.c

clean: main
	rm -rf $(EXEC_NAME) $(OBJECTS) && make all