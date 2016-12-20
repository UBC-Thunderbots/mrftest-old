#include "main.h"
#include "mrf/constants.h"
#include "util/codec.h"
#include "util/crc32.h"
#include "util/exception.h"
#include "util/fd.h"
#include "util/string.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iterator>
#include <iostream>
#include <locale>
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
	constexpr off_t SECTOR_SIZE = 512;
	constexpr off_t LOG_RECORD_SIZE = 128;
	constexpr off_t RECORDS_PER_SECTOR = SECTOR_SIZE / LOG_RECORD_SIZE;
	constexpr uint32_t LOG_MAGIC_TICK = UINT32_C(0xE2468845);
	constexpr off_t UPGRADE_AREA_SECTORS = 1024 * 4096 / SECTOR_SIZE;
	constexpr unsigned int UPGRADE_AREA_COUNT = 2;
	const std::array<uint8_t, SECTOR_SIZE> ZERO_SECTOR{0};

	const std::array<std::string, UPGRADE_AREA_COUNT> UPGRADE_AREA_NAMES = {
		"firmware",
		"fpga",
	};

	constexpr uint32_t UPGRADE_AREA_MAGICS[UPGRADE_AREA_COUNT] = {
		UINT32_C(0x1453CABE),
		UINT32_C(0x74E4BCC5),
	};

	class SectorArray final : public NonCopyable {
		public:
			const FileDescriptor &fd;

			explicit SectorArray(const FileDescriptor &fd);
			off_t size() const;
			void get(off_t i, void *buffer) const;
			void put(off_t i, const void *data);
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
	size_ = rc / SECTOR_SIZE;
}

off_t SectorArray::size() const {
	return size_;
}

void SectorArray::get(off_t i, void *buffer) const {
	assert(i < size());
	uint8_t *ptr = static_cast<uint8_t *>(buffer);
	std::size_t len = SECTOR_SIZE;
	off_t off = i * SECTOR_SIZE;
	while (len) {
		ssize_t rc = pread(fd.fd(), ptr, len, off);
		if (rc < 0) {
			throw SystemError("pread", errno);
		}
		ptr += rc;
		len -= static_cast<std::size_t>(rc);
		off += rc;
	}
}

void SectorArray::put(off_t i, const void *data) {
	assert(i < size());
	const uint8_t *ptr = static_cast<const uint8_t *>(data);
	std::size_t len = SECTOR_SIZE;
	off_t off = i * SECTOR_SIZE;
	while (len) {
		ssize_t rc = pwrite(fd.fd(), ptr, len, off);
		if (rc < 0) {
			throw SystemError("pwrite", errno);
		}
		ptr += rc;
		len -= static_cast<std::size_t>(rc);
		off += rc;
	}
}

void SectorArray::zero(off_t i) {
	assert(i < size());
	std::size_t len = SECTOR_SIZE;
	off_t off = i * SECTOR_SIZE;
	while (len) {
		ssize_t rc = pwrite(fd.fd(), &ZERO_SECTOR[0], len, off);
		if (rc < 0) {
			throw SystemError("pwrite", errno);
		}
		len -= static_cast<std::size_t>(rc);
		off += rc;
	}
}

namespace {
	class ScanResult final {
		public:
			enum class UpgradeAreaStatus {
				BLANK,
				OK,
				CORRUPT,
			};

			struct UpgradeArea final {
				UpgradeAreaStatus status;
				bool ephemeral;
				uint32_t length;
				uint32_t crc;
			};

			struct Epoch final {
				off_t first_sector, last_sector;
				uint64_t stamp;
			};

			explicit ScanResult(const SectorArray &sarray);
			off_t nonblank_size() const;
			const std::array<UpgradeArea, UPGRADE_AREA_COUNT> &upgrade_areas() const;
			const std::vector<Epoch> &epochs() const;

		private:
			off_t nonblank_size_;
			std::array<UpgradeArea, UPGRADE_AREA_COUNT> upgrade_areas_;
			std::vector<Epoch> epochs_;
	};
}

