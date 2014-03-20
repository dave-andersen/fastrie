#include"global.h"
#ifdef __linux__
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#endif 

CL_API_ENTRY cl_int (CL_API_CALL *clGetDeviceIDs)(cl_platform_id, cl_device_type, cl_uint, cl_device_id*, cl_uint*);

CL_API_ENTRY cl_context (CL_API_CALL
						 *clCreateContext)(const cl_context_properties * /* properties */,
						 cl_uint                 /* num_devices */,
						 const cl_device_id *    /* devices */,
						 void (CL_CALLBACK * /* pfn_notify */)(const char *, const void *, size_t, void *),
						 void *                  /* user_data */,
						 cl_int *                /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_command_queue (CL_API_CALL
							   *clCreateCommandQueue)(cl_context                     /* context */, 
							   cl_device_id                   /* device */, 
							   cl_command_queue_properties    /* properties */,
							   cl_int *                       /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clGetPlatformIDs)(cl_uint          /* num_entries */,
					 cl_platform_id * /* platforms */,
					 cl_uint *        /* num_platforms */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_mem (CL_API_CALL
					 *clCreateBuffer)(cl_context   /* context */,
					 cl_mem_flags /* flags */,
					 size_t       /* size */,
					 void *       /* host_ptr */,
					 cl_int *     /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_program (CL_API_CALL
						 *clCreateProgramWithSource)(cl_context        /* context */,
						 cl_uint           /* count */,
						 const char **     /* strings */,
						 const size_t *    /* lengths */,
						 cl_int *          /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clBuildProgram)(cl_program           /* program */,
					 cl_uint              /* num_devices */,
					 const cl_device_id * /* device_list */,
					 const char *         /* options */, 
					 void (CL_CALLBACK *  /* pfn_notify */)(cl_program /* program */, void * /* user_data */),
					 void *               /* user_data */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clGetProgramBuildInfo)(cl_program            /* program */,
					 cl_device_id          /* device */,
					 cl_program_build_info /* param_name */,
					 size_t                /* param_value_size */,
					 void *                /* param_value */,
					 size_t *              /* param_value_size_ret */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_kernel (CL_API_CALL
						*clCreateKernel)(cl_program      /* program */,
						const char *    /* kernel_name */,
						cl_int *        /* errcode_ret */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clSetKernelArg)(cl_kernel    /* kernel */,
					 cl_uint      /* arg_index */,
					 size_t       /* arg_size */,
					 const void * /* arg_value */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clEnqueueNDRangeKernel)(cl_command_queue /* command_queue */,
					 cl_kernel        /* kernel */,
					 cl_uint          /* work_dim */,
					 const size_t *   /* global_work_offset */,
					 const size_t *   /* global_work_size */,
					 const size_t *   /* local_work_size */,
					 cl_uint          /* num_events_in_wait_list */,
					 const cl_event * /* event_wait_list */,
					 cl_event *       /* event */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clEnqueueReadBuffer)(cl_command_queue    /* command_queue */,
					 cl_mem              /* buffer */,
					 cl_bool             /* blocking_read */,
					 size_t              /* offset */,
					 size_t              /* size */, 
					 void *              /* ptr */,
					 cl_uint             /* num_events_in_wait_list */,
					 const cl_event *    /* event_wait_list */,
					 cl_event *          /* event */) CL_API_SUFFIX__VERSION_1_0;

CL_API_ENTRY cl_int (CL_API_CALL
					 *clEnqueueWriteBuffer)(cl_command_queue   /* command_queue */, 
					 cl_mem             /* buffer */, 
					 cl_bool            /* blocking_write */, 
					 size_t             /* offset */, 
					 size_t             /* size */, 
					 const void *       /* ptr */, 
					 cl_uint            /* num_events_in_wait_list */, 
					 const cl_event *   /* event_wait_list */, 
					 cl_event *         /* event */) CL_API_SUFFIX__VERSION_1_0;
// global OpenCL stuff
cl_context openCL_context;
cl_command_queue openCL_queue;
cl_device_id openCL_device;

//typedef struct  
//{
//	bool kernelInited;
//	// constants
//	sint32 numSegments;
//	sint32 sizeSegment;
//	sint32 sieveMaskSize;
//	// memory buffers
//	//uint8* mem_maskC1;
//	//cl_mem buffer_maskC1;
//	//uint8* mem_maskC2;
//	//cl_mem buffer_maskC2;
//	//uint8* mem_maskBT;
//	//cl_mem buffer_maskBT;
//	cl_int nCLPrimesToSieve;
//	uint32* buffer_candidateList;
//	cl_mem mem_candidateList;
//	uint32 candidatesPerT;
//
//	uint32* mem_primes;
//	cl_mem buffer_primes;
//	uint32* mem_twoInverse;
//	cl_mem buffer_twoInverse;
//	uint32* mem_primeBase;
//	cl_mem buffer_primeBase;
//	// kernels
//	cl_kernel kernel_sievePrimes;
//	// number of workgroups (number of how many small sieves we generate for each kernel execution)
//	uint32 workgroupNum;
//}gpuCL_t;
//
//gpuCL_t gpuCL = {0};

void CL_API_CALL pfn_notifyCB (

							   const char *errinfo, 
							   const void *private_info, 
							   size_t cb, 
							   void *user_data
							   )
{
	printf("OpenCL: %s\n", errinfo);
}

void openCL_createContext()
{
	cl_int error = 0;   // Used to handle error codes
	cl_platform_id platformIds[32];
	cl_uint numPlatforms = 0;
	clGetPlatformIDs(32, platformIds, &numPlatforms);
	// CL_INVALID_PLATFORM
	printf("OpenCL platforms: %d\n", numPlatforms);
	if( numPlatforms != 1 )
	{
		__debugbreak();
	}
	// Device
	error = clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_GPU, 1, &openCL_device, NULL);
	if (error != CL_SUCCESS) {
		exit(error);
	}
	// Context
	openCL_context = clCreateContext(0, 1, &openCL_device, pfn_notifyCB, NULL, &error);
	if (error != CL_SUCCESS) {
		exit(error);
	}
	// Command-queue
	openCL_queue = clCreateCommandQueue(openCL_context, openCL_device, 0, &error);
	if (error != CL_SUCCESS) {
		exit(error);
	}
}

bool openCLInited = false;
void openCL_init()
{
	if( openCLInited == true )
		return;
	printf("Init OpenCL...\n");
	openCLInited = true;
#ifdef _WIN32
	HMODULE openCLModule = LoadLibraryA("opencl.dll");
#else
	void *openCLModule;
	double (*cosine)(double);
	char *error;
	   openCLModule = dlopen("libOpenCL.so", RTLD_LAZY);
    if (!openCLModule) {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }
	//*(void **) (&cosine) = dlsym(openCLModule, "cos");
#define GetProcAddress(Handle, Name) dlsym(Handle, Name)
#endif
	if( openCLModule == NULL )
	{
		printf("Failed to load OpenCL :(\n");
		exit(-1);
	}

	*(void**)&clGetDeviceIDs = (void*)GetProcAddress(openCLModule, "clGetDeviceIDs");
	*(void**)&clCreateContext = (void*)GetProcAddress(openCLModule, "clCreateContext");
	*(void**)&clCreateCommandQueue = (void*)GetProcAddress(openCLModule, "clCreateCommandQueue");
	*(void**)&clGetPlatformIDs = (void*)GetProcAddress(openCLModule, "clGetPlatformIDs");
	*(void**)&clCreateBuffer = (void*)GetProcAddress(openCLModule, "clCreateBuffer");
	*(void**)&clCreateProgramWithSource = (void*)GetProcAddress(openCLModule, "clCreateProgramWithSource");
	*(void**)&clBuildProgram = (void*)GetProcAddress(openCLModule, "clBuildProgram");
	*(void**)&clGetProgramBuildInfo = (void*)GetProcAddress(openCLModule, "clGetProgramBuildInfo");
	*(void**)&clCreateKernel = (void*)GetProcAddress(openCLModule, "clCreateKernel");
	*(void**)&clSetKernelArg = (void*)GetProcAddress(openCLModule, "clSetKernelArg");
	*(void**)&clEnqueueNDRangeKernel = (void*)GetProcAddress(openCLModule, "clEnqueueNDRangeKernel");
	*(void**)&clEnqueueReadBuffer = (void*)GetProcAddress(openCLModule, "clEnqueueReadBuffer");
	*(void**)&clEnqueueWriteBuffer = (void*)GetProcAddress(openCLModule, "clEnqueueWriteBuffer");

	printf("Creating OpenCL context...\n");
	openCL_createContext();
}

cl_context openCL_getActiveContext()
{
	return openCL_context;
}

cl_device_id* openCL_getActiveDeviceID()
{
	return &openCL_device;
}

cl_command_queue openCL_getActiveCommandQueue()
{
	return openCL_queue;
}
