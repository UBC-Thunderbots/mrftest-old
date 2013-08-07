#include "util/bzip2.h"
#include <cassert>
#include <limits>
#include <stdexcept>

BZip2::InputStream::InputStream(google::protobuf::io::ZeroCopyInputStream *input) : error(false), eof(false), input(input), output_backed_up(0) {
	bzs.next_in = nullptr;
	bzs.avail_in = 0;
	bzs.total_in_lo32 = 0;
	bzs.total_in_hi32 = 0;
	bzs.next_out = nullptr;
	bzs.avail_out = 0;
	bzs.total_out_lo32 = 0;
	bzs.total_out_hi32 = 0;
	bzs.state = nullptr;
	bzs.bzalloc = nullptr;
	bzs.bzfree = nullptr;
	bzs.opaque = nullptr;
	if (BZ2_bzDecompressInit(&bzs, 0, 0) != BZ_OK) {
		throw std::runtime_error("Failed to initialize BZip2.");
	}
}

BZip2::InputStream::~InputStream() {
	BZ2_bzDecompressEnd(&bzs);
}

bool BZip2::InputStream::Next(const void **data, int *size) {
	if (error) {
		return false;
	}

	if (output_backed_up) {
		*data = &bzs.next_out[-output_backed_up];
		*size = output_backed_up;
		output_backed_up = 0;
		return true;
	}

	if (eof) {
		return false;
	}

	bzs.next_out = output_buffer;
	bzs.avail_out = sizeof(output_buffer);
	while (bzs.avail_out) {
		while (!bzs.avail_in) {
			const void *p;
			int sz;
			if (!input->Next(&p, &sz)) {
				error = true;
				return false;
			}
			bzs.next_in = const_cast<char *>(static_cast<const char *>(p));
			bzs.avail_in = static_cast<unsigned int>(sz);
		}

		int rc = BZ2_bzDecompress(&bzs);
		if (rc == BZ_STREAM_END) {
			*data = output_buffer;
			*size = static_cast<int>(bzs.next_out - output_buffer);
			eof = true;
			return true;
		} else if (rc != BZ_OK) {
			error = true;
			return false;
		}
	}

	*data = output_buffer;
	*size = sizeof(output_buffer);
	return true;
}

void BZip2::InputStream::BackUp(int count) {
	assert(output_backed_up + count < bzs.next_out - output_buffer);
	output_backed_up += count;
}

bool BZip2::InputStream::Skip(int count) {
	if (error) {
		return false;
	}

	while (count > 0) {
		const void *data;
		int sz;
		if (!Next(&data, &sz)) {
			return false;
		}
		count -= sz;
	}

	if (count < 0) {
		BackUp(-count);
	}

	return true;
}

int64_t BZip2::InputStream::ByteCount() const {
	return static_cast<int64_t>((static_cast<uint64_t>(bzs.total_out_hi32) << 32) | static_cast<uint64_t>(bzs.total_out_lo32));
}