ScanResult::ScanResult(const SectorArray &sarray) {
	// Examine the upgrade areas.
	for (unsigned int i = 0; i != UPGRADE_AREA_COUNT; ++i) {
		off_t pos = i * UPGRADE_AREA_SECTORS;
		std::array<uint8_t, SECTOR_SIZE> header_sector;
		sarray.get(pos, &header_sector[0]);
		uint32_t magic = decode_u32_le(&header_sector[0]);
		uint32_t flags = decode_u32_le(&header_sector[4]);
		uint32_t length = decode_u32_le(&header_sector[8]);
		uint32_t expected_crc = decode_u32_le(&header_sector[12]);

		upgrade_areas_[i].status = UpgradeAreaStatus::BLANK;
		upgrade_areas_[i].ephemeral = false;
		upgrade_areas_[i].length = 0;
		upgrade_areas_[i].crc = 0;
		if (magic == UPGRADE_AREA_MAGICS[i]) {
			upgrade_areas_[i].status = UpgradeAreaStatus::CORRUPT;
			if (!(flags & UINT32_C(0xFFFFFFFE)) && length && (length <= (UPGRADE_AREA_SECTORS - 1) * SECTOR_SIZE)) {
				off_t first_sector = i * UPGRADE_AREA_SECTORS + 1;
				off_t length_sectors = (length + SECTOR_SIZE - 1) / SECTOR_SIZE;
				uint32_t actual_crc = CRC32::INITIAL;
				for (off_t j = 0; j != length_sectors; ++j) {
					uint32_t sector_base = static_cast<uint32_t>(j * SECTOR_SIZE);
					std::array<uint8_t, SECTOR_SIZE> sector_data;
					sarray.get(first_sector + j, &sector_data[0]);
					actual_crc = CRC32::calculate(&sector_data[0], MIN(SECTOR_SIZE, length - sector_base), actual_crc);
				}
				if (actual_crc == expected_crc) {
					upgrade_areas_[i].status = UpgradeAreaStatus::OK;
					upgrade_areas_[i].ephemeral = !!(flags & 1);
					upgrade_areas_[i].length = length;
					upgrade_areas_[i].crc = actual_crc;
				}
			}
		}
	}

	// Binary search on the non-blank size.
	off_t low = UPGRADE_AREA_COUNT * UPGRADE_AREA_SECTORS, high = sarray.size() + 1;
	while (low + 1 < high) {
		off_t pos = (low + high - 1) / 2;
		std::array<uint8_t, SECTOR_SIZE> sector;
		sarray.get(pos, &sector[0]);
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
			std::array<uint8_t, SECTOR_SIZE> sector;
			sarray.get(nonblank_size() - 1, &sector[0]);
			num_epochs = decode_u32_le(&sector[4]);
		}

		// Find locations of epochs.
		for (std::size_t epoch = 1; epoch <= num_epochs; ++epoch) {
			Epoch epoch_struct;

			// Search for start of epoch.
			off_t low = UPGRADE_AREA_COUNT * UPGRADE_AREA_SECTORS, high = nonblank_size();
			while (low + 1 < high) {
				off_t pos = (low + high - 1) / 2;
				std::array<uint8_t, SECTOR_SIZE> sector;
				sarray.get(pos, &sector[0]);
				std::size_t sector_epoch = decode_u32_le(&sector[4]);
				if (sector_epoch >= epoch) {
					high = pos + 1;
				} else {
					low = pos + 1;
				}
			}
			epoch_struct.first_sector = low;

			// Search for end of epoch.
			low = epoch_struct.first_sector, high = nonblank_size() + 1;
			while (low + 1 < high) {
				off_t pos = (low + high - 1) / 2;
				std::array<uint8_t, SECTOR_SIZE> sector;
				sarray.get(pos, &sector[0]);
				std::size_t sector_epoch = decode_u32_le(&sector[4]);
				if (sector_epoch > epoch) {
					high = pos + 1;
				} else {
					low = pos + 1;
				}
			}
			epoch_struct.last_sector = low - 1;

			// Extract epoch start timestamp.
			{
				std::array<uint8_t, SECTOR_SIZE> sector;
				sarray.get(epoch_struct.first_sector, &sector[0]);
				epoch_struct.stamp = decode_u64_le(&sector[8]);
			}

			epochs_.push_back(epoch_struct);
		}
	}
}

