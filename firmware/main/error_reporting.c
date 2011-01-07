#include "error_reporting.h"
#include "feedback.h"

void error_reporting_add(uint8_t err) {
	feedback_block.faults[err / 8] |= 1 << (err % 8);
}

