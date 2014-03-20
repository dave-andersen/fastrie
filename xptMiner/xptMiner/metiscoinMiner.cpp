#include"global.h"
#include"aes_helper.h"

#define GROUPED_HASHES	(512)

/*
 * Called once when Metiscoin was detected as the active algorithm for the current worker
 */
void metiscoin_init()
{
	printf("Using Metiscoin algorithm\n");
}


void metiscoin_process(minerMetiscoinBlock_t* block)
{
	sph_keccak512_context	 ctx_keccak;
	static unsigned char	 pblank[1];
	block->nonce = 0;
	uint32 target = *(uint32*)(block->targetShare+28);


	_ALIGNED(32) unsigned int metisPartData[36*GROUPED_HASHES];
	_ALIGNED(32) uint64 hash0[8*GROUPED_HASHES];
	// since only the nonce changes we can calculate the first keccak round in advance
	unsigned long long keccakPre[25];
	sph_keccak512_init(&ctx_keccak);
	keccak_core_prepare(&ctx_keccak, block, keccakPre);
	for(uint32 n=0; n<0x10000; n++)
	{
		if( block->height != monitorCurrentBlockHeight )
			break;
		if( (block->nTime+60) < monitorCurrentBlockTime )
		{
			// need to update time
			block->nTime = monitorCurrentBlockTime;
			// update initial keccak round
			sph_keccak512_init(&ctx_keccak);
			keccak_core_prepare(&ctx_keccak, block, keccakPre);
		}
		for(uint32 f=0; f<0x8000; f += GROUPED_HASHES )
		{
			// todo: Generate multiple hashes for multiple nonces at once
			block->nonce = n*0x10000+f;
			for(uint32 i=0; i<GROUPED_HASHES; i++)
			{
				keccak_core_opt(&ctx_keccak, keccakPre, *(unsigned long long*)(&block->nBits), hash0+i*8);
				block->nonce++;
			}
			for(uint32 i=0; i<GROUPED_HASHES; i++)
			{
				shavite_big_core_opt((unsigned long long int *)hash0+i*8, (unsigned long long int *)hash0+i*8);
			}
			block->nonce = n*0x10000+f;
			//for(uint32 i=0; i<GROUPED_HASHES; i++)
			//{
			//	unsigned v1, v2;
			//	if( metis4_core_opt(hash0+i*8) <= target )
			//	{
			//		totalShareCount++;
			//		//block->nonce = rawBlock.nonce;
			//		xptMiner_submitShare(block);
			//	}
			//	block->nonce++;
			//}
			//for(uint32 i=0; i<GROUPED_HASHES; i += 2)
			//{
			//	unsigned int v1, v2;
			//	//unsigned int v3, v4;
			//	metis4_core_opt_interleaved((unsigned int *)(hash0+i*8), (unsigned int *)(hash0+i*8+8), &v1, &v2);
			//	//metis4_core_opt_interleaved((unsigned int *)(hash0+i*8+8), (unsigned int *)(hash0+i*8+16), &v3, &v4);
			//	if( v1 <= target )
			//	{
			//		printf("[DEBUG] Share A\n");
			//		totalShareCount++;
			//		//block->nonce = rawBlock.nonce;
			//		xptMiner_submitShare(block);
			//	}
			//	block->nonce++;
			//	if( v2 <= target )
			//	{
			//		printf("[DEBUG] Share B\n");
			//		totalShareCount++;
			//		//block->nonce = rawBlock.nonce;
			//		xptMiner_submitShare(block);
			//	}
			//	block->nonce++;
			//}
			for(uint32 i=0; i<GROUPED_HASHES; i++)
			{
				metis4_core_opt_p1((unsigned int *)(hash0+i*8), metisPartData+i*36);
			}
			for(uint32 i=0; i<GROUPED_HASHES; i++)
			{
				//unsigned int v1, v2;
				//unsigned int v3, v4;
				//metis4_core_opt_interleaved((unsigned int *)(hash0+i*8), (unsigned int *)(hash0+i*8+8), &v1, &v2);
				//metis4_core_opt_interleaved((unsigned int *)(hash0+i*8+8), (unsigned int *)(hash0+i*8+16), &v3, &v4);
				if( metis4_core_opt_p2(metisPartData+i*36) <= target )
				{
					totalShareCount++;
					//block->nonce = rawBlock.nonce;
					xptMiner_submitShare(block);
				}
				block->nonce++;
			}
			//void metis4_core_opt_p1(unsigned int* data, unsigned int* pOut);
			//unsigned int metis4_core_opt_p2(unsigned int* pIn);
		}
		totalCollisionCount += 1; // count in steps of 0x8000
	}
}