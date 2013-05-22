#include "main.h"
#include "util/exception.h"
#include "util/fd.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	class SectorArray : public NonCopyable {
		public:
			explicit SectorArray(const FileDescriptor &fd);
			off_t size() const;
			std::vector<uint8_t> get(off_t i) const;

		private:
			const FileDescriptor &fd;
			off_t size_;
	};
}

SectorArray::SectorArray(const FileDescriptor &fd) : fd(fd) {
	off_t rc = lseek(fd.fd(), 0, SEEK_END);
	if (rc == static_cast<off_t>(-1)) {
		throw SystemError("lseek", errno);
	}
	size_ = rc / 512;
}

off_t SectorArray::size() const {
	return size_;
}

std::vector<uint8_t> SectorArray::get(off_t i) const {
	assert(i < size());
	std::vector<uint8_t> buffer(512);
	{
		uint8_t *ptr = &buffer[0];
		std::size_t len = 512;
		off_t off = i * 512U;
		while (len) {
			ssize_t rc = pread(fd.fd(), ptr, len, off);
			if (rc < 0) {
				throw SystemError("pread", errno);
			}
			ptr += rc;
			len -= rc;
			off += rc;
		}
	}
	return buffer;
}

namespace {
	class ScanResult {
		public:
			struct Epoch {
				off_t first_sector, last_sector;
			};

			explicit ScanResult(const SectorArray &sarray);
			off_t nonblank_size() const;
			const std::vector<Epoch> &epochs() const;

		private:
			off_t nonblank_size_;
			std::vector<Epoch> epochs_;
	};
}

ScanResult::ScanResult(const SectorArray &sarray) {
	off_t low = 0, high = sarray.size() + 1;
	while (low + 1 < high) {
		off_t pos = (low + high - 1) / 2;
		const std::vector<uint8_t> &sector = sarray.get(pos);
		bool blank = std::count(sector.begin(), sector.end(), static_cast<uint8_t>(0)) == static_cast<std::vector<uint8_t>::difference_type>(sector.size());
		if (blank) {
			high = pos + 1;
		} else {
			low = pos + 1;
		}
	}
	nonblank_size_ = low;

	if (nonblank_size()) {
		// Find number of epochs.
		std::size_t num_epochs;
		{
			const std::vector<uint8_t> &sector = sarray.get(nonblank_size() - 1);
			num_epochs = sector[128 * 3] | (sector[128 * 3 + 1] << 8);
		}

		// Find locations of epochs.
		for (std::size_t epoch = 1; epoch <= num_epochs; ++epoch) {
			std::cout << "Looking for epoch " << epoch << '\n';
			Epoch epoch_struct;

			// Search for start of epoch.
			off_t low = 0, high = nonblank_size();
			while (low + 1 < high) {
				off_t pos = (low + high - 1) / 2;
				const std::vector<uint8_t> &sector = sarray.get(pos);
				std::size_t sector_epoch = sector[128 * 3] | (sector[128 * 3 + 1] << 8);
				std::cout << "Search start, low=" << low << ", high=" << high << ", pos=" << pos << ", sector_epoch=" << sector_epoch << '\n';
				if (sector_epoch >= epoch) {
					high = pos + 1;
				} else {
					low = pos + 1;
				}
			}
			epoch_struct.first_sector = low;

			// Search for end of epoch.
			low = 0, high = nonblank_size() + 1;
			while (low + 1 < high) {
				off_t pos = (low + high - 1) / 2;
				const std::vector<uint8_t> &sector = sarray.get(pos);
				std::size_t sector_epoch = sector[128 * 3] | (sector[128 * 3 + 1] << 8);
				std::cout << "Search end, low=" << low << ", high=" << high << ", pos=" << pos << ", sector_epoch=" << sector_epoch << '\n';
				if (sector_epoch > epoch) {
					high = pos + 1;
				} else {
					low = pos + 1;
				}
			}
			epoch_struct.last_sector = low - 1;

			epochs_.push_back(epoch_struct);
		}
	}
}

off_t ScanResult::nonblank_size() const {
	return nonblank_size_;
}

const std::vector<ScanResult::Epoch> &ScanResult::epochs() const {
	return epochs_;
}

namespace {
	int do_erase(const SectorArray &sdcard, const ScanResult *) {
		std::cout << "Not implemented.\n";
		return 1;
	}

	int do_info(const SectorArray &sdcard, const ScanResult *scan_result) {
		std::cout << "Sectors: " << sdcard.size() << '\n';
		std::cout << "Used: " << scan_result->nonblank_size() << '\n';
		std::cout << "Epochs:\n";
		for (auto i : scan_result->epochs()) {
			std::cout << '[' << i.first_sector << ',' << i.last_sector << "]\n";
		}
		return 1;
	}

	struct Command {
		std::string command;
		int args;
		bool needs_write;
		bool needs_scan;
		int (*handler)(const SectorArray &sdcard, const ScanResult *scan_result);
	};

	const struct Command COMMANDS[] = {
		{ "erase", 0, true, false, &do_erase },
		{ "info", 0, false, true, &do_info },
	};

	const Command *find_command(const char *command) {
		for (std::size_t i = 0; i < sizeof(COMMANDS) / sizeof(*COMMANDS); ++i) {
			if (COMMANDS[i].command == command) {
				return &COMMANDS[i];
			}
		}
		return 0;
	}

	void usage(const char *app) {
		std::cerr << "Usage:\n";
		std::cerr << app << " disk command [args…]\n";
		std::cerr << '\n';
		std::cerr << "Possible commands are:\n";
		for (std::size_t i = 0; i < sizeof(COMMANDS) / sizeof(*COMMANDS); ++i) {
			std::cout << COMMANDS[i].command << '\n';
		}
	}
}

int app_main(int argc, char **argv) {
	// Check for at least a device node and a command.
	if (argc < 3) {
		usage(argv[0]);
		return 1;
	}

	// Parse command-line parameters.
	const Command *command = find_command(argv[2]);
	if (!command) {
		usage(argv[0]);
		return 1;
	}
	if (argc != command->args + 3) {
		std::cerr << "Wrong number of parameters for command " << command->command << ", found " << (argc - 3) << ", expected " << command->args << ".\n";
		return 1;
	}

	// Open the device.
	FileDescriptor sdfd(FileDescriptor::create_open(argv[1], command->needs_write ? O_RDWR : O_RDONLY, 0));
	SectorArray sdcard(sdfd);

	// Do a scan, if needed by the command.
	std::unique_ptr<ScanResult> scan_result;
	if (command->needs_scan) {
		std::cout << "Scanning card... ";
		std::cout.flush();
		scan_result.reset(new ScanResult(sdcard));
		std::cout << "OK.\n";
	}

	// Execute the command.
	return command->handler(sdcard, scan_result.get());
}

