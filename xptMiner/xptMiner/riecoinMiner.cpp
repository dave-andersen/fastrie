#include"global.h"
#include <chrono>
#include "updater.h"

#define zeroesBeforeHashInPrime	8

#define DEBUG 0

static const int NONCE_REGEN_SECONDS = 195;
static const int core_prime_n = 8; /* 8=23, 9=29 */
static const uint32_t riecoin_sieveBits = 23; /* 8 million, or 1MB, tuned for Haswell L3 */
static const uint32_t riecoin_sieveSize = (1<<riecoin_sieveBits); /* 1MB, tuned for L3 of Haswell */
uint32_t riecoin_primeTestLimit;
uint32_t num_entries_per_segment = 0;

uint32 riecoin_primorialNumber = 40; /* 15 is the 64 bit limit */

/* Based on the primorial (40 is 226 bits), we only have about 2^29
 * increments before overflowing the 256 bit nonce field in Riecoin.
 * Each loop goes through riecoin_sieveSize increments, which means
 * that our max loop iter count is 2^29/riecoin_sieveSize.  With
 * the current settings of 8M for sieveSize, this means 64
 * iterations.
 */
static const uint64_t max_increments = (1<<29);
static const uint32_t maxiter = (max_increments/riecoin_sieveSize);

//static const uint32_t primorial_offset = 97;
static const uint32_t primorial_offset = 16057; /* For > 26 or so */

const uint32 riecoin_denseLimit = 16384; /* A few cachelines */
uint32_t* riecoin_primeTestTable;
uint32_t riecoin_primeTestSize;
uint32_t riecoin_primeTestStoreOffsetsSize;
int32_t *inverts;
mpz_t  z_primorial;

unsigned int int_invert_mpz(mpz_t &z_a, uint32_t nPrime);

void riecoin_init(uint64_t sieveMax)
{
        riecoin_primeTestLimit = sieveMax;
	printf("Generating table of small primes for Riecoin...\n");
	// generate prime table
	riecoin_primeTestTable = (uint32*)malloc(sizeof(uint32)*(riecoin_primeTestLimit/4+10));
	riecoin_primeTestSize = 0;

	// generate prime table using Sieve of Eratosthenes
	uint8* vfComposite = (uint8*)malloc(sizeof(uint8)*(riecoin_primeTestLimit+7)/8);
	memset(vfComposite, 0x00, sizeof(uint8)*(riecoin_primeTestLimit+7)/8);
	for (unsigned int nFactor = 2; nFactor * nFactor < riecoin_primeTestLimit; nFactor++)
	{
		if( vfComposite[nFactor>>3] & (1<<(nFactor&7)) )
			continue;
		for (unsigned int nComposite = nFactor * nFactor; nComposite < riecoin_primeTestLimit; nComposite += nFactor)
			vfComposite[nComposite>>3] |= 1<<(nComposite&7);
	}
	for (unsigned int n = 2; n < riecoin_primeTestLimit; n++)
	{
		if ( (vfComposite[n>>3] & (1<<(n&7)))==0 )
		{
			riecoin_primeTestTable[riecoin_primeTestSize] = n;
			riecoin_primeTestSize++;
		}
	}
	riecoin_primeTestTable = (uint32_t*)realloc(riecoin_primeTestTable, sizeof(uint32)*riecoin_primeTestSize);
	free(vfComposite);
#if DEBUG
	printf("Table with %d entries generated\n", riecoin_primeTestSize);
#endif

	// generate primorial
	mpz_init_set_ui(z_primorial, riecoin_primeTestTable[0]);
	for(uint32_t i=1; i<riecoin_primorialNumber; i++)
	{
		mpz_mul_ui(z_primorial, z_primorial, riecoin_primeTestTable[i]);
	}
#if DEBUG
	gmp_printf("z_primorial: %Zd\n", z_primorial);
#endif
	inverts = (int32_t *)malloc(sizeof(int32_t) * (riecoin_primeTestSize));

	for (uint32_t i = 5; i < riecoin_primeTestSize; i++) {
	  inverts[i] = int_invert_mpz(z_primorial, riecoin_primeTestTable[i]);
	}

	uint64_t high_segment_entries = 0;
	double high_floats = 0.0;
	riecoin_primeTestStoreOffsetsSize = 0;
	for (uint32_t i = 5; i < riecoin_primeTestSize; i++) {
	  uint32_t p = riecoin_primeTestTable[i];
	  if (p < max_increments) {
	    riecoin_primeTestStoreOffsetsSize++;
	  } else {
	    high_floats += ((6.0f * max_increments) / (double)p);
	  }
	}
	high_segment_entries = ceil(high_floats);

	num_entries_per_segment = high_segment_entries/maxiter;
	num_entries_per_segment = (num_entries_per_segment + (num_entries_per_segment>>3));
}

