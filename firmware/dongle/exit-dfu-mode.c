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

static const char *dfu_state_name(uint8_t state) {
	static const char *NAMES[] = {
		"appIDLE",
		"appDETACH",
		"dfuIDLE",
		"dfuDNLOAD-SYNC",
		"dfuDNBUSY",
		"dfuDNLOAD-IDLE",
		"dfuMANIFEST-SYNC",
		"dfuMANIFEST",
		"dfuMANIFEST-WAIT-RESET",
		"dfuUPLOAD-IDLE",
		"dfuERROR",
	};
	if (state < sizeof(NAMES) / sizeof(*NAMES)) {
		return NAMES[state];
	} else {
		return "<unknown state>";
	}
}

static int dfu_getstatus(struct libusb_device_handle *handle, void *buffer) {
	int rc;
	if ((rc = libusb_control_transfer(handle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 3, 0, 0, buffer, 6, 0)) != 6) {
		fprintf(stderr, "DFU_GETSTATUS: error %d\n", rc);
		return 0;
	} else {
		return 1;
	}
}

static int dfu_clrstatus(struct libusb_device_handle *handle) {
	int rc;
	if ((rc = libusb_control_transfer(handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 4, 0, 0, 0, 0, 0)) != 0) {
		fprintf(stderr, "DFU_CLRSTATUS: error %d\n", rc);
		return 0;
	} else {
		return 1;
	}
}

static int exit_dfu_mode(struct libusb_device_handle *handle) {
	unsigned int clear_attempts = 3;
	uint8_t dfu_status[6];
	int rc;

	fputs("Transitioning to dfu[DNLOAD-]IDLE state... ", stdout);
	fflush(stdout);

	for (;;) {
		if (!dfu_getstatus(handle, dfu_status)) {
			return EXIT_FAILURE;
		}
		printf("%s... ", dfu_state_name(dfu_status[4]));
		fflush(stdout);
		if (dfu_status[4] == 2 || dfu_status[4] == 5) {
			puts("OK");
			break;
		} else if (!clear_attempts--) {
			fputs("Giving up.\n", stderr);
			return EXIT_FAILURE;
		}

		fputs("Clear status... ", stdout);
		fflush(stdout);
		if (!dfu_clrstatus(handle)) {
			return EXIT_FAILURE;
		}
	}

	fputs("Issuing zero-length download... ", stdout);
	fflush(stdout);

	if ((rc = libusb_control_transfer(handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 0x01, 0, 0, 0, 0, 0)) != 0) {
		fprintf(stderr, "DFU_DNLOAD: error %d\n", rc);
		return EXIT_FAILURE;
	}
	if (!dfu_getstatus(handle, dfu_status)) {
		return EXIT_FAILURE;
	}
	if (dfu_status[4] != 7) {
		fprintf(stderr, "Improper DFU state transition (expected dfu[DNLOAD-]IDLE -> dfuMANIFEST, got %s).\n", dfu_state_name(dfu_status[4]));
		return EXIT_FAILURE;
	}

	puts("OK");

	return EXIT_SUCCESS;
}

static int find_device_and_exit_dfu_mode(uint16_t vid, uint16_t pid) {
	struct libusb_device_handle *handle;
	int rc;

	if (!(handle = libusb_open_device_with_vid_pid(0, vid, pid))) {
		fprintf(stderr, "libusb_open_device_with_vid_pid: error\n");
		return EXIT_FAILURE;
	}

	rc = exit_dfu_mode(handle);

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

	rc = find_device_and_exit_dfu_mode((uint16_t) vid, (uint16_t) pid);

	libusb_exit(0);

	return rc;
}

