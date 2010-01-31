#include "log/reader/index.h"
#include "util/codec.h"
#include <stdexcept>
#include <glibmm.h>
#include <cassert>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>



namespace {
	std::string get_index_filename(const std::string &name) {
		return Glib::get_user_data_dir() + "/thunderbots/" + name + ".idx";
	}
}



log_reader_index::log_reader_index(const std::string &name) : fd(get_index_filename(name).c_str(), O_RDONLY) {
	struct stat st;

	if (fstat(fd, &st) < 0) {
		throw std::runtime_error("Cannot get length of index file!");
	}
	if (st.st_size % 16 != 0) {
		throw std::runtime_error("Index has nonintegral number of entries!");
	}
	the_size = st.st_size / 16;
}



uint64_t log_reader_index::size() {
	return load_record(the_size - 1).first;
}



log_reader_index::record_type log_reader_index::find_frame(uint64_t frame) {
	// Special case, check if it's before the first index record (i.e. in the
	// first block)
	if (frame < load_record(0).first) {
		return std::make_pair(0, 0);
	}

	// Do a binary search.
	uint64_t before_rec = 0, after_rec = the_size - 1;
	while (after_rec - before_rec > 1) {
		uint64_t mid_rec = (before_rec + after_rec) / 2;
		record_type rec = load_record(mid_rec);
		if (rec.first == frame) {
			return rec;
		} else if (rec.first < frame) {
			before_rec = mid_rec;
		} else if (rec.first > frame) {
			after_rec = mid_rec;
		}
	}

	// Return the record before the target frame.
	return load_record(before_rec);
}



log_reader_index::record_type log_reader_index::load_record(uint64_t rec) {
	assert(rec < the_size);
	off_t addr = static_cast<off_t>(rec) * 16;
	unsigned char buffer[16];
	if (pread(fd, buffer, sizeof(buffer), addr) != sizeof(buffer)) {
		throw std::runtime_error("Cannot read index file!");
	}
	std::size_t i = 0;
	uint64_t frame = decode_u64(buffer, i);
	uint64_t byte = decode_u64(buffer, i);
	return std::make_pair(frame, byte);
}