off_t ScanResult::nonblank_size() const {
	return nonblank_size_;
}

const std::array<ScanResult::UpgradeArea, UPGRADE_AREA_COUNT> &ScanResult::upgrade_areas() const {
	return upgrade_areas_;
}

const std::vector<ScanResult::Epoch> &ScanResult::epochs() const {
	return epochs_;
}

namespace {
	std::string load_file(const std::string &filename) {
		std::ifstream ifs;
		ifs.exceptions(std::ios_base::badbit | std::ios_base::failbit);
		ifs.open(filename, std::ios_base::in | std::ios_base::binary);
		std::ostringstream oss;
		oss << ifs.rdbuf();
		return oss.str();
	}

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
			std::array<uint8_t, SECTOR_SIZE> buffer;
			sdcard.get(sector, &buffer[0]);
			const uint8_t *ptr = &buffer[0];
			std::size_t len = SECTOR_SIZE;
			while (len) {
				ssize_t rc = write(fd.fd(), ptr, len);
				if (rc < 0) {
					throw SystemError("write", errno);
				}
				ptr += rc;
				len -= static_cast<std::size_t>(rc);
			}
		}
		fd.close();

		std::cout << "OK.\n";
		return 0;
	}

	int do_erase(SectorArray &sdcard, const ScanResult *, char **params) {
		off_t first, last;

		std::string erase_type(params[0]);
		if (erase_type == "all") {
			first = 0;
			last = sdcard.size() - 1;
		} else if (erase_type == "log") {
			first = UPGRADE_AREA_COUNT * UPGRADE_AREA_SECTORS;
			last = sdcard.size() - 1;
		} else if (erase_type == "upgrade") {
			first = 0;
			last = UPGRADE_AREA_COUNT * UPGRADE_AREA_SECTORS - 1;
		} else {
			bool found = false;
			for (unsigned int i = 0; !found && (i != UPGRADE_AREA_COUNT); ++i) {
				if (erase_type == UPGRADE_AREA_NAMES[i]) {
					first = i * UPGRADE_AREA_SECTORS;
					last = first + UPGRADE_AREA_SECTORS - 1;
					found = true;
				}
			}
			if (!found) {
				std::cerr << "Valid erase targets are:\nall\nlog\nupgrade\n";
				std::copy(UPGRADE_AREA_NAMES.begin(), UPGRADE_AREA_NAMES.end(), std::ostream_iterator<std::string>(std::cerr, "\n"));
				return 1;
			}
		}

		std::cout << "Erasing sectors " << first << " through " << last << ": ";
		std::cout.flush();
		uint64_t blkdiscard_params[2];
		blkdiscard_params[0] = static_cast<uint64_t>(first * SECTOR_SIZE);
		blkdiscard_params[1] = static_cast<uint64_t>((last - first + 1) * SECTOR_SIZE);
		if (ioctl(sdcard.fd.fd(), BLKDISCARD, blkdiscard_params) < 0) {
			if (errno == EOPNOTSUPP) {
				std::cout << "Well your card reader dun sucks so I card scans and manual overwrite" << std::endl;
				ScanResult scan_result(sdcard);
				constexpr std::size_t WRITE_AMOUNT = UPGRADE_AREA_SECTORS * SECTOR_SIZE;
				uint8_t blank_data[WRITE_AMOUNT];
				std::memset(blank_data, 0, WRITE_AMOUNT);
				std::size_t write_size;
				off_t file_offset = first * SECTOR_SIZE; 
				off_t past_end_offset = MIN(scan_result.nonblank_size(), last + 1) * SECTOR_SIZE;
				do {
					write_size = std::min(WRITE_AMOUNT, static_cast<std::size_t>(past_end_offset - file_offset));
					ssize_t ret_val = pwrite(sdcard.fd.fd(), blank_data, write_size, file_offset);
					if (ret_val < 0) {
						throw SystemError("pwrite", errno);
					}
					file_offset += ret_val;
				} while (file_offset < past_end_offset);
			} else {
				throw SystemError("ioctl(BLKDISCARD)", errno);
			}
		}
		std::cout << "OK.\n";
		return 0;
	}

	int do_info(SectorArray &sdcard, const ScanResult *scan_result, char **) {
		std::cout << "Sectors: " << sdcard.size() << '\n';
		std::cout << "Used: " << scan_result->nonblank_size() << '\n';

		std::cout << "Upgrade Areas:\n";
		std::size_t max_name_length = 0;
		for (unsigned int i = 0; i != UPGRADE_AREA_COUNT; ++i) {
			max_name_length = MAX(max_name_length, UPGRADE_AREA_NAMES[i].size());
		}
		for (unsigned int i = 0; i != UPGRADE_AREA_COUNT; ++i) {
			std::cout << (i + 1) << " (" << UPGRADE_AREA_NAMES[i] << std::string(max_name_length - UPGRADE_AREA_NAMES[i].size(), ' ') << "): ";
			const ScanResult::UpgradeArea &area = scan_result->upgrade_areas()[i];
			switch (area.status) {
				case ScanResult::UpgradeAreaStatus::BLANK:
					std::cout << "blank\n";
					break;

				case ScanResult::UpgradeAreaStatus::CORRUPT:
					std::cout << "corrupt\n";
					break;

				case ScanResult::UpgradeAreaStatus::OK:
					std::cout << "OK (length " << area.length << " bytes, CRC 0x" << tohex(area.crc, 8);
					if (area.ephemeral) {
						std::cout << ", ephemeral";
					}
					std::cout << ")\n";
					break;
			}
		}

		std::cout << "Epochs:\n";
		for (std::size_t i = 0; i < scan_result->epochs().size(); ++i) {
			const ScanResult::Epoch &epoch = scan_result->epochs()[i];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast" // The cast to time_t is useless on 64-bit platforms but not on 32-bit platforms.
			std::time_t t = static_cast<std::time_t>(epoch.stamp / 1000000);
#pragma GCC diagnostic pop
			char stamp_formatted[4096];
			std::strftime(stamp_formatted, sizeof(stamp_formatted), "%c", std::localtime(&t));
			std::cout << (i + 1) << '@' << stamp_formatted << " + 0." << todecu(epoch.stamp % 1000000, 6) << " s: " << '[' << epoch.first_sector << ',' << epoch.last_sector << "] length " << (epoch.last_sector - epoch.first_sector + 1) << '\n';
		}
		return 1;
	}

	int do_install(SectorArray &sdcard, const ScanResult *, char **params) {
		const std::string filename(params[0]);
		const std::string area_name(params[1]);
		const std::string flag(params[2]);

		unsigned int area = static_cast<unsigned int>(std::find(UPGRADE_AREA_NAMES.begin(), UPGRADE_AREA_NAMES.end(), area_name) - UPGRADE_AREA_NAMES.begin());
		if (area == UPGRADE_AREA_COUNT) {
			std::cerr << "Second parameter must be an upgrade area, one of:\n";
			std::copy(UPGRADE_AREA_NAMES.begin(), UPGRADE_AREA_NAMES.end(), std::ostream_iterator<std::string>(std::cerr, "\n"));
			return 1;
		}

		uint32_t flags;
		if (flag == "ephemeral") {
			flags = 1;
		} else if (flag == "normal") {
			flags = 0;
		} else {
			std::cerr << "Third parameter must be flags, one of:\nnormal\nephemeral\n";
			return 1;
		}

		std::string data = load_file(filename);
		if (data.size() > ((UPGRADE_AREA_SECTORS - 1) * SECTOR_SIZE)) {
			std::cerr << "File is too large (" << data.size() << " bytes, maximum is " << ((UPGRADE_AREA_SECTORS - 1) * SECTOR_SIZE) << ")\n";
			return 1;
		}
		std::size_t original_size = data.size();
		if(data.size() % SECTOR_SIZE) {
			data.append(static_cast<size_t>(SECTOR_SIZE) - (data.size() % static_cast<size_t>(SECTOR_SIZE)), 0);
		}

		// Wipe the header.
		sdcard.zero(area * UPGRADE_AREA_SECTORS);

		// Copy the data.
		for (std::size_t i = 0; i != data.size() / SECTOR_SIZE; ++i) {
			sdcard.put(area * UPGRADE_AREA_SECTORS + 1 + i, &data[i * static_cast<size_t>(SECTOR_SIZE)]);
		}

		// CRC the data.
		uint32_t crc = CRC32::calculate(data.data(), original_size);

		// Write the header.
		std::array<uint8_t, SECTOR_SIZE> header{0};
		encode_u32_le(&header[0], UPGRADE_AREA_MAGICS[area]);
		encode_u32_le(&header[4], flags);
		encode_u32_le(&header[8], static_cast<uint32_t>(original_size));
		encode_u32_le(&header[12], crc);
		sdcard.put(area * UPGRADE_AREA_SECTORS, &header[0]);

		std::cout << "Image written (" << original_size << " bytes, CRC 0x" << tohex(crc, 8) << ").\n";

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
		ofs << "Epoch\tUNIX Time\tBreakbeam\tBattery (V)\tCapacitor (V)\t";
		ofs << "DR X (m)\tDR Y (m)\tDR θ (r)\tDR VX (m/s)\tDR VY (m/s)\tDR Vθ (r/s)\t";
		ofs << "Drive Serial\tPrimitive\t";
		for (unsigned int i = 0; i != 10; ++i) {
			ofs << "Primitive Data " << i << '\t';
		}
		for (unsigned int i = 0; i != 4; ++i) {
			ofs << "Encoder " << i << " (¼°/t)\t";
		}
		for (unsigned int i = 0; i != 4; ++i) {
			ofs << "Motor " << i << " PWM (/255)\t";
		}
		for (unsigned int i = 0; i != 4; ++i) {
			ofs << "Motor " << i << " Temp (°C)\t";
		}
		ofs << "Dribbler Ticked?\tDribbler PWM (/255)\tDribbler Speed (rpm)\tDribbler Temp (°C)\tIdle Cycles";
		for (unsigned int i = 0; i != MRF::ERROR_LT_COUNT; ++i) {
			ofs << '\t' << MRF::ERROR_LT_MESSAGES[i];
		}
		for (unsigned int i = 0; i != MRF::ERROR_ET_COUNT; ++i) {
			ofs << '\t' << MRF::ERROR_ET_MESSAGES[i];
		}
		ofs << '\n';
		for (off_t sector = epoch.first_sector; sector <= epoch.last_sector; ++sector) {
			std::array<uint8_t, SECTOR_SIZE> buffer;
			sdcard.get(sector, &buffer[0]);
			for (std::size_t record = 0; record < static_cast<size_t>(RECORDS_PER_SECTOR); ++record) {
				const uint8_t *ptr = &buffer[record * static_cast<size_t>(LOG_RECORD_SIZE)];

				// Decode the record.
				uint32_t magic = decode_u32_le(ptr); ptr += 4;
				uint32_t record_epoch = decode_u32_le(ptr); ptr += 4;
				uint64_t stamp = decode_u64_le(ptr); ptr += 8;
				if (magic == LOG_MAGIC_TICK && record_epoch == epoch_index) {
					float breakbeam_diff = decode_float_le(ptr); ptr += 4;
					float battery_voltage = decode_float_le(ptr); ptr += 4;
					float capacitor_voltage = decode_float_le(ptr); ptr += 4;

					float dr_data[6];
					for (float &value : dr_data) {
						value = decode_float_le(ptr); ptr += 4;
					}

					unsigned int drive_serial = decode_u8_le(ptr); ptr += 1;
					unsigned int primitive = decode_u8_le(ptr); ptr += 1;

					float primitive_data[10];
					for (float &value : primitive_data) {
						value = decode_float_le(ptr); ptr += 4;
					}
					int16_t wheels_encoder_counts[4];
					for (int16_t &value : wheels_encoder_counts) {
						value = static_cast<int16_t>(decode_u16_le(ptr)); ptr += 2;
					}
					int16_t wheels_drives[4];
					for (int16_t &value : wheels_drives) {
						value = static_cast<int16_t>(decode_u16_le(ptr)); ptr += 2;
					}
					uint8_t wheels_temperatures[4];
					for (uint8_t &value : wheels_temperatures) {
						value = decode_u8_le(ptr); ptr += 1;
					}

					uint8_t dribbler_ticked = decode_u8_le(ptr); ptr += 1;
					uint8_t dribbler_pwm = decode_u8_le(ptr); ptr += 1;
					uint8_t dribbler_speed = decode_u8_le(ptr); ptr += 1;
					uint8_t dribbler_temperature = decode_u8_le(ptr); ptr += 1;

					uint32_t idle_cycles = decode_u32_le(ptr); ptr += 4;

					uint8_t errors[MRF::ERROR_BYTES];
					for (unsigned int i = 0; i != MRF::ERROR_BYTES; ++i) {
						errors[i] = *ptr++;
					}

					ofs << epoch_index << '\t' << (stamp / 1000000) << '.' << todecu(stamp % 1000000, 6) << '\t' << breakbeam_diff << '\t' << battery_voltage << '\t' << capacitor_voltage;
					for (float dd : dr_data) {
						ofs << '\t' << dd;
					}
					ofs << '\t' << drive_serial << '\t' << primitive;
					for (float pd : primitive_data) {
						ofs << '\t' << pd;
					}
					for (unsigned int i = 0; i < 4; ++i) {
						ofs << '\t' << wheels_encoder_counts[i];
					}
					for (unsigned int i = 0; i < 4; ++i) {
						ofs << '\t' << wheels_drives[i];
					}
					for (unsigned int temp : wheels_temperatures) {
						ofs << '\t' << temp;
					}
					ofs << '\t' << static_cast<unsigned int>(dribbler_ticked);
					ofs << '\t' << static_cast<unsigned int>(dribbler_pwm) << '\t' << static_cast<unsigned int>(dribbler_speed);
					ofs << '\t' << static_cast<unsigned int>(dribbler_temperature);
					ofs << '\t' << idle_cycles;
					for (unsigned int i = 0; i != MRF::ERROR_COUNT; ++i) {
						unsigned int word = i / CHAR_BIT;
						unsigned int bit = i % CHAR_BIT;
						ofs << '\t' << ((errors[word] & (1U << bit)) != 0);
					}
					ofs << '\n';
				}
			}
		}
		ofs.close();

		std::cout << "OK.\n";
		return 0;
	}

	struct Command final {
		std::string command;
		int args;
		bool needs_write;
		bool needs_scan;
		int (*handler)(SectorArray &sdcard, const ScanResult *scan_result, char **args);
	};

	const Command COMMANDS[] = {
		{ "copy", 2, false, true, &do_copy },
		{ "erase", 1, true, false, &do_erase },
		{ "info", 0, false, true, &do_info },
		{ "install", 3, true, false, &do_install },
		{ "tsv", 2, false, true, &do_tsv },
	};

	const Command *find_command(const char *command) {
		for (const Command &i : COMMANDS) {
			if (i.command == command) {
				return &i;
			}
		}
		return nullptr;
	}

	void usage(const char *app) {
		std::cerr << "Usage:\n";
		std::cerr << app << " disk command [args…]\n";
		std::cerr << '\n';
		std::cerr << "Possible commands are:\n";
		for (const Command &i : COMMANDS) {
			std::cout << i.command << '\n';
		}
	}
}

int app_main(int argc, char **argv) {
	// Set the current locale from environment variables.
	std::locale::global(std::locale(""));

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
	int status = command->handler(sdcard, scan_result.get(), argv + 3);

	// Sync the card if it was opened for writing.
	if (command->needs_write) {
		if (fsync(sdfd.fd()) < 0) {
			throw SystemError("fsync", errno);
		}
	}

	return status;
}
