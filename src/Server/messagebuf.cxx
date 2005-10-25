#include "messagebuf.hxx"

#include <netinet/in.h>

unsigned char FGMPSMessageBuf::get(bool raw)
{
	if (pos >= buf.length())
		throw FGMPSDataException( "FGMPSMessageBuf: Read past end of buffer" );
	if (raw) return buf[pos++];
	if ((unsigned char)buf[pos] == 0xff) {
		pos++;
		return ((unsigned char)buf[pos++] ^ 0xff);
	}
	return buf[pos++];
}

void FGMPSMessageBuf::put(unsigned char byte, bool raw)
{
	if (!raw) {
		if ((byte & 0xf0) == 0xf0) {
			buf += 0xff;
			byte = byte ^ 0xff;
		}
	}
	buf += byte;
}

void FGMPSMessageBuf::write1(void* data)
{
	put(*(unsigned char *)data);
}

void FGMPSMessageBuf::write2(void* data)
{
	*((uint16_t*)tmp) = htons(*((uint16_t*)data));
	put(tmp[0]); 
	put(tmp[1]); 
}

void FGMPSMessageBuf::write4(void* data)
{
	*((uint32_t*)tmp) = htonl(*((uint32_t*)data));
	put(tmp[0]); 
	put(tmp[1]); 
	put(tmp[2]); 
	put(tmp[3]); 
}

void FGMPSMessageBuf::write8(void* data)
{
	*((uint32_t*)tmp+0) = htonl(*((uint32_t*)data+4));
	*((uint32_t*)tmp+4) = htonl(*((uint32_t*)data+0));
	put(tmp[0]); 
	put(tmp[1]); 
	put(tmp[2]); 
	put(tmp[3]); 
	put(tmp[4]); 
	put(tmp[5]); 
	put(tmp[6]); 
	put(tmp[7]); 
}

void FGMPSMessageBuf::writef(float data)
{
	write4(&data);
}

void FGMPSMessageBuf::writed(double data)
{
	write8(&data);
}

void FGMPSMessageBuf::writes(const string& data)
{
	put(data.length());
	for (int i=0; i<data.length(); i++) put(data[i]);
}

void* FGMPSMessageBuf::read1()
{
	tmp[0] = get();
	return tmp;
}

void* FGMPSMessageBuf::read2()
{
	tmp[0] = get();
	tmp[1] = get();
	*((uint16_t*)tmp) = ntohs(*((uint16_t*)tmp));
	return tmp;
}

void* FGMPSMessageBuf::read4()
{
	tmp[0] = get();
	tmp[1] = get();
	tmp[2] = get();
	tmp[3] = get();
	*((uint32_t*)tmp) = ntohl(*((uint32_t*)tmp));
	return tmp;
}

void* FGMPSMessageBuf::read8()
{
	unsigned char res[32];

	res[0] = get();
	res[1] = get();
	res[2] = get();
	res[3] = get();
	res[4] = get();
	res[5] = get();
	res[6] = get();
	res[7] = get();
	*((uint32_t*)tmp+4) = ntohl(*((uint32_t*)res+0));
	*((uint32_t*)tmp+0) = ntohl(*((uint32_t*)res+4));
	return tmp;
}

float FGMPSMessageBuf::readf()
{
	return *((float*)read4());
}

double FGMPSMessageBuf::readd()
{
	return *((double*)read8());
}

string FGMPSMessageBuf::reads(size_t length)
{
	string	res = "";
	for (int i=0; i<length; i++) res += get();
	return res;
}



