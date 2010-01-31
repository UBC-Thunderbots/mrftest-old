#include "log/reader/reader.h"
#include "log/reader/robot.h"



log_reader_robot::log_reader_robot(log_reader &reader) : reader(reader), pos(), ori(0) {
}



void log_reader_robot::update() {
	int16_t x = reader.read_u16();
	int16_t y = reader.read_u16();
	int16_t o = reader.read_u16();
	pos.x = x / 1000.0;
	pos.y = y / 1000.0;
	ori = o / 10000.0;
}

