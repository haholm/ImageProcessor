#ifdef __APPLE__
#include <OpenCL/opencl.h>

#else
#include <CL/cl.h>
#endif

typedef struct cl_handle
{
    cl_context context;
    cl_command_queue command_queue;
    cl_program program;
} cl_handle;

void cl_handle_err(cl_int err_nr, int i);

void cl_init(cl_handle *, const char *);

void cl_terminate(cl_handle *);

cl_mem cl_alloc(size_t size, cl_handle *handle, void *data);

void cl_execute_kernel(cl_handle *handle, cl_kernel *kernel, char *name, int num_args, void **args, int *args_sizes, int num_threads);
