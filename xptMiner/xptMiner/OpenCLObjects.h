/*
 * OpenCLObjects.h
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

#ifndef OPENCLOBJECTS_H_
#define OPENCLOBJECTS_H_

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

#include <string>
#include <vector>
#include <map>

void check_error(int err_code);

// forward declarations so i can point to parents;
class OpenCLProgram;
class OpenCLContext;
class OpenCLKernel;
class OpenCLDevice;
class OpenCLPlatform;

class OpenCLBuffer {
public:
	OpenCLBuffer(cl_mem _buffer);
	~OpenCLBuffer();
	cl_mem buffer;
};

class OpenCLCommandQueue {
public:
	OpenCLCommandQueue(cl_command_queue _queue);
	~OpenCLCommandQueue();

	void enqueueWriteBuffer(OpenCLBuffer* dest, void* origin, size_t size);
	void enqueueWriteBufferBlocking(OpenCLBuffer* dest, void* origin, size_t size);
	void enqueueKernel1D(OpenCLKernel *kernel,	size_t worksize, size_t work_items);
	void enqueueReadBuffer(OpenCLBuffer* origin, void* dest, size_t size);
	void enqueueReadBufferBlocking(OpenCLBuffer* origin, void* dest, size_t size);

	void finish();
private:
	cl_command_queue queue;
};

class OpenCLKernel {
public:
	OpenCLKernel(cl_kernel _kernel, OpenCLProgram* _parent);
	~OpenCLKernel();

	cl_kernel getKernel();

	void resetArgs();
	void addScalarLong(cl_long arg);
	void addScalarInt(cl_int arg);
	void addScalarULong(cl_ulong arg);
	void addScalarUInt(cl_uint arg);
	void addGlobalArg(OpenCLBuffer* arg);
	void addLocalArg(size_t size);

	// info
	size_t getWorkGroupSize(OpenCLDevice * device);
private:
	cl_kernel kernel;
	// must know its parent;
	OpenCLProgram* parent;
	int arg_count;
};

class OpenCLProgram {
public:
	OpenCLProgram(cl_program _program, OpenCLContext* _parent);
	~OpenCLProgram();

	OpenCLKernel *getKernel(std::string name);
private:
	cl_program program;
	std::map<std::string, OpenCLKernel*> kernels;
	OpenCLContext* parent;
};

class OpenCLDevice {
public:
	OpenCLDevice(cl_device_id _id, OpenCLPlatform* _parent);
	virtual ~OpenCLDevice();
	std::string getName();
	long getMaxWorkGroupSize();
	long getMaxMemAllocSize();
	long getLocalMemSize();
	long getMaxParamSize();
	int getMaxWorkItemDimensions();
	std::vector<long> getMaxWorkItemSizes();
	cl_device_id getDeviceId();
	OpenCLPlatform* getPlatform();
	OpenCLContext* getContext();
private:
	cl_device_id my_id;
	OpenCLPlatform* parent;
	OpenCLContext* context;
};

class OpenCLContext {
public:
	OpenCLContext(cl_context _context, std::vector<OpenCLDevice*> _devices);
	OpenCLContext(cl_context _context, OpenCLDevice* _device);
	~OpenCLContext();

	OpenCLProgram * loadProgramFromFiles(std::vector<std::string> filename);
	OpenCLProgram * loadProgramFromStrings(std::vector<std::string> program);

	OpenCLCommandQueue* createCommandQueue(int deviceIndex=0);
	OpenCLCommandQueue* createCommandQueue(OpenCLDevice* device);

	// buffers are not stored in the context. Algos have the responsibility to dealocate them;
	OpenCLBuffer * createBuffer(size_t size, cl_mem_flags flags=CL_MEM_READ_WRITE, void* original=NULL);

	OpenCLProgram* getProgram(int pos);
private:
	cl_context context;
	std::vector<OpenCLProgram*> programs;
	std::vector<OpenCLDevice*> devices;
};

class OpenCLPlatform {
public:
	OpenCLPlatform(cl_platform_id id, cl_device_type device_type = CL_DEVICE_TYPE_ALL);
	virtual ~OpenCLPlatform();

	OpenCLDevice* getDevice(int pos);
	int getNumDevices();

	std::string getName();
	cl_platform_id getId();
private:
	std::vector<OpenCLDevice *> devices;
	cl_platform_id my_id;
};

class OpenCLMain {
public:
	static OpenCLMain& getInstance() {
        static OpenCLMain instance; // Guaranteed to be destroyed.
        return instance;
	}
	OpenCLPlatform* getPlatform(int pos);
	int getNumPlatforms();
	OpenCLDevice* getDevice(int pos);
	OpenCLDevice* getDevice(int platform_pos, int pos);
	int getNumDevices();
	void listDevices();
private:
	OpenCLMain();
	virtual ~OpenCLMain();
	// avoid copies
	OpenCLMain(OpenCLMain const&);     // Don't Implement
    void operator=(OpenCLMain const&); // Don't implement

	std::vector<OpenCLPlatform*> platforms;
};

typedef struct _error_struct {
	int err_code;
	std::string err_name;
	std::string err_funcs;
	std::string err_desc;
} error_struct;

// error codes
extern error_struct _errors[];
#endif /* OPENCLOBJECTS_H_ */
