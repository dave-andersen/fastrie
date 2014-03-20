/*
 * OpenCLObjects.cpp
 *
 *  Created on: 08/01/2014
 *      Author: girino
 *
 * Copyright (c) 2014 Girino Vey.
 *
 * All code in this file is copyrighted to me, Girino Vey, and licensed under Girino's
 * Anarchist License, available at http://girino.org/license and is available on this
 * repository as the file girino_license.txt
 *
 */
#include "OpenCLObjects.h"
#include <fstream>
#include <iostream>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

error_struct _errors[] = {
{-1,"CL_DEVICE_NOT_FOUND","clGetDeviceIDs","if no OpenCL devices that matched device_type were found."},
{-2,"CL_DEVICE_NOT_AVAILABLE","clCreateContext","if a device in devices is currently not available even though the device was returned by clGetDeviceIDs."},
{-3,"CL_COMPILER_NOT_AVAILABLE","clBuildProgram","if program is created with clCreateProgramWithSource and a compiler is not available i.e. CL_DEVICE_COMPILER_AVAILABLE specified in the table of OpenCL Device Queries for clGetDeviceInfo is set to CL_FALSE."},
{-4,"CL_MEM_OBJECT_ALLOCATION_FAILURE","","if there is a failure to allocate memory for buffer object."},
{-5,"CL_OUT_OF_RESOURCES","","if there is a failure to allocate resources required by the OpenCL implementation on the device."},
{-6,"CL_OUT_OF_HOST_MEMORY","","if there is a failure to allocate resources required by the OpenCL implementation on the host."},
{-7,"CL_PROFILING_INFO_NOT_AVAILABLE","clGetEventProfilingInfo","if the CL_QUEUE_PROFILING_ENABLE flag is not set for the command-queue, if the execution status of the command identified by event is not CL_COMPLETE or if event is a user event object."},
{-8,"CL_MEM_COPY_OVERLAP","clEnqueueCopyBuffer, clEnqueueCopyBufferRect, clEnqueueCopyImage","if src_buffer and dst_buffer are the same buffer or subbuffer object and the source and destination regions overlap or if src_buffer and dst_buffer are different sub-buffers of the same associated buffer object and they overlap. The regions overlap if src_offset <= to dst_offset <= to src_offset + size - 1, or if dst_offset <= to src_offset <= to dst_offset + size - 1."},
{-9,"CL_IMAGE_FORMAT_MISMATCH","clEnqueueCopyImage","if src_image and dst_image do not use the same image format."},
{-10,"CL_IMAGE_FORMAT_NOT_SUPPORTED","clCreateImage","if the image_format is not supported."},
{-11,"CL_BUILD_PROGRAM_FAILURE","clBuildProgram","if there is a failure to build the program executable. This error will be returned if clBuildProgram does not return until the build has completed."},
{-12,"CL_MAP_FAILURE","clEnqueueMapBuffer, clEnqueueMapImage"," if there is a failure to map the requested region into the host address space. This error cannot occur for image objects created with CL_MEM_USE_HOST_PTR or CL_MEM_ALLOC_HOST_PTR."},
{-13,"CL_MISALIGNED_SUB_BUFFER_OFFSET","","if a sub-buffer object is specified as the value for an argument that is a buffer object and the offset specified when the sub-buffer object is created is not aligned to CL_DEVICE_MEM_BASE_ADDR_ALIGN value for device associated with queue."},
{-14,"CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST","","if the execution status of any of the events in event_list is a negative integer value."},
{-15,"CL_COMPILE_PROGRAM_FAILURE","clCompileProgram","if there is a failure to compile the program source. This error will be returned if clCompileProgram does not return until the compile has completed."},
{-16,"CL_LINKER_NOT_AVAILABLE","clLinkProgram","if a linker is not available i.e. CL_DEVICE_LINKER_AVAILABLE specified in the table of allowed values for param_name for clGetDeviceInfo is set to CL_FALSE."},
{-17,"CL_LINK_PROGRAM_FAILURE","clLinkProgram","if there is a failure to link the compiled binaries and/or libraries."},
{-18,"CL_DEVICE_PARTITION_FAILED","clCreateSubDevices"," if the partition name is supported by the implementation but in_device could not be further partitioned."},
{-19,"CL_KERNEL_ARG_INFO_NOT_AVAILABLE","clGetKernelArgInfo","if the argument information is not available for kernel."},
{-30,"CL_INVALID_VALUE","clGetDeviceIDs, clCreateContext","This depends on the function: two or more coupled parameters had errors."},
{-31,"CL_INVALID_DEVICE_TYPE","clGetDeviceIDs","if an invalid device_type is given"},
{-32,"CL_INVALID_PLATFORM","clGetDeviceIDs","if an invalid platform was given"},
{-33,"CL_INVALID_DEVICE","clCreateContext, clBuildProgram","if devices contains an invalid device or are not associated with the specified platform."},
{-34,"CL_INVALID_CONTEXT","","if context is not a valid context."},
{-35,"CL_INVALID_QUEUE_PROPERTIES","clCreateCommandQueue","if specified command-queue-properties are valid but are not supported by the device."},
{-36,"CL_INVALID_COMMAND_QUEUE","","if command_queue is not a valid command-queue."},
{-37,"CL_INVALID_HOST_PTR","clCreateImage, clCreateBuffer","This flag is valid only if host_ptr is not NULL. If specified, it indicates that the application wants the OpenCL implementation to allocate memory for the memory object and copy the data from memory referenced by host_ptr.CL_MEM_COPY_HOST_PTR and CL_MEM_USE_HOST_PTR are mutually exclusive.CL_MEM_COPY_HOST_PTR can be used with CL_MEM_ALLOC_HOST_PTR to initialize the contents of the cl_mem object allocated using host-accessible (e.g. PCIe) memory."},
{-38,"CL_INVALID_MEM_OBJECT","","if memobj is not a valid OpenCL memory object."},
{-39,"CL_INVALID_IMAGE_FORMAT_DESCRIPTOR","","if the OpenGL/DirectX texture internal format does not map to a supported OpenCL image format."},
{-40,"CL_INVALID_IMAGE_SIZE","","if an image object is specified as an argument value and the image dimensions (image width, height, specified or compute row and/or slice pitch) are not supported by device associated with queue."},
{-41,"CL_INVALID_SAMPLER","clGetSamplerInfo, clReleaseSampler, clRetainSampler, clSetKernelArg","if sampler is not a valid sampler object."},
{-42,"CL_INVALID_BINARY","clCreateProgramWithBinary, clBuildProgram",""},
{-43,"CL_INVALID_BUILD_OPTIONS","clBuildProgram","if the build options specified by options are invalid."},
{-44,"CL_INVALID_PROGRAM","","if program is a not a valid program object."},
{-45,"CL_INVALID_PROGRAM_EXECUTABLE","","if there is no successfully built program executable available for device associated with command_queue."},
{-46,"CL_INVALID_KERNEL_NAME","clCreateKernel","if kernel_name is not found in program."},
{-47,"CL_INVALID_KERNEL_DEFINITION","clCreateKernel","if the function definition for __kernel function given by kernel_name such as the number of arguments, the argument types are not the same for all devices for which the program executable has been built."},
{-48,"CL_INVALID_KERNEL","","if kernel is not a valid kernel object."},
{-49,"CL_INVALID_ARG_INDEX","clSetKernelArg, clGetKernelArgInfo","if arg_index is not a valid argument index."},
{-50,"CL_INVALID_ARG_VALUE","clSetKernelArg, clGetKernelArgInfo","if arg_value specified is not a valid value."},
{-51,"CL_INVALID_ARG_SIZE","clSetKernelArg","if arg_size does not match the size of the data type for an argument that is not a memory object or if the argument is a memory object and arg_size != sizeof(cl_mem) or if arg_size is zero and the argument is declared with the __local qualifier or if the argument is a sampler and arg_size != sizeof(cl_sampler)."},
{-52,"CL_INVALID_KERNEL_ARGS","","if the kernel argument values have not been specified."},
{-53,"CL_INVALID_WORK_DIMENSION","","if work_dim is not a valid value (i.e. a value between 1 and 3)."},
{-54,"CL_INVALID_WORK_GROUP_SIZE","","if local_work_size is specified and number of work-items specified by global_work_size is not evenly divisable by size of work-group given by local_work_size or does not match the work-group size specified for kernel using the __attribute__ ((reqd_work_group_size(X, Y, Z))) qualifier in program source.if local_work_size is specified and the total number of work-items in the work-group computed as local_work_size[0] *_local_work_size[work_dim - 1] is greater than the value specified by CL_DEVICE_MAX_WORK_GROUP_SIZE in the table of OpenCL Device Queries for clGetDeviceInfo.if local_work_size is NULL and the __attribute__ ((reqd_work_group_size(X, Y, Z))) qualifier is used to declare the work-group size for kernel in the program source."},
{-55,"CL_INVALID_WORK_ITEM_SIZE","","if the number of work-items specified in any of local_work_size[0], _local_work_size[work_dim - 1] is greater than the corresponding values specified by CL_DEVICE_MAX_WORK_ITEM_SIZES[0], _. CL_DEVICE_MAX_WORK_ITEM_SIZES[work_dim - 1]."},
{-56,"CL_INVALID_GLOBAL_OFFSET","","if the value specified in global_work_size + the corresponding values in global_work_offset for any dimensions is greater than the sizeof(size_t) for the device on which the kernel execution will be enqueued."},
{-57,"CL_INVALID_EVENT_WAIT_LIST","","if event_wait_list is NULL and num_events_in_wait_list > 0, or event_wait_list is not NULL and num_events_in_wait_list is 0, or if event objects in event_wait_list are not valid events."},
{-58,"CL_INVALID_EVENT","","if event objects specified in event_list are not valid event objects."},
{-59,"CL_INVALID_OPERATION","","if interoperability is specified by setting CL_CONTEXT_ADAPTER_D3D9_KHR, CL_CONTEXT_ADAPTER_D3D9EX_KHR or CL_CONTEXT_ADAPTER_DXVA_KHR to a non-NULL value, and interoperability with another graphics API is also specified. (only if the cl_khr_dx9_media_sharing extension is supported)."},
{-60,"CL_INVALID_GL_OBJECT","","if texture is not a GL texture object whose type matches texture_target, if the specified miplevel of texture is not defined, or if the width or height of the specified miplevel is zero."},
{-61,"CL_INVALID_BUFFER_SIZE","clCreateBuffer, clCreateSubBuffer","if size is 0.Implementations may return CL_INVALID_BUFFER_SIZE if size is greater than the CL_DEVICE_MAX_MEM_ALLOC_SIZE value specified in the table of allowed values for param_name for clGetDeviceInfo for all devices in context."},
{-62,"CL_INVALID_MIP_LEVEL","OpenGL-functions","if miplevel is greater than zero and the OpenGL implementation does not support creating from non-zero mipmap levels."},
{-63,"CL_INVALID_GLOBAL_WORK_SIZE","","if global_work_size is NULL, or if any of the values specified in global_work_size[0], _global_work_size [work_dim - 1] are 0 or exceed the range given by the sizeof(size_t) for the device on which the kernel execution will be enqueued."},
{-64,"CL_INVALID_PROPERTY","clCreateContext","Vague error, depends on the function"},
{-65,"CL_INVALID_IMAGE_DESCRIPTOR","clCreateImage","if values specified in image_desc are not valid or if image_desc is NULL."},
{-66,"CL_INVALID_COMPILER_OPTIONS","clCompileProgram","if the compiler options specified by options are invalid."},
{-67,"CL_INVALID_LINKER_OPTIONS","clLinkProgram","if the linker options specified by options are invalid."},
{-68,"CL_INVALID_DEVICE_PARTITION_COUNT","clCreateSubDevices","if the partition name specified in properties is CL_DEVICE_PARTITION_BY_COUNTS and the number of sub-devices requested exceeds CL_DEVICE_PARTITION_MAX_SUB_DEVICES or the total number of compute units requested exceeds CL_DEVICE_PARTITION_MAX_COMPUTE_UNITS for in_device, or the number of compute units requested for one or more sub-devices is less than zero or the number of sub-devices requested exceeds CL_DEVICE_PARTITION_MAX_COMPUTE_UNITS for in_device."},
{-1000,"CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR","clGetGLContextInfoKHR, clCreateContext","CL and GL not on the same device (only when using a GPU)."},
{-1001,"","clGetPlatform","No valid ICDs found"},
{0,"CL_SUCCESS","","The sweet spot."} // terminator
};

