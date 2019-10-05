#ifndef UTIL_BZIP2_H
#define UTIL_BZIP2_H

#include <bzlib.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <cstdint>
#include "util/noncopyable.h"

namespace BZip2
{
class InputStream final : public google::protobuf::io::ZeroCopyInputStream,
                          public NonCopyable
{
   public:
    explicit InputStream(google::protobuf::io::ZeroCopyInputStream *input);
    ~InputStream();
    bool Next(const void **data, int *size) override;
    void BackUp(int count) override;
    bool Skip(int count) override;
    int64_t ByteCount() const override;

   private:
    bool error, eof;
    google::protobuf::io::ZeroCopyInputStream *input;
    bz_stream bzs;
    char output_buffer[65536];
    int output_backed_up;
};
}

#endif
