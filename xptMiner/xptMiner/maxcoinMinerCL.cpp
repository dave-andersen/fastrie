#include"global.h"
#include "ticker.h"
bool maxcoinOpenCLKernelInited = false;

struct  
{
	// input buffer (block data)
	void* buffer_blockInputData;
	cl_mem clBuffer_blockInputData;
	// output buffer (nonce indices)
	void* buffer_nonceOutputData;
	cl_mem clBuffer_nonceOutputData;
	// kernel
	cl_kernel kernel_keccak;
}maxcoinGPU;

void maxcoinMiner_openCL_appendKeccakFunction(char* kernel_src)
{
	// static arrays
	strcat(kernel_src, 
	"__constant const unsigned long RC[] = {\r\n"
	"	0x0000000000000001, 0x0000000000008082,\r\n"
	"	0x800000000000808A, 0x8000000080008000,\r\n"
	"	0x000000000000808B, 0x0000000080000001,\r\n"
	"	0x8000000080008081, 0x8000000000008009,\r\n"
	"	0x000000000000008A, 0x0000000000000088,\r\n"
	"	0x0000000080008009, 0x000000008000000A,\r\n"
	"	0x000000008000808B, 0x800000000000008B,\r\n"
	"	0x8000000000008089, 0x8000000000008003,\r\n"
	"	0x8000000000008002, 0x8000000000000080,\r\n"
	"	0x000000000000800A, 0x800000008000000A,\r\n"
	"	0x8000000080008081, 0x8000000000008080,\r\n"
	"	0x0000000080000001, 0x8000000080008008\r\n"
	"};\r\n");


	// defines for keccak

	strcat(kernel_src,
		"#define ROL64(_t, x, n)   _t = ((x) << (n)) | ((x) >> (64 - (n)))\r\n\r\n");

	strcat(kernel_src, 
	"#define TH_ELT_O(t, c0, c1, c2, c3, c4, d0, d1, d2, d3, d4) \\\r\n"
	"	tt0 = d0^d1; \\\r\n"
	"	tt1 = d2^d3; \\\r\n"
	"	tt0 = tt0^d4; \\\r\n"
	"	tt0 = tt0^tt1; \\\r\n"
	"	ROL64(tt0, tt0, 1); \\\r\n"
	"	tt2 = c0^c1; \\\r\n"
	"	tt3 = c2^c3; \\\r\n"
	"	tt0 = tt0^c4; \\\r\n"
	"	tt2 = tt2^tt3; \\\r\n"
	"	t = tt0^tt2;\r\n\r\n");

	// keccak
	strcat(kernel_src, 
	"unsigned long keccak256_maxcoin_opt_v(__global unsigned long *data, unsigned long nonceAndBits)"
	"{"
	"	int j;\r\n"
	"	unsigned long t;\r\n"
	"	unsigned long c0;\r\n"
	"	unsigned long c1;\r\n"
	"	unsigned long c2;\r\n"
	"	unsigned long c3;\r\n"
	"	unsigned long c4;\r\n"
	"	unsigned long tt0;\r\n"
	"	unsigned long tt1;\r\n"
	"	unsigned long tt2;\r\n"
	"	unsigned long tt3;\r\n"
	"	unsigned long a00, a01, a02, a03, a04;\r\n"
	"	unsigned long a10, a11, a12, a13, a14;\r\n"
	"	unsigned long a20, a21, a22, a23, a24;\r\n"
	"	unsigned long a30, a31, a32, a33, a34;\r\n"
	"	unsigned long a40, a41, a42, a43, a44;\r\n"
	"	a00 = data[0];\r\n"
	"	a10 = ~data[1];\r\n"
	"	a20 = ~data[2];\r\n"
	"	a30 = data[3];\r\n"
	"	a40 = data[4];\r\n"
	"	a01 = data[5];\r\n"
	"	a11 = data[6];\r\n"
	"	a21 = data[7];\r\n"
	"	a31 = ~data[8];\r\n"
	//"	a41 = data[9];\r\n"
	"	a41 = nonceAndBits;\r\n"
	"	a02 = 1;\r\n"
	"	a12 = 0;\r\n"
	"	a22 = 0xFFFFFFFFFFFFFFFF;\r\n"
	"	a32 = 0;\r\n"
	"	a42 = 0;\r\n"
	"	a03 = 0;\r\n"
	"	a13 = 0x8000000000000000;\r\n"
	"	a23 = 0xFFFFFFFFFFFFFFFF;\r\n"
	"	a33 = 0;\r\n"
	"	a43 = 0;\r\n" 
	"	a04 = 0xFFFFFFFFFFFFFFFF;\r\n" 
	"	a14 = 0;\r\n"
	"	a24 = 0;\r\n"
	"	a34 = 0;\r\n"
	"	a44 = 0;\r\n"
	"	for (j = 0; j < 24-1; j++)"
	"	{\r\n"
	"		TH_ELT_O(c0, a40, a41, a42, a43, a44, a10, a11, a12, a13, a14);\r\n"
	"		TH_ELT_O(c1, a00, a01, a02, a03, a04, a20, a21, a22, a23, a24);\r\n"
	"		TH_ELT_O(c2, a10, a11, a12, a13, a14, a30, a31, a32, a33, a34);\r\n"
	"		TH_ELT_O(c3, a20, a21, a22, a23, a24, a40, a41, a42, a43, a44);\r\n"
	"		TH_ELT_O(c4, a30, a31, a32, a33, a34, a00, a01, a02, a03, a04);\r\n"
	"		a00 = a00 ^ c0;\r\n"
	"		a01 = a01 ^ c0;\r\n"
	"		a02 = a02 ^ c0;\r\n"
	"		a03 = a03 ^ c0;\r\n"
	"		a04 = a04 ^ c0;\r\n"
	"		a10 = a10 ^ c1;\r\n"
	"		a11 = a11 ^ c1;\r\n"
	"		a12 = a12 ^ c1;\r\n"
	"		a13 = a13 ^ c1;\r\n"
	"		a14 = a14 ^ c1;\r\n"
	"		a20 = a20 ^ c2;\r\n"
	"		a21 = a21 ^ c2;\r\n"
	"		a22 = a22 ^ c2;\r\n"
	"		a23 = a23 ^ c2;\r\n"
	"		a24 = a24 ^ c2;\r\n"
	"		a30 = a30 ^ c3;\r\n"
	"		a31 = a31 ^ c3;\r\n"
	"		a32 = a32 ^ c3;\r\n"
	"		a33 = a33 ^ c3;\r\n"
	"		a34 = a34 ^ c3;\r\n"
	"		a40 = a40 ^ c4;\r\n"
	"		a41 = a41 ^ c4;\r\n"
	"		a42 = a42 ^ c4;\r\n"
	"		a43 = a43 ^ c4;\r\n"
	"		a44 = a44 ^ c4;\r\n"
	"		ROL64(a01, a01, 36);\r\n"
	"		ROL64(a02, a02,  3);\r\n"
	"		ROL64(a03, a03, 41);\r\n"
	"		ROL64(a04, a04, 18);\r\n"
	"		ROL64(a10, a10,  1);\r\n"
	"		ROL64(a11, a11, 44);\r\n"
	"		ROL64(a12, a12, 10);\r\n"
	"		ROL64(a13, a13, 45);\r\n"
	"		ROL64(a14, a14,  2);\r\n"
	"		ROL64(a20, a20, 62);\r\n"
	"		ROL64(a21, a21,  6);\r\n"
	"		ROL64(a22, a22, 43);\r\n"
	"		ROL64(a23, a23, 15);\r\n"
	"		ROL64(a24, a24, 61);\r\n"
	"		ROL64(a30, a30, 28);\r\n"
	"		ROL64(a31, a31, 55);\r\n"
	"		ROL64(a32, a32, 25);\r\n"
	"		ROL64(a33, a33, 21);\r\n"
	"		ROL64(a34, a34, 56);\r\n"
	"		ROL64(a40, a40, 27);\r\n"
	"		ROL64(a41, a41, 20);\r\n"
	"		ROL64(a42, a42, 39);\r\n"
	"		ROL64(a43, a43,  8);\r\n"
	"		ROL64(a44, a44, 14);\r\n"
	"		t = ~a22;\r\n"
	"		c0 = a00 ^ (a11|a22);\r\n"
	"		c1 = a11 ^ (t|a33);\r\n"
	"		c2 = a22 ^ (a33&a44);\r\n"
	"		c3 = a33 ^ (a44|a00);\r\n"
	"		c4 = a44 ^ (a00&a11);\r\n"
	"		a00 = c0;\r\n"
	"		a11 = c1;\r\n"
	"		a22 = c2;\r\n"
	"		a33 = c3;\r\n"
	"		a44 = c4;\r\n"
	"		t = ~a24;\r\n"
	"		c0 = a30 ^ (a41|a02);\r\n"
	"		c1 = a41 ^ (a02&a13);\r\n"
	"		c2 = a02 ^ (a13|t);\r\n"
	"		c3 = a13 ^ (a24|a30);\r\n"
	"		c4 = a24 ^ (a30&a41);\r\n"
	"		a30 = c0;\r\n"
	"		a41 = c1;\r\n"
	"		a02 = c2;\r\n"
	"		a13 = c3;\r\n"
	"		a24 = c4;\r\n"
	"		t = ~a43;\r\n"
	"		c0 = a10 ^ (a21|a32);\r\n"
	"		c1 = a21 ^ (a32&a43);\r\n"
	"		c2 = a32 ^ (t&a04);\r\n"
	"		c3 = t ^ (a04|a10);\r\n"
	"		c4 = a04 ^ (a10&a21);\r\n"
	"		a10 = c0;\r\n"
	"		a21 = c1;\r\n"
	"		a32 = c2;\r\n"
	"		a43 = c3;\r\n"
	"		a04 = c4;\r\n"
	"		t = ~a23;\r\n"
	"		c0 = a40 ^ (a01&a12);\r\n"
	"		c1 = a01 ^ (a12|a23);\r\n"
	"		c2 = a12 ^ (t|a34);\r\n"
	"		c3 = t ^ (a34&a40);\r\n"
	"		c4 = a34 ^ (a40|a01);\r\n"
	"		a40 = c0;\r\n"
	"		a01 = c1;\r\n"
	"		a12 = c2;\r\n"
	"		a23 = c3;\r\n"
	"		a34 = c4;\r\n"
	"		t = ~a31;\r\n"
	"		c0 = a20 ^ (t&a42);\r\n"
	"		c1 = t ^ (a42|a03);\r\n"
	"		c2 = a42 ^ (a03&a14);\r\n"
	"		c3 = a03 ^ (a14|a20);\r\n"
	"		c4 = a14 ^ (a20&a31);\r\n"
	"		a20 = c0;\r\n"
	"		a31 = c1;\r\n"
	"		a42 = c2;\r\n"
	"		a03 = c3;\r\n"
	"		a14 = c4;\r\n"
	"		a00 ^= RC[j];\r\n"
	"		t = a01;\r\n"
	"		a01 = a30;\r\n"
	"		a30 = a33;\r\n"
	"		a33 = a23;\r\n"
	"		a23 = a12;\r\n"
	"		a12 = a21;\r\n"
	"		a21 = a02;\r\n"
	"		a02 = a10;\r\n"
	"		a10 = a11;\r\n"
	"		a11 = a41;\r\n"
	"		a41 = a24;\r\n"
	"		a24 = a42;\r\n"
	"		a42 = a04;\r\n"
	"		a04 = a20;\r\n"
	"		a20 = a22;\r\n"
	"		a22 = a32;\r\n"
	"		a32 = a43;\r\n"
	"		a43 = a34;\r\n"
	"		a34 = a03;\r\n"
	"		a03 = a40;\r\n"
	"		a40 = a44;\r\n"
	"		a44 = a14;\r\n"
	"		a14 = a31;\r\n"
	"		a31 = a13;\r\n"
	"		a13 = t;\r\n"
	"	}"
	"	TH_ELT_O(c0, a40, a41, a42, a43, a44, a10, a11, a12, a13, a14);\r\n"
	"	TH_ELT_O(c3, a20, a21, a22, a23, a24, a40, a41, a42, a43, a44);\r\n"
	"	TH_ELT_O(c4, a30, a31, a32, a33, a34, a00, a01, a02, a03, a04);\r\n"
	"	a00 = a00 ^ c0;\r\n"
	"	a33 = a33 ^ c3;\r\n"
	"	a44 = a44 ^ c4;\r\n"
	"	ROL64(a33, a33, 21);\r\n"
	"	ROL64(a44, a44, 14);\r\n"
	"	return a33 ^ (a44|a00);\r\n"
	"}");
}

