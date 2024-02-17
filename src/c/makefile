all: main

GCC_LD_FLAGS := -Wl,-rpath,'@executable_path/lib' # might be @rpath on linux
LIB_FLAGS := -framework OpenCL -framework OpenGL $(shell pkg-config --static --libs glfw3)
GCC_OPTIONS = -Wall -g $(GCC_LD_FLAGS) $(LIB_FLAGS)#-lglfw -framework OpenCL -framework OpenGL -framework Cocoa -framework IOKit#-fsanitize=address 
OBJECTS = main.o filterimage.o cl_helper.o gl_helper.o lodepng.o
EXEC_NAME = main.out

ifdef CL
CXX_FLAGS += -DCL -DGL_SILENCE_DEPRECATION
endif

main: $(OBJECTS)
	gcc $(GCC_OPTIONS) $(CXX_FLAGS) -o $(EXEC_NAME) $(OBJECTS)

main.o: main.c
	gcc -c -g $(CXX_FLAGS) main.c

lodepng.o: lodepng.c lodepng.h
	gcc -c -g $(CXX_FLAGS) lodepng.c

cl_helper.o: cl_helper.c cl_helper.h
	gcc -c -g $(CXX_FLAGS) cl_helper.c

gl_helper.o: gl_helper.c gl_helper.h
	gcc -c -g $(CXX_FLAGS) gl_helper.c

filterimage.o: filterimage.c filterimage.h filterimage_types.h cl_helper.h
	gcc -c -g $(CXX_FLAGS) filterimage.c

clean: main
	rm -rf $(EXEC_NAME) $(OBJECTS) && make all