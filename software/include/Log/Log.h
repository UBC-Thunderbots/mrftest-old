#ifndef TB_LOG_H
#define TB_LOG_H

#include <ostream>
#include <string>

namespace Log {
	enum Level {
		LEVEL_ERROR,
		LEVEL_WARNING,
		LEVEL_INFO,
		LEVEL_DEBUG
	};

	std::ostream &log(Level level, const std::string &module);
	void setLevel(Level level);
}

#endif

