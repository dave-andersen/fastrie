// protoshares
void protoshares_process_512(minerProtosharesBlock_t* block);
void protoshares_process_256(minerProtosharesBlock_t* block);
void protoshares_process_128(minerProtosharesBlock_t* block);
void protoshares_process_32(minerProtosharesBlock_t* block);
void protoshares_process_8(minerProtosharesBlock_t* block);
// scrypt
void scrypt_process_cpu(minerScryptBlock_t* block);
// primecoin
void primecoin_process(minerPrimecoinBlock_t* block);
// metiscoin
void metiscoin_init();
void metiscoin_process(minerMetiscoinBlock_t* block);
// maxcoin
void maxcoin_init();
void maxcoin_process(minerMaxcoinBlock_t* block);
void maxcoin_processGPU(minerMaxcoinBlock_t* block);
// riecoin
void riecoin_init(uint64_t sieveMax);
void riecoin_process(minerRiecoinBlock_t* block);
