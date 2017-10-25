#include "hidapi.h"
#include "hidparse.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const char padding[] = "                "; // sixteen spaces




void print_report_descriptor(FILE* fp, int depth, hid_item* item)
{
	if (item->collection) {
		hid_item* c = item->collection;
		for (; c != NULL; c = c->next) {
			print_report_descriptor(fp, depth + 1, c);
		}
	} else {
		// it's a leaf
		fprintf(fp, "%sitem type %02x usage %08x bits %2d, report %02x\n", padding + (16 - depth), 
				item->type, item->usage, item->report_size, item->report_id);
	}
}

int main(int argc, char* argv[])
{
	#define MAX_STR 255
	wchar_t wstr[MAX_STR];
    int res;
    
	if (hid_init())
		return EXIT_FAILURE;

	if (argc == 1) {
		struct hid_device_info *devs, *cur_dev;
		devs = hid_enumerate(0x0, 0x0);
		cur_dev = devs;	
		while (cur_dev) {
			printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
			printf("\n");
			printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
			printf("  Product:      %ls\n", cur_dev->product_string);
			printf("  Release:      %hx\n", cur_dev->release_number);
			printf("  Interface:    %d\n",  cur_dev->interface_number);
			printf("\n");
			cur_dev = cur_dev->next;
		}
		hid_free_enumeration(devs);
		return EXIT_SUCCESS;
	}

	// assume argv[1] is path
    hid_device *handle;
	handle = hid_open_path(argv[1]);
	if (!handle) {
		fprintf(stderr, "unable to open device with path %s\n", argv[1]);
 		return EXIT_FAILURE;
	}
    
	wstr[0] = 0x0000;
	res = hid_get_product_string(handle, wstr, MAX_STR);
	if (res < 0)
		fprintf(stderr, "Unable to read product string\n");
	printf("Product String: %ls\n", wstr);
	
	unsigned char descriptorBuffer[8192];
	int descriptorBytes = hid_get_descriptor(handle, descriptorBuffer, sizeof(descriptorBuffer));
	if (descriptorBytes < 0) {
		fprintf(stderr, "failed to read report descriptor");
		return EXIT_FAILURE;
	}

	hid_item* rootItem;
	res = hid_parse_reportdesc(descriptorBuffer, descriptorBytes, &rootItem);
	if (res != 0) {
		fprintf(stderr, "failure parsing report");
		return EXIT_FAILURE;
	}

	print_report_descriptor(stdout, 0, rootItem);


	hid_free_reportdesc(rootItem);

	return EXIT_SUCCESS;
}