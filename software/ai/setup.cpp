#include "ai/setup.h"
#include "util/codec.h"
#include <cstddef>
#include <fstream>
#include <inttypes.h>
#include <stdint.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	const uint32_t MAGIC = UINT32_C(0x4B045549);
	const uint32_t VERSION = UINT32_C(0x00000001);
	Glib::ustring AI::Setup::*const STRINGS[] = { &AI::Setup::high_level_name, &AI::Setup::navigator_name, &AI::Setup::robot_controller_name, &AI::Setup::ball_filter_name };

	std::string get_cache_filename() {
		const std::string &cache_dir = Glib::get_user_cache_dir();
		mkdir(cache_dir.c_str(), 0777);
		return Glib::build_filename(cache_dir, "thunderbots.setup");
	}
}

AI::Setup::Setup() : defending_end(AI::BE::Backend::FieldEnd::WEST), friendly_colour(AI::Common::Team::Colour::YELLOW) {
	const std::string &cache_filename = get_cache_filename();
	std::ifstream ifs;
	ifs.exceptions(std::ios_base::eofbit | std::ios_base::failbit | std::ios_base::badbit);
	try {
		ifs.open(cache_filename.c_str(), std::ios_base::in | std::ios_base::binary);
		char u32buf[4], ch;

		ifs.read(u32buf, sizeof(u32buf));
		if (decode_u32(u32buf) != MAGIC) {
			return;
		}

		ifs.read(u32buf, sizeof(u32buf));
		if (decode_u32(u32buf) != VERSION) {
			return;
		}

		for (std::size_t i = 0; i < G_N_ELEMENTS(STRINGS); ++i) {
			ifs.read(u32buf, sizeof(u32buf));
			char buffer[decode_u32(u32buf)];
			ifs.read(buffer, static_cast<std::streamsize>(sizeof(buffer)));
			(this->*STRINGS[i]).assign(buffer, sizeof(buffer));
		}

		ifs.read(&ch, 1);
		defending_end = ch ? AI::BE::Backend::FieldEnd::EAST : AI::BE::Backend::FieldEnd::WEST;
		ifs.read(&ch, 1);
		friendly_colour = ch ? AI::Common::Team::Colour::BLUE : AI::Common::Team::Colour::YELLOW;
	} catch (const std::ios_base::failure &) {
		// Swallow
	}
}

void AI::Setup::save() {
	const std::string &cache_filename = get_cache_filename();
	std::ofstream ofs;
	ofs.open(cache_filename.c_str(), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	char u32buf[4], ch;

	encode_u32(u32buf, MAGIC);
	ofs.write(u32buf, sizeof(u32buf));
	encode_u32(u32buf, VERSION);
	ofs.write(u32buf, sizeof(u32buf));

	for (std::size_t i = 0; i < G_N_ELEMENTS(STRINGS); ++i) {
		encode_u32(u32buf, static_cast<uint32_t>((this->*STRINGS[i]).bytes()));
		ofs.write(u32buf, sizeof(u32buf));
		ofs.write((this->*STRINGS[i]).data(), static_cast<std::streamsize>((this->*STRINGS[i]).bytes()));
	}

	ch = defending_end == AI::BE::Backend::FieldEnd::EAST;
	ofs.write(&ch, 1);
	ch = friendly_colour == AI::Common::Team::Colour::BLUE;
	ofs.write(&ch, 1);
}

