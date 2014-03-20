/*
 * Copyright 2009 Colin Percival, 2011 ArtForz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file was originally written by Colin Percival as part of the Tarsnap
 * online backup system.
 */

#include"global.h"
#define __align
#define _rotl
static inline uint32 be32dec(const void *pp)
{
	const uint8 *p = (uint8 const *)pp;
	return ((uint32)(p[3]) + ((uint32)(p[2]) << 8) +
	    ((uint32)(p[1]) << 16) + ((uint32)(p[0]) << 24));
}

static inline void be32enc(void *pp, uint32 x)
{
	uint8 *p = (uint8 *)pp;
	p[3] = x & 0xff;
	p[2] = (x >> 8) & 0xff;
	p[1] = (x >> 16) & 0xff;
	p[0] = (x >> 24) & 0xff;
}

static inline uint32 le32dec(const void *pp)
{
	const uint8 *p = (uint8 const *)pp;
	return ((uint32)(p[0]) + ((uint32)(p[1]) << 8) +
	    ((uint32)(p[2]) << 16) + ((uint32)(p[3]) << 24));
}

static inline void le32enc(void *pp, uint32 x)
{
	uint8 *p = (uint8 *)pp;
	p[0] = x & 0xff;
	p[1] = (x >> 8) & 0xff;
	p[2] = (x >> 16) & 0xff;
	p[3] = (x >> 24) & 0xff;
}


typedef struct HMAC_SHA256Context {
	sha256_ctx ictx;
	sha256_ctx octx;
} HMAC_SHA256_CTX;

/* Initialize an HMAC-SHA256 operation with the given key. */
static void
HMAC_SHA256_Init(HMAC_SHA256_CTX *ctx, const void *_K, uint32 Klen)
{
	unsigned char pad[64];
	unsigned char khash[32];
	const unsigned char *K = (const unsigned char*)_K;
	uint32 i;

	/* If Klen > 64, the key is really SHA256(K). */
	if (Klen > 64) {
		sha256_init(&ctx->ictx);
		sha256_update(&ctx->ictx, (uint8*)K, Klen);
		sha256_final(&ctx->ictx, khash);
		K = khash;
		Klen = 32;
	}

	/* Inner SHA256 operation is SHA256(K xor [block of 0x36] || data). */
	sha256_init(&ctx->ictx);
	memset(pad, 0x36, 64);
	for (i = 0; i < Klen; i++)
		pad[i] ^= K[i];
	sha256_update(&ctx->ictx, (uint8*)pad, 64);

	/* Outer SHA256 operation is SHA256(K xor [block of 0x5c] || hash). */
	sha256_init(&ctx->octx);
	memset(pad, 0x5c, 64);
	for (i = 0; i < Klen; i++)
		pad[i] ^= K[i];
	sha256_update(&ctx->octx, (uint8*)pad, 64);

	/* Clean the stack. */
	memset(khash, 0, 32);
}

/* Add bytes to the HMAC-SHA256 operation. */
static void
HMAC_SHA256_Update(HMAC_SHA256_CTX *ctx, const void *inData, uint32 len)
{
	/* Feed data to the inner SHA256 operation. */
	sha256_update(&ctx->ictx, (uint8*)inData, len);
}

/* Finish an HMAC-SHA256 operation. */
static void
HMAC_SHA256_Final(unsigned char digest[32], HMAC_SHA256_CTX *ctx)
{
	unsigned char ihash[32];

	/* Finish the inner SHA256 operation. */
	sha256_final(&ctx->ictx, ihash);

	/* Feed the inner hash to the outer SHA256 operation. */
	sha256_update(&ctx->octx, (uint8*)ihash, 32);

	/* Finish the outer SHA256 operation. */
	sha256_final(&ctx->octx, digest);

	/* Clean the stack. */
	memset(ihash, 0, 32);
}

/**
 * Optimized version for first pass
 * PBKDF2_SHA256(passwd, passwdlen, salt, saltlen, c, buf, dkLen):
 */