void print_err_msg(cl_int err_code) {
	for (int i = 0; _errors[i].err_code < 0; i++) {
		if (err_code == _errors[i].err_code) {
			std::cerr << "ERROR: " << _errors[i].err_code << ", "
					<< _errors[i].err_name << ", " << _errors[i].err_desc
					<< std::endl;
			break;
		}
	}
}

#define check_error(x) {cl_int _MY_ERR_X = x; if (_MY_ERR_X != CL_SUCCESS) print_err_msg(_MY_ERR_X); assert(_MY_ERR_X == CL_SUCCESS);}

//void check_error(cl_int err_code) {
//	if (err_code != CL_SUCCESS) {
//		print_err_msg(err_code);
//		assert(err_code == CL_SUCCESS);
//	}
//}

void error_callback_func (const char *errinfo,
                    const void *private_info, size_t cb,
                    void *user_data) {
#ifdef DEBUG
	std::cerr << "ERROR (callback): " << errinfo << std::endl;
#endif
}

OpenCLPlatform::OpenCLPlatform(cl_platform_id id, cl_device_type device_type) {
	my_id = id;
	// debug
#ifdef DEBUG
	char name[128];
	check_error(clGetPlatformInfo(my_id, CL_PLATFORM_NAME, 128 * sizeof(char), name,
			NULL));
	fprintf(stdout, "Platform: %s\n", name);
#endif

	// devices;
	cl_uint num_devices;
	check_error(clGetDeviceIDs(my_id, device_type, 0, NULL, &num_devices));

	if (num_devices <= 0) {
		fprintf(stderr, "ERROR: no valid devices found\n");
		assert(num_devices > 0);
	}

	// iterates through devices
	cl_device_id all_devices[num_devices];
	check_error(clGetDeviceIDs(my_id, device_type, num_devices, all_devices, NULL));
	for (int i = 0; i < num_devices; i++) {
		devices.push_back(new OpenCLDevice(all_devices[i], this));
	}
}

