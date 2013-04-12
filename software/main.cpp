#include "main.h"
#include <exception>
#include <iostream>
#include <typeinfo>
#include <glibmm/exception.h>

int main(int argc, char **argv) {
	try {
		return app_main(argc, argv);
	} catch (const Glib::Exception &exp) {
		std::cerr << typeid(exp).name() << ": " << exp.what() << '\n';
	} catch (const std::exception &exp) {
		std::cerr << typeid(exp).name() << ": " << exp.what() << '\n';
	} catch (...) {
		std::cerr << "Unknown error!\n";
	}
	return 1;
}