static void
PBKDF2_SHA256_O1(const uint8 *passwd, const uint8 *salt,
    uint8 *buf)
{
	// uint32 saltlen = 80, uint64 c = 1, 
	HMAC_SHA256_CTX PShctx, hctx;
	uint32 i;
	uint8 ivec[4];
	uint8 U[32];
	uint8 T[32];
	uint32 clen;

	/* Compute HMAC state after processing P and S. */
	HMAC_SHA256_Init(&PShctx, passwd, 80);
	HMAC_SHA256_Update(&PShctx, salt, 80);

	/* Iterate through the blocks. */
	for (i = 0; i < 4; i++)
	{
		/* Generate INT(i + 1). */
		be32enc(ivec, (uint32)(i + 1));

		/* Compute U_1 = PRF(P, S || INT(i)). */
		memcpy(&hctx, &PShctx, sizeof(HMAC_SHA256_CTX));
		HMAC_SHA256_Update(&hctx, ivec, 4);
		HMAC_SHA256_Final(U, &hctx);

		/* T_i = U_1 ... */
		memcpy(T, U, 32);

		//for (j = 2; j <= 1; j++) {
		//	/* Compute U_j. */
		//	HMAC_SHA256_Init(&hctx, passwd, 80);
		//	HMAC_SHA256_Update(&hctx, U, 32);
		//	HMAC_SHA256_Final(U, &hctx);

		//	/* ... xor U_j ... */
		//	for (k = 0; k < 32; k++)
		//		T[k] ^= U[k];
		//}

		/* Copy as many bytes as necessary into buf. */
		clen = 128 - i * 32;
		if (clen > 32)
			clen = 32;
		memcpy(&buf[i * 32], T, clen);
	}
}

/**
 * Optimized version for second pass
 * PBKDF2_SHA256(passwd, passwdlen, salt, saltlen, c, buf, dkLen):
 */
static void
PBKDF2_SHA256_O2(const uint8 *passwd, uint32 passwdlen, const uint8 *salt,
    uint32 saltlen, uint64 c, uint8 *buf)
{
	HMAC_SHA256_CTX PShctx, hctx;
	uint32 i;
	uint8 ivec[4];
	uint8 U[32];
	uint8 T[32];
	uint64 j;
	int k;
	uint32 clen;

	/* Compute HMAC state after processing P and S. */
	HMAC_SHA256_Init(&PShctx, passwd, passwdlen);
	HMAC_SHA256_Update(&PShctx, salt, saltlen);

	/* Iterate through the blocks. */
	for (i = 0; i < 1; i++)
	{
		/* Generate INT(i + 1). */
		be32enc(ivec, (uint32)(i + 1));

		/* Compute U_1 = PRF(P, S || INT(i)). */
		memcpy(&hctx, &PShctx, sizeof(HMAC_SHA256_CTX));
		HMAC_SHA256_Update(&hctx, ivec, 4);
		HMAC_SHA256_Final(U, &hctx);

		/* T_i = U_1 ... */
		memcpy(T, U, 32);

		for (j = 2; j <= c; j++) {
			/* Compute U_j. */
			HMAC_SHA256_Init(&hctx, passwd, passwdlen);
			HMAC_SHA256_Update(&hctx, U, 32);
			HMAC_SHA256_Final(U, &hctx);

			/* ... xor U_j ... */
			for (k = 0; k < 32; k++)
				T[k] ^= U[k];
		}

		/* Copy as many bytes as necessary into buf. */
		clen = 32 - i * 32;
		if (clen > 32)
			clen = 32;
		memcpy(&buf[i * 32], T, clen);
	}
}

//#define ROTL(a, b) (((a) << (b)) | ((a) >> (32 - (b))))

#define ROTL _rotl