typedef uint32_t sixoff[6];

thread_local uint8_t* riecoin_sieve = NULL;
thread_local sixoff *offsets = NULL;
thread_local uint32_t *segment_hits[maxiter];


uint32 _getHexDigitValue(uint8 c)
{
	if( c >= '0' && c <= '9' )
		return c-'0';
	else if( c >= 'a' && c <= 'f' )
		return c-'a'+10;
	else if( c >= 'A' && c <= 'F' )
		return c-'A'+10;
	return 0;
}

/*
 * Parses a hex string
 * Length should be a multiple of 2
 */
void debug_parseHexString(char* hexString, uint32 length, uint8* output)
{
	uint32 lengthBytes = length / 2;
	for(uint32 i=0; i<lengthBytes; i++)
	{
		// high digit
		uint32 d1 = _getHexDigitValue(hexString[i*2+0]);
		// low digit
		uint32 d2 = _getHexDigitValue(hexString[i*2+1]);
		// build byte
		output[i] = (uint8)((d1<<4)|(d2));	
	}
}

void debug_parseHexStringLE(char* hexString, uint32 length, uint8* output)
{
	uint32 lengthBytes = length / 2;
	for(uint32 i=0; i<lengthBytes; i++)
	{
		// high digit
		uint32 d1 = _getHexDigitValue(hexString[i*2+0]);
		// low digit
		uint32 d2 = _getHexDigitValue(hexString[i*2+1]);
		// build byte
		output[lengthBytes-i-1] = (uint8)((d1<<4)|(d2));	
	}
}

unsigned int int_invert_internal(int32_t rem1, uint32_t nPrime)
{
  // Extended Euclidean algorithm to calculate the inverse of a in finite field defined by nPrime
  int32_t rem0 = nPrime;
  int32_t rem2;
  int32_t  aux0 = 0, aux1 = 1, aux2;
  int32_t quotient;
  int32_t inverse;

	while (1)
	{
		if (rem1 <= 1)
		{
			inverse = aux1;
			break;
		}

		rem2 = rem0 % rem1;
		quotient = rem0 / rem1;
		aux2 = -quotient * aux1 + aux0;

		if (rem2 <= 1)
		{
			inverse = aux2;
			break;
		}

		rem0 = rem1 % rem2;
		quotient = rem1 / rem2;
		aux0 = -quotient * aux2 + aux1;

		if (rem0 <= 1)
		{
			inverse = aux0;
			break;
		}

		rem1 = rem2 % rem0;
		quotient = rem2 / rem0;
		aux1 = -quotient * aux0 + aux2;
	}

	return (inverse + nPrime) % nPrime;
}

unsigned int int_invert_mpz(mpz_t &z_a, uint32_t nPrime)
{
  int32_t rem1 = mpz_tdiv_ui(z_a, nPrime); // rem1 = a % nPrime
  return int_invert_internal(rem1, nPrime);
}

inline void silly_sort_indexes(uint32_t indexes[6]) {
  for (int i = 0; i < 5; i++) {
    for (int j = i+1; j < 6; j++) {
      if (indexes[j] < indexes[i]) {
	std::swap(indexes[i], indexes[j]);
      }
    }
  }
}

inline void add_to_pending(uint8_t *sieve, uint32_t pending[16], uint32_t &pos, uint32_t ent) {
  __builtin_prefetch(&(sieve[ent>>3]));
  uint32_t old = pending[pos];
  if (old != 0) {
    sieve[old>>3] |= (1<<(old&7));
  }
  pending[pos] = ent;
  pos++;
  pos &= 0xf;
}


