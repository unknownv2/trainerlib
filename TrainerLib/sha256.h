#pragma once

#include <stdint.h>

class SHA256
{
	uint8_t data[64];
	uint32_t datalen;
	uint32_t bitlen[2];
	uint32_t state[8];

	void Transform();

public:
	SHA256();

	void Update(const void* data, size_t len);

	void Finalize(uint8_t hash[]);
};