OpenCLPlatform::~OpenCLPlatform() {
	for (int i = 0; i < devices.size(); i++) {
		delete devices[i];
	}
}

OpenCLDevice* OpenCLPlatform::getDevice(int pos) {
	return devices[pos];
}

int OpenCLPlatform::getNumDevices() {
	return devices.size();
}


OpenCLMain::OpenCLMain() {
	// lazy instantiation, inits with NULL
	unsigned int num_platforms;
	check_error(clGetPlatformIDs(0, NULL, &num_platforms));
	if (num_platforms <= 0) {
		fprintf(stderr, "ERROR: no valid platforms found\n");
		assert(num_platforms > 0);
	}
	cl_platform_id all_platforms[num_platforms];
	check_error(clGetPlatformIDs(num_platforms, all_platforms, NULL));
	for (int i = 0; i < num_platforms; i++) {
		platforms.push_back(new OpenCLPlatform(all_platforms[i]));
	}
}

OpenCLMain::~OpenCLMain() {
	for (int i = 0; i < platforms.size(); i++) {
		delete platforms[i];
	}
}

OpenCLPlatform* OpenCLMain::getPlatform(int pos) {
	return platforms[pos];
}

OpenCLDevice::OpenCLDevice(cl_device_id _id, OpenCLPlatform* _parent) {
	my_id = _id;
	parent = _parent;
	// debug
#ifdef DEBUG
	char name[128];
	check_error(clGetDeviceInfo(my_id, CL_DEVICE_NAME, 128 * sizeof(char), name,
			NULL));
	fprintf(stdout, "  Device: %s\n", name);
#endif

	// inits kernels or whatever...
	context = NULL; // lazy instantiation

}
OpenCLDevice::~OpenCLDevice() {
	//clReleaseDevice(this->my_id);
}