static inline void xor_salsa8_org(uint32 B[16], const uint32 Bx[16])
{
	uint32 x00,x01,x02,x03,x04,x05,x06,x07,x08,x09,x10,x11,x12,x13,x14,x15;
	int i;

	x00 = (B[ 0] ^= Bx[ 0]);
	x01 = (B[ 1] ^= Bx[ 1]);
	x02 = (B[ 2] ^= Bx[ 2]);
	x03 = (B[ 3] ^= Bx[ 3]);
	x04 = (B[ 4] ^= Bx[ 4]);
	x05 = (B[ 5] ^= Bx[ 5]);
	x06 = (B[ 6] ^= Bx[ 6]);
	x07 = (B[ 7] ^= Bx[ 7]);
	x08 = (B[ 8] ^= Bx[ 8]);
	x09 = (B[ 9] ^= Bx[ 9]);
	x10 = (B[10] ^= Bx[10]);
	x11 = (B[11] ^= Bx[11]);
	x12 = (B[12] ^= Bx[12]);
	x13 = (B[13] ^= Bx[13]);
	x14 = (B[14] ^= Bx[14]);
	x15 = (B[15] ^= Bx[15]);
	for (i = 0; i < 8; i += 2) {
		/* Operate on columns. */
		x04 ^= ROTL(x00 + x12,  7);  x09 ^= ROTL(x05 + x01,  7);
		x14 ^= ROTL(x10 + x06,  7);  x03 ^= ROTL(x15 + x11,  7);

		x08 ^= ROTL(x04 + x00,  9);  x13 ^= ROTL(x09 + x05,  9);
		x02 ^= ROTL(x14 + x10,  9);  x07 ^= ROTL(x03 + x15,  9);

		x12 ^= ROTL(x08 + x04, 13);  x01 ^= ROTL(x13 + x09, 13);
		x06 ^= ROTL(x02 + x14, 13);  x11 ^= ROTL(x07 + x03, 13);

		x00 ^= ROTL(x12 + x08, 18);  x05 ^= ROTL(x01 + x13, 18);
		x10 ^= ROTL(x06 + x02, 18);  x15 ^= ROTL(x11 + x07, 18);

		/* Operate on rows. */
		x01 ^= ROTL(x00 + x03,  7);  x06 ^= ROTL(x05 + x04,  7);
		x11 ^= ROTL(x10 + x09,  7);  x12 ^= ROTL(x15 + x14,  7);

		x02 ^= ROTL(x01 + x00,  9);  x07 ^= ROTL(x06 + x05,  9);
		x08 ^= ROTL(x11 + x10,  9);  x13 ^= ROTL(x12 + x15,  9);

		x03 ^= ROTL(x02 + x01, 13);  x04 ^= ROTL(x07 + x06, 13);
		x09 ^= ROTL(x08 + x11, 13);  x14 ^= ROTL(x13 + x12, 13);

		x00 ^= ROTL(x03 + x02, 18);  x05 ^= ROTL(x04 + x07, 18);
		x10 ^= ROTL(x09 + x08, 18);  x15 ^= ROTL(x14 + x13, 18);
	}
	B[ 0] += x00;
	B[ 1] += x01;
	B[ 2] += x02;
	B[ 3] += x03;
	B[ 4] += x04;
	B[ 5] += x05;
	B[ 6] += x06;
	B[ 7] += x07;
	B[ 8] += x08;
	B[ 9] += x09;
	B[10] += x10;
	B[11] += x11;
	B[12] += x12;
	B[13] += x13;
	B[14] += x14;
	B[15] += x15;
}

//#include <emmintrin.h>

