#include "message.hxx"

main(int argc, char **argv)
{
	FGMPSMessageBuf	buf1, buf2;

	buf1.clr();
	buf1.put(fgmps_som8, true);
	buf1.put(1);
	for (int i=0; i<256; i++) {
		buf1.put(fgmps_uchar, true);
		buf1.put(i);
	}
	buf1.put(fgmps_eom, true);

	buf2.set(buf1.str());

	bool done=false;
	while (!done) {
		unsigned char		mid8, uval8;
		unsigned int		mid16, uval16;
		unsigned long		uval32;
		unsigned long long	uval64;
		char			val8;
		int			val16;
		long			val32;
		long long		val64;
		float			valf;
		double			vald;
		string			vals;
		printf("dump: ");
		for (int i=0; i<16; i++) printf("%02x ", buf2.peek(i));
		printf("\n");
		try {
			int tag = buf2.get(true);
			switch (tag) {
			case fgmps_som8:
				mid8 = *(unsigned char *)buf2.read1();
				printf("Start Message ID = %02x\n", mid8);
				break;
			case fgmps_som16:
				mid16 = *(unsigned int *)buf2.read2();
				printf("Start Message ID = %04x\n", mid16);
				break;
			case fgmps_eom:
				printf("End Of Message\n", tag);
				done = true;
				break;
			case fgmps_uchar:
				uval8 = *(unsigned char *)buf2.read1();
				printf("uchar = %02x\n", uval8);
				break;
			case fgmps_uint:
				uval16 = *(unsigned int *)buf2.read2();
				printf("uint = %04x\n", uval16);
				break;
			case fgmps_ulong:
				uval32 = *(unsigned long *)buf2.read4();
				printf("ulong = %08lx\n", uval32);
				break;
			case fgmps_ulonglong:
				uval64 = *(unsigned long long *)buf2.read8();
				printf("ulonglong = %16llx\n", uval64);
				break;
			default:
				printf("Unknown prefix = %02x\n", tag);
				done = true;
				break;
			}
		} catch (FGMPSDataException e) {
			done = true;
		}
	}
}