cl_device_id OpenCLDevice::getDeviceId() {
	return my_id;
}


std::string OpenCLDevice::getName() {
	size_t param_value_size_ret;
	check_error(clGetDeviceInfo(my_id, CL_DEVICE_NAME, 0, NULL, &param_value_size_ret));
	char name[param_value_size_ret];
	check_error(clGetDeviceInfo(my_id, CL_DEVICE_NAME, param_value_size_ret, name, NULL));
	return std::string(name);
}

long OpenCLDevice::getMaxWorkGroupSize() {
	cl_ulong value;
	check_error(clGetDeviceInfo(my_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, (sizeof(cl_ulong)), &value, NULL));
	return value;
}

long OpenCLDevice::getMaxMemAllocSize() {
	cl_ulong value;
	check_error(clGetDeviceInfo(my_id, CL_DEVICE_MAX_MEM_ALLOC_SIZE, (sizeof(cl_ulong)), &value, NULL));
	return value;
}

long OpenCLDevice::getLocalMemSize() {
	cl_ulong value;
	check_error(clGetDeviceInfo(my_id, CL_DEVICE_LOCAL_MEM_SIZE, (sizeof(cl_ulong)), &value, NULL));
	return value;
}

long OpenCLDevice::getMaxParamSize() {
	cl_ulong value;
	check_error(clGetDeviceInfo(my_id, CL_DEVICE_MAX_PARAMETER_SIZE, (sizeof(cl_ulong)), &value, NULL));
	return value;
}

