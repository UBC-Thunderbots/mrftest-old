#include "util/misc.h"
#include "util/shm.h"
#include <stdexcept>
#include <cstdlib>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	class UMaskModifier : public NonCopyable {
		public:
			UMaskModifier(mode_t newmode) : oldmode(umask(newmode)) {
			}

			~UMaskModifier() {
				umask(oldmode);
			}

		private:
			mode_t oldmode;
	};

	FileDescriptor::Ptr create_new_temp_file(std::size_t sz) {
		UMaskModifier um(0077);
		char path[] = "/tmp/xbeed.XXXXXX";
		int ret = mkstemp(path);
		if (ret < 0) {
			throw std::runtime_error("Cannot create temporary file!");
		}
		const FileDescriptor::Ptr fd(FileDescriptor::create_from_fd(ret));
		unlink(path);
		if (ftruncate(fd, sz) < 0) {
			throw std::runtime_error("Cannot resize temporary file!");
		}
		return fd;
	}

	uint8_t *map_data(const FileDescriptor::Ptr fd, std::size_t sz) {
		void *ret = mmap(0, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd->fd(), 0);
		if (ret == get_map_failed()) {
			throw std::runtime_error("Cannot map temporary file!");
		}
		return static_cast<uint8_t *>(ret);
	}
}

RawShmBlock::RawShmBlock(std::size_t sz) : fd_(create_new_temp_file(sz)), size_(sz), data_(map_data(fd_, size_)) {
}

RawShmBlock::RawShmBlock(FileDescriptor::Ptr fd, std::size_t sz) : fd_(fd), size_(sz), data_(map_data(fd_, size_)) {
}

RawShmBlock::~RawShmBlock() {
	munmap(data_, size_);
}