static inline void xor_salsa8(uint32 B[16], const uint32 Bx[16])
{
	//uint32 x00,x01,x02,x03,x04,x05,x06,x07,x08,x09,x10,x11,x12,x13,x14,x15;
	uint32 x[16];
	//__m128i* f = (__m128i*)x;
	int i;

	uint64* B64 = (uint64*)B;
	uint64* Bx64 = (uint64*)Bx;
	uint64* x64 = (uint64*)x;

	//x64[0] = (B64[ 0] ^= Bx64[ 0]);
	//x64[1] = (B64[ 1] ^= Bx64[ 1]);
	//x64[2] = (B64[ 2] ^= Bx64[ 2]);
	//x64[3] = (B64[ 3] ^= Bx64[ 3]);
	//x64[4] = (B64[ 4] ^= Bx64[ 4]);
	//x64[5] = (B64[ 5] ^= Bx64[ 5]);
	//x64[6] = (B64[ 6] ^= Bx64[ 6]);
	//x64[7] = (B64[ 7] ^= Bx64[ 7]);


	// i00 i01 i02 i03
	// i05 i06 i07 i04
	// i10 i11 i08 i09
	// i15 i12 i13 i14

	x[0] = (B[ 0] ^= Bx[ 0]);
	x[1] = (B[ 1] ^= Bx[ 1]);
	x[2] = (B[ 2] ^= Bx[ 2]);
	x[3] = (B[ 3] ^= Bx[ 3]);
	x[7] = (B[ 4] ^= Bx[ 4]);
	x[4] = (B[ 5] ^= Bx[ 5]);
	x[5] = (B[ 6] ^= Bx[ 6]);
	x[6] = (B[ 7] ^= Bx[ 7]);
	x[10] = (B[ 8] ^= Bx[ 8]);
	x[11] = (B[ 9] ^= Bx[ 9]);
	x[8] = (B[10] ^= Bx[10]);
	x[9] = (B[11] ^= Bx[11]);
	x[13] = (B[12] ^= Bx[12]);
	x[14] = (B[13] ^= Bx[13]);
	x[15] = (B[14] ^= Bx[14]);
	x[12] = (B[15] ^= Bx[15]);

	for (i = 0; i < 8; i += 2) {
		/* Operate on columns. */
		x[3] ^= ROTL(x[12] + x[9],  7);
		x[7] ^= ROTL(x[0] + x[13],  7);
		x[11] ^= ROTL(x[4] + x[1],  7);
		x[15] ^= ROTL(x[8] + x[5],  7);

		x[2] ^= ROTL(x[15] + x[8],  9);
		x[6] ^= ROTL(x[3] + x[12],  9);
		x[10] ^= ROTL(x[7] + x[0],  9);
		x[14] ^= ROTL(x[11] + x[4],  9);

		x[1] ^= ROTL(x[14] + x[11], 13);
		x[5] ^= ROTL(x[2] + x[15], 13);
		x[9] ^= ROTL(x[6] + x[3], 13);
		x[13] ^= ROTL(x[10] + x[7], 13);

		x[0] ^= ROTL(x[13] + x[10], 18);
		x[4] ^= ROTL(x[1] + x[14], 18);
		x[8] ^= ROTL(x[5] + x[2], 18);
		x[12] ^= ROTL(x[9] + x[6], 18);

		/* Operate on rows. */
		x[1] ^= ROTL(x[0] + x[3],  7); 
		x[5] ^= ROTL(x[4] + x[7],  7);
		x[9] ^= ROTL(x[8] + x[11],  7);
		x[13] ^= ROTL(x[12] + x[15],  7);

		x[2] ^= ROTL(x[1] + x[0],  9);
		x[6] ^= ROTL(x[5] + x[4],  9);
		x[10] ^= ROTL(x[9] + x[8],  9);
		x[14] ^= ROTL(x[13] + x[12],  9);

		x[3] ^= ROTL(x[2] + x[1], 13);
		x[7] ^= ROTL(x[6] + x[5], 13);
		x[11] ^= ROTL(x[10] + x[9], 13);
		x[15] ^= ROTL(x[14] + x[13], 13);

		x[0] ^= ROTL(x[3] + x[2], 18);
		x[4] ^= ROTL(x[7] + x[6], 18);
		x[8] ^= ROTL(x[11] + x[10], 18);
		x[12] ^= ROTL(x[15] + x[14], 18);
	}
	B[ 0] += x[0];
	B[ 1] += x[1];
	B[ 2] += x[2];
	B[ 3] += x[3];
	B[ 4] += x[7];
	B[ 5] += x[4];
	B[ 6] += x[5];
	B[ 7] += x[6];
	B[ 8] += x[10];
	B[ 9] += x[11];
	B[10] += x[8];
	B[11] += x[9];
	B[12] += x[13];
	B[13] += x[14];
	B[14] += x[15];
	B[15] += x[12];
}

