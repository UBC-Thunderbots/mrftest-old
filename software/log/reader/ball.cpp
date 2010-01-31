#include "log/reader/ball.h"
#include "log/reader/reader.h"



log_reader_ball::log_reader_ball(log_reader &r) : reader(r), pos() {
}



void log_reader_ball::update() {
	int16_t x = reader.read_u16();
	int16_t y = reader.read_u16();
	pos.x = x / 1000.0;
	pos.y = y / 1000.0;
}