void riecoin_process(minerRiecoinBlock_t* block)
{
	uint32 searchBits = block->targetCompact;

	if( !riecoin_sieve ) {
		riecoin_sieve = (uint8*)malloc(riecoin_sieveSize/8);
		size_t offsize = sizeof(sixoff) * (riecoin_primeTestStoreOffsetsSize+1);
		offsets = (sixoff *)malloc(offsize);
		memset(offsets, 0, offsize);
		for (int i = 0; i < maxiter; i++) {
		  segment_hits[i] = (uint32_t *)malloc(sizeof(uint32_t) * num_entries_per_segment);
		}
	}
	uint8* sieve = riecoin_sieve;

	time_t start_time = time(NULL);
#if DEBUG
	auto start = std::chrono::system_clock::now();
#endif

	// test data
	// getblock 16ee31c116b75d0299dc03cab2b6cbcb885aa29adf292b2697625bc9d28b2b64
	//debug_parseHexStringLE("c59ba5357285de73b878fed43039a37f85887c8960e66bcb6e86bdad565924bd", 64, block->merkleRoot);
	//block->version = 2;
	//debug_parseHexStringLE("c64673c670fb327c2e009b3b626d2def01d51ad4131a7a1040e9cef7bfa34838", 64, block->prevBlockHash);
	//block->nTime = 1392151955;
	//block->nBits = 0x02013000;
	//debug_parseHexStringLE("0000000000000000000000000000000000000000000000000000000070b67515", 64, block->nOffset);
	// generate PoW hash (version to nBits)
	uint8 powHash[32];
	sha256_ctx ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, (uint8*)block, 80);
	sha256_final(&ctx, powHash);
	sha256_init(&ctx);
	sha256_update(&ctx, powHash, 32);
	sha256_final(&ctx, powHash);
	// generatePrimeBase
	uint32* powHashU32 = (uint32*)powHash;
	mpz_t z_target;
	mpz_t z_temp;
	mpz_init(z_temp);
	mpz_t z_temp2;
	mpz_init(z_temp2);
	mpz_t z_remainderPrimorial;
	mpz_init(z_remainderPrimorial);

	mpz_init_set_ui(z_target, 1);
	mpz_mul_2exp(z_target, z_target, zeroesBeforeHashInPrime);
	for(uint32 i=0; i<256; i++)
	{
		mpz_mul_2exp(z_target, z_target, 1);
		if( (powHashU32[i/32]>>(i))&1 )
			z_target->_mp_d[0]++;
	}
	unsigned int trailingZeros = searchBits - 1 - zeroesBeforeHashInPrime - 256;
	mpz_mul_2exp(z_target, z_target, trailingZeros);
	// find first offset where target%primorial = primorial_offset

	mpz_tdiv_r(z_remainderPrimorial, z_target, z_primorial);
	mpz_abs(z_remainderPrimorial, z_remainderPrimorial);
	mpz_sub(z_remainderPrimorial, z_primorial, z_remainderPrimorial);
	mpz_tdiv_r(z_remainderPrimorial, z_remainderPrimorial, z_primorial);
	mpz_abs(z_remainderPrimorial, z_remainderPrimorial);
	mpz_add_ui(z_remainderPrimorial, z_remainderPrimorial, primorial_offset);

	mpz_add(z_temp, z_target, z_remainderPrimorial);

	mpz_t z_ft_r;
	mpz_init(z_ft_r);
	mpz_t z_ft_b;
	mpz_init_set_ui(z_ft_b, 2);
	mpz_t z_ft_n;
	mpz_init(z_ft_n);

	static uint32 primeTupleBias[6] = {0,4,6,10,12,16};
	static uint32 primeTupleOffset[6] = {0, 4, 2, 4, 2, 4};
	mpz_set(z_temp2, z_primorial);

	uint32_t primeIndex = riecoin_primorialNumber;
	
	uint32_t off_offset = 0;
	uint32_t startingPrimeIndex = primeIndex;
	uint32_t n_dense = 0;
	uint32_t n_sparse = 0;
	uint32_t n_once_only = 0;
	uint32_t n_once_only_used = 0;
	mpz_set(z_temp2, z_primorial);
	uint32_t segment_counts[maxiter];
	for (int i = 0; i < maxiter; i++) {
	  segment_counts[i] = 0;
	}

	//printf("primeIndex: %d  is %d\n", primeIndex, riecoin_primeTestTable[primeIndex]);
	for( ; primeIndex < riecoin_primeTestSize; primeIndex++)
	{
	  uint32 p = riecoin_primeTestTable[primeIndex];
	  int32_t inverted = inverts[primeIndex];
	  bool is_once_only = false;
	  if (p < riecoin_denseLimit) {
	    n_dense++;
	  } else if (p < max_increments) {
	    n_sparse++;
	  } else {
	    n_once_only++;
	    is_once_only = true;
	  }

	  /* Compute remainder = (rounded_up_target + offset)%p efficiently.  Instead of %p
	   * in the inner loop, just do one test - the primeTupleOffets are all smaller
	   * than the smallest prime in the sieve, and so can never increase reminder
	   * too much. */
	  uint32 remainder = mpz_tdiv_ui(z_temp, p);
	  for (uint32 f = 0; f < 6; f++) {
	    remainder += primeTupleOffset[f];
	    if (remainder > p) {
	      remainder -= p;
	    }
	    int64_t pa = p-remainder;
	    uint64_t index = pa*inverted;
	    index %= p;
	    if (!is_once_only) {
	      offsets[off_offset][f] = index;
	    } else {
	      if (index < max_increments) {
		uint32_t segment = index>>riecoin_sieveBits;
		uint32_t sc = segment_counts[segment];
		if (sc >= num_entries_per_segment) { 
		  printf("EEEEK segment %u  %u for prime %u with index %u is > %u\n", segment, sc, p, index, num_entries_per_segment); exit(-1);
		}
		segment_hits[segment][sc] = index - (riecoin_sieveSize*segment);
		segment_counts[segment]++;
		n_once_only_used++;
	      }
	    }
	  }
	  off_offset++;
	}

