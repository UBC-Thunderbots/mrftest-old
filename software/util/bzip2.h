#ifndef UTIL_BZIP2_H
#define UTIL_BZIP2_H

#include "util/noncopyable.h"
#include <bzlib.h>
#include <cstdint>
#include <google/protobuf/io/zero_copy_stream.h>

namespace BZip2 {
	class InputStream : public google::protobuf::io::ZeroCopyInputStream, public NonCopyable {
		public:
			explicit InputStream(google::protobuf::io::ZeroCopyInputStream *input);
			~InputStream();
			bool Next(const void **data, int *size);
			void BackUp(int count);
			bool Skip(int count);
			int64_t ByteCount() const;

		private:
			bool error, eof;
			google::protobuf::io::ZeroCopyInputStream *input;
			bz_stream bzs;
			char output_buffer[65536];
			int output_backed_up;
	};
}

#endif

