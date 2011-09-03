#include <cstdlib>
#include <locale>
#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

int main() {
	// Set the current locale from environment variables.
	std::locale::global(std::locale(""));

	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
	CppUnit::TextTestRunner runner;
	runner.addTest(suite);
	return runner.run() ? EXIT_SUCCESS : EXIT_FAILURE;
}

