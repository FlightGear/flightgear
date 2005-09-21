
#include <stdio.h>
#include <stdint.h>
#include "tiny_xdr.hpp"

int main() 
{
   uint32_t ui32 = 0x01234567;
   uint64_t ui64 = 0x0123456789ABCDEFLL;

   printf("UI32: (normal) %x\nUI32: (swapped) %x\n\n", ui32, bswap_32(ui32) );
   printf("UI64: (normal) %llx\nUI64: (swapped) %llx\n\n", ui64, bswap_64(ui64) );

   return 0;
}
