#include"global.h"

//#define BNUM_MAX_ACCURACY	(2000/64)
//
//typedef struct  
//{
//	uint8 size;
//	uint64 d[BNUM_MAX_ACCURACY];
//}bnum_t;
//
//#ifdef _WIN64
//#define bnum_mul64ov(_overflow, _a, _b) (_umul128(_a, _b, &_overflow))
//#endif
//
//void bnum_set(bnum_t* bnum, uint32 v)
//{
//	bnum->d[0] = v;
//	bnum->size = 1;
//}
//
//void bnum_multiplyW32(bnum_t* bnum, uint32 b)
//{
//	
//}

void primecoin_process(minerPrimecoinBlock_t* block)
{
	// todo
	printf("Primecoin mining not yet supported\n");
	Sleep(30*1000);
}