void maxcoinMiner_openCL_generateOrUpdateKernel()
{
	if( maxcoinOpenCLKernelInited )
		return;
	maxcoinOpenCLKernelInited = true;
	printf("Compiling OpenCL kernel...\n");

	char* kernel_src = (char*)malloc(1024*512);
	strcpy(kernel_src, "");

	cl_int clerr = 0;
	// init input buffer
	maxcoinGPU.buffer_blockInputData = malloc(80+8); // endian swapped block data + share target attached at the end
	memset(maxcoinGPU.buffer_blockInputData, 0x00, 80+8);
	maxcoinGPU.clBuffer_blockInputData = clCreateBuffer(openCL_getActiveContext(), CL_MEM_READ_WRITE, 88, maxcoinGPU.buffer_blockInputData, &clerr);
	// init output buffer
	sint32 outputBlocks = 256;
	maxcoinGPU.buffer_nonceOutputData = malloc(outputBlocks*4*sizeof(uint32));
	memset(maxcoinGPU.buffer_nonceOutputData, 0x00, outputBlocks*4*sizeof(uint32));
	maxcoinGPU.clBuffer_nonceOutputData = clCreateBuffer(openCL_getActiveContext(), CL_MEM_READ_WRITE, outputBlocks*4*sizeof(uint32), maxcoinGPU.buffer_nonceOutputData, &clerr);

	maxcoinMiner_openCL_appendKeccakFunction(kernel_src);

	strcat(kernel_src, "__kernel void xptMiner_cl_maxcoin_keccak(__global unsigned long *blockData, __global unsigned int *nonceIndexOut)\r\n");
	strcat(kernel_src, "{\r\n");

	strcat(kernel_src, 
	"unsigned long nonceAndBits = blockData[9] & 0x00000000FFFFFFFF;\r\n"
	"unsigned long shareTarget = blockData[10];\r\n"
	"nonceIndexOut[get_local_id(0)] = 0xFFFFFFFF;\r\n"

	"nonceAndBits += 0x100000000UL*0x1000UL*(unsigned long)get_local_id(0);\r\n"
	
	//"nonceAndBits = 0x01f94bdb00000000UL;\r\n"

	"for(int i=0; i<0x1000; i++) {\r\n"
	//"for(int i=0; i<1; i++) {\r\n"
	//"if( keccak256_maxcoin_opt_v(blockData, nonceAndBits) < shareTarget ) nonceIndexOut[0] = nonceAndBits>>32;\r\n"
	"if( keccak256_maxcoin_opt_v(blockData, nonceAndBits) < 0x0040000000000000UL ) nonceIndexOut[get_local_id(0)] = nonceAndBits>>32;\r\n"
	"nonceAndBits += 0x100000000UL;\r\n"
	"}\r\n");

	strcat(kernel_src, "}\r\n");

	const char* source = kernel_src;
	size_t src_size = strlen(kernel_src);
	cl_program program = clCreateProgramWithSource(openCL_getActiveContext(), 1, &source, &src_size, &clerr);
	if(clerr != CL_SUCCESS)
		printf("Error creating OpenCL program\n");
	// builds the program
	clerr = clBuildProgram(program, 1, openCL_getActiveDeviceID(), NULL, NULL, NULL);
	if(clerr != CL_SUCCESS)
		printf("Error compiling OpenCL program\n");
	// shows the log
	char* build_log;
	size_t log_size;
	// First call to know the proper size
	clGetProgramBuildInfo(program, *openCL_getActiveDeviceID(), CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
	build_log = (char*)malloc(log_size+1);
	memset(build_log, 0x00, log_size+1);
	// Second call to get the log
	clGetProgramBuildInfo(program, *openCL_getActiveDeviceID(), CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
	build_log[log_size] = '\0';
	puts(build_log);
	free(build_log);

	maxcoinGPU.kernel_keccak = clCreateKernel(program, "xptMiner_cl_maxcoin_keccak", &clerr);

	clerr = clSetKernelArg(maxcoinGPU.kernel_keccak, 0, sizeof(cl_mem), &maxcoinGPU.clBuffer_blockInputData);
	clerr = clSetKernelArg(maxcoinGPU.kernel_keccak, 1, sizeof(cl_mem), &maxcoinGPU.clBuffer_nonceOutputData);

	free(kernel_src);
}

void maxcoin_init()
{
	if( minerSettings.useGPU )
	{
		openCL_init();
		maxcoinMiner_openCL_generateOrUpdateKernel();
	}
}

inline uint32 maxcoin_swapEndianU32(uint32 v)
{
	return (v>>24)|((v>>8)&0xFF00)|((v<<8)&0xFF0000)|(v<<24);
}

void xptMiner_submitShare_test(minerMaxcoinBlock_t* block)
{
	// debug method to check if share is 100% valid without server access
	uint32* blockInputData = (uint32*)block;
	uint64 hash0[4];
	sph_keccak256_context	 ctx_keccak;
	sph_keccak256_init(&ctx_keccak);	
	sph_keccak256(&ctx_keccak, blockInputData, 80);
	sph_keccak256_close(&ctx_keccak, hash0);
	if( (hash0[3]>>32) <= *(uint32*)(block->targetShare+32-4) )
	{
		printf("Share valid (nonce: %08x)\n", block->nonce);
	}
	else
	{
		printf("Share invalid (nonce: %08x)\n", block->nonce);
		totalRejectedShareCount++;
	}
}

void maxcoin_processGPU(minerMaxcoinBlock_t* block)
{
	//memset(block, 0x00, 80); // debug
//	if( GetAsyncKeyState(VK_ESCAPE) )
//		ExitProcess(0);
	// write endian swapped data to block data buffer
	uint32* blockInputData = (uint32*)block;
	uint32* blockDataBE = (uint32*)maxcoinGPU.buffer_blockInputData;
	for(sint32 i=0; i<20-1; i++)
		blockDataBE[i] = blockInputData[i];//maxcoin_swapEndianU32(blockInputData[i]);
	blockDataBE[19] = 0; // set nonce to zero	
	blockDataBE[20] = *(uint32*)(block->targetShare+24);
	blockDataBE[21] = *(uint32*)(block->targetShare+28);

	cl_int clerr;
	clerr = clEnqueueWriteBuffer(openCL_getActiveCommandQueue(), maxcoinGPU.clBuffer_blockInputData, true, 0, 80+8, blockDataBE, 0, NULL, NULL);

	size_t work_dim[1];

	work_dim[0] = 1 * 128;
	size_t work_size[1];
	work_size[0] = work_dim[0];//min(1, work_dim[0]);

	cl_event event_kernelExecute;

	clerr = clEnqueueNDRangeKernel(openCL_getActiveCommandQueue(), maxcoinGPU.kernel_keccak, 1, NULL, work_dim, work_size, 0, NULL, &event_kernelExecute);
	clerr = clEnqueueReadBuffer(openCL_getActiveCommandQueue(), maxcoinGPU.clBuffer_nonceOutputData, true, 0, 256*4*4, maxcoinGPU.buffer_nonceOutputData, 0, NULL, NULL);
	uint32* resultNonces = (uint32*)(maxcoinGPU.buffer_nonceOutputData);
	for(uint32 i=0; i<work_dim[0]; i++)
	{
		if( resultNonces[i] != 0xFFFFFFFF )
		{
			block->nonce = resultNonces[i];
			block->nonce = *(uint32*)(maxcoinGPU.buffer_nonceOutputData);
			xptMiner_submitShare_test(block);	
		}
	}
	totalCollisionCount += work_dim[0]/8;

}

void maxcoin_process(minerMaxcoinBlock_t* block)
{
	//memset(block, 0x00, 80); // debug

	_ALIGNED(32) uint64 hash0[4];

	// endian swap block data (input data needs to be big endian)
	//uint32 blockDataBE[80/4];
	uint32* blockInputData = (uint32*)block;
	//for(sint32 i=0; i<20-1; i++)
	//	blockDataBE[i] = maxcoin_swapEndianU32(blockInputData[i]);
	//blockDataBE[19] = 0; // set nonce to zero	
	uint64 targetU64 = *(uint64*)(block->targetShare+24);
	//keccak_core_prepare(&ctx_keccak, block, keccakPre);
	for(uint32 n=0; n<0x10000; n++)
	{
		if( block->height != monitorCurrentBlockHeight )
			break;
		if( (block->nTime+60) < monitorCurrentBlockTime )
		{
			// need to update time
			block->nTime = monitorCurrentBlockTime;
			//blockInputData[68/4] = blockInputData[68/4];
		}
		for(uint32 f=0; f<0x8000; f++ )
		{	
			if( keccak256_maxcoin_opt_v((unsigned long long*)blockInputData) <= targetU64 )
			{
				totalShareCount++;
				block->nonce = blockInputData[19];
				xptMiner_submitShare(block);
			}
			blockInputData[19]++;
			//void metis4_core_opt_p1(unsigned int* data, unsigned int* pOut);
			//unsigned int metis4_core_opt_p2(unsigned int* pIn);
		}
		totalCollisionCount += 1; // count in steps of 0x8000
	}
	// printf("[DEBUG] Exited with final nonce 0x%08x\n", blockDataBE[19]);
}