static inline void xor_salsa8_doubleround(uint32 B[16], uint32 Bx[16])
{
	uint32 x[16];
	int i;
	uint64* B64 = (uint64*)B;
	uint64* Bx64 = (uint64*)Bx;
	uint64* x64 = (uint64*)x;
	// round 1
	x[0] = (B[ 0] ^= Bx[ 0]);
	x[1] = (B[ 1] ^= Bx[ 1]);
	x[2] = (B[ 2] ^= Bx[ 2]);
	x[3] = (B[ 3] ^= Bx[ 3]);
	x[7] = (B[ 4] ^= Bx[ 4]);
	x[4] = (B[ 5] ^= Bx[ 5]);
	x[5] = (B[ 6] ^= Bx[ 6]);
	x[6] = (B[ 7] ^= Bx[ 7]);
	x[10] = (B[ 8] ^= Bx[ 8]);
	x[11] = (B[ 9] ^= Bx[ 9]);
	x[8] = (B[10] ^= Bx[10]);
	x[9] = (B[11] ^= Bx[11]);
	x[13] = (B[12] ^= Bx[12]);
	x[14] = (B[13] ^= Bx[13]);
	x[15] = (B[14] ^= Bx[14]);
	x[12] = (B[15] ^= Bx[15]);
	for (i = 0; i < 8; i += 2) {
		/* Operate on columns. */
		x[3] ^= ROTL(x[12] + x[9],  7);
		x[7] ^= ROTL(x[0] + x[13],  7);
		x[11] ^= ROTL(x[4] + x[1],  7);
		x[15] ^= ROTL(x[8] + x[5],  7);

		x[2] ^= ROTL(x[15] + x[8],  9);
		x[6] ^= ROTL(x[3] + x[12],  9);
		x[10] ^= ROTL(x[7] + x[0],  9);
		x[14] ^= ROTL(x[11] + x[4],  9);

		x[1] ^= ROTL(x[14] + x[11], 13);
		x[5] ^= ROTL(x[2] + x[15], 13);
		x[9] ^= ROTL(x[6] + x[3], 13);
		x[13] ^= ROTL(x[10] + x[7], 13);

		x[0] ^= ROTL(x[13] + x[10], 18);
		x[4] ^= ROTL(x[1] + x[14], 18);
		x[8] ^= ROTL(x[5] + x[2], 18);
		x[12] ^= ROTL(x[9] + x[6], 18);

		/* Operate on rows. */
		x[1] ^= ROTL(x[0] + x[3],  7); 
		x[5] ^= ROTL(x[4] + x[7],  7);
		x[9] ^= ROTL(x[8] + x[11],  7);
		x[13] ^= ROTL(x[12] + x[15],  7);

		x[2] ^= ROTL(x[1] + x[0],  9);
		x[6] ^= ROTL(x[5] + x[4],  9);
		x[10] ^= ROTL(x[9] + x[8],  9);
		x[14] ^= ROTL(x[13] + x[12],  9);

		x[3] ^= ROTL(x[2] + x[1], 13);
		x[7] ^= ROTL(x[6] + x[5], 13);
		x[11] ^= ROTL(x[10] + x[9], 13);
		x[15] ^= ROTL(x[14] + x[13], 13);

		x[0] ^= ROTL(x[3] + x[2], 18);
		x[4] ^= ROTL(x[7] + x[6], 18);
		x[8] ^= ROTL(x[11] + x[10], 18);
		x[12] ^= ROTL(x[15] + x[14], 18);
	}
	B[ 0] += x[0];
	B[ 1] += x[1];
	B[ 2] += x[2];
	B[ 3] += x[3];
	B[ 4] += x[7];
	B[ 5] += x[4];
	B[ 6] += x[5];
	B[ 7] += x[6];
	B[ 8] += x[10];
	B[ 9] += x[11];
	B[10] += x[8];
	B[11] += x[9];
	B[12] += x[13];
	B[13] += x[14];
	B[14] += x[15];
	B[15] += x[12];
	// round 2
	x[0] = (Bx[ 0] ^= B[ 0]);
	x[1] = (Bx[ 1] ^= B[ 1]);
	x[2] = (Bx[ 2] ^= B[ 2]);
	x[3] = (Bx[ 3] ^= B[ 3]);
	x[7] = (Bx[ 4] ^= B[ 4]);
	x[4] = (Bx[ 5] ^= B[ 5]);
	x[5] = (Bx[ 6] ^= B[ 6]);
	x[6] = (Bx[ 7] ^= B[ 7]);
	x[10] = (Bx[ 8] ^= B[ 8]);
	x[11] = (Bx[ 9] ^= B[ 9]);
	x[8] = (Bx[10] ^= B[10]);
	x[9] = (Bx[11] ^= B[11]);
	x[13] = (Bx[12] ^= B[12]);
	x[14] = (Bx[13] ^= B[13]);
	x[15] = (Bx[14] ^= B[14]);
	x[12] = (Bx[15] ^= B[15]);
	for (i = 0; i < 8; i += 2) {
		/* Operate on columns. */
		x[3] ^= ROTL(x[12] + x[9],  7);
		x[7] ^= ROTL(x[0] + x[13],  7);
		x[11] ^= ROTL(x[4] + x[1],  7);
		x[15] ^= ROTL(x[8] + x[5],  7);

		x[2] ^= ROTL(x[15] + x[8],  9);
		x[6] ^= ROTL(x[3] + x[12],  9);
		x[10] ^= ROTL(x[7] + x[0],  9);
		x[14] ^= ROTL(x[11] + x[4],  9);

		x[1] ^= ROTL(x[14] + x[11], 13);
		x[5] ^= ROTL(x[2] + x[15], 13);
		x[9] ^= ROTL(x[6] + x[3], 13);
		x[13] ^= ROTL(x[10] + x[7], 13);

		x[0] ^= ROTL(x[13] + x[10], 18);
		x[4] ^= ROTL(x[1] + x[14], 18);
		x[8] ^= ROTL(x[5] + x[2], 18);
		x[12] ^= ROTL(x[9] + x[6], 18);

		/* Operate on rows. */
		x[1] ^= ROTL(x[0] + x[3],  7); 
		x[5] ^= ROTL(x[4] + x[7],  7);
		x[9] ^= ROTL(x[8] + x[11],  7);
		x[13] ^= ROTL(x[12] + x[15],  7);

		x[2] ^= ROTL(x[1] + x[0],  9);
		x[6] ^= ROTL(x[5] + x[4],  9);
		x[10] ^= ROTL(x[9] + x[8],  9);
		x[14] ^= ROTL(x[13] + x[12],  9);

		x[3] ^= ROTL(x[2] + x[1], 13);
		x[7] ^= ROTL(x[6] + x[5], 13);
		x[11] ^= ROTL(x[10] + x[9], 13);
		x[15] ^= ROTL(x[14] + x[13], 13);

		x[0] ^= ROTL(x[3] + x[2], 18);
		x[4] ^= ROTL(x[7] + x[6], 18);
		x[8] ^= ROTL(x[11] + x[10], 18);
		x[12] ^= ROTL(x[15] + x[14], 18);
	}
	Bx[ 0] += x[0];
	Bx[ 1] += x[1];
	Bx[ 2] += x[2];
	Bx[ 3] += x[3];
	Bx[ 4] += x[7];
	Bx[ 5] += x[4];
	Bx[ 6] += x[5];
	Bx[ 7] += x[6];
	Bx[ 8] += x[10];
	Bx[ 9] += x[11];
	Bx[10] += x[8];
	Bx[11] += x[9];
	Bx[12] += x[13];
	Bx[13] += x[14];
	Bx[14] += x[15];
	Bx[15] += x[12];
}

