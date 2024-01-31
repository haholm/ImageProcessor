#include <stdio.h>
#include "cl_helper.h"

// Rounds to closest power of 2. Taken from: https://stackoverflow.com/a/9194117
int round_to_closest_p2(int num_to_round, int multiple)
{
    if (multiple && ((multiple & (multiple - 1)) == 0))
    {
        return (num_to_round + multiple - 1) & -multiple;
    }

    exit(-1);
}

void cl_handle_err(cl_int err_nr, int i)
{
    if (err_nr != CL_SUCCESS)
    {
        printf("CL Error %i %i\n", i, err_nr);
        exit(err_nr);
    }
}

void cl_init(cl_context *context, cl_command_queue *command_queue, cl_program *program, const char *source)
{
    cl_platform_id platform_id = NULL;
    cl_device_id device_id = NULL;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms); // TODO: Multiple platforms/devices?
    cl_handle_err(ret, 0);
    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &ret_num_devices);
    cl_handle_err(ret, 1);

    *context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    cl_handle_err(ret, 2);

    *command_queue = clCreateCommandQueue(*context, device_id, 0, &ret);
    cl_handle_err(ret, 3);

    size_t source_len = strlen(source);
    *program = clCreateProgramWithSource(*context, 1, (const char **)&source, (const size_t *)&source_len, &ret);
    cl_handle_err(ret, 4);

    ret = clBuildProgram(*program, 1, &device_id, "-I. -g", NULL, NULL);
    size_t len = 0;
    ret = clGetProgramBuildInfo(*program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
    char *buffer = calloc(len, sizeof(char));
    ret = clGetProgramBuildInfo(*program, device_id, CL_PROGRAM_BUILD_LOG, len, buffer, NULL);
    if (len > 1)
        printf("OpenCL build info:\n%s\n", buffer);
    cl_handle_err(ret, 5);
}

cl_mem cl_alloc(size_t size, cl_context context, cl_command_queue command_queue, void *data)
{
    cl_int ret;
    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, size, NULL, &ret);
    cl_handle_err(ret, 7);
    if (data != NULL)
    {
        ret = clEnqueueWriteBuffer(command_queue, buffer, CL_TRUE, 0, size, data, 0, NULL, NULL);
        cl_handle_err(ret, 8);
    }

    return buffer;
}

void cl_execute_kernel(cl_program program, cl_command_queue command_queue, cl_kernel *kernel, char *name, int num_args, void **args, int *args_sizes, int num_threads)
{
    cl_int ret;
    if (name != NULL)
    {
        *kernel = clCreateKernel(program, name, &ret);
        cl_handle_err(ret, 6);
    }

    for (int i = 0; i < num_args; i++)
    {
        ret = clSetKernelArg(*kernel, i, args_sizes[i], args[i]);
        cl_handle_err(ret, 9);
    }

    size_t local_item_size = 256;
    size_t global_item_size = round_to_closest_p2(num_threads, local_item_size);
    cl_event event;
    ret = clEnqueueNDRangeKernel(command_queue, *kernel, 1, NULL, &global_item_size, &local_item_size, 0, NULL, &event);
    cl_handle_err(ret, 10);
    clWaitForEvents(1, &event);
}