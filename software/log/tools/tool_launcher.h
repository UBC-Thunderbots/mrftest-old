#ifndef LOG_TOOLS_TOOL_LAUNCHER_H
#define LOG_TOOLS_TOOL_LAUNCHER_H

#include "util/noncopyable.h"

class log_tool_launcher_impl;

//
// A window that allows log-management tools to be invoked.
//
class log_tool_launcher : public noncopyable {
	public:
		log_tool_launcher();
		~log_tool_launcher();

	private:
		log_tool_launcher_impl *impl;
};

#endif

