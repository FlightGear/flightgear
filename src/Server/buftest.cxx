#include "message.hxx"

void dumpmessage(string msg)
{
	FGMPSMessageBuf	buf;

	buf.set(msg);
	buf.ofs(0);

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
		for (int i=0; i<16; i++) printf("%02x ", buf.peek(i));
		printf("\n");
		try {
			int tag = buf.get(true);
			switch (tag) {
			case fgmps_som8:
				mid8 = *(unsigned char *)buf.read1();
				printf("Start Message ID = %02x\n", mid8);
				break;
			case fgmps_som16:
				mid16 = *(unsigned int *)buf.read2();
				printf("Start Message ID = %04x\n", mid16);
				break;
			case fgmps_eom:
				printf("End Of Message\n", tag);
				done = true;
				break;
			case fgmps_uchar:
				uval8 = *(unsigned char *)buf.read1();
				printf("uchar = %02x\n", uval8);
				break;
			case fgmps_uint:
				uval16 = *(unsigned int *)buf.read2();
				printf("uint = %04x\n", uval16);
				break;
			case fgmps_ulong:
				uval32 = *(unsigned long *)buf.read4();
				printf("ulong = %08lx\n", uval32);
				break;
			case fgmps_ulonglong:
				uval64 = *(unsigned long long *)buf.read8();
				printf("ulonglong = %16llx\n", uval64);
				break;
			case fgmps_char:
				val8 = *(char *)buf.read1();
				printf("char = %02x\n", val8);
				break;
			case fgmps_int:
				val16 = *(int *)buf.read2();
				printf("int = %04x\n", val16);
				break;
			case fgmps_long:
				val32 = *(long *)buf.read4();
				printf("long = %08lx\n", val32);
				break;
			case fgmps_longlong:
				val64 = *(long long *)buf.read8();
				printf("longlong = %16llx\n", val64);
				break;
			case fgmps_float:
				valf = buf.readf();
				printf("float = %f\n", valf);
				break;
			case fgmps_double:
				vald = buf.readd();
				printf("double = %g\n", vald);
				break;
			case fgmps_string:
				uval8 = buf.get();
				vals = buf.reads(uval8);
				printf("string = %s\n", vals.c_str());
				break;
			default:
				printf("Unknown prefix = %02x\n", tag);
				done = true;
				break;
			}
		} catch (FGMPSDataException e) {
			printf("Data Exception\n");
			done = true;
		}
	}
}

main(int argc, char **argv)
{
	FGMPSMessageBuf	buf;

	unsigned char		uval8;
	unsigned int		uval16;
	unsigned long		uval32;
	unsigned long long	uval64;
	char			val8;
	int			val16;
	long			val32;
	long long		val64;
	float			valf;
	double			vald;
	string			vals;

	buf.clr();
	buf.put(fgmps_som8, true);
	buf.put(1);
	for (int i=0; i<256; i++) {
		buf.put(fgmps_uchar, true);
		buf.put(i);
	}
	buf.put(fgmps_eom, true);
	dumpmessage(buf.str());

	buf.clr();
	buf.put(fgmps_som8, true);	buf.put(1);

	uval8  = 251;		buf.put(fgmps_uchar, true);	buf.write1(&uval8);
	uval16 = 34567;		buf.put(fgmps_uint, true);	buf.write2(&uval16);
	uval32 = 1345678901;	buf.put(fgmps_ulong, true);	buf.write4(&uval32);
	//uval64 = 9999999999;	buf.put(fgmps_ulonglong, true);	buf.write8(&uval64);
	val8   = -120;		buf.put(fgmps_char, true);	buf.write1(&val8);
	val16  = -17890;	buf.put(fgmps_int, true);	buf.write2(&val16);
	val32  = -1345678901;	buf.put(fgmps_long, true);	buf.write4(&val32);
	//val64  = -9999999999;	buf.put(fgmps_longlong, true);	buf.write8(&val64);
	valf   = 2 * 3.14;	buf.put(fgmps_float, true);	buf.writef(valf);
	vald   = 3 * 3.1415927;	buf.put(fgmps_double, true);	buf.writed(vald);
	vals   = "hi there";	buf.put(fgmps_string, true);	buf.writes(vals);
	buf.put(fgmps_eom, true);
	dumpmessage(buf.str());
}
