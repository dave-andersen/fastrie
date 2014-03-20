#ifdef _ECLIPSE_OPENCL_HEADER
#   include "OpenCLKernel.hpp"
#   include "keccak.cl"
#   include "shavite.cl"
#   include "metis.cl"
#   include "OpenCLKernel.hpp"
#endif

kernel void metiscoin_process(global char* in, global uint* out, global uint* outcount, uint begin_nonce, uint target) {

	size_t id = get_global_id(0);
	uint nonce = (uint)id + begin_nonce;

	keccak_context	 ctx_keccak;
	shavite_context ctx_shavite;
	metis_context ctx_metis;
	char data[80];
	ulong hash0[8];
	ulong hash1[8];
	ulong hash2[8];

	// prepares data
	for (int i = 0; i < 80; i++) {
		data[i] = in[i];
	}
	char * p = (char*)&nonce;
	for (int i = 0; i < 4; i++) {
		data[76+i] = p[i];
	}

	// keccak
	keccak_init(&ctx_keccak);
	keccak_core_80(&ctx_keccak, data);
	keccak_close(&ctx_keccak, hash0);

	// shavite
	shavite_init(&ctx_shavite);
	shavite_core_64(&ctx_shavite, hash0);
	shavite_close(&ctx_shavite, hash1);

	// metis
	metis_init(&ctx_metis);
	metis_core_64(&ctx_metis, hash1);
	metis_close(&ctx_metis, hash2);

	if( *(uint*)((uchar*)hash2+28) <= target )
	{
		uint pos = atomic_inc(out) + 1; //saves first pos for counter
		out[pos] = nonce;
	}

}

kernel void keccak_step(constant const char* in, global ulong* out, uint begin_nonce) {

	size_t id = get_global_id(0);
	uint nonce = (uint)id + begin_nonce;

	keccak_context	 ctx_keccak;
	char data[80];
	ulong hash[8];

	// prepares data
	for (int i = 0; i < 80; i++) {
		data[i] = in[i];
	}
	char * p = (char*)&nonce;
	for (int i = 0; i < 4; i++) {
		data[76+i] = p[i];
	}

	// keccak
	keccak_init(&ctx_keccak);
	keccak_core_80(&ctx_keccak, data);
	keccak_close(&ctx_keccak, hash);

	for (int i = 0; i < 8; i++) {
		out[(id * 8)+i] = hash[i];
	}
}

kernel void keccak_step_noinit(constant const ulong* u, constant const char* buff, global ulong* out, uint begin_nonce) {

	size_t id = get_global_id(0);
	uint nonce = (uint)id + begin_nonce;

	ulong hash[8];

	// inits context
	keccak_context	 ctx_keccak;
	ctx_keccak.lim = 72;
	ctx_keccak.ptr = 8;
#pragma unroll
	for (int i = 0; i < 4; i++) {
		ctx_keccak.buf[i] = buff[i];
	}
	*((uint*)(ctx_keccak.buf+4)) = nonce;
#pragma unroll
	for (int i = 0; i < 25; i++) {
		ctx_keccak.u.wide[i] = u[i];
	}

	// keccak
	keccak_close(&ctx_keccak, hash);

#pragma unroll
	for (int i = 0; i < 8; i++) {
		out[(id * 8)+i] = hash[i];
	}
}

kernel void shavite_step(global ulong* in_out) {

	size_t id = get_global_id(0);

	shavite_context	 ctx_shavite;
	ulong hash0[8];
	ulong hash1[8];

	// prepares data
	for (int i = 0; i < 8; i++) {
		hash0[i] = in_out[(id * 8)+i];
	}

	shavite_init(&ctx_shavite);
	shavite_core_64(&ctx_shavite, hash0);
	shavite_close(&ctx_shavite, hash1);

	for (int i = 0; i < 8; i++) {
		in_out[(id * 8)+i] = hash1[i];
	}
}

kernel void metis_step(global ulong* in, global uint* out, global uint* outcount, uint begin_nonce, uint target) {

	size_t id = get_global_id(0);
	uint nonce = (uint)id + begin_nonce;

	metis_context ctx_metis;
	ulong hash0[8];
	ulong hash1[8];

	// prepares data
	for (int i = 0; i < 8; i++) {
		hash0[i] = in[(id * 8)+i];
	}

	// metis
	metis_init(&ctx_metis);
	metis_core_64(&ctx_metis, hash0);
	metis_close(&ctx_metis, hash1);

//	// for debug
//	for (int i = 0; i < 8; i++) {
//		in[(id * 8)+i] = hash1[i];
//	}

	if( *(uint*)((uchar*)hash1+28) <= target )
	{
		uint pos = atomic_inc(out) + 1; //saves first pos for counter
		out[pos] = nonce;
	}

}
