#include <iostream>
#include <locale>
#include <string>

namespace {
	bool is01(wchar_t ch) {
		return ch == L'0' || ch == L'1';
	}
}

int main(void) {
	// Set the current locale from environment variables.
	std::locale::global(std::locale(""));

	// Ask for input.
	std::wcout << L"Direction (+/-): " << std::flush;
	std::wstring direction;
	std::getline(std::wcin, direction);
	std::wcout << L"Hall sensors: " << std::flush;
	std::wstring halls_string;
	std::getline(std::wcin, halls_string);

	// Validate input.
	if (direction.size() != 1 || (direction[0] != L'+' && direction[0] != L'-')) {
		std::wcerr << "Invalid direction (must be + or -).\n";
		return 1;
	}
	if (halls_string.size() != 3 || !is01(halls_string[0]) || !is01(halls_string[1]) || !is01(halls_string[2])) {
		std::wcerr << "Invalid halls (must be 3 binay digits).\n";
		return 1;
	}

	// Compute output.
	bool halls[3];
	for (unsigned int i = 0; i < 3; ++i) {
		halls[i] = halls_string[i] == L'1';
	}
	bool swapped[3];
	for (unsigned int i = 0; i < 3; ++i) {
		if (direction[0] == '+') {
			swapped[i] = !halls[i];
		} else {
			swapped[i] = halls[i];
		}
	}
	if (swapped[0] == swapped[1] && swapped[1] == swapped[2]) {
		std::wcout << "Invalid sensor state.\n";
		return 0;
	}
	bool pphase[3];
	bool nphase[3];
	for (unsigned int i = 0; i < 3; ++i) {
		pphase[i] = !(swapped[(i + 1) % 3] || !swapped[i]);
		nphase[i] = !swapped[i] && swapped[(i + 1) % 3];
	}
	for (unsigned int i = 0; i < 3; ++i) {
		if (pphase[i]) {
			std::wcout << "Phase " << i << " is positive.\n";
		}
	}
	for (unsigned int i = 0; i < 3; ++i) {
		if (nphase[i]) {
			std::wcout << "Phase " << i << " is negative.\n";
		}
	}
	return 0;
}

