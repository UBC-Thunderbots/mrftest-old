#include "log/loader.h"
#include "log/shared/magic.h"
#include "util/bzip2.h"
#include "util/fd.h"
#include <fcntl.h>
#include <limits>
#include <stdexcept>
#include <string>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>
namespace {
	bool check_magic(google::protobuf::io::ZeroCopyInputStream &zcis) {
		google::protobuf::io::CodedInputStream cis(&zcis);
		std::string buffer;
		if (!cis.ReadString(&buffer, static_cast<int>(Log::MAGIC.size()))) {
			return false;
		}
		return buffer == Log::MAGIC;
	}

	std::vector<Log::Record> load(google::protobuf::io::ZeroCopyInputStream &zcis) {
		Log::Record record;
		std::vector<Log::Record> records;
		for (;; ) {
			google::protobuf::io::CodedInputStream cis(&zcis);
			uint32_t record_size;
			if (!cis.ReadVarint32(&record_size)) {
				return records;
			}
			if (static_cast<uintmax_t>(record_size) > static_cast<uintmax_t>(std::numeric_limits<int>::max())) {
				throw std::runtime_error("Record too big.");
			}
			google::protobuf::io::CodedInputStream::Limit limit = cis.PushLimit(static_cast<int>(record_size));
			if (!record.ParseFromCodedStream(&cis)) {
				return records;
				throw std::runtime_error("I/O error or log file corrupt.");
			}
			cis.PopLimit(limit);
			records.push_back(record);
		}
		return records;
	}
}

bool LogLoader::is_current_version(const std::string &filename) {
	// First check for uncompressed data.
	{
		FileDescriptor::Ptr fd = FileDescriptor::create_open(filename.c_str(), O_RDONLY, 0);
		google::protobuf::io::FileInputStream fis(fd->fd());
		if (check_magic(fis)) {
			return true;
		}
	}

	// Second check for BZip2 compressed data.
	{
		FileDescriptor::Ptr fd = FileDescriptor::create_open(filename.c_str(), O_RDONLY, 0);
		google::protobuf::io::FileInputStream fis(fd->fd());
		BZip2::InputStream bzis(&fis);
		if (check_magic(bzis)) {
			return true;
		}
	}

	return false;
}

bool LogLoader::needs_compressing(const std::string &filename) {
	// Just check for the uncompressed magic.
	FileDescriptor::Ptr fd = FileDescriptor::create_open(filename.c_str(), O_RDONLY, 0);
	google::protobuf::io::FileInputStream fis(fd->fd());
	return check_magic(fis);
}

std::vector<Log::Record> LogLoader::load(const std::string &filename) {
	// First try uncompressed data.
	{
		FileDescriptor::Ptr fd = FileDescriptor::create_open(filename.c_str(), O_RDONLY, 0);
		google::protobuf::io::FileInputStream fis(fd->fd());
		if (check_magic(fis)) {
			return ::load(fis);
		}
	}

	// Second try BZip2 compressed data.
	{
		FileDescriptor::Ptr fd = FileDescriptor::create_open(filename.c_str(), O_RDONLY, 0);
		google::protobuf::io::FileInputStream fis(fd->fd());
		BZip2::InputStream bzis(&fis);
		if (check_magic(bzis)) {
			return ::load(bzis);
		}
	}

	throw std::runtime_error("Unrecognized log format.");
}

