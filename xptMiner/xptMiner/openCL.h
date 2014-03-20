#include"CL/cl.h"

void openCL_init();
cl_context openCL_getActiveContext();
cl_device_id* openCL_getActiveDeviceID();
cl_command_queue openCL_getActiveCommandQueue();