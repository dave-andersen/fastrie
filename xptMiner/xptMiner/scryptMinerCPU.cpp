#include"global.h"

void scrypt_1024_1_1_256_sp(const char *input, char *output, char *scratchpad);

void scrypt_testStuff();

void scrypt_process_cpu(minerScryptBlock_t* block)
{
	scrypt_testStuff();
	uint32 outputHash[32/4];	
	char scratchpad[SCRYPT_SCRATCHPAD_SIZE];
	uint32 shareCompare = *(uint32*)(block->targetShare+32-4);
	uint32 nonce = 0;
	for(uint32 t=0; t<32768; t++)
	{
		for(uint32 x=0; x<4096; x++)
		{
			block->nonce = nonce;
			nonce++;
			scrypt_1024_1_1_256_sp((const char*)block, (char*)outputHash, scratchpad);
			if( outputHash[7] < shareCompare )
			{
				totalShareCount++;
				xptMiner_submitShare(block);
			}
			//__debugbreak();
			totalCollisionCount++;
		}
		//if( block->height != monitorCurrentBlockHeight )
		//{
		//	printf("cancled\n");
		//	break;
		//}
	}
	
	//printf("next\n");
	


}