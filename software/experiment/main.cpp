#include "util/algorithm.h"
#include "util/annunciator.h"
#include "util/crc16.h"
#include "util/noncopyable.h"
#include "util/string.h"
#include "xbee/dongle.h"
#include <cstdlib>
#include <ctime>
#include <glibmm.h>
#include <iostream>
#include <locale>
#include <memory>
#include <sstream>
#include <stdint.h>
#include <string>

namespace {
	class RobotExperimentReceiver : public NonCopyable, public sigc::trackable {
		public:
			RobotExperimentReceiver(uint8_t control_code, XBeeRobot::Ptr robot, Glib::RefPtr<Glib::MainLoop> main_loop) : control_code(control_code), robot(robot), main_loop(main_loop) {
				std::fill(received, received + G_N_ELEMENTS(received), false);
				robot->signal_experiment_data.connect(sigc::mem_fun(this, &RobotExperimentReceiver::on_experiment_data));
				Glib::signal_idle().connect_once(sigc::mem_fun(this, &RobotExperimentReceiver::start_operation));
			}

		private:
			uint8_t control_code;
			XBeeRobot::Ptr robot;
			Glib::RefPtr<Glib::MainLoop> main_loop;
			sigc::connection alive_changed_connection;
			uint8_t data[256];
			bool received[256];

			void start_operation() {
				std::cerr << "Waiting for robot to appear... " << std::flush;
				alive_changed_connection = robot->alive.signal_changed().connect(sigc::mem_fun(this, &RobotExperimentReceiver::on_alive_changed));
				on_alive_changed();
			}

			void on_alive_changed() {
				if (robot->alive) {
					alive_changed_connection.disconnect();
					alive_changed_connection = robot->alive.signal_changed().connect(sigc::mem_fun(this, &RobotExperimentReceiver::on_alive_changed2));
					std::cerr << "OK\n";
					std::cerr << "Issuing control code... " << std::flush;
					robot->start_experiment(control_code);
					std::cerr << "OK\n";
				}
			}

			void on_alive_changed2() {
				if (!robot->alive) {
					std::cerr << "Robot unexpectedly died\n";
					main_loop->quit();
				}
			}

			void on_experiment_data(const void *vp, std::size_t len) {
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
						std::cerr << c << " / " << G_N_ELEMENTS(received) << '\n';
					}
				}
			}
	};

	int main_impl(int argc, char **argv) {
		// Set the current locale from environment variables.
		std::locale::global(std::locale(""));

		// Seed the PRNGs.
		std::srand(static_cast<unsigned int>(std::time(0)));
		srand48(static_cast<long>(std::time(0)));

		// Handle command-line arguments.
		if (argc != 3) {
			std::cerr << "Usage:\n  " << argv[0] << " ROBOT_INDEX CONTROL_CODE\n\nLaunches a controlled experiment.\n\nApplication Options:\n  ROBOT_INDEX     Selects the robot to run the experiment.\n  CONTROL_CODE    Specifies a one-byte control code to select an experiment to run\n\n";
			return 1;
		}
		unsigned int robot_index;
		{
			Glib::ustring str(argv[1]);
			std::wistringstream iss(ustring2wstring(str));
			iss >> robot_index;
			if (!iss || robot_index > 15) {
				std::cerr << "Invalid robot index (must be a decimal number from 0 to 15).\n";
				return 1;
			}
		}
		uint8_t control_code;
		{
			Glib::ustring str(argv[2]);
			if (str.size() != 2 || !std::isxdigit(static_cast<wchar_t>(str[0]), std::locale()) || !std::isxdigit(static_cast<wchar_t>(str[1]), std::locale())) {
				std::cerr << "Invalid control code (must be a 2-digit hex number).\n";
				return 1;
			}
			std::wistringstream iss(ustring2wstring(str));
			iss.setf(std::ios_base::hex, std::ios_base::basefield);
			unsigned int ui;
			iss >> ui;
			control_code = static_cast<uint8_t>(ui);
		}

		std::cerr << "Finding and resetting dongle... " << std::flush;
		XBeeDongle dongle(true);
		std::cerr << "OK\n";

		std::cerr << "Enabling radios... " << std::flush;
		dongle.enable();
		std::cerr << "OK\n";

		Glib::RefPtr<Glib::MainLoop> main_loop = Glib::MainLoop::create();
		RobotExperimentReceiver rx(control_code, dongle.robot(robot_index), main_loop);
		main_loop->run();

		return 0;
	}
}

int main(int argc, char **argv) {
	try {
		return main_impl(argc, argv);
	} catch (const Glib::Exception &exp) {
		std::cerr << exp.what() << '\n';
	} catch (const std::exception &exp) {
		std::cerr << exp.what() << '\n';
	} catch (...) {
		std::cerr << "Unknown error!\n";
	}
	return 1;
}