int OpenCLDevice::getMaxWorkItemDimensions() {
	cl_uint value;
	check_error(clGetDeviceInfo(my_id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, (sizeof(cl_uint)), &value, NULL));
	return value;
}

std::vector<long> OpenCLDevice::getMaxWorkItemSizes() {
	int size = getMaxWorkItemDimensions();
	cl_ulong value[size];
	check_error(clGetDeviceInfo(my_id, CL_DEVICE_MAX_WORK_ITEM_SIZES, (sizeof(cl_ulong)*size), &value, NULL));
	std::vector<long> ret;
	for (int i = 0; i < size; i++) {
		ret.push_back(value[i]);
	}
	return ret;
}

OpenCLContext::OpenCLContext(cl_context _context, std::vector<OpenCLDevice*> _devices) {
	context = _context;
	devices = _devices;
}

OpenCLContext::OpenCLContext(cl_context _context, OpenCLDevice* _device) {
	context = _context;
	devices.push_back(_device);
}

OpenCLContext::~OpenCLContext() {
	for (int i = 0; i < programs.size(); i++) {
		delete programs[i];
	}
	clReleaseContext(context);
}

OpenCLProgram::OpenCLProgram(cl_program _program, OpenCLContext* _parent) {
	program = _program;
	parent = _parent;
}

OpenCLProgram::~OpenCLProgram() {
	for (std::map<std::string, OpenCLKernel*>::iterator it=kernels.begin(); it!=kernels.end(); ++it)
	    delete it->second;
	clReleaseProgram(program);
}

OpenCLProgram* OpenCLContext::loadProgramFromFiles(std::vector<std::string> filenames) {
	std::vector<std::string> file_strs;
	for (int i = 0; i < filenames.size(); i++) {
		std::ifstream file (filenames[i].c_str());
		std::string file_str((std::istreambuf_iterator<char>(file)),
		                 std::istreambuf_iterator<char>());
		file_strs.push_back(file_str);
	}
	return loadProgramFromStrings(file_strs);
}

OpenCLProgram* OpenCLContext::loadProgramFromStrings(std::vector<std::string> file_strs) {

	const char * str_ptr[file_strs.size()];
	size_t size_ptr[file_strs.size()];
	for (int i = 0; i < file_strs.size(); i++) {
		str_ptr[i] = file_strs[i].c_str();
		size_ptr[i] = file_strs[i].size();
	}

	cl_int error;
	cl_program program = clCreateProgramWithSource (context, file_strs.size(), str_ptr, size_ptr, &error);
	check_error(error);

	// build for all devices, I can implement different devices later;
	error = clBuildProgram(program, 0, NULL, "", NULL, NULL);
	if (error) {
		print_err_msg(error);
		// uses first device for error messages
		size_t length;
		check_error(clGetProgramBuildInfo(program, devices[0]->getDeviceId(), CL_PROGRAM_BUILD_LOG, 0, NULL, &length));
		char buffer[length+1];
		check_error(clGetProgramBuildInfo(program, devices[0]->getDeviceId(), CL_PROGRAM_BUILD_LOG, length, buffer, &length));
		std::cout<<"--- Build log ---"<<std::endl<<buffer<<std::endl<<"---  End log  ---"<<std::endl;
		assert(!error);
	}

	OpenCLProgram* ret = new OpenCLProgram(program, this);
	programs.push_back(ret);
	return ret;
}

