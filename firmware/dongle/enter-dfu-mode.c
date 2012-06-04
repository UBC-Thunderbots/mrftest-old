#include <errno.h>
#include <libusb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static bool convert_hex(const char *src, unsigned long *dest) {
	char *endptr = 0;
	errno = 0;
	*dest = strtoul(src, &endptr, 16);
	if (errno != 0) {
		return false;
	}
	if (*endptr) {
		return false;
	}
	return true;
}

static int enter_dfu_mode(struct libusb_device_handle *handle) {
	int config, rc;

	if ((rc = libusb_get_configuration(handle, &config)) != 0) {
		fprintf(stderr, "libusb_get_configuration: error %d\n", rc);
		return EXIT_FAILURE;
	}

	if (config != 0) {
		if ((rc = libusb_set_configuration(handle, 0)) != 0) {
			fprintf(stderr, "libusb_set_configuration: error %d\n", rc);
			return EXIT_FAILURE;
		}
	}

	if ((rc = libusb_control_transfer(handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 0x13, 0, 0, 0, 0, 0)) != 0) {
		fprintf(stderr, "libusb_control_transfer: error %d\n", rc);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int find_device_and_enter_dfu_mode(uint16_t vid, uint16_t pid) {
	struct libusb_device_handle *handle;
	int rc;

	if (!(handle = libusb_open_device_with_vid_pid(0, vid, pid))) {
		fprintf(stderr, "libusb_open_device_with_vid_pid: error\n");
		return EXIT_FAILURE;
	}

	rc = enter_dfu_mode(handle);

	libusb_close(handle);

	return rc;
}

int main(int argc, char **argv) {
	int rc;
	unsigned long vid, pid;

	if (argc != 3 || !convert_hex(argv[1], &vid) || !convert_hex(argv[2], &pid) || vid > 0xFFFFUL || pid > 0xFFFFUL) {
		fprintf(stderr, "Usage:\n%s vendor_id product_id\n", argv[0]);
		return EXIT_FAILURE;
	}

	if ((rc = libusb_init(0)) != 0) {
		fprintf(stderr, "libusb_init: %d\n", rc);
		return EXIT_FAILURE;
	}

	rc = find_device_and_enter_dfu_mode((uint16_t) vid, (uint16_t) pid);

	libusb_exit(0);

	return rc;
}

