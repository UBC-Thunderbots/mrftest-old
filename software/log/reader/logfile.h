#ifndef LOG_READER_LOGFILE_H
#define LOG_READER_LOGFILE_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include <string>
#include <utility>
#include <vector>
#include <cstddef>
#include <stdint.h>

//
// Allows low-level access to a log file on a block-by-block basis.
//
class log_reader_logfile : public noncopyable {
	public:
		//
		// The data about a particular block is the combination of the block's
		// compressed length and the block's uncompressed data.
		//
		typedef std::pair<std::size_t, std::vector<uint8_t> > block_type;

		//
		// Opens the log file.
		//
		log_reader_logfile(const std::string &name);

		//
		// Loads a the block at a given file offset.
		//
		const block_type &load_block(uint64_t address);

	private:
		file_descriptor fd;
		std::pair<uint64_t, block_type> block_cache[2];
		unsigned int block_cache_mru;
};

#endif

