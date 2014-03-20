typedef ulong sph_u64;

#define SPH_C64(x)    ((sph_u64)(x ## UL))
#define SPH_T64(x)    ((x) & SPH_C64(0xFFFFFFFFFFFFFFFF))
#define SPH_ROTL64(x, n)   SPH_T64(((x) << (n)) | ((x) >> (64 - (n))))

#define DECL64(x)        sph_u64 x
#define MOV64(d, s)      (d = s)
#define XOR64(d, a, b)   (d = a ^ b)
#define AND64(d, a, b)   (d = a & b)
#define OR64(d, a, b)    (d = a | b)
#define NOT64(d, s)      (d = SPH_T64(~s))
#define ROL64(d, v, n)   (d = SPH_ROTL64(v, n))

#define TH_ELT_O(t, c0, c1, c2, c3, c4, d0, d1, d2, d3, d4) \
	XOR64(tt0, d0, d1); \
	XOR64(tt1, d2, d3); \
	XOR64(tt0, tt0, d4); \
	XOR64(tt0, tt0, tt1); \
	ROL64(tt0, tt0, 1); \
	XOR64(tt2, c0, c1); \
	XOR64(tt3, c2, c3); \
	XOR64(tt0, tt0, c4); \
	XOR64(tt2, tt2, tt3); \
	XOR64(t, tt0, tt2);

