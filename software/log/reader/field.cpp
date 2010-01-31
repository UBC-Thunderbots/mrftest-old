#include "log/reader/field.h"
#include "log/reader/reader.h"

log_reader_field::log_reader_field(log_reader &r) : reader(r), len(0), tlen(0), wid(0), twid(0), gwid(0), ccr(0), dar(0), das(0) {
}



void log_reader_field::update() {
	len = reader.read_u16() / 1000.0;
	tlen = reader.read_u16() / 1000.0;
	wid = reader.read_u16() / 1000.0;
	twid = reader.read_u16() / 1000.0;
	gwid = reader.read_u16() / 1000.0;
	ccr = reader.read_u16() / 1000.0;
	dar = reader.read_u16() / 1000.0;
	das = reader.read_u16() / 1000.0;
}

