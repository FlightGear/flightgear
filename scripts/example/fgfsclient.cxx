// $Id$
// g++ -O2 -g -pedantic -Wall fgfsclient.cxx -o fgfsclient -lstdc++
// Public Domain

#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

const int maxlen = 256;


class FGFSSocket {
	int		sock;
	bool		connected;
	unsigned	timeout;
    public:
			FGFSSocket(const char *name, const unsigned port);
			~FGFSSocket() { close(); };

	int		write(const char *msg, ...);
	const char 	*read(void);
	inline void 	flush(void);
	void		settimeout(unsigned t) { timeout = t; };
    private:
	int		close(void);
};


FGFSSocket::FGFSSocket(const char *hostname = "localhost", const unsigned port = 5501)
	:
	sock(-1),
	connected(false),
	timeout(1)
{
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
		throw("FGFSSocket/socket");

	struct hostent *hostinfo;
	hostinfo = gethostbyname(hostname);
	if (!hostinfo) {
		close();
		throw("FGFSSocket/gethostbyname: unknown host");
	}

	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr = *(struct in_addr *)hostinfo->h_addr;

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		close();
		throw("FGFSSocket/connect");
	}
	connected = true;
	try {
		write("data");
	} catch (...) {
		close();
		throw;
	}
}


int FGFSSocket::close(void)
{
	if (connected)
		write("quit");
	if (sock < 0)
		return 0;
	int ret = ::close(sock);
	sock = -1;
	return ret;
}


int FGFSSocket::write(const char *msg, ...)
{
	va_list va;
	ssize_t len;
	char buf[maxlen];
	fd_set fd;
	struct timeval tv;

	FD_ZERO(&fd);
	FD_SET(sock, &fd);
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	if (!select(FD_SETSIZE, 0, &fd, 0, &tv))
		throw("FGFSSocket::write/select: timeout exceeded");

	va_start(va, msg);
	vsprintf(buf, msg, va);
	va_end(va);
	std::cout << "SEND: " << buf << std::endl;
	strcat(buf, "\015\012");

	len = ::write(sock, buf, strlen(buf));
	if (len < 0)
		throw("FGFSSocket::write");
	return len;
}


const char *FGFSSocket::read(void)
{
	static char buf[maxlen];
	char *p;
	fd_set fd;
	struct timeval tv;
	ssize_t len;

	FD_ZERO(&fd);
	FD_SET(sock, &fd);
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	if (!select(FD_SETSIZE, &fd, 0, 0, &tv)) {
		if (timeout == 0)
			return 0;
		else
			throw("FGFSSocket::read/select: timeout exceeded");
	}

	len = ::read(sock, buf, maxlen - 1);
	if (len < 0)
		throw("FGFSSocket::read/read");
	if (len == 0)
		return 0;

	for (p = &buf[len - 1]; p >= buf; p--)
		if (*p != '\015' && *p != '\012')
			break;
	*++p = '\0';
	return strlen(buf) ? buf : 0;
}


inline void FGFSSocket::flush(void)
{
	int i = timeout;
	timeout = 0;
	while (read())
		;
	timeout = i;
}


int main(const int argc, const char *argv[])
try {
	const char *hostname = argc > 1 ? argv[1] : "localhost";
	int port = argc > 2 ? atoi(argv[2]) : 5501;

	FGFSSocket f(hostname, port);
	f.flush();
	f.write("set /controls/engines/engine[%d]/throttle %lg", 0, 1.0);
	f.write("set /controls/engines/engine[%d]/throttle %lg", 1, 1.0);
	f.write("get /sim/aircraft");
	const char *p = f.read();
	if (p)
		std::cout << "RECV: " << p << std::endl;
	return 0;

} catch (const char s[]) {
	std::cerr << "Error: " << s << ": " << strerror(errno) << std::endl;
	return -1;

} catch (...) {
	std::cerr << "Error: unknown exception" << std::endl;
	return -1;
}


// vim:cindent
