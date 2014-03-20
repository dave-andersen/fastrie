#ifdef _ECLIPSE_OPENCL_HEADER
#   include "OpenCLKernel.hpp"
#endif

#define SPH_C64(x)    ((ulong)(x))

typedef struct {
	unsigned char buf[144];    /* first field, for alignment */
	size_t ptr, lim;
	union {
		ulong wide[25];
		uint narrow[50];
	} u;
} keccak_context;

constant ulong RC[] = {
	SPH_C64(0x0000000000000001), SPH_C64(0x0000000000008082),
	SPH_C64(0x800000000000808A), SPH_C64(0x8000000080008000),
	SPH_C64(0x000000000000808B), SPH_C64(0x0000000080000001),
	SPH_C64(0x8000000080008081), SPH_C64(0x8000000000008009),
	SPH_C64(0x000000000000008A), SPH_C64(0x0000000000000088),
	SPH_C64(0x0000000080008009), SPH_C64(0x000000008000000A),
	SPH_C64(0x000000008000808B), SPH_C64(0x800000000000008B),
	SPH_C64(0x8000000000008089), SPH_C64(0x8000000000008003),
	SPH_C64(0x8000000000008002), SPH_C64(0x8000000000000080),
	SPH_C64(0x000000000000800A), SPH_C64(0x800000008000000A),
	SPH_C64(0x8000000080008081), SPH_C64(0x8000000000008080),
	SPH_C64(0x0000000080000001), SPH_C64(0x8000000080008008)
};

#define a00   (kc->u.wide[ 0])
#define a10   (kc->u.wide[ 1])
#define a20   (kc->u.wide[ 2])
#define a30   (kc->u.wide[ 3])
#define a40   (kc->u.wide[ 4])
#define a01   (kc->u.wide[ 5])
#define a11   (kc->u.wide[ 6])
#define a21   (kc->u.wide[ 7])
#define a31   (kc->u.wide[ 8])
#define a41   (kc->u.wide[ 9])
#define a02   (kc->u.wide[10])
#define a12   (kc->u.wide[11])
#define a22   (kc->u.wide[12])
#define a32   (kc->u.wide[13])
#define a42   (kc->u.wide[14])
#define a03   (kc->u.wide[15])
#define a13   (kc->u.wide[16])
#define a23   (kc->u.wide[17])
#define a33   (kc->u.wide[18])
#define a43   (kc->u.wide[19])
#define a04   (kc->u.wide[20])
#define a14   (kc->u.wide[21])
#define a24   (kc->u.wide[22])
#define a34   (kc->u.wide[23])
#define a44   (kc->u.wide[24])

ulong
dec64le_aligned(const void *src)
{
	return (ulong)(((const unsigned char *)src)[0])
		| ((ulong)(((const unsigned char *)src)[1]) << 8)
		| ((ulong)(((const unsigned char *)src)[2]) << 16)
		| ((ulong)(((const unsigned char *)src)[3]) << 24)
		| ((ulong)(((const unsigned char *)src)[4]) << 32)
		| ((ulong)(((const unsigned char *)src)[5]) << 40)
		| ((ulong)(((const unsigned char *)src)[6]) << 48)
		| ((ulong)(((const unsigned char *)src)[7]) << 56);
}

// this does not work on AMD for some reason, reverting
//#define dec64le_aligned(x) (*((ulong*)(x)))

//void
//enc64le_aligned(void *dst, ulong val) {
//	((unsigned char *)dst)[0] = val;
//	((unsigned char *)dst)[1] = (val >> 8);
//	((unsigned char *)dst)[2] = (val >> 16);
//	((unsigned char *)dst)[3] = (val >> 24);
//	((unsigned char *)dst)[4] = (val >> 32);
//	((unsigned char *)dst)[5] = (val >> 40);
//	((unsigned char *)dst)[6] = (val >> 48);
//	((unsigned char *)dst)[7] = (val >> 56);
//}

#define enc64le_aligned(dst, val) (*((ulong*)(dst)) = (val))

#define INPUT_BUF(size)   { \
		size_t j; \
		for (j = 0; j < (size); j += 8) { \
			kc->u.wide[j >> 3] ^= dec64le_aligned(buf + j); \
		} \
	}

#define SPH_T64(x)    ((x) & SPH_C64(0xFFFFFFFFFFFFFFFF))
#define SPH_ROTL64(x, n)   SPH_T64(((x) << (n)) | ((x) >> (64 - (n))))
#define SPH_ROTR64(x, n)   SPH_ROTL64(x, (64 - (n)))
#define INPUT_BUF144   INPUT_BUF(144)
#define INPUT_BUF136   INPUT_BUF(136)
#define INPUT_BUF104   INPUT_BUF(104)
#define INPUT_BUF72    INPUT_BUF(72)
#define DECL64(x)        ulong x
#define MOV64(d, s)      (d = s)
#define XOR64(d, a, b)   (d = a ^ b)
#define AND64(d, a, b)   (d = a & b)
#define OR64(d, a, b)    (d = a | b)
#define NOT64(d, s)      (d = SPH_T64(~s))
#define ROL64(d, v, n)   (d = SPH_ROTL64(v, n))
#define XOR64_IOTA       XOR64


