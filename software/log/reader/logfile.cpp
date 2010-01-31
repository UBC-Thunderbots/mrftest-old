#include "log/reader/logfile.h"
#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <glibmm.h>
#include <bzlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>



namespace {
	std::string get_log_filename(const std::string &name) {
		return Glib::get_user_data_dir() + "/thunderbots/" + name + ".log";
	}
}



log_reader_logfile::log_reader_logfile(const std::string &name) : fd(get_log_filename(name).c_str(), O_RDONLY), block_cache_mru(0) {
	block_cache[0].first = block_cache[1].first = std::numeric_limits<uint64_t>::max();
}



const log_reader_logfile::block_type &log_reader_logfile::load_block(uint64_t address) {
	// See if it's in the cache.
	for (unsigned int i = 0; i < sizeof(block_cache) / sizeof(*block_cache); ++i) {
		if (block_cache[i].first == address) {
			block_cache_mru = i;
			return block_cache[i].second;
		}
	}

	// We need to load it from disk.
	unsigned int index = (block_cache_mru + 1) % (sizeof(block_cache) / sizeof(*block_cache));
	block_cache[index].first = address;
	block_cache[index].second.first = 0;
	block_cache[index].second.second.clear();
	bz_stream bzs;
	bzs.bzalloc = 0;
	bzs.bzfree = 0;
	bzs.opaque = 0;
	if (BZ2_bzDecompressInit(&bzs, 0, 0) != BZ_OK) {
		throw std::runtime_error("Cannot initialize BZ2 decompression library!");
	}
	char buffer_in[1024], buffer_out[1024];
	bzs.avail_in = bzs.avail_out = 0;
	int rc;
	do {
		ssize_t bytes_read;
		if (bzs.avail_in < sizeof(buffer_in)) {
			bytes_read = pread(fd, buffer_in + bzs.avail_in, sizeof(buffer_in) - bzs.avail_in, address);
		} else {
			bytes_read = 0;
		}
		address += bytes_read;
		bzs.next_in = buffer_in;
		bzs.avail_in += bytes_read;
		bzs.next_out = buffer_out;
		bzs.avail_out = sizeof(buffer_out);
		rc = BZ2_bzDecompress(&bzs);
		if (rc != BZ_OK && rc != BZ_STREAM_END) {
			BZ2_bzDecompressEnd(&bzs);
			throw std::runtime_error("Cannot decompress log block!");
		}
		if (rc != BZ_STREAM_END && bytes_read == 0 && bzs.avail_in == 0) {
			BZ2_bzDecompressEnd(&bzs);
			throw std::runtime_error("Log is truncated!");
		}
		std::copy(buffer_out, bzs.next_out, std::back_inserter(block_cache[index].second.second));
		std::copy(bzs.next_in, bzs.next_in + bzs.avail_in, buffer_in);
	} while (rc != BZ_STREAM_END);
	block_cache[index].second.first = bzs.total_in_lo32;
	BZ2_bzDecompressEnd(&bzs);

	// Return the loaded block.
	block_cache_mru = index;
	return block_cache[index].second;
}

