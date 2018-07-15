#ifndef LOG_LOADER_H
#define LOG_LOADER_H

#include <string>
#include <vector>
#include "proto/log_record.pb.h"

namespace LogLoader
{
bool is_current_version(const std::string &filename);
bool needs_compressing(const std::string &filename);
std::vector<Log::Record> load(const std::string &filename);
}

#endif