void scrypt_1024_1_1_256_sp(const char *input, char *output, char *scratchpad)
{
	uint8 B[128];
	uint32 X[32];
	uint32 *V;
	uint32 i, j, k;

	V = (uint32 *)(((uintptr_t)(scratchpad) + 63) & ~ (uintptr_t)(63));
	
	PBKDF2_SHA256_O1((const uint8 *)input, (const uint8 *)input, B);

	for (k = 0; k < 32; k++)
		X[k] = le32dec(&B[4 * k]);

	for (i = 0; i < 1024; i++) {
		memcpy(&V[i * 32], X, 128);
		xor_salsa8_doubleround(&X[0], &X[16]);
		//xor_salsa8(&X[0], &X[16]);
		//xor_salsa8(&X[16], &X[0]);
	}
	for (i = 0; i < 1024; i++) {
		j = 32 * (X[16] & 1023);
		for (k = 0; k < 32; k++)
			X[k] ^= V[j + k];
		xor_salsa8(&X[0], &X[16]);
		xor_salsa8(&X[16], &X[0]);
	}

	for (k = 0; k < 32; k++)
		le32enc(&B[4 * k], X[k]);

	PBKDF2_SHA256_O2((const uint8 *)input, 80, B, 128, 1, (uint8 *)output);
}

void scrypt_1024_1_1_256(const char *input, char *output)
{
	char scratchpad[SCRYPT_SCRATCHPAD_SIZE];
	scrypt_1024_1_1_256_sp(input, output, scratchpad);
}

void scrypt_testStuff()
{
	uint32 B[32];
	uint32 B2[32];
	uint32 Bx[32];
	memset(B, 0x00, sizeof(B));
	memset(B2, 0x00, sizeof(B2));
	memset(Bx, 0x00, sizeof(Bx));
	for(uint32 i=0; i<16; i++)
	{
		B[i] = rand()&0x7FFF;
		B2[i] = B[i];
	}
//	__debugbreak(); 
	xor_salsa8(B, Bx);
	xor_salsa8_org(B2, Bx);
	if( memcmp(B, B2, 16*4) )
	{
		printf("invalid result\n");

		__debugbreak(); // :(
	}
	else
		printf("valid result\n");
//	__debugbreak(); 


	// test double round
	xor_salsa8_org(B, B+16);
	xor_salsa8_org(B+16, B);
	xor_salsa8_doubleround(B2, B2+16);
	if( memcmp(B, B2, 32*4) )
	{
		printf("invalid result\n");
		__debugbreak();  // :(
	}
	else
		printf("valid result\n");


}