__constant const sph_u64 RC[] = {
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

ulong keccak256_maxcoin_opt_v( ulong *data)
{
	int j;
	sph_u64 t;
	DECL64(c0);
	DECL64(c1);
	DECL64(c2);
	DECL64(c3);
	DECL64(c4);
	DECL64(tt0);
	DECL64(tt1);
	DECL64(tt2);
	DECL64(tt3);
	sph_u64 a00, a01, a02, a03, a04;
	sph_u64 a10, a11, a12, a13, a14;
	sph_u64 a20, a21, a22, a23, a24;
	sph_u64 a30, a31, a32, a33, a34;
	sph_u64 a40, a41, a42, a43, a44;
	a00 = data[0];
	a10 = ~data[1];
	a20 = ~data[2];
	a30 = data[3];
	a40 = data[4];
	a01 = data[5];
	a11 = data[6];
	a21 = data[7];
	a31 = ~data[8];
	a41 = data[9];
	a02 = 1;

	a12 = 0;
	a22 = 0xFFFFFFFFFFFFFFFF;
	a32 = 0;
	a42 = 0;
	a03 = 0;
	a13 = 0x8000000000000000UL;
	a23 = 0xFFFFFFFFFFFFFFFF;
	a33 = 0;
	a43 = 0; 
	a04 = 0xFFFFFFFFFFFFFFFF; 
	a14 = 0;
	a24 = 0;
	a34 = 0;
	a44 = 0;

	for (j = 0; j < 24-1; j++) 
	{
		TH_ELT_O(c0, a40, a41, a42, a43, a44, a10, a11, a12, a13, a14);
		TH_ELT_O(c1, a00, a01, a02, a03, a04, a20, a21, a22, a23, a24);
		TH_ELT_O(c2, a10, a11, a12, a13, a14, a30, a31, a32, a33, a34);
		TH_ELT_O(c3, a20, a21, a22, a23, a24, a40, a41, a42, a43, a44);
		TH_ELT_O(c4, a30, a31, a32, a33, a34, a00, a01, a02, a03, a04);
		a00 = a00 ^ c0;
		a01 = a01 ^ c0;
		a02 = a02 ^ c0;
		a03 = a03 ^ c0;
		a04 = a04 ^ c0;
		a10 = a10 ^ c1;
		a11 = a11 ^ c1;
		a12 = a12 ^ c1;
		a13 = a13 ^ c1;
		a14 = a14 ^ c1;
		a20 = a20 ^ c2;
		a21 = a21 ^ c2;
		a22 = a22 ^ c2;
		a23 = a23 ^ c2;
		a24 = a24 ^ c2;
		a30 = a30 ^ c3;
		a31 = a31 ^ c3;
		a32 = a32 ^ c3;
		a33 = a33 ^ c3;
		a34 = a34 ^ c3;
		a40 = a40 ^ c4;
		a41 = a41 ^ c4;
		a42 = a42 ^ c4;
		a43 = a43 ^ c4;
		a44 = a44 ^ c4;
		/* ROL64(b00, b00,  0); */
		ROL64(a01, a01, 36);
		ROL64(a02, a02,  3);
		ROL64(a03, a03, 41);
		ROL64(a04, a04, 18);
		ROL64(a10, a10,  1);
		ROL64(a11, a11, 44);
		ROL64(a12, a12, 10);
		ROL64(a13, a13, 45);
		ROL64(a14, a14,  2);
		ROL64(a20, a20, 62);
		ROL64(a21, a21,  6);
		ROL64(a22, a22, 43);
		ROL64(a23, a23, 15);
		ROL64(a24, a24, 61);
		ROL64(a30, a30, 28);
		ROL64(a31, a31, 55);
		ROL64(a32, a32, 25);
		ROL64(a33, a33, 21);
		ROL64(a34, a34, 56);
		ROL64(a40, a40, 27);
		ROL64(a41, a41, 20);
		ROL64(a42, a42, 39);
		ROL64(a43, a43,  8);
		ROL64(a44, a44, 14);
		t = ~a22;
		c0 = a00 ^ (a11|a22);
		c1 = a11 ^ (t|a33);
		c2 = a22 ^ (a33&a44);
		c3 = a33 ^ (a44|a00);
		c4 = a44 ^ (a00&a11);
		a00 = c0;
		a11 = c1;
		a22 = c2;
		a33 = c3;
		a44 = c4;
		t = ~a24;
		c0 = a30 ^ (a41|a02);
		c1 = a41 ^ (a02&a13);
		c2 = a02 ^ (a13|t);
		c3 = a13 ^ (a24|a30);
		c4 = a24 ^ (a30&a41);
		a30 = c0;
		a41 = c1;
		a02 = c2;
		a13 = c3;
		a24 = c4;
		t = ~a43;
		c0 = a10 ^ (a21|a32);
		c1 = a21 ^ (a32&a43);
		c2 = a32 ^ (t&a04);
		c3 = t ^ (a04|a10);
		c4 = a04 ^ (a10&a21);
		a10 = c0;
		a21 = c1;
		a32 = c2;
		a43 = c3;
		a04 = c4;
		t = ~a23;
		c0 = a40 ^ (a01&a12);
		c1 = a01 ^ (a12|a23);
		c2 = a12 ^ (t|a34);
		c3 = t ^ (a34&a40);
		c4 = a34 ^ (a40|a01);
		a40 = c0;
		a01 = c1;
		a12 = c2;
		a23 = c3;
		a34 = c4;
		t = ~a31;
		c0 = a20 ^ (t&a42);
		c1 = t ^ (a42|a03);
		c2 = a42 ^ (a03&a14);
		c3 = a03 ^ (a14|a20);
		c4 = a14 ^ (a20&a31);
		a20 = c0;
		a31 = c1;
		a42 = c2;
		a03 = c3;
		a14 = c4;
		a00 ^= RC[j];
		t = a01;
		a01 = a30;
		a30 = a33;
		a33 = a23;
		a23 = a12;
		a12 = a21;
		a21 = a02;
		a02 = a10;
		a10 = a11;
		a11 = a41;
		a41 = a24;
		a24 = a42;
		a42 = a04;
		a04 = a20;
		a20 = a22;
		a22 = a32;
		a32 = a43;
		a43 = a34;
		a34 = a03;
		a03 = a40;
		a40 = a44;
		a44 = a14;
		a14 = a31;
		a31 = a13;
		a13 = t;
	}
	// last round isolated
	TH_ELT_O(c0, a40, a41, a42, a43, a44, a10, a11, a12, a13, a14);
	TH_ELT_O(c3, a20, a21, a22, a23, a24, a40, a41, a42, a43, a44);
	TH_ELT_O(c4, a30, a31, a32, a33, a34, a00, a01, a02, a03, a04);
	a00 = a00 ^ c0;
	a33 = a33 ^ c3;
	a44 = a44 ^ c4;
	ROL64(a33, a33, 21);
	ROL64(a44, a44, 14);
	return a33 ^ (a44|a00);
}


kernel void maxcoin_process(global uint* in, global uint* out,global uint* out_count, uint begin_nonce, ulong target) {
	size_t id = get_global_id(0);
	uint nonce = (uint)id + begin_nonce;
	
	uint data[20];
	
	for(int i = 0;i < 19;i++)
	{
		data[i]=in[i];
	}
	data[19]=nonce;
	
	if(keccak256_maxcoin_opt_v((ulong*)data) <= target)
	{
		uint pos = atomic_inc(out_count);
		out[pos] = nonce;
	}

}
