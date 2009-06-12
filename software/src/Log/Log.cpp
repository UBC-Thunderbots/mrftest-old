#include "Log/Log.h"

#include <ctime>
#include <ostream>
#include <streambuf>
#include <iostream>

namespace {
	Log::Level curLevel = Log::LEVEL_INFO;

	template<typename charT, typename traits = std::char_traits<charT> >
	class NullStreambuf : public std::basic_streambuf<charT, traits> {
	public:
		typedef traits traits_type;
		typedef typename traits_type::int_type int_type;
		typedef typename traits_type::pos_type pos_type;
		typedef typename traits_type::off_type off_type;

		static NullStreambuf &instance() {
			static NullStreambuf inst;
			return inst;
		}

	protected:
		virtual int_type overflow(__attribute__((__unused__)) int_type c) {
			setp(buffer, buffer + sizeof(buffer));
			return 0;
		}

	private:
		NullStreambuf() {
			setp(buffer, buffer + sizeof(buffer));
		}

		~NullStreambuf() {
		}

		charT buffer[32];
	};

	std::ostream &nullStream() {
		static std::ostream str(&NullStreambuf<char>::instance());
		return str;
	}
}

std::ostream &Log::log(Log::Level level, const std::string &module) {
	if (level > curLevel)
		return nullStream();

	std::cout << '[' << module << "] ";
	return std::cout;
}

void Log::setLevel(Log::Level level) {
	curLevel = level;
}

