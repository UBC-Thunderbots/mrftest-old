#include "util/algorithm.h"
#include "util/annunciator.h"
#include "util/crc16.h"
#include "util/noncopyable.h"
#include "util/string.h"
#include "xbee/dongle.h"
#include <cstdlib>
#include <glibmm.h>
#include <iostream>
#include <locale>
#include <memory>
#include <stdint.h>
#include <string>

namespace {
	class RobotExperimentReceiver : public NonCopyable, public sigc::trackable {
		public:
			RobotExperimentReceiver(XBeeDongle &dongle, unsigned int index, Glib::RefPtr<Glib::MainLoop> main_loop) : dongle(dongle), index(index), main_loop(main_loop) {
				std::fill(received, received + G_N_ELEMENTS(received), false);
				dongle.robot(index)->signal_experiment_data.connect(sigc::mem_fun(this, &RobotExperimentReceiver::on_experiment_data));
			}

		private:
			XBeeDongle &dongle;
			unsigned int index;
			Glib::RefPtr<Glib::MainLoop> main_loop;
			uint8_t data[256];
			bool received[256];

			void on_experiment_data(const void *vp, std::size_t len) {
				if (len == 33) {
					const uint8_t *p = static_cast<const uint8_t *>(vp);
					if (p[0] + len - 1 <= G_N_ELEMENTS(received)) {
						std::copy(p + 1, p + len, data + p[0]);
						std::fill(received + p[0], received + p[0] + len - 1, true);
						std::ptrdiff_t c = std::count(received, received + G_N_ELEMENTS(received), true);
						if (c == G_N_ELEMENTS(received)) {
							std::cerr << "Done!\n";
							for (std::size_t i = 0; i < G_N_ELEMENTS(data); ++i) {
								std::cout << static_cast<unsigned int>(data[i]) << '\n';
							}
							main_loop->quit();
						} else {
							std::cerr << "Bot " << index << ": " << c << " / " << G_N_ELEMENTS(received) << '\n';
						}
					}
				}
			}
	};

	int main_impl() {
		// Set the current locale from environment variables.
		std::locale::global(std::locale(""));

		std::cout << "Finding dongle... " << std::flush;
		XBeeDongle dongle;
		std::cout << "OK\n";

		std::cout << "Enabling radios... " << std::flush;
		dongle.enable();
		std::cout << "OK\n";

		Glib::RefPtr<Glib::MainLoop> main_loop = Glib::MainLoop::create();
		std::vector<std::unique_ptr<RobotExperimentReceiver>> v;
		for (unsigned int i = 0; i < 16; ++i) {
			v.push_back(std::unique_ptr<RobotExperimentReceiver>(new RobotExperimentReceiver(dongle, i, main_loop)));
		}
		main_loop->run();

		return 0;
	}
}

int main() {
	try {
		return main_impl();
	} catch (const Glib::Exception &exp) {
		std::cerr << exp.what() << '\n';
	} catch (const std::exception &exp) {
		std::cerr << exp.what() << '\n';
	} catch (...) {
		std::cerr << "Unknown error!\n";
	}
	return 1;
}