OpenCLContext* OpenCLDevice::getContext() {

	if (context == NULL) {
		// creates the context
		int error;
		cl_context_properties properties[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)parent->getId(), 0};
		context = new OpenCLContext(clCreateContext (properties, 1, &my_id, error_callback_func, NULL, &error), this);
		check_error(error);
	}
	return context;
}

int not_main(int argc, char **argv) {
	std::vector<std::string> files;
	files.push_back("opencl/cryptsha512_kernel.cl");
	files.push_back("OpenCLMomentum.cl");
	//OpenCLMain::getInstance().getPlatform(0)->getContext()->loadProgramFromFiles(files);
}

OpenCLKernel* OpenCLProgram::getKernel(std::string name) {
	if (kernels.find(name) == kernels.end()) {
		int error;
		cl_kernel kernel = clCreateKernel (program, name.c_str(), &error);
		check_error(error);
		kernels[name] = new OpenCLKernel(kernel, this);
	}
	return kernels[name];
}

OpenCLKernel::OpenCLKernel(cl_kernel _kernel, OpenCLProgram* _parent) {
	kernel = _kernel;
	parent = _parent;
	arg_count = 0;
}

OpenCLKernel::~OpenCLKernel() {
	clReleaseKernel(kernel);
}

//OpenCLCommandQueue* OpenCLContext::createCommandQueue(int deviceIndex) {
//	OpenCLDevice* device = devices[deviceIndex];
//	return createCommandQueue(device);
//}

OpenCLCommandQueue* OpenCLContext::createCommandQueue(OpenCLDevice* device) {
	cl_int error;
	cl_command_queue ret = clCreateCommandQueue (context, device->getDeviceId(), 0, &error);
	check_error(error);
	return new OpenCLCommandQueue(ret);
}

OpenCLBuffer* OpenCLContext::createBuffer(size_t size, cl_mem_flags flags,
		void* original) {
	int error;
	cl_mem ret = clCreateBuffer(this->context, flags, size, original, &error);
	check_error(error);
	return new OpenCLBuffer(ret);
}

OpenCLBuffer::OpenCLBuffer(cl_mem _buffer) {
	this->buffer = _buffer;
}

OpenCLBuffer::~OpenCLBuffer() {
	clReleaseMemObject(buffer);
}

OpenCLCommandQueue::OpenCLCommandQueue(cl_command_queue _queue) {
	this->queue = _queue;
}

OpenCLCommandQueue::~OpenCLCommandQueue() {
	clReleaseCommandQueue(queue);
}

void OpenCLKernel::resetArgs() {
	arg_count = 0;
}

void OpenCLKernel::addGlobalArg(OpenCLBuffer *buffer) {
	check_error(clSetKernelArg (kernel, arg_count, sizeof(buffer->buffer), &(buffer->buffer)));
	arg_count++;
}

void OpenCLKernel::addScalarLong(cl_long arg) {
	check_error(clSetKernelArg (kernel, arg_count, sizeof(arg), &arg));
	arg_count++;
}

void OpenCLKernel::addScalarInt(cl_int arg) {
	check_error(clSetKernelArg (kernel, arg_count, sizeof(arg), &arg));
	arg_count++;
}

void OpenCLKernel::addScalarULong(cl_ulong arg) {
	check_error(clSetKernelArg (kernel, arg_count, sizeof(arg), &arg));
	arg_count++;
}

void OpenCLKernel::addScalarUInt(cl_uint arg) {
	check_error(clSetKernelArg (kernel, arg_count, sizeof(arg), &arg));
	arg_count++;
}

