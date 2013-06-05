#ifndef MD5_H
#define MD5_H

#ifndef _MSC_VER
#include <inttypes.h>
#else
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
#endif

struct MD5Context {
	uint32_t buf[4];
	uint32_t bits[2];
	uint8_t in[64];
};

void MD5Init(struct MD5Context *context);
void MD5Update(struct MD5Context *context, uint8_t const *buf, unsigned int len);
void MD5Final(uint8_t digest[16], struct MD5Context *context);
void MD5Transform(uint32_t buf[4], uint32_t const in[16]);

/*
 * This is needed to make RSAREF happy on some MS-DOS compilers.
 */
typedef struct MD5Context MD5_CTX;

#endif /* !MD5_H */
