#include <iostream>
#include <string>

#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

using namespace std;


void usage()
{
	cerr << "Usage:  propmerge [-o <outfile>] <infiles>" << endl;
}


int main(int argc, char *argv[])
{
	if (argc < 2) {
		usage();
		return 1;
	}

	sglog().setLogLevels(SG_ALL, SG_ALERT);

	int numfiles = 0;
	string outfile;
	SGPropertyNode root;

	for (int i = 1; i < argc; i++) {
		string s = argv[i];
		if (s == "-h" || s == "--help") {
			usage();
			return 0;
		}

		if (s == "-o" || s == "--output") {
			if (i + 1 == argc)
				break;
			outfile = argv[++i];
			continue;
		}

		try {
			readProperties(s, &root);
			numfiles++;
		} catch (const sg_exception &e) {
			cerr << "Error: " << e.getFormattedMessage() << endl;
			return 2;
		}
	}

	if (!numfiles) {
		cerr << "Error: Nothing to merge." << endl;
		return 3;
	}

	try {
		if (outfile.empty())
			writeProperties(cout, &root, true);
		else
			writeProperties(outfile, &root, true);

	} catch (const sg_exception &e) {
		cerr << "Error: " << e.getFormattedMessage() << endl;
		return 4;
	}

	return 0;
}