#if DEBUG
	printf("Sieve set up with %u dense %u sparse and %u/%u once-only\n", n_dense, n_sparse, n_once_only_used, n_once_only);
	printf("used %d offsets slots\n", off_offset);

	auto end = std::chrono::system_clock::now();
	auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
	printf("Loop start offset compute time:  %d ms\n", dur);
#endif

	/* Main processing loop:
	 * 1)  Sieve "dense" primes;
	 * 2)  Sieve "sparse" primes;
	 * 3)  Sieve "so sparse they happen at most once" primes;
	 * 4)  Scan sieve for candidates, test, report
	 */

	uint32 countCandidates = 0;
	uint32 countPrimes = 0;

	for (int loop = 0; loop < maxiter; loop++) {
	    __sync_synchronize(); /* gcc specific - memory barrier for checking height */
	    if( block->height != monitorCurrentBlockHeight ) {
	      break;
	    }
	    time_t cur_time = time(NULL);
	    if ((cur_time - start_time) > NONCE_REGEN_SECONDS) {
	      break;
	    }
#if DEBUG
	    printf("Loop %d after %d seconds\n", loop, (cur_time-start_time)); fflush(stdout);
#endif

	    memset(sieve, 0, riecoin_sieveSize/8);

	    for (unsigned int i = 0; i < n_dense; i++) {
	      silly_sort_indexes(offsets[i]);
	      uint32_t p = riecoin_primeTestTable[i+startingPrimeIndex];
	      for (uint32 f = 0; f < 6; f++) {
		while (offsets[i][f] < riecoin_sieveSize) {
		  sieve[offsets[i][f]>>3] |= (1<<((offsets[i][f]&7)));
		  offsets[i][f] += p;
		}
		offsets[i][f] -= riecoin_sieveSize;
	      }
	    }
	    

	    uint32_t pending[16];
	    uint32_t pending_pos = 0;
	    for (int i = 0; i < 16; i++) { pending[i] = 0; }
	    
	    for (unsigned int i = n_dense; i < (n_dense+n_sparse); i++) {
	      uint32_t p = riecoin_primeTestTable[i+startingPrimeIndex];
	      for (uint32 f = 0; f < 6; f++) {
		while (offsets[i][f] < riecoin_sieveSize) {
		  add_to_pending(sieve, pending, pending_pos, offsets[i][f]);
		  offsets[i][f] += p;
		}
		offsets[i][f] -= riecoin_sieveSize;
	      }
	    }

#if 1
	    for (uint32_t i = 0; i < segment_counts[loop]; i++) {
	      add_to_pending(sieve, pending, pending_pos, segment_hits[loop][i]);
	    }
#endif

	    for (int i = 0; i < 16; i++) {
	      uint32_t old = pending[i];
	      sieve[old>>3] |= (1<<(old&7));
	    }
	  

	    // scan for candidates
	    for(uint32 i=1; i<riecoin_sieveSize; i++) {
	      if( sieve[(i)>>3] & (1<<((i)&7)) )
		continue;
	      countCandidates++;

	      /* Check for a prime cluster.  A "share" on ypool is any
	       * four or more of the elements prime, but for speed,
	       * check further only if the first passes the primality
	       * test.  The first test is the bottleneck for the
	       * miner.
	       *
	       * Uses the fermat test - jh's code noted that it is slightly faster.
	       * Could do an MR test as a follow-up, but the server can do this too
	       * for the one-in-a-whatever case that Fermat is wrong.
	       */

	      int nPrimes = 0;
	      // p1

	      mpz_set(z_temp, z_primorial);
	      mpz_mul_ui(z_temp, z_temp, loop);
	      mpz_mul_ui(z_temp, z_temp, riecoin_sieveSize);
	      mpz_set(z_temp2, z_primorial);
	      mpz_mul_ui(z_temp2, z_temp2, i);
	      mpz_add(z_temp, z_temp, z_temp2);
	      mpz_add(z_temp, z_temp, z_remainderPrimorial);
	      mpz_add(z_temp, z_temp, z_target);

	      mpz_sub_ui(z_ft_n, z_temp, 1);
	      mpz_powm(z_ft_r, z_ft_b, z_ft_n, z_temp);
	      if (mpz_cmp_ui(z_ft_r, 1) != 0)
		continue;
	      else
		countPrimes++;

	      nPrimes++;

	      /* Low overhead but still often enough */
	      __sync_synchronize(); /* gcc specific - memory barrier for checking height */
	      if( block->height != monitorCurrentBlockHeight ) {
		break;
	      }

	      /* New definition of shares:  Any 4+ valid primes.  Search method 
	       * is for 1st + any 3 to avoid doing too much primality testing.
	       */

	      /* Note start at 1 - we've already tested bias 0 */
	      for (int i = 1; i < 6; i++) {
		mpz_add_ui(z_temp, z_temp, primeTupleOffset[i]);
		mpz_sub_ui(z_ft_n, z_temp, 1);
		mpz_powm(z_ft_r, z_ft_b, z_ft_n, z_temp);
		if (mpz_cmp_ui(z_ft_r, 1) == 0) {
		  nPrimes++;
		}
                int candidatesRemaining = 5-i;
                if ((nPrimes + candidatesRemaining) < 4) { continue; }
	      }

	      /* These statistics are a little confusing because of the interaction
	       * with early-exit above.  They overcount relative to finding consecutive
	       * primes, but undercount relative to counting all primes.  But they're
	       * still useful for benchmarking within a variant of the program with
	       * all else held equal. */
	      if (nPrimes >= 2) total2ChainCount++;
	      if (nPrimes >= 3) total3ChainCount++;
	      if (nPrimes >= 4) total4ChainCount++;

	      if (nPrimes < 4) continue;

	      mpz_set(z_temp, z_primorial);
	      mpz_mul_ui(z_temp, z_temp, loop);
	      mpz_mul_ui(z_temp, z_temp, riecoin_sieveSize);
	      mpz_set(z_temp2, z_primorial);
	      mpz_mul_ui(z_temp2, z_temp2, i);
	      mpz_add(z_temp, z_temp, z_temp2);
	      mpz_add(z_temp, z_temp, z_remainderPrimorial);
	      mpz_add(z_temp, z_temp, z_target);

	      mpz_sub(z_temp2, z_temp, z_target); // offset = tested - target
	      // submit share
	      uint8 nOffset[32];
	      memset(nOffset, 0x00, 32);
#if defined _WIN64 || __X86_64__
	      for(uint32 d=0; d<std::min(32/8, z_temp2->_mp_size); d++)
		{
		  *(uint64*)(nOffset+d*8) = z_temp2->_mp_d[d];
		}
#elif defined _WIN32 
	      for(uint32 d=0; d<std::min(32/4, z_temp2->_mp_size); d++)
		{
		  *(uint32*)(nOffset+d*4) = z_temp2->_mp_d[d];
		}
#elif defined __GNUC__
#ifdef	__x86_64__
	      for(uint32 d=0; d<std::min(32/8, z_temp2->_mp_size); d++)
		{
		  *(uint64*)(nOffset+d*8) = z_temp2->_mp_d[d];
		}
#else  
	      for(uint32 d=0; d<std::min(32/4, z_temp2->_mp_size); d++)
		{
		  *(uint32*)(nOffset+d*4) = z_temp2->_mp_d[d];
		}
#endif
#endif
	      totalShareCount++;
	      xptMiner_submitShare(block, nOffset);
	    }
	}
	mpz_clears(z_target, z_temp, z_temp2, z_ft_r, z_ft_b, z_ft_n, z_remainderPrimorial, NULL);

}
