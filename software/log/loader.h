#ifndef LOG_LOADER_H
#define LOG_LOADER_H

#include "proto/log_record.pb.h"
#include <string>
#include <vector>

namespace LogLoader {
	bool is_current_version(const std::string &filename);
	bool needs_compressing(const std::string &filename);
	std::vector<Log::Record> load(const std::string &filename);
}

#endif

