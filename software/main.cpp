#include "main.h"
#include <exception>
#include <iostream>
#include <typeinfo>
#include <glibmm/exception.h>

namespace {
	void print_exception(const Glib::Exception &exp) {
		std::cerr << "\nUnhandled exception:\n";
		std::cerr << "Type:   " << typeid(exp).name() << '\n';
		std::cerr << "Detail: " << exp.what() << '\n';
	}

	void print_exception(const std::exception &exp, bool first = true) {
		if (first) {
			std::cerr << "\nUnhandled exception:\n";
		} else {
			std::cerr << "Caused by:\n";
		}
		std::cerr << "Type:   " << typeid(exp).name() << '\n';
		std::cerr << "Detail: " << exp.what() << '\n';
		try {
			std::rethrow_if_nested(exp);
		} catch (const std::exception &exp) {
			print_exception(exp, false);
		}
	}
}

int main(int argc, char **argv) {
	try {
		return app_main(argc, argv);
	} catch (const Glib::Exception &exp) {
		print_exception(exp);
	} catch (const std::exception &exp) {
		print_exception(exp);
	} catch (...) {
		std::cerr << "\nUnhandled exception:\n";
		std::cerr << "Type:   Unknown\n";
		std::cerr << "Detail: Unknown\n";
	}
	return 1;
}


