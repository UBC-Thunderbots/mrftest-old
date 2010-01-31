#include "log/reader/reader.h"
#include "log/reader/team.h"



log_reader_team::log_reader_team(log_reader &r, bool flip) : reader(r), flip(flip), the_score(0), is_yellow(false), the_playtype(playtype::halt) {
}



void log_reader_team::update() {
	uint8_t u8 = reader.read_u8();
	is_yellow = !!(u8 & 0x80);
	if (u8 & 0x40) {
		++the_score;
	}
	u8 &= 0x3F;
	while (lrrs.size() > u8) {
		lrrs.pop_back();
		bots.pop_back();
	}
	while (lrrs.size() < u8) {
		lrrs.push_back(log_reader_robot::create(reader));
		bots.push_back(robot::ptr(new robot(lrrs.back(), flip)));
	}

	for (std::size_t i = 0; i < lrrs.size(); ++i) {
		lrrs[i]->update();
	}
}

