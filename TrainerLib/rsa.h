#pragma once

#include <inttypes.h>

#define SHA256_DIGEST_SIZE 32
#define RSA2048NUMBYTES 256
#define RSA2048NUMWORDS (RSA2048NUMBYTES / sizeof(uint32_t))

struct RSAPublicKey {

	uint32_t n0inv;
	uint32_t n[RSA2048NUMWORDS];
	uint32_t rr[RSA2048NUMWORDS];

	void geM(uint32_t *a, bool& result);
	void subM(uint32_t *a);
	void montMulAdd(uint32_t* c, const uint32_t a, const uint32_t* b);
	void montMul(uint32_t* c, uint32_t* a, const uint32_t b[RSA2048NUMWORDS]);
	void modpowF4(uint8_t* inout);

	void Verify(const uint8_t sig[RSA2048NUMBYTES], const uint8_t hash[SHA256_DIGEST_SIZE], bool& result);
};