#define TH_ELT(t, c0, c1, c2, c3, c4, d0, d1, d2, d3, d4)   { \
		DECL64(tt0); \
		DECL64(tt1); \
		DECL64(tt2); \
		DECL64(tt3); \
		XOR64(tt0, d0, d1); \
		XOR64(tt1, d2, d3); \
		XOR64(tt0, tt0, d4); \
		XOR64(tt0, tt0, tt1); \
		ROL64(tt0, tt0, 1); \
		XOR64(tt2, c0, c1); \
		XOR64(tt3, c2, c3); \
		XOR64(tt0, tt0, c4); \
		XOR64(tt2, tt2, tt3); \
		XOR64(t, tt0, tt2); \
	}

#define THETA(b00, b01, b02, b03, b04, b10, b11, b12, b13, b14, \
	b20, b21, b22, b23, b24, b30, b31, b32, b33, b34, \
	b40, b41, b42, b43, b44) \
	{ \
		DECL64(t0); \
		DECL64(t1); \
		DECL64(t2); \
		DECL64(t3); \
		DECL64(t4); \
		TH_ELT(t0, b40, b41, b42, b43, b44, b10, b11, b12, b13, b14); \
		TH_ELT(t1, b00, b01, b02, b03, b04, b20, b21, b22, b23, b24); \
		TH_ELT(t2, b10, b11, b12, b13, b14, b30, b31, b32, b33, b34); \
		TH_ELT(t3, b20, b21, b22, b23, b24, b40, b41, b42, b43, b44); \
		TH_ELT(t4, b30, b31, b32, b33, b34, b00, b01, b02, b03, b04); \
		XOR64(b00, b00, t0); \
		XOR64(b01, b01, t0); \
		XOR64(b02, b02, t0); \
		XOR64(b03, b03, t0); \
		XOR64(b04, b04, t0); \
		XOR64(b10, b10, t1); \
		XOR64(b11, b11, t1); \
		XOR64(b12, b12, t1); \
		XOR64(b13, b13, t1); \
		XOR64(b14, b14, t1); \
		XOR64(b20, b20, t2); \
		XOR64(b21, b21, t2); \
		XOR64(b22, b22, t2); \
		XOR64(b23, b23, t2); \
		XOR64(b24, b24, t2); \
		XOR64(b30, b30, t3); \
		XOR64(b31, b31, t3); \
		XOR64(b32, b32, t3); \
		XOR64(b33, b33, t3); \
		XOR64(b34, b34, t3); \
		XOR64(b40, b40, t4); \
		XOR64(b41, b41, t4); \
		XOR64(b42, b42, t4); \
		XOR64(b43, b43, t4); \
		XOR64(b44, b44, t4); \
	}

#define RHO(b00, b01, b02, b03, b04, b10, b11, b12, b13, b14, \
	b20, b21, b22, b23, b24, b30, b31, b32, b33, b34, \
	b40, b41, b42, b43, b44) \
	{ \
		/* ROL64(b00, b00,  0); */ \
		ROL64(b01, b01, 36); \
		ROL64(b02, b02,  3); \
		ROL64(b03, b03, 41); \
		ROL64(b04, b04, 18); \
		ROL64(b10, b10,  1); \
		ROL64(b11, b11, 44); \
		ROL64(b12, b12, 10); \
		ROL64(b13, b13, 45); \
		ROL64(b14, b14,  2); \
		ROL64(b20, b20, 62); \
		ROL64(b21, b21,  6); \
		ROL64(b22, b22, 43); \
		ROL64(b23, b23, 15); \
		ROL64(b24, b24, 61); \
		ROL64(b30, b30, 28); \
		ROL64(b31, b31, 55); \
		ROL64(b32, b32, 25); \
		ROL64(b33, b33, 21); \
		ROL64(b34, b34, 56); \
		ROL64(b40, b40, 27); \
		ROL64(b41, b41, 20); \
		ROL64(b42, b42, 39); \
		ROL64(b43, b43,  8); \
		ROL64(b44, b44, 14); \
	}

/*
 * The KHI macro integrates the "lane complement" optimization. On input,
 * some words are complemented:
 *    a00 a01 a02 a04 a13 a20 a21 a22 a30 a33 a34 a43
 * On output, the following words are complemented:
 *    a04 a10 a20 a22 a23 a31
 *
 * The (implicit) permutation and the theta expansion will bring back
 * the input mask for the next round.
 */