cl_kernel OpenCLKernel::getKernel() {
	return this->kernel;
}

void OpenCLKernel::addLocalArg(size_t size) {
	check_error(clSetKernelArg (kernel, arg_count, size, NULL));
	arg_count++;
}

void OpenCLCommandQueue::enqueueWriteBuffer(OpenCLBuffer* dest, void* origin,
		size_t size) {
	check_error(clEnqueueWriteBuffer(this->queue, dest->buffer, CL_FALSE, 0, size, origin, 0, NULL, NULL));
}

void OpenCLCommandQueue::enqueueWriteBufferBlocking(OpenCLBuffer* dest,
		void* origin, size_t size) {
	check_error(clEnqueueWriteBuffer(this->queue, dest->buffer, CL_TRUE, 0, size, origin, 0, NULL, NULL));
}

void OpenCLCommandQueue::enqueueKernel1D(OpenCLKernel *kernel,
		size_t worksize, size_t work_items) {
	check_error(clEnqueueNDRangeKernel (this->queue, kernel->getKernel(), 1, NULL, &worksize, &work_items, 0, NULL, NULL));
}

void OpenCLCommandQueue::enqueueReadBuffer(OpenCLBuffer* origin, void* dest,
		size_t size) {
	check_error(clEnqueueReadBuffer(this->queue, origin->buffer, CL_FALSE, 0, size, dest, 0, NULL, NULL));
}

void OpenCLCommandQueue::enqueueReadBufferBlocking(OpenCLBuffer* origin,
		void* dest, size_t size) {
	check_error(clEnqueueReadBuffer(this->queue, origin->buffer, CL_TRUE, 0, size, dest, 0, NULL, NULL));
}

void OpenCLCommandQueue::finish() {
	check_error(clFinish(this->queue));
}

OpenCLProgram* OpenCLContext::getProgram(int pos) {
	return this->programs[pos];
}

size_t OpenCLKernel::getWorkGroupSize(OpenCLDevice* device) {
	size_t ret = 0;
	check_error(clGetKernelWorkGroupInfo(this->kernel, device->getDeviceId(),
				CL_KERNEL_WORK_GROUP_SIZE, sizeof(ret), &ret, NULL));
	return ret;
}

int OpenCLMain::getNumPlatforms() {
	return platforms.size();
}

OpenCLDevice* OpenCLMain::getDevice(int pos) {
	int count = 0;
	for(int i = 0; i < getNumPlatforms(); i++) {
		OpenCLPlatform* p = getPlatform(i);
		for (int j = 0; j < p->getNumDevices(); j++) {
			if (count == pos) {
				return p->getDevice(j);
			}
			count++;
		}
	}
	fprintf(stderr, "WARNING: Device not found %d\n", pos);
	return NULL;
}

OpenCLDevice* OpenCLMain::getDevice(int platform_pos, int pos) {
	return getPlatform(platform_pos)->getDevice(pos);
}

int OpenCLMain::getNumDevices() {
	int count = 0;
	for(int i = 0; i < getNumPlatforms(); i++) {
		count += getPlatform(i)->getNumDevices();
	}
	return count;
}

OpenCLPlatform* OpenCLDevice::getPlatform() {
	return this->parent;
}

void OpenCLMain::listDevices() {
	int count = 0;
	for(int i = 0; i < getNumPlatforms(); i++) {
		OpenCLPlatform* p = getPlatform(i);
		printf("Platform %02d: %s\n", i, p->getName().c_str());
		for (int j = 0; j < p->getNumDevices(); j++) {
			printf("  Device %02d: %s\n", count, p->getDevice(j)->getName().c_str());
			p->getDevice(j);
			count++;
		}
	}
}

std::string OpenCLPlatform::getName() {
	size_t param_value_size_ret;
	check_error(clGetPlatformInfo(my_id, CL_PLATFORM_NAME, 0, NULL, &param_value_size_ret));
	char name[param_value_size_ret];
	check_error(clGetPlatformInfo(my_id, CL_PLATFORM_NAME, param_value_size_ret, name, NULL));
	return std::string(name);
}

cl_platform_id OpenCLPlatform::getId() {
	return my_id;
}
