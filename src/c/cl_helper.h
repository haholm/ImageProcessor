#ifdef __APPLE__
#include <OpenCL/opencl.h>
// #include <GLUT/glut.h>
// #include <OpenGL/gl.h>
#else
#include <CL/cl.h>
// #include <GL/glut.h>
#endif

void cl_handle_err(cl_int err_nr, int i);

void cl_init(cl_context *, cl_command_queue *, cl_program *, const char *);

cl_mem cl_alloc(size_t size, cl_context context, cl_command_queue command_queue, void *data);

void cl_execute_kernel(cl_program program, cl_command_queue command_queue, cl_kernel *kernel, char *name, int num_args, void **args, int *args_sizes, int num_threads);