#define KHI_XO(d, a, b, c)   { \
		DECL64(kt); \
		OR64(kt, b, c); \
		XOR64(d, a, kt); \
	}

#define KHI_XA(d, a, b, c)   { \
		DECL64(kt); \
		AND64(kt, b, c); \
		XOR64(d, a, kt); \
	}

#define KHI(b00, b01, b02, b03, b04, b10, b11, b12, b13, b14, \
	b20, b21, b22, b23, b24, b30, b31, b32, b33, b34, \
	b40, b41, b42, b43, b44) \
	{ \
		DECL64(c0); \
		DECL64(c1); \
		DECL64(c2); \
		DECL64(c3); \
		DECL64(c4); \
		DECL64(bnn); \
		NOT64(bnn, b20); \
		KHI_XO(c0, b00, b10, b20); \
		KHI_XO(c1, b10, bnn, b30); \
		KHI_XA(c2, b20, b30, b40); \
		KHI_XO(c3, b30, b40, b00); \
		KHI_XA(c4, b40, b00, b10); \
		MOV64(b00, c0); \
		MOV64(b10, c1); \
		MOV64(b20, c2); \
		MOV64(b30, c3); \
		MOV64(b40, c4); \
		NOT64(bnn, b41); \
		KHI_XO(c0, b01, b11, b21); \
		KHI_XA(c1, b11, b21, b31); \
		KHI_XO(c2, b21, b31, bnn); \
		KHI_XO(c3, b31, b41, b01); \
		KHI_XA(c4, b41, b01, b11); \
		MOV64(b01, c0); \
		MOV64(b11, c1); \
		MOV64(b21, c2); \
		MOV64(b31, c3); \
		MOV64(b41, c4); \
		NOT64(bnn, b32); \
		KHI_XO(c0, b02, b12, b22); \
		KHI_XA(c1, b12, b22, b32); \
		KHI_XA(c2, b22, bnn, b42); \
		KHI_XO(c3, bnn, b42, b02); \
		KHI_XA(c4, b42, b02, b12); \
		MOV64(b02, c0); \
		MOV64(b12, c1); \
		MOV64(b22, c2); \
		MOV64(b32, c3); \
		MOV64(b42, c4); \
		NOT64(bnn, b33); \
		KHI_XA(c0, b03, b13, b23); \
		KHI_XO(c1, b13, b23, b33); \
		KHI_XO(c2, b23, bnn, b43); \
		KHI_XA(c3, bnn, b43, b03); \
		KHI_XO(c4, b43, b03, b13); \
		MOV64(b03, c0); \
		MOV64(b13, c1); \
		MOV64(b23, c2); \
		MOV64(b33, c3); \
		MOV64(b43, c4); \
		NOT64(bnn, b14); \
		KHI_XA(c0, b04, bnn, b24); \
		KHI_XO(c1, bnn, b24, b34); \
		KHI_XA(c2, b24, b34, b44); \
		KHI_XO(c3, b34, b44, b04); \
		KHI_XA(c4, b44, b04, b14); \
		MOV64(b04, c0); \
		MOV64(b14, c1); \
		MOV64(b24, c2); \
		MOV64(b34, c3); \
		MOV64(b44, c4); \
	}

#define IOTA(r)   XOR64_IOTA(a00, a00, r)

#define KF_ELT01(k)   { \
		THETA ( a00, a01, a02, a03, a04, a10, a11, a12, a13, a14, a20, a21, \
	              a22, a23, a24, a30, a31, a32, a33, a34, a40, a41, a42, a43, a44 ); \
		RHO ( a00, a01, a02, a03, a04, a10, a11, a12, a13, a14, a20, a21, \
	              a22, a23, a24, a30, a31, a32, a33, a34, a40, a41, a42, a43, a44 ); \
		KHI ( a00, a30, a10, a40, a20, a11, a41, a21, a01, a31, a22, a02, \
	              a32, a12, a42, a33, a13, a43, a23, a03, a44, a24, a04, a34, a14 ); \
		IOTA(k); \
	}

#define P1_TO_P0   { \
		DECL64(t); \
		MOV64(t, a01); \
		MOV64(a01, a30); \
		MOV64(a30, a33); \
		MOV64(a33, a23); \
		MOV64(a23, a12); \
		MOV64(a12, a21); \
		MOV64(a21, a02); \
		MOV64(a02, a10); \
		MOV64(a10, a11); \
		MOV64(a11, a41); \
		MOV64(a41, a24); \
		MOV64(a24, a42); \
		MOV64(a42, a04); \
		MOV64(a04, a20); \
		MOV64(a20, a22); \
		MOV64(a22, a32); \
		MOV64(a32, a43); \
		MOV64(a43, a34); \
		MOV64(a34, a03); \
		MOV64(a03, a40); \
		MOV64(a40, a44); \
		MOV64(a44, a14); \
		MOV64(a14, a31); \
		MOV64(a31, a13); \
		MOV64(a13, t); \
	}

