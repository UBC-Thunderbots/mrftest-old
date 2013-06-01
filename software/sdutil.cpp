#include "main.h"
#include "util/codec.h"
#include "util/exception.h"
#include "util/fd.h"
#include "util/thermal.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>
#include <glibmm/convert.h>
#include <glibmm/ustring.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	class SectorArray : public NonCopyable {
		public:
			const FileDescriptor &fd;

			explicit SectorArray(const FileDescriptor &fd);
			off_t size() const;
			std::vector<uint8_t> get(off_t i) const;
			void zero(off_t i);

		private:
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

void SectorArray::zero(off_t i) {
	assert(i < size());
	std::vector<uint8_t> buffer(512, static_cast<uint8_t>(i));
	{
		const uint8_t *ptr = &buffer[0];
		std::size_t len = 512;
		off_t off = i * 512U;
		while (len) {
			ssize_t rc = pwrite(fd.fd(), ptr, len, off);
			if (rc < 0) {
				throw SystemError("pwrite", errno);
			}
			ptr += rc;
			len -= rc;
			off += rc;
		}
	}
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
			Epoch epoch_struct;

			// Search for start of epoch.
			off_t low = 0, high = nonblank_size();
			while (low + 1 < high) {
				off_t pos = (low + high - 1) / 2;
				const std::vector<uint8_t> &sector = sarray.get(pos);
				std::size_t sector_epoch = sector[128 * 3] | (sector[128 * 3 + 1] << 8);
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
	int do_copy(SectorArray &sdcard, const ScanResult *scan_result, char **args) {
		std::size_t epoch_index = static_cast<std::size_t>(std::stoll(args[0], nullptr, 0));
		if (!epoch_index) {
			std::cerr << "Epoch indices start from 1.\n";
			return 1;
		}
		if (epoch_index > scan_result->epochs().size()) {
			std::cerr << "This card has only " << scan_result->epochs().size() << " epochs.\n";
			return 1;
		}
		const ScanResult::Epoch &epoch = scan_result->epochs()[epoch_index - 1];

		std::cout << "Copying sectors " << epoch.first_sector << " through " << epoch.last_sector << " to file \"" << args[1] << "\": ";
		std::cout.flush();

		FileDescriptor fd(FileDescriptor::create_open(args[1], O_WRONLY | O_CREAT | O_TRUNC, 0666));
		for (off_t sector = epoch.first_sector; sector <= epoch.last_sector; ++sector) {
			const std::vector<uint8_t> &buffer = sdcard.get(sector);
			const uint8_t *ptr = &buffer[0];
			std::size_t len = 512;
			while (len) {
				ssize_t rc = write(fd.fd(), ptr, len);
				if (rc < 0) {
					throw SystemError("write", errno);
				}
				ptr += rc;
				len -= rc;
			}
		}
		fd.close();

		std::cout << "OK.\n";
		return 0;
	}

	int do_erase(SectorArray &sdcard, const ScanResult *, char **) {
		std::cout << "Erasing sectors 0 through " << (sdcard.size() - 1) << ": ";
		std::cout.flush();
		uint64_t params[2];
		params[0] = 0;
		params[1] = static_cast<uint64_t>(sdcard.size()) * UINT64_C(512);
		if (ioctl(sdcard.fd.fd(), BLKDISCARD, params) < 0) {
			if (errno == EOPNOTSUPP) {
				std::cout << "Well your card reader dun sucks so I card scans and manual overwrite" << std::endl;
				ScanResult scan_result(sdcard);
				const off_t WRITE_AMOUNT = 4*1024*1024;
				uint8_t blank_data[WRITE_AMOUNT];
				std::memset(blank_data, 0,WRITE_AMOUNT); //4 megs of nothing
				std::size_t write_size;
				off_t file_offset = 0; 
				do  {
					write_size = std::min(WRITE_AMOUNT, scan_result.nonblank_size()*512 - file_offset);
					ssize_t ret_val = pwrite(sdcard.fd.fd(),blank_data,write_size,file_offset);
					if (ret_val < 0) {
						throw SystemError("pwrite", errno);
					}
					file_offset += ret_val;
				} while(file_offset < scan_result.nonblank_size()*512);

			} else {
				throw SystemError("ioctl(BLKDISCARDS)", errno);
			}
		}
		std::cout << "OK.\n";
		return 0;
	}

	int do_info(SectorArray &sdcard, const ScanResult *scan_result, char **) {
		std::cout << "Sectors: " << sdcard.size() << '\n';
		std::cout << "Used: " << scan_result->nonblank_size() << '\n';
		std::cout << "Epochs:\n";
		for (std::size_t i = 0; i < scan_result->epochs().size(); ++i) {
			std::cout << (i + 1) << ": " << '[' << scan_result->epochs()[i].first_sector << ',' << scan_result->epochs()[i].last_sector << "]\n";
		}
		return 1;
	}

	int do_pcap(SectorArray &sdcard, const ScanResult *scan_result, char **args) {
		// Get the epoch.
		std::size_t epoch_index = static_cast<std::size_t>(std::stoll(args[0], nullptr, 0));
		if (!epoch_index) {
			std::cerr << "Epoch indices start from 1.\n";
			return 1;
		}
		if (epoch_index > scan_result->epochs().size()) {
			std::cerr << "This card has only " << scan_result->epochs().size() << " epochs.\n";
			return 1;
		}
		const ScanResult::Epoch &epoch = scan_result->epochs()[epoch_index - 1];

		std::cout << "Outputting sectors " << epoch.first_sector << " through " << epoch.last_sector << " to PCAP file \"" << args[1] << "\": ";
		std::cout.flush();

		// Open the capture file.
		std::cout.flush();
		std::ofstream ofs;
		ofs.exceptions(std::ios_base::eofbit | std::ios_base::failbit | std::ios_base::badbit);
		ofs.open(args[1], std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
		{
			uint32_t u32;
			uint16_t u16;
			u32 = 0xA1B2C3D4;
			ofs.write(reinterpret_cast<const char *>(&u32), 4); // Magic number
			u16 = 2;
			ofs.write(reinterpret_cast<const char *>(&u16), 2); // Major version number
			u16 = 4;
			ofs.write(reinterpret_cast<const char *>(&u16), 2); // Minor version number
			u32 = 0;
			ofs.write(reinterpret_cast<const char *>(&u32), 4); // Timezone offset (we use UTC)
			ofs.write(reinterpret_cast<const char *>(&u32), 4); // Significant figures in timestamps (everyone sets this to zero)
			u32 = 131;
			ofs.write(reinterpret_cast<const char *>(&u32), 4); // Maximum length of a captured packet
			u32 = 195;
			ofs.write(reinterpret_cast<const char *>(&u32), 4); // Network data link type
		}

		// Copy the data.
		unsigned int packets = 0;
		for (off_t sector = epoch.first_sector; sector <= epoch.last_sector; ++sector) {
			const std::vector<uint8_t> &buffer = sdcard.get(sector);

			uint32_t ticks = 0;
			{
				const uint8_t *ptr = &buffer[128 * 3];
				ptr += 2;
				for (unsigned int i = 0; i < 4; ++i) {
					ticks <<= 8;
					ticks |= ptr[3 - i];
				}
			}

			for (unsigned int packet_index = 0; packet_index < 3; ++packet_index) {
				const uint8_t *ptr = &buffer[packet_index * 128];
				std::size_t len = *ptr++;
				if (len) {
					unsigned long seconds = ticks / 200;
					unsigned long microseconds = (ticks % 200) * 5000;
					{
						uint32_t u32;
						u32 = static_cast<uint32_t>(seconds);
						ofs.write(reinterpret_cast<const char *>(&u32), 4);
						u32 = static_cast<uint32_t>(microseconds);
						ofs.write(reinterpret_cast<const char *>(&u32), 4);
						u32 = static_cast<uint32_t>(len);
						ofs.write(reinterpret_cast<const char *>(&u32), 4);
						ofs.write(reinterpret_cast<const char *>(&u32), 4);
					}
					ofs.write(reinterpret_cast<const char *>(ptr), static_cast<std::streamsize>(len));
					++packets;
				}
			}
		}
		ofs.close();
		std::cout << "Copied " << packets << " packets.\n";
		return 0;
	}

	int do_tsv(SectorArray &sdcard, const ScanResult *scan_result, char **args) {
		std::size_t epoch_index = static_cast<std::size_t>(std::stoll(args[0], nullptr, 0));
		if (!epoch_index) {
			std::cerr << "Epoch indices start from 1.\n";
			return 1;
		}
		if (epoch_index > scan_result->epochs().size()) {
			std::cerr << "This card has only " << scan_result->epochs().size() << " epochs.\n";
			return 1;
		}
		const ScanResult::Epoch &epoch = scan_result->epochs()[epoch_index - 1];

		std::cout << "Outputting sectors " << epoch.first_sector << " through " << epoch.last_sector << " to TSV file \"" << args[1] << "\": ";
		std::cout.flush();

		std::ofstream ofs;
		ofs.exceptions(std::ios_base::badbit | std::ios_base::failbit);
		ofs.open(args[1], std::ios_base::out | std::ios_base::trunc);
		ofs << "Epoch\tTicks\tBreakbeam\tCapacitor (V)\tBattery (V)\tBoard Temperature (C)\tLPS\tEncoder 0\tEncoder 1\tEncoder 2\tEncoder 3\tSetpoint 0\tSetpoint 1\tSetpoint 2\tSetpoint 3\tMotor 0\tMotor 1\tMotor 2\tMotor 3\tMotor 4\n";
		for (off_t sector = epoch.first_sector; sector <= epoch.last_sector; ++sector) {
			const std::vector<uint8_t> &buffer = sdcard.get(sector);

			const uint8_t *ptr = &buffer[128 * 3];
			unsigned int epoch = static_cast<uint16_t>(ptr[0] | static_cast<uint16_t>(ptr[1] << 8));
			ptr += 2;
			uint32_t ticks = 0;
			for (unsigned int i = 0; i < 4; ++i) {
				ticks <<= 8;
				ticks |= ptr[3 - i];
			}
			ptr += 4;
			int breakbeam_diff = static_cast<int16_t>(static_cast<uint16_t>(ptr[0] | static_cast<uint16_t>(ptr[1] << 8)));
			ptr += 2;
			unsigned int adc_channels[8];
			for (std::size_t i = 0; i < sizeof(adc_channels) / sizeof(*adc_channels); ++i) {
				adc_channels[i] = static_cast<uint16_t>(ptr[0] | static_cast<uint16_t>(ptr[1] << 8));
				ptr += 2;
			}
			int encoder_counts[4];
			for (std::size_t i = 0; i < sizeof(encoder_counts) / sizeof(*encoder_counts); ++i) {
				encoder_counts[i] = static_cast<int16_t>(static_cast<uint16_t>(ptr[0] | static_cast<uint16_t>(ptr[1] << 8)));
				ptr += 2;
			}
			float setpoint[4];
			for (std::size_t i = 0; i < sizeof(setpoint) / sizeof(*setpoint); ++i) {
				uint32_t store = 0;
				for (unsigned int j = 0; j < 4; ++j) {
					store <<= 8;
					store |= ptr[3 - j];
				}
				ptr += 4;
				setpoint[i] = decode_u32_to_float(store);
			}
			uint8_t motor_directions = *ptr++;
			int motor_drives[5];
			for (std::size_t i = 0; i < 5; ++i) {
				motor_drives[i] = static_cast<int8_t>(*ptr++);
				if (motor_directions & (UINT8_C(1) << i)) {
					motor_drives[i] = -motor_drives[i];
				}
			}
			uint8_t encoders_failed = *ptr++;
			uint8_t wheel_hall_sensors_failed = *ptr++;
			uint8_t dribbler_hall_sensors_failed = *ptr++;
			double capacitor_voltage = adc_channels[0] / 1024.0 * 3.3 / 2200 * (2200 + 200000);
			double battery_voltage = adc_channels[1] / 1024.0 * 3.3 / 2200 * (2200 + 20000);
			double board_temperature = adc_voltage_to_board_temp(adc_channels[5] / 1024.0 * 3.3);
			unsigned int lps_reading = adc_channels[6];
			ofs << epoch << '\t' << ticks << '\t' << breakbeam_diff << '\t' << capacitor_voltage << '\t' << battery_voltage << '\t' << board_temperature << '\t' << lps_reading;
			for (unsigned int i = 0; i < 4; ++i) {
				if (encoders_failed & (1 << i)) {
					ofs << "\tNaN";
				} else {
					ofs << '\t' << encoder_counts[i];
				}
			}
			for (unsigned int i = 0; i < 4; ++i) {
				ofs << '\t' << setpoint[i];
			}
			for (unsigned int i = 0; i < 5; ++i) {
				bool failed = i == 4 ? !!(dribbler_hall_sensors_failed & 3) : !!((wheel_hall_sensors_failed >> (2 * i)) & 3);
				if (failed) {
					ofs << "\tNaN";
				} else {
					ofs << '\t' << motor_drives[i];
				}
			}
			ofs << '\n';
		}
		ofs.close();

		std::cout << "OK.\n";
		return 0;
	}

	struct Command {
		std::string command;
		int args;
		bool needs_write;
		bool needs_scan;
		int (*handler)(SectorArray &sdcard, const ScanResult *scan_result, char **args);
	};

	const struct Command COMMANDS[] = {
		{ "copy", 2, false, true, &do_copy },
		{ "erase", 0, true, false, &do_erase },
		{ "info", 0, false, true, &do_info },
		{ "pcap", 2, false, true, &do_pcap },
		{ "tsv", 2, false, true, &do_tsv },
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
	return command->handler(sdcard, scan_result.get(), argv + 3);
}

