#ifndef LOG_READER_INDEX_H
#define LOG_READER_INDEX_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include <string>
#include <utility>
#include <cstddef>
#include <stdint.h>

//
// An accessor for the index file associated with each log.
//
class log_reader_index : public noncopyable {
	public:
		//
		// An index record consists of the frame number of the first frame in a
		// block and the location in the file where the block starts.
		//
		typedef std::pair<uint64_t, uint64_t> record_type;

		//
		// Opens the index file for a log.
		//
		log_reader_index(const std::string &name);

		//
		// Returns the number of frames in the log, as indicated by the index.
		//
		uint64_t size();

		//
		// Finds the index record that contains the specified frame.
		//
		record_type find_frame(uint64_t frame);

	private:
		file_descriptor fd;
		std::size_t the_size;

		record_type load_record(uint64_t rec);
};

#endif

