#include "msg_0001_hello.hxx"

main(int argc, char **argv)
{
	string			str;
	FGMPSMsg0001Hello	msg1, *msg2;

	FGMPSMsg0001Hello::registerme();

	msg1.vermajor = 3;
	msg1.verminor = 7;
	msg1.verpatch = 42;
	msg1.servname = "test";

	str = msg1.encodemsg();

	printf("Message ID = %ui\n", msg1.getmessageid());
	printf("major = %u\n", msg1.vermajor);
	printf("minor = %u\n", msg1.verminor);
	printf("patch = %u\n", msg1.verpatch);
	printf("sname = %s\n", msg1.servname.c_str());

	printf("dump: ");
	for (int i=0; i<str.length(); i++) printf("%02x ", (unsigned char)str[i]);
	printf("\n");

	try {
		msg2 = (FGMPSMsg0001Hello*)FGMPSMessage::decodemsg(str);
	} catch (FGMPSDataException e) {
		printf("Exception: %s\n", e.what());
		exit(1);
	}

	printf("Message ID = %u\n", msg2->getmessageid());
	printf("major = %u\n", msg2->vermajor);
	printf("minor = %u\n", msg2->verminor);
	printf("patch = %u\n", msg2->verpatch);
	printf("sname = %s\n", msg2->servname.c_str());
	
}