#define KECCAK_F_1600_   { \
		int j; \
		for (j = 0; j < 24; j ++) { \
			KF_ELT01(RC[j + 0]); \
			P1_TO_P0; \
		} \
	}

void
keccak_init(keccak_context *kc)
{
	int i;

#pragma unroll
	for (i = 0; i < 25; i ++)
		kc->u.wide[i] = 0;
	/*
	 * Initialization for the "lane complement".
	 */
	kc->u.wide[ 1] = SPH_C64(0xFFFFFFFFFFFFFFFFL);
	kc->u.wide[ 2] = SPH_C64(0xFFFFFFFFFFFFFFFFL);
	kc->u.wide[ 8] = SPH_C64(0xFFFFFFFFFFFFFFFFL);
	kc->u.wide[12] = SPH_C64(0xFFFFFFFFFFFFFFFFL);
	kc->u.wide[17] = SPH_C64(0xFFFFFFFFFFFFFFFFL);
	kc->u.wide[20] = SPH_C64(0xFFFFFFFFFFFFFFFFL);
	kc->ptr = 0;
	kc->lim = 200 - (512 >> 2);
}

void keccak_core_80(keccak_context *kc, const void *data)
{
	unsigned char *buf;

	buf = kc->buf;

// two passes
	size_t clen = 72;
#pragma unroll
	for (int i = 0; i < 72; i++) buf[i] = ((const unsigned char *)data)[i];
	data = (const unsigned char *)data + 72;
#pragma unroll
	for (size_t j = 0; j < 9; j ++) {
		kc->u.wide[j] ^= dec64le_aligned(buf + (j<<3));
	}
#pragma unroll
	for (int j = 0; j < 24; j ++) {
		KF_ELT01(RC[j + 0]);
		P1_TO_P0;
	}

	*((uchar8*)buf) = *((uchar8*)data);
	kc->ptr = 8;
}

void keccak_core_end_64_8(keccak_context *kc, const void *data)
{
	unsigned char *buf;

	buf = kc->buf;

	buf[8] = 1;
#pragma unroll
	for (int i = 9; i < 71; i++) buf[i] = 0;
	buf[71] = 0x80;

#pragma unroll
	for (size_t j = 0; j < (72); j += 8) {
		kc->u.wide[j >> 3] ^= (*((ulong*)(buf + j)));
	}
#pragma unroll
	for (int j = 0; j < 24; j ++) {
		KF_ELT01(RC[j + 0]);
		P1_TO_P0;
	}
}

// d = 64, lim = 72, ub = 0. n = 0
	static void keccak_close(keccak_context *kc, void *dst)
	{
		union {
			unsigned char tmp[72 + 1];
			ulong dummy;   /* for alignment */
		} u;
		size_t j;

		keccak_core_end_64_8(kc, u.tmp);
		/* Finalize the "lane complement" */
		kc->u.wide[ 1] = ~kc->u.wide[ 1];
		kc->u.wide[ 2] = ~kc->u.wide[ 2];
		kc->u.wide[ 8] = ~kc->u.wide[ 8];
		kc->u.wide[12] = ~kc->u.wide[12];
		kc->u.wide[17] = ~kc->u.wide[17];
		kc->u.wide[20] = ~kc->u.wide[20];
		for (j = 0; j < 64; j += 8)
			enc64le_aligned(((uchar*)dst) + j, kc->u.wide[j >> 3]);
	}

kernel void keccak_init_g(global keccak_context* out) {
	keccak_context	 ctx_keccak;
	keccak_init(&ctx_keccak);

	(*out) = ctx_keccak;
}

kernel void keccak_update_g(global char* in, global keccak_context* out) {
	keccak_context	 ctx_keccak;
	char data[80];

	for (int i = 0; i < 80; i++) {
		data[i] = in[i];
	}
	keccak_init(&ctx_keccak);
	keccak_core_80(&ctx_keccak, data);

	(*out) = ctx_keccak;
}

kernel void keccak(global char* in, global ulong* out) {

	keccak_context	 ctx_keccak;
	char data[80];
	ulong hash[8];
	for (int i = 0; i < 80; i++) {
		data[i] = in[i];
	}

	keccak_init(&ctx_keccak);
	keccak_core_80(&ctx_keccak, data);
	keccak_close(&ctx_keccak, hash);

	for (int i = 0; i < 8; i++) {
		out[i] = hash[i];
	}